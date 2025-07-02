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

#include "dsfdds/discovery/endpoint_proxy.h"
#include <cstring>
#include <cstdint>
#include <iostream>

namespace li {
namespace dsfdds {

EndpointProxy::EndpointProxy(uint8_t endpoint_type, uint16_t local_domain_id, uint32_t local_participant_id,
                             uint32_t local_entity_id, uint16_t remote_domain_id, uint32_t remote_participant_id,
                             uint32_t remote_entity_id, uint64_t pid)
    : msg_type_(0U),
      endpoint_type_(endpoint_type),
      local_domain_id_(local_domain_id),
      local_participant_id_(local_participant_id),
      local_entity_id_(local_entity_id),
      local_guid_(),
      remote_domain_id_(remote_domain_id),
      remote_participant_id_(remote_participant_id),
      remote_entity_id_(remote_entity_id),
      remote_guid_(),
      pid_(pid) {}

EndpointProxy::EndpointProxy(uint32_t domain_id, vbsutil::xmlparser::GUID_t local_guid,
                             vbsutil::xmlparser::GUID_t remote_guid)
    : msg_type_(0U),
      endpoint_type_(local_guid.is_writer() ? EndpointType::kWriter : EndpointType::kReader),
      local_domain_id_(domain_id),
      local_participant_id_(local_guid.get_participant_id()),
      local_entity_id_(local_guid.entityId.to_uint32()),
      local_guid_(local_guid),
      remote_domain_id_(domain_id),
      remote_participant_id_(remote_guid.get_participant_id()),
      remote_entity_id_(remote_guid.entityId.to_uint32()),
      remote_guid_(remote_guid),
      pid_(local_guid.get_process_id()) {}

EndpointProxy::EndpointProxy()
    : msg_type_(0U),
      endpoint_type_(0U),
      local_domain_id_(0U),
      local_participant_id_(0U),
      local_entity_id_(0U),
      local_guid_(),
      remote_domain_id_(0U),
      remote_participant_id_(0U),
      remote_entity_id_(0U),
      remote_guid_(),
      pid_(0U) {}

bool EndpointProxy::operator==(const EndpointProxy& rhs) const {
    return (local_domain_id_ == rhs.local_domain_id_) && (local_participant_id_ == rhs.local_participant_id_) &&
           (local_entity_id_ == rhs.local_entity_id_) && (remote_domain_id_ == rhs.remote_domain_id_) &&
           (remote_participant_id_ == rhs.remote_participant_id_) && (remote_entity_id_ == rhs.remote_entity_id_) &&
           (pid_ == rhs.pid_);
}
bool EndpointProxy::operator!=(const EndpointProxy& rhs) const {
    return !(rhs == *this);
}

uint64_t EndpointProxy::get_pid() const {
    return pid_;
}

EndpointProxy::EndpointProxy(const EndpointProxy& ep) {
    msg_type_ = ep.msg_type_;
    endpoint_type_ = ep.endpoint_type_;
    local_domain_id_ = ep.local_domain_id_;
    local_participant_id_ = ep.local_participant_id_;
    local_entity_id_ = ep.local_entity_id_;
    remote_domain_id_ = ep.remote_domain_id_;
    remote_participant_id_ = ep.remote_participant_id_;
    remote_entity_id_ = ep.remote_entity_id_;
    local_guid_ = ep.local_guid_;
    remote_guid_ = ep.remote_guid_;
    pid_ = ep.pid_;
}
EndpointProxy& EndpointProxy::operator=(const EndpointProxy& ep) {
    msg_type_ = ep.msg_type_;
    endpoint_type_ = ep.endpoint_type_;
    local_domain_id_ = ep.local_domain_id_;
    local_participant_id_ = ep.local_participant_id_;
    local_entity_id_ = ep.local_entity_id_;
    remote_domain_id_ = ep.remote_domain_id_;
    remote_participant_id_ = ep.remote_participant_id_;
    remote_entity_id_ = ep.remote_entity_id_;
    local_guid_ = ep.local_guid_;
    remote_guid_ = ep.remote_guid_;
    pid_ = ep.pid_;
    return *this;
}
void EndpointProxy::SwapCoreInfo() {
    uint16_t local_domain_id = local_domain_id_;
    uint32_t local_participant_id = local_participant_id_;
    uint32_t local_entity_id = local_entity_id_;

    this->local_domain_id_ = remote_domain_id_;
    this->local_participant_id_ = remote_participant_id_;
    this->local_entity_id_ = remote_entity_id_;

    this->remote_domain_id_ = local_domain_id;
    this->remote_participant_id_ = local_participant_id;
    this->remote_entity_id_ = local_entity_id;
}
}  // namespace dsfdds
}  // namespace li
