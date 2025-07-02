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

#ifndef DSFDDS_TRANSPORT_SHMPARTICIPANT_H_
#define DSFDDS_TRANSPORT_SHMPARTICIPANT_H_

// std include
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// dds entity include
#include "dsfdds/discovery/discovery_endpoint.h"
#include "dsfdds/transport/channel_resource.h"
#include "dsfdds/transport/membuf_pool_crash_strategy.h"
#include "dsfdds/transport/shm_reader.h"
#include "dsfdds/transport/shm_writer.h"
#include "shmmq/message_queue.h"

namespace li {
namespace dsfdds {

class CrashListener {
    friend class MqDiscoveryEndpoint;
    friend class GlobalDiscoveryEndpoint;
    friend class VbsDiscoveryEndpoint;
    friend class DataWriterImpl;
    friend class DSFWriterImpl;
    friend class DSFReaderImpl;
    friend class SharingPayloadPool;

 public:
    static std::shared_ptr<CrashListener> GetInstance(std::shared_ptr<DiscoveryEndpoint>);
    virtual ~CrashListener();
    inline void thread(std::thread&& pThread) {
        if (wait_signal_thread_.joinable()) {
            wait_signal_thread_.join();
        }
        wait_signal_thread_.swap(pThread);
    }

 protected:
    std::atomic<bool> alive_;
    std::shared_ptr<DiscoveryEndpoint> discovery_endpoint_;

    template <typename LocalRegisterEntities,
              typename std::enable_if<std::is_same<ShmReader*, typename LocalRegisterEntities::value_type>::value ||
                                          std::is_same<ShmWriter*, typename LocalRegisterEntities::value_type>::value,
                                      int>::type = 0>
    void EndpointCrashHandle(EndpointType type, LocalRegisterEntities& register_entities, uint32_t queue_id,
                             uint64_t pid);

    template <typename Role, typename RoleProxy>
    void EntityProxyHandle(const EndpointType& type, uint32_t queue_id, uint64_t pid, const Role* role,
                           const RoleProxy& role_proxy);

    void WaitSignalFunc();

    // 不一定需要运行期动态可以改为编译时期的多态
    MembufPoolCrashStrategy poolCrashStrategy;
    // 处理对端进程crash的通知，每个进程共享一个
    std::thread wait_signal_thread_;

 protected:
    explicit CrashListener(std::shared_ptr<DiscoveryEndpoint>);
    CrashListener(const CrashListener&) = delete;
    CrashListener(CrashListener&&) = delete;
    CrashListener& operator=(const CrashListener&) = delete;

 protected:
    virtual void DiscoveryServiceCrashedHandle(const shmmq::signal_info& signal_info) const;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_SHMPARTICIPANT_H_
