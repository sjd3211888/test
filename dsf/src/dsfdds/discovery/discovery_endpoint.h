
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

#ifndef DSFDDS_DISCOVERY_DISCOVERYENDPOINT_H
#define DSFDDS_DISCOVERY_DISCOVERYENDPOINT_H

// std include
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// dds entity
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "dsfdds/discovery/endpoint_proxy.h"
#include "dsfdds/transport/shm_reader.h"
#include "dsfdds/transport/shm_writer.h"

namespace li {
namespace dsfdds {
class DiscoveryEndpoint {
    friend class CrashListener;
    friend class MembufPoolCrashStrategy;

 protected:
    DiscoveryEndpoint();
    DiscoveryEndpoint(const DiscoveryEndpoint&) = delete;
    DiscoveryEndpoint(DiscoveryEndpoint&&) = delete;
    DiscoveryEndpoint& operator=(const DiscoveryEndpoint&) = delete;

 public:
    static std::shared_ptr<DiscoveryEndpoint> GetInstance();
    virtual ~DiscoveryEndpoint();
    inline void thread(std::thread&& pThread, EndpointType et) {
        if (discovery_threads_[et].joinable()) {
            discovery_threads_[et].join();
        }
        discovery_threads_[et].swap(pThread);
    }

 public:
    virtual bool RegisterRemoteReaderInfo(const EndpointProxy& info, ShmWriter* shm_writer);
    virtual bool RemoveRemoteReaderInfo(const EndpointProxy& info);
    virtual bool RemoveRemoteReaderInfo(const ShmWriter* shm_writer);
    virtual bool RegisterRemoteWriterInfo(const EndpointProxy& info, ShmReader* shm_reader);
    virtual bool RemoveRemoteWriterInfo(const EndpointProxy& info);
    virtual bool RemoveRemoteWriterInfo(const ShmReader* shm_reader);

    virtual bool RegisterLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos);
    virtual bool RemoveLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos);
    virtual bool RegisterLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos);
    virtual bool RemoveLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos);

 protected:
    // 一个写线程周期广播自己的状态，一个读线程读取广播队列中的消息。[0] 写 [1] 读
    std::array<std::thread, 2> discovery_threads_;

    // 本地注册结构
    mutable std::mutex mtx_local_register_reader_gs_;
    std::unordered_set<ShmReader*> local_register_readers_;
    mutable std::mutex mtx_local_register_writer_gs_;
    std::unordered_set<ShmWriter*> local_register_writers_;

    // 远端注册结构 key EndpointProxy是远端的信息  value是对应的匹配的本地的信息
    mutable std::mutex mtx_remote_register_reader_gs_;
    std::unordered_map<EndpointProxy, ShmWriter*, EndpointProxyHasher> remote_register_readers_;
    mutable std::mutex mtx_remote_register_writer_gs_;
    std::unordered_map<EndpointProxy, ShmReader*, EndpointProxyHasher> remote_register_writers_;

    std::atomic<bool> alive_;
    uint32_t discovery_send_period_ms_ = 10;

 private:
    void LocalReaderOffline(ShmReader* shm_reader);
};

}  // namespace dsfdds
}  // namespace li
#ifndef DSFDDS_MACRO_TO_STRING
#define DSFDDS_MACRO_TO_STRING
#define MAKE_STRING(x) #x
#define DSF_MACRO_TO_STRING(x) MAKE_STRING(x)
#endif  /*DSFDDS_MACRO_TO_STRING*/
#endif  // DSFDDS_DISCOVERY_DISCOVERYENDPOINT_H
