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

#ifndef DSFDDS_SHARED_CHANNEL_RESOURCE_H
#define DSFDDS_SHARED_CHANNEL_RESOURCE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>
#include "channel_resource.h"
#include "common/types/TypesBase.h"

namespace li {
namespace dsfdds {

class ShmReader;

enum SharedReaderStatus { ONLINE, OFFLINE };

class SharedChannelResource : public ChannelResource {
    friend class DomainParticipantImpl;

 public:
    SharedChannelResource(const std::function<void(ChannelResource*)>& func, const std::string& name,
                          DomainId_t domain_id);

    ~SharedChannelResource() override;

    void clear() override;

    void AddReader(const ShmReader* const reader);
    void RemoveReader(const ShmReader* const reader);
    ReturnCode_t CallBackReader(const uint32_t reader_id);

 private:
    // the domain_id of the participant to which this SharedChannelResource belongs
    DomainId_t domain_id_;
    mutable std::mutex call_back_map_mtx_;
    // key:uint32_t reader id
    // value: ShmReader*
    std::unordered_map<uint32_t, const ShmReader* const> call_back_map_;
    std::unordered_map<uint32_t, SharedReaderStatus> reader_status_map_;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_SHARED_CHANNEL_RESOURCE_H
