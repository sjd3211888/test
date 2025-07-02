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

#include "vbs/status/EndpointCrashStatus.hpp"
#include "core/Global.hpp"

namespace vbs {

EndpointCrashStatus::EndpointCrashStatus()
    : status_domain_id_(static_cast<uint16_t>(0)),
      status_participant_id_(static_cast<uint16_t>(0)),
      status_entity_id_(static_cast<uint16_t>(0)),
      status_pid_(static_cast<pid_t>(0)) {}

EndpointCrashStatus::EndpointCrashStatus(uint16_t domain_id, const vbsutil::xmlparser::GUID_t& guid)
    : status_domain_id_(domain_id),
      status_participant_id_(guid.get_participant_id()),
      status_entity_id_(guid.entityId.to_uint32()),
      status_pid_(guid.get_process_id()) {}

}  // namespace vbs
