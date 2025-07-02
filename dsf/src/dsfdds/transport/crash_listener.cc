/*
* Copyright (c) 2025 Li Auto Inc. and its affiliates
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "dsfdds/transport/crash_listener.h"

#include <cstdint>
#include <ios>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "DsfLog.hpp"
#include "dsfdds/transport/shm_creator.h"
#include "dsfdds/transport/membuf_pool_crash_strategy.h"

namespace li {
namespace dsfdds {

CrashListener::CrashListener(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint)
    : alive_(true),
      discovery_endpoint_(discovery_endpoint),
      poolCrashStrategy(MembufPoolCrashStrategy(discovery_endpoint)),
      wait_signal_thread_([this] { this->WaitSignalFunc(); }) {}

void CrashListener::WaitSignalFunc() {
#if defined(__unix__)
    (void)pthread_setname_np(pthread_self(), "DSF:CrashListen");
#endif  // defined(__unix__)
    while (alive_) {
        std::vector<shmmq::signal_info> signals = shmmq::ShmmqManager::WaitSignal(1000U);
        if (!signals.empty()) {
            for (auto& it : signals) {
                DSF_LOG_DEBUG(CrashListener, "signal_info: name:" << it.name << ",pid:" << it.pid << ",is_mbuf_flag:"
                                                                  << std::boolalpha << it.is_mbuf_flag);
                DiscoveryServiceCrashedHandle(it);
                if (it.is_mbuf_flag) {
                    DSF_LOG_DEBUG(CrashListener,
                                  "receive mbuf crashed name:" << it.name << ",uuid:" << it.uuid << ",pid:" << it.pid);
                    poolCrashStrategy.UpdateCrashedMbufPools(it.uuid, it.pid);
                } else {
                    // queue crash handle
                    // 1: discovery update matched entity info,resume peer msg
                    // sending,trigger on_endpoint_crash call back
                    EndpointCrashHandle(EndpointType::kReader, discovery_endpoint_->local_register_writers_, it.uuid,
                                        it.pid);
                    EndpointCrashHandle(EndpointType::kWriter, discovery_endpoint_->local_register_readers_, it.uuid,
                                        it.pid);
                }
            }
        } else {  // NOLINT
            poolCrashStrategy.RecycleMbufPool();
            // 该调用主要为了处理writer异常或者下线时候reader未读取过pool消息这种场景Corner Cases
            poolCrashStrategy.CheckWriterStatusUpdatePoolIds();
        }
        // end of um driver info receive and process
    }  // end of while loop
}

void CrashListener::DiscoveryServiceCrashedHandle(const shmmq::signal_info& signal_info) const {
    static_cast<void>(signal_info);
}

std::shared_ptr<CrashListener> CrashListener::GetInstance(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint) {
    static std::shared_ptr<CrashListener> instance = nullptr;
    if (!instance) {
        static std::once_flag flag;
        std::call_once(flag, [&discovery_endpoint] {
            instance = std::shared_ptr<CrashListener>(new (std::nothrow) CrashListener(discovery_endpoint));
            assert(instance);
        });
    }
    return instance;
}

CrashListener::~CrashListener() {
    alive_.store(false);
    if (wait_signal_thread_.joinable()) {
        DSF_LOG_DEBUG(CrashListener, "CrashListener wait_signal_thread_ joinable,thread_.get_id() = "
                                         << wait_signal_thread_.get_id()
                                         << ",std::this_thread::get_id() = " << std::this_thread::get_id());
        if (wait_signal_thread_.get_id() != std::this_thread::get_id()) {
            //  wait for it to finish
            wait_signal_thread_.join();
            DSF_LOG_DEBUG(CrashListener, "CrashListener wait_signal_thread_ join");
        } else {
            // killing my own thread
            wait_signal_thread_.detach();
            DSF_LOG_DEBUG(CrashListener, "CrashListener wait_signal_thread_ detach");
        }
    }
    DSF_LOG_DEBUG(CrashListener, "CrashListener wait_signal_thread_ not joinable,thread is already over!!!");
}

template <typename LocalRegisterEntities,
          typename std::enable_if<std::is_same<ShmReader*, typename LocalRegisterEntities::value_type>::value ||
                                      std::is_same<ShmWriter*, typename LocalRegisterEntities::value_type>::value,
                                  int>::type>
void CrashListener::EndpointCrashHandle(EndpointType type, LocalRegisterEntities& register_entities, uint32_t queue_id,
                                        uint64_t pid) {
    std::lock_guard<std::mutex> lock((type == EndpointType::kReader)
                                         ? discovery_endpoint_->mtx_local_register_writer_gs_
                                         : discovery_endpoint_->mtx_local_register_reader_gs_);
    for (auto& role : register_entities) {
        if (role != nullptr) {
            std::lock_guard<std::mutex> lk(role->GetMtxMatchedGs());
            auto role_proxy = role->GetMatchedProxy().begin();
            while (role_proxy != role->GetMatchedProxy().end()) {
                if (role_proxy->second.IsPeerMatched()) {
                    // 只判断queue name还不行，比如2写1的场景一个写w1异常会导致reader认为w2也异常了
                    // 因为w1和w2使用的是相同的队列名字
                    if ((role_proxy->second.get_pid() == pid) && (role_proxy->second.get_queue_id() == queue_id)) {
                        // 处理完异常后清除
                        role->matched_is_crashed_[role_proxy->second.get_entity_id()] = true;
                        // mbuf pool 异常处理
                        role->GetPool()->OnPeerCrash(pid);
                        EntityProxyHandle(type, queue_id, pid, role, role_proxy);
                        if (!role->is_static_discovery_) {
                            if (type == EndpointType::kWriter) {
                                // 处理 crashed writer 发过来的所有 pool id
                                reinterpret_cast<ShmReader*>(role)->HandleMatchedWriterStates(
                                    role_proxy->second.get_entity_id(), role_proxy->second.get_pid(),
                                    role_proxy->second.get_msg_queue());
                                reinterpret_cast<ShmReader*>(role)->HandleOfflineWriterPoolIds(
                                    *reinterpret_cast<WriterProxy*>(&role_proxy->second), [this](mbuf::PoolID pool_id) {
                                        poolCrashStrategy.UpdateUnmatchedMbufPools(pool_id);
                                    });
                            }
                            //直接删除role proxy
                            DSF_LOG_DEBUG(CrashListener, "dynamic discovery erase role proxy:" << role_proxy->second);
                            role_proxy = role->GetMatchedProxy().erase(role_proxy);
                            continue;
                        }
                        if (type == EndpointType::kReader) {
                            // 如果reader异常 把队列释放掉
                            role_proxy->second.ResetMsgQueue();
                        }
                    } else {
                        DSF_LOG_DEBUG(CrashListener,
                                      "[crash pid:" << pid << ",role proxy pid:" << role_proxy->second.get_pid()
                                                    << "],[crash queue id:" << queue_id << ",role proxy queue id:"
                                                    << role_proxy->second.get_queue_id() << "]");
                    }
                }
                ++role_proxy;
            }
        }
    }
}

template <typename Role, typename RoleProxy>
void CrashListener::EntityProxyHandle(const EndpointType& type, uint32_t queue_id, uint64_t pid, const Role* role,
                                      const RoleProxy& role_proxy) {
    // update proxy info
    role_proxy->second.SetUnmatched();
    role_proxy->second.SetCrashed();
    role_proxy->second.ClearUnmatchedReverseStatus();

    DSF_LOG_DEBUG(CrashListener, "Finded matched queue id:" << queue_id << ", pid:" << pid
                                                            << ",crashed, set unmatched! matched "
                                                            << EndpointProxy::GetEndpointTypeStr(type)
                                                            << " proxy info:" << role_proxy->second);

    // 判断通过discovery传递过来的pid和驱动传递过来的pid是否一致？理论上应该相同
    if (role_proxy->first.get_pid() != pid) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_PID_CHECK_ERR,
                      "enity id:" << role_proxy->second.get_entity_id()
                                  << ", discovery received pid:" << role_proxy->first.get_pid()
                                  << " != um driver return pid:" << pid << ", queue id:" << queue_id);
    }
    DSF_LOG_INFO(CrashListener, "remote crash. endpoint_proxy_ info:" << role_proxy->first);
    // trigger unmatched call back
    if (role->on_endpoint_matched_) {
        role->on_endpoint_matched_(role_proxy->second);
    }
    // trigger crashed call back
    role->on_endpoint_crash_(role_proxy->first);
}

}  // namespace dsfdds
}  // namespace li
