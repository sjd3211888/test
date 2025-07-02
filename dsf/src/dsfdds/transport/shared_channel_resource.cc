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

#include "shared_channel_resource.h"
#include <cstdint>
#include <memory>
#include "common/log/DsfLog.hpp"
#include "dsfdds/transport/shm_reader.h"
#include "common/types/TypesBase.h"

namespace li {
namespace dsfdds {
SharedChannelResource::SharedChannelResource(const std::function<void(ChannelResource*)>& func, const std::string& name,
                                             DomainId_t domain_id)
    : ChannelResource(func, name), domain_id_(domain_id), call_back_map_mtx_(), call_back_map_() {
    DSF_LOG_INFO(SharedChannelResource,
                 "Created SharedChannelResource with queue name: " << queue_name_ << ",domain id:" << domain_id);
}

SharedChannelResource::~SharedChannelResource() {
    {
        std::lock_guard<std::mutex> lk(call_back_map_mtx_);
        call_back_map_.clear();
        reader_status_map_.clear();
    }
    SharedChannelResource::clear();
}

void SharedChannelResource::clear() {
    ChannelResource::clear();
}

void SharedChannelResource::AddReader(const ShmReader* const reader) {
    if (reader) {
        std::lock_guard<std::mutex> lk(call_back_map_mtx_);
        auto ret_call_back = call_back_map_.emplace(reader->get_entity_id(), reader);
        auto ret_status = reader_status_map_.emplace(reader->get_entity_id(), SharedReaderStatus::ONLINE);
        if ((!ret_call_back.second) || (!ret_status.second)) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                          "reader id" << reader->get_entity_id() << ",already in call_back_map_ or reader_status_map_"
                                      << ",ret_call_back.second:" << ret_call_back.second
                                      << ",ret_status.second:" << ret_status.second);
        }
        DSF_LOG_DEBUG(SharedChannelResource, "insert reader entity id:" << reader->get_entity_id());
    }
}

void SharedChannelResource::RemoveReader(const ShmReader* const reader) {
    std::lock_guard<std::mutex> lk(call_back_map_mtx_);
    auto it = call_back_map_.begin();
    while (it != call_back_map_.end()) {
        if (it->second == reader) {
            DSF_LOG_DEBUG(SharedChannelResource, "erase reader id:" << it->first << " from call_back_map_");
            it = call_back_map_.erase(it);
            auto it_status = reader_status_map_.find(reader->get_entity_id());
            if (it_status != reader_status_map_.end()) {
                it_status->second = SharedReaderStatus::OFFLINE;
            } else {
                DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                              "reader id:" << reader->get_entity_id() << " not in from reader_status_map_");
            }
        } else {
            ++it;
        }
    }
}

ReturnCode_t SharedChannelResource::CallBackReader(const uint32_t reader_id) {
    std::lock_guard<std::mutex> lk(call_back_map_mtx_);
    auto it = call_back_map_.find(reader_id);
    if (it == call_back_map_.end()) {
        auto status_it = reader_status_map_.find(reader_id);
        if (status_it == reader_status_map_.end()) {
            DSF_LOG_DEBUG(SharedChannelResource, "reader id:" << reader_id << ",is not in call_back_map_ not matched");
            return ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_NOT_MATCH;
        } else if (status_it->second == SharedReaderStatus::OFFLINE) {
            DSF_LOG_DEBUG(SharedChannelResource, "reader id:" << reader_id << ",already offline");
            return ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_OFFLINE;
        } else {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_STATUS_ERR,
                          "reader id:" << reader_id << ",current status:" << status_it->second << ",not as expected");
            return ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_STATUS_ERR;
        }
    }

    if (it->second->get_shared_queue_enable() && it->second->GetListener()) {
        it->second->GetListener()->on_data_available(const_cast<ShmReader*>(it->second));
        return ReturnCode_t::RETCODE_PUBSUB_OK;
    } else {
        DSF_LOG_WARNING(SharedChannelResource, "reader ids:" << domain_id_ << "," << it->second->get_entity_id()
                                                             << ",readerListener:" << it->second->GetListener()
                                                             << ",enable:" << it->second->get_shared_queue_enable());
        return ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_NOT_ENABLE_OR_LISTENER_NULL;
    }
}

}  // namespace dsfdds
}  // namespace li