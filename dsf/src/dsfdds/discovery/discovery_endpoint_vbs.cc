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

#include "discovery_endpoint_vbs.h"
#include "dsfdds/transport/shared_channel_resource.h"
#include "dsfdds/transport/shm_reader.h"
#include "dsfdds/transport/shm_writer.h"
#include <history/SubstrateWriter.h>
#include <xmlparser/xmlutils/DSFLocator.hpp>
#include "dsfdds/transport/crash_listener.h"

namespace li {
namespace dsfdds {

constexpr uint32_t ReaderMatchedWriterMaxCount = 128U;

std::shared_ptr<VbsDiscoveryEndpoint> VbsDiscoveryEndpoint::GetInstance() {
    static std::shared_ptr<VbsDiscoveryEndpoint> instance = nullptr;
    if (!instance) {
        static std::once_flag flag;
        std::call_once(flag, [&] {
            try {
                instance = std::shared_ptr<VbsDiscoveryEndpoint>(new (std::nothrow) VbsDiscoveryEndpoint());
            } catch (...) {
                DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_ERROR, "call_once exception");
            }
        });
    }
    return instance;
}

VbsDiscoveryEndpoint::VbsDiscoveryEndpoint() : DiscoveryEndpoint() {}

VbsDiscoveryEndpoint::~VbsDiscoveryEndpoint() {
    DSF_LOG_DEBUG(VbsDiscoveryEndpoint, "~VbsDiscoveryEndpoint");
}

// dsf 本端writer创建
bool VbsDiscoveryEndpoint::RegisterLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) {
    static_cast<void>(qos);
    std::pair<std::unordered_set<ShmWriter*>::iterator, bool> it;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
        it = local_register_writers_.insert(shm_writer);
    }
    if (it.second) {
        DSF_LOG_INFO(VbsDiscoveryEndpoint,
                     "dsf writer online, did:" << shm_writer->get_domain_id() << ", guid:" << shm_writer->guid());
    } else {
        DSF_LOG_INFO(VbsDiscoveryEndpoint,
                     "shm_writer did:" << shm_writer->get_domain_id() << ", guid:" << shm_writer->guid()
                                       << " already in local writer set. PrintLocalEndpointProxies : "
                                       << shm_writer->PrintLocalEndpointProxies());
    }
    return true;
}

// dsf 本端reader创建
bool VbsDiscoveryEndpoint::RegisterLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) {
    static_cast<void>(qos);
    std::pair<std::unordered_set<ShmReader*>::iterator, bool> it;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
        it = local_register_readers_.insert(shm_reader);
    }
    if (it.second) {
        if (shm_reader->use_shared_queue_) {
            std::dynamic_pointer_cast<SharedChannelResource>(shm_reader->channel_resource_)->AddReader(shm_reader);
        }
        DSF_LOG_INFO(VbsDiscoveryEndpoint,
                     "dsf reader online, did:" << shm_reader->get_domain_id() << ", guid:" << shm_reader->guid());
    } else {
        DSF_LOG_INFO(VbsDiscoveryEndpoint,
                     "shm_reader did:" << shm_reader->get_domain_id() << ", guid:" << shm_reader->guid()
                                       << " already in local reader set. PrintLocalEndpointProxies : "
                                       << shm_reader->PrintLocalEndpointProxies());
    }
    return true;
}

// 本端writer，对端reader匹配
bool VbsDiscoveryEndpoint::ProcessWriterReaderMatch(const uint32_t domain_id,
                                                    const vbsutil::xmlparser::GUID_t& local_guid,
                                                    const vbs::RemoteEndpointInfo& rdata) {
    ShmWriter* writer = FindLocalWriter(domain_id, local_guid);
    if (writer != nullptr) {
        EndpointProxy ep(domain_id, rdata.guid(), local_guid);

        ReaderProxy reader_proxy(nullptr, domain_id, rdata);
        // get queue id
        uint32_t queue_id = 0U;
        uint32_t pool_id = 0U;
        auto locator = rdata.locator();
        using namespace vbsutil::xmlparser;
        if (!DSFLocator::get_locator_information(locator, pool_id, queue_id)) {
            DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_WRITER_OPEN_QUEUE_ERR,
                          "get_locator_information failed. queue_id = "
                              << queue_id << " , pool_id = " << pool_id << " and local shm_writer ids: " << domain_id
                              << "_" << local_guid.get_participant_id() << "_" << local_guid.entityId.to_uint32()
                              << " and remote shm_reader ids: " << domain_id << "_" << rdata.guid().get_participant_id()
                              << "_" << rdata.guid().entityId.to_uint32());
            return false;
        }
        bool ret = reader_proxy.set_msg_queue(queue_id, (rdata.reliabilityKind() == ReliabilityKind_t::RELIABLE)
                                                            ? RELIABLE_RELIABILITY_QOS
                                                            : BEST_EFFORT_RELIABILITY_QOS);
        if (!ret) {
            DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_WRITER_OPEN_QUEUE_ERR,
                          "writer:" << ep << " open queue failed. queue_id=" << queue_id);
            return false;
        }
        auto user_id = writer->pool_owner_manager_->RegisterUser(static_cast<uint64_t>(rdata.guid().get_process_id()));
        bool id_invalid =
            writer->pool_owner_manager_->IsMultiReader() ? user_id.IsMultiUserInvalid() : user_id.IsUserInvalid();
        if (id_invalid) {
            DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_REGISTER_USERID_ERR,
                          "writer ids:" << domain_id << "_" << local_guid.get_participant_id() << "_"
                                        << local_guid.entityId.to_uint32() << ",register user id:" << user_id
                                        << " is invalid, in reader proxy:" << reader_proxy);
            return false;
        }
        //初次匹配时，授权对端reader访问pool权限
        writer->pool_owner_manager_->AddPeerId(static_cast<uint32_t>(rdata.guid().get_process_id()));

        reader_proxy.set_user_id(user_id);
        // set match
        reader_proxy.SetLocalMatched();
        reader_proxy.SetRemoteMatched();
        reader_proxy.ClearMatchedReverseStatus();
        auto mq_state = reader_proxy.get_msg_queue()->get_segment()->GetSegmentInfo()->p_state;
        for (size_t i = 0; i < ReaderMatchedWriterMaxCount; i++) {
            if (!mq_state->write_info[i].is_used_flag.load()) {
                reader_proxy.set_index(i);
                mq_state->write_info[i].is_used_flag.store(true);
                DSF_LOG_DEBUG(VbsDiscoveryEndpoint, "write_info ["
                                                        << i << "] is set to be used! writer ids:" << domain_id << "_"
                                                        << local_guid.get_participant_id() << "_"
                                                        << local_guid.entityId.to_uint32()
                                                        << ", write_count: " << mq_state->write_count + 1);
                break;
            } else {
                DSF_LOG_DEBUG(VbsDiscoveryEndpoint, "write_info ["
                                                        << i << "] has already been used! writer ids:" << domain_id
                                                        << "_" << local_guid.get_participant_id() << "_"
                                                        << local_guid.entityId.to_uint32()
                                                        << ", write_count: " << mq_state->write_count + 1);
            }
        }
        // add reader_proxy in matched_readers_
        static_cast<void>(writer->AddMatchedReader(ep, reader_proxy));
        // set enable
        writer->set_enabled(true);
        mq_state->write_count++;
        DSF_LOG_INFO(VbsDiscoveryEndpoint, "writer ids:" << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                         << local_guid.entityId.to_uint32()
                                                         << ",register user id:" << user_id
                                                         << ",and AddMatchedReader, reader proxy [" << reader_proxy
                                                         << "]" << ", write_count:" << mq_state->write_count);
        return true;
    } else {
        DSF_LOG_WARNING(VbsDiscoveryEndpoint, "local_register_writers_ can not find the writer with id: "
                                                  << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                  << local_guid.entityId.to_uint32());
        return false;
    }
}

// 本端reader，授权对端writer访问队列权限
void VbsDiscoveryEndpoint::ReaderAddPeerId(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& reader_guid,
                                           const vbsutil::xmlparser::GUID_t& writer_guid) {
    auto peer_id = static_cast<uint32_t>(writer_guid.get_process_id());
    ShmReader* reader = FindLocalReader(domain_id, reader_guid);
    if (reader != nullptr) {
        reader->get_msg_queue()->AddPeerId(peer_id);
    }
}

// 本端reader，对端writer匹配
bool VbsDiscoveryEndpoint::ProcessReaderWriterMatch(const uint32_t domain_id,
                                                    const vbsutil::xmlparser::GUID_t& local_guid,
                                                    const vbs::RemoteEndpointInfo& wdata) {
    ShmReader* reader = FindLocalReader(domain_id, local_guid);
    if (reader != nullptr) {
        EndpointProxy ep(domain_id, wdata.guid(), local_guid);
        // get pool id
        uint32_t queue_id = 0U;
        uint32_t pool_id = 0U;
        auto locator = wdata.locator();
        using namespace vbsutil::xmlparser;
        if (!DSFLocator::get_locator_information(locator, pool_id, queue_id)) {
            DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_MEMBUF_OPEN_POOL_ERR,
                          "get_locator_information failed. queue_id = "
                              << queue_id << " , pool_id = " << pool_id << " and shm_writer ids: " << domain_id << "_"
                              << local_guid.get_participant_id() << "_" << local_guid.entityId.to_uint32()
                              << " and remote shm_writer ids: " << domain_id << "_" << wdata.guid().get_participant_id()
                              << "_" << wdata.guid().entityId.to_uint32());
            return false;
        }
        WriterProxy write_proxy(reader->get_msg_queue(), domain_id, wdata, pool_id);
        // set match
        write_proxy.SetLocalMatched();
        write_proxy.SetRemoteMatched();
        write_proxy.ClearMatchedReverseStatus();
        // add write_proxy in matched_writers_
        static_cast<void>(reader->AddMatchedWriter(ep, write_proxy));
        // set enable
        reader->set_enabled(true);
        DSF_LOG_INFO(VbsDiscoveryEndpoint, "reader ids:" << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                         << local_guid.entityId.to_uint32()
                                                         << ", and AddMatchedWriter, writer proxy [" << write_proxy
                                                         << "]");
        return true;
    } else {
        DSF_LOG_WARNING(VbsDiscoveryEndpoint, "local_register_readers_ can not find the reader with id: "
                                                  << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                  << local_guid.entityId.to_uint32());
        return false;
    }
}

// dsf 本端writer下线
bool VbsDiscoveryEndpoint::RemoveLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) {
    static_cast<void>(qos);
    // set enable false
    {
        std::unique_lock<std::mutex> lk(shm_writer->mtx_enable_);
        if (shm_writer->enabled_) {
            shm_writer->enabled_ = false;
            lk.unlock();
            DSF_LOG_INFO(VbsDiscoveryEndpoint, "local dsf writer offline. did:" << shm_writer->get_domain_id()
                                                                                << ", guid:" << shm_writer->guid()
                                                                                << ",set enbale false.");
        }
        DSF_LOG_DEBUG(VbsDiscoveryEndpoint,
                      "writer did:" << shm_writer->get_domain_id() << ", guid:" << shm_writer->guid()
                                    << ",set enable from true to false success, enable = " << std::boolalpha
                                    << shm_writer->enabled_);
    }
    // 清理local_register_writers_
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
        static_cast<void>(local_register_writers_.erase(shm_writer));
    }
    return true;
}

// dsf 本端reader下线
bool VbsDiscoveryEndpoint::RemoveLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) {
    static_cast<void>(qos);
    // set enable false
    {
        std::unique_lock<std::mutex> lk(shm_reader->mtx_enable_);
        if (shm_reader->enabled_) {
            shm_reader->enabled_ = false;
            lk.unlock();
            DSF_LOG_INFO(VbsDiscoveryEndpoint, "local dsf reader offline. did:" << shm_reader->get_domain_id()
                                                                                << ", guid:" << shm_reader->guid()
                                                                                << ",set enbale false.");
        }
        DSF_LOG_DEBUG(VbsDiscoveryEndpoint,
                      "reader did:" << shm_reader->get_domain_id() << ", guid:" << shm_reader->guid()
                                    << ",set enable from true to false success, enable = " << std::boolalpha
                                    << shm_reader->enabled_);
    }
    // 清理local_register_readers_
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
        static_cast<void>(local_register_readers_.erase(shm_reader));
    }
    return true;
}

// 本端writer，对端reader下线
bool VbsDiscoveryEndpoint::ProcessWriterReaderUnMatch(const uint32_t domain_id,
                                                      const vbsutil::xmlparser::GUID_t& local_guid,
                                                      const vbs::RemoteEndpointInfo& rdata) {
    ShmWriter* writer = FindLocalWriter(domain_id, local_guid);
    if (writer != nullptr) {
        EndpointProxy ep(domain_id, rdata.guid(), local_guid);
        std::lock_guard<std::mutex> lock_guard(writer->mtx_matched_reader_gs_);
        auto rit = writer->matched_readers_.find(ep);
        if (rit != writer->matched_readers_.end()) {
            rit->second.SetUnmatched();
            rit->second.ClearUnmatchedReverseStatus();
            auto ret = writer->pool_owner_manager_->UnRegisterUser(rit->second.get_user_id());
            if (!ret) {
                DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_UNREGISTER_USERID_ERR,
                              "writer did:" << writer->get_domain_id() << ", guid:" << writer->guid()
                                            << ",UnRegisterUser usedId:" << rit->second.get_user_id()
                                            << ",in reader proxy:" << &rit->second << "," << rit->second << " failed!");
                return false;
            }
            // 无论对端crash还是offline这里都把队列资源释放掉
            rit->second.ResetMsgQueue();
            // 清理matched_readers_
            static_cast<void>(writer->matched_readers_.erase(rit));
            DSF_LOG_INFO(VbsDiscoveryEndpoint, "ProcessWriterReaderUnMatch Success! "
                                                   << " did:" << domain_id << " writer guid:" << local_guid
                                                   << " reader guid:" << rdata.guid());
        } else {
            // matched_readers_被重复清理
            DSF_LOG_WARNING(VbsDiscoveryEndpoint,
                            "writer did:" << writer->get_domain_id() << ", guid:" << writer->guid()
                                          << ", matched_readers_ can not find endpoint_proxy:" << ep);
        }
    } else {
        DSF_LOG_WARNING(VbsDiscoveryEndpoint, "local_register_writers_ can not find the writer with id: "
                                                  << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                  << local_guid.entityId.to_uint32());
    }
    return true;
}

// 本端reader，对端write下线
bool VbsDiscoveryEndpoint::ProcessReaderWriterUnMatch(const uint32_t domain_id,
                                                      const vbsutil::xmlparser::GUID_t& local_guid,
                                                      const vbs::RemoteEndpointInfo& wdata) {
    ShmReader* reader = FindLocalReader(domain_id, local_guid);
    if (reader != nullptr) {
        EndpointProxy ep(domain_id, wdata.guid(), local_guid);
        std::lock_guard<std::mutex> lock_guard(reader->mtx_matched_writer_gs_);
        auto wit = reader->matched_writers_.find(ep);
        if (wit != reader->matched_writers_.end()) {
            reader->HandleMatchedWriterStates(wit->second.get_entity_id(), wit->second.get_pid(),
                                              reader->get_msg_queue());
            wit->second.SetUnmatched();
            wit->second.ClearUnmatchedReverseStatus();
            // reader 处理下线 writer 所有发过来的 pool id
            reader->HandleOfflineWriterPoolIds(wit->second, [](mbuf::PoolID pool_id) {
                CrashListener::GetInstance(VbsDiscoveryEndpoint::GetInstance())
                    ->poolCrashStrategy.UpdateUnmatchedMbufPools(pool_id);
            });
            // 清理matched_writers_
            static_cast<void>(reader->matched_writers_.erase(wit));
            DSF_LOG_INFO(VbsDiscoveryEndpoint, "ProcessReaderWriterUnMatch Success! "
                                                   << " did:" << domain_id << " reader guid:" << local_guid
                                                   << " writer guid:" << wdata.guid());
        } else {
            // matched_writers_被重复清理
            DSF_LOG_WARNING(VbsDiscoveryEndpoint,
                            "reader ids:" << domain_id << "," << local_guid.get_participant_id() << ","
                                          << local_guid.entityId.to_uint32()
                                          << ",matched_writers_ can not find endpoint_proxy:" << ep);
        }
    } else {
        DSF_LOG_WARNING(VbsDiscoveryEndpoint, "local_register_readers_ can not find the reader with id:"
                                                  << domain_id << "_" << local_guid.get_participant_id() << "_"
                                                  << local_guid.entityId.to_uint32());
    }
    return true;
}

ShmWriter* VbsDiscoveryEndpoint::FindLocalWriter(const uint32_t domain_id,
                                                 const vbsutil::xmlparser::GUID_t& local_guid) {
    std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
    for (ShmWriter* item : local_register_writers_) {
        if ((item->get_domain_id() == domain_id) && (item->get_entity_id() == local_guid.entityId.to_uint32()) &&
            (item->get_participant_id() == local_guid.get_participant_id())) {
            return item;
        }
    }
    return nullptr;
}

ShmReader* VbsDiscoveryEndpoint::FindLocalReader(const uint32_t domain_id,
                                                 const vbsutil::xmlparser::GUID_t& local_guid) {
    std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
    for (ShmReader* item : local_register_readers_) {
        if ((item->get_domain_id() == domain_id) && (item->get_entity_id() == local_guid.entityId.to_uint32()) &&
            (item->get_participant_id() == local_guid.get_participant_id())) {
            return item;
        }
    }
    return nullptr;
}

}  // namespace dsfdds
}  // namespace li