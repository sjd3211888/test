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

#include <chrono>

#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "dsfdds/discovery/discovery_endpoint.h"
#include "shmmq/shm_manager.h"

#include "dsfdds/transport/reader_proxy.h"
#include "dsfdds/transport/writer_proxy.h"

#include "dsfdds/transport/shm_creator.h"

namespace li {
namespace dsfdds {

DiscoveryEndpoint::DiscoveryEndpoint() : alive_(true), discovery_send_period_ms_(10U) {
    const char* env = std::getenv("DSF_DISCOVERY_SEND_PERIOD");
    if (env == nullptr) {
        DSF_LOG_DEBUG(DiscoveryEndpoint, "Environment variable DSF_DISCOVERY_SEND_PERIOD not set,use default 10ms");
    } else {
        int env_period = std::stoi(env);
        if ((env_period < 0) || (env_period > 10000)) {
            throw std::out_of_range("DSF_DISCOVERY_SEND_PERIOD :" + std::to_string(env_period) +
                                    ",out of range[0,10000]");
        } else {
            discovery_send_period_ms_ = static_cast<uint32_t>(env_period);
        }
    }
    DSF_LOG_DEBUG(DiscoveryEndpoint, "discovery_send_period_ms_:" << discovery_send_period_ms_);
}

DiscoveryEndpoint::~DiscoveryEndpoint() {}

std::shared_ptr<DiscoveryEndpoint> DiscoveryEndpoint::GetInstance() {
    static std::shared_ptr<DiscoveryEndpoint> instance = nullptr;
    if (!instance) {
        static std::once_flag flag;
        std::call_once(flag, [&] {
            try {
                instance = std::shared_ptr<DiscoveryEndpoint>(new (std::nothrow) DiscoveryEndpoint());
            } catch (...) {
                DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_ERROR, "call_once exception");
            }
        });
    }
    return instance;
}

bool DiscoveryEndpoint::RegisterRemoteReaderInfo(const EndpointProxy& info, ShmWriter* shm_writer) {
    std::lock_guard<std::mutex> lock(mtx_remote_register_reader_gs_);
    const auto& it = remote_register_readers_.find(info);
    if (it != remote_register_readers_.end()) {
        DSF_LOG_DEBUG(DiscoveryEndpoint, "reader info : " << info << ",have already registered");
        return false;
    } else {
        static_cast<void>(remote_register_readers_.emplace(info, shm_writer));
        DSF_LOG_DEBUG(DiscoveryEndpoint, "register reader info : " << info);
    }
    return true;
}

bool DiscoveryEndpoint::RemoveRemoteReaderInfo(const EndpointProxy& info) {
    std::unordered_map<EndpointProxy, ShmWriter*, EndpointProxyHasher>::size_type count = 0U;
    {
        std::lock_guard<std::mutex> lock(mtx_remote_register_reader_gs_);
        count = remote_register_readers_.erase(info);
        DSF_LOG_DEBUG(DiscoveryEndpoint, "remove remote reader count:" << count << ",endpoint info : " << info);
    }
    return count != 0U;
}

bool DiscoveryEndpoint::RemoveRemoteReaderInfo(const ShmWriter* shm_writer) {
    std::lock_guard<std::mutex> lock(mtx_remote_register_reader_gs_);
    auto it = remote_register_readers_.begin();
    while (it != remote_register_readers_.end()) {
        if (it->second == shm_writer) {
            DSF_LOG_DEBUG(DiscoveryEndpoint,
                          "remove remote_register_readers_ local writer entity id:" << it->second->get_entity_id());
            it = remote_register_readers_.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

bool DiscoveryEndpoint::RegisterRemoteWriterInfo(const EndpointProxy& info, ShmReader* shm_reader) {
    std::lock_guard<std::mutex> lock(mtx_remote_register_writer_gs_);
    const auto& it = remote_register_writers_.find(info);
    if (it != remote_register_writers_.end()) {
        DSF_LOG_DEBUG(DiscoveryEndpoint, "writer info:" << info << ",have already registered");
        return false;
    } else {
        static_cast<void>(remote_register_writers_.emplace(info, shm_reader));
        DSF_LOG_DEBUG(DiscoveryEndpoint, "register writer info : " << info);
    }
    return true;
}

bool DiscoveryEndpoint::RemoveRemoteWriterInfo(const EndpointProxy& info) {
    std::unordered_map<EndpointProxy, ShmReader*, EndpointProxyHasher>::size_type count = 0U;
    {
        std::lock_guard<std::mutex> lock(mtx_remote_register_writer_gs_);
        count = remote_register_writers_.erase(info);
        DSF_LOG_DEBUG(DiscoveryEndpoint, "remove remote writer count:" << count << ",endpoint info : " << info);
    }
    return count != 0U;
}

bool DiscoveryEndpoint::RemoveRemoteWriterInfo(const ShmReader* shm_reader) {
    std::lock_guard<std::mutex> lock(mtx_remote_register_writer_gs_);
    auto it = remote_register_writers_.begin();
    while (it != remote_register_writers_.end()) {
        if (it->second == shm_reader) {
            DSF_LOG_DEBUG(DiscoveryEndpoint,
                          "remove remote_register_writers_ local reader entity id:" << it->second->get_entity_id());
            it = remote_register_writers_.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

bool DiscoveryEndpoint::RegisterLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) {
    static_cast<void>(qos);
    std::pair<std::unordered_set<ShmReader*>::iterator, bool> it;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
        it = local_register_readers_.insert(shm_reader);
    }
    if (it.second) {
        DSF_LOG_DEBUG(DiscoveryEndpoint,
                      "insert into local reader set, shm_reader : " << shm_reader->PrintLocalEndpointProxies());
    } else {
        DSF_LOG_DEBUG(DiscoveryEndpoint,
                      "shm reader : " << shm_reader->PrintLocalEndpointProxies() << " already in local reader set");
    }
    return true;
}

void DiscoveryEndpoint::LocalReaderOffline(ShmReader* shm_reader) {
    {
        std::lock_guard<std::mutex> lk(shm_reader->mtx_local_endpoint_proxies_);
        for (auto& it_lo : shm_reader->local_endpoint_proxies_) {
            it_lo.SetDestructMsg();
            DSF_LOG_INFO(DiscoveryEndpoint, "reader ids:" << shm_reader->get_domain_id() << ","
                                                          << shm_reader->get_participant_id() << ","
                                                          << shm_reader->get_entity_id() << ",change it_lo: " << it_lo
                                                          << ",msg type to destruct.");
        }
    }

    if (shm_reader->enabled_) {
        // wait for enable false
        std::unique_lock<std::mutex> lk(shm_reader->mtx_enable_);
        if (!shm_reader->IsAllUnmatched()) {
            static_cast<void>(shm_reader->cv_enable_.wait_for(lk, std::chrono::seconds(10), [shm_reader] {
                return ((!shm_reader->enabled_) || shm_reader->IsAllUnmatched());
            }));
        }
        if (shm_reader->enabled_) {
            DSF_LOG_DEBUG(DiscoveryEndpoint,
                          "reader ids:" << shm_reader->get_domain_id() << "," << shm_reader->get_participant_id() << ","
                                        << shm_reader->get_entity_id() << ",wait timeout set enbale false!!!");
            shm_reader->enabled_ = false;
            lk.unlock();
            shm_reader->cv_enable_.notify_all();
        }
        DSF_LOG_DEBUG(DiscoveryEndpoint,
                      "reader ids:" << shm_reader->get_domain_id() << "," << shm_reader->get_participant_id() << ","
                                    << shm_reader->get_entity_id()
                                    << ",wait enable from true to false success, enable = " << std::boolalpha
                                    << shm_reader->enabled_);
    } else {
        std::unique_lock<std::mutex> lk(shm_reader->mtx_enable_);
        shm_reader->cv_enable_.notify_all();
        DSF_LOG_DEBUG(DiscoveryEndpoint,
                      "reader ids:" << shm_reader->get_domain_id() << "," << shm_reader->get_participant_id() << ","
                                    << shm_reader->get_entity_id()
                                    << ",wait enable from false to false success, enable = " << std::boolalpha
                                    << shm_reader->enabled_);
    }

    {
        std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
        static_cast<void>(local_register_readers_.erase(shm_reader));
    }
}

bool DiscoveryEndpoint::RemoveLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) {
    static_cast<void>(qos);
    bool is_found = false;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_reader_gs_);
        auto it = local_register_readers_.find(shm_reader);
        if (it != local_register_readers_.end()) {
            is_found = true;
        }
    }

    if (is_found) {
        LocalReaderOffline(shm_reader);
    } else {
        DSF_LOG_DEBUG(DiscoveryEndpoint, "reader ids:" << shm_reader->get_domain_id() << ","
                                                       << shm_reader->get_participant_id() << ","
                                                       << shm_reader->get_entity_id() << ",remove shm reader ptr"
                                                       << shm_reader << ",not in set");
        return false;
    }
    return true;
}

bool DiscoveryEndpoint::RegisterLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) {
    static_cast<void>(qos);
    std::pair<std::unordered_set<ShmWriter*>::iterator, bool> it;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
        it = local_register_writers_.insert(shm_writer);
    }
    if (it.second) {
        DSF_LOG_DEBUG(DiscoveryEndpoint, "insert into local writer set: " << shm_writer->PrintLocalEndpointProxies());
    } else {
        DSF_LOG_DEBUG(DiscoveryEndpoint,
                      "shm writer : " << shm_writer->PrintLocalEndpointProxies() << " already in local writer set");
    }
    return true;
}

bool DiscoveryEndpoint::RemoveLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) {
    static_cast<void>(qos);
    bool is_found = false;
    {
        std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
        auto it = local_register_writers_.find(shm_writer);
        if (it != local_register_writers_.end()) {
            is_found = true;
        }
    }
    if (is_found) {
        {
            std::lock_guard<std::mutex> lk(shm_writer->mtx_local_endpoint_proxies_);
            for (auto& it_lo : shm_writer->local_endpoint_proxies_) {
                it_lo.SetDestructMsg();
                DSF_LOG_DEBUG(DiscoveryEndpoint,
                              "writer ids:" << shm_writer->get_domain_id() << "," << shm_writer->get_participant_id()
                                            << "," << shm_writer->get_entity_id() << ",change it_lo: " << it_lo
                                            << ",msg type to destruct.");
            }
        }

        if (shm_writer->enabled_) {
            // wait for enable false
            std::unique_lock<std::mutex> lk(shm_writer->mtx_enable_);
            if (!shm_writer->IsAllUnmatched()) {
                static_cast<void>(shm_writer->cv_enable_.wait_for(lk, std::chrono::seconds(10), [shm_writer] {
                    return ((!shm_writer->enabled_) || shm_writer->IsAllUnmatched());
                }));
            }
            if (shm_writer->enabled_) {
                DSF_LOG_DEBUG(DiscoveryEndpoint, "writer ids:" << shm_writer->get_domain_id() << ","
                                                               << shm_writer->get_participant_id() << ","
                                                               << shm_writer->get_entity_id()
                                                               << ",wait timeout set enbale false!!!");
                shm_writer->enabled_ = false;
                lk.unlock();
                shm_writer->cv_enable_.notify_all();
            }
            DSF_LOG_DEBUG(DiscoveryEndpoint,
                          "writer ids:" << shm_writer->get_domain_id() << "," << shm_writer->get_participant_id() << ","
                                        << shm_writer->get_entity_id()
                                        << ",wait enable from true to false success, enable = " << std::boolalpha
                                        << shm_writer->enabled_);
        }
        {
            std::lock_guard<std::mutex> lock(mtx_local_register_writer_gs_);
            static_cast<void>(local_register_writers_.erase(shm_writer));
        }
    } else {
        DSF_LOG_INFO(DiscoveryEndpoint,
                     "remove shm writer not in set, endpoint prxoy : " << shm_writer->PrintLocalEndpointProxies());
        return false;
    }
    return true;
}

}  // namespace dsfdds
}  // namespace li
