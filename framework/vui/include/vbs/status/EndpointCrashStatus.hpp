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

#ifndef VBS_STATUS_ENDPOINTCRASHSTATUS_HPP_
#define VBS_STATUS_ENDPOINTCRASHSTATUS_HPP_
// getpid
#include <sys/types.h>
#include <unistd.h>
#include <set>
#include <cstdint>
#include <ostream>

namespace vbsutil {
namespace xmlparser {
struct GUID_t;
}
}  // namespace vbsutil

namespace vbs {

class EndpointCrashStatus {
 public:
    EndpointCrashStatus();
    EndpointCrashStatus(uint16_t domain_id, const vbsutil::xmlparser::GUID_t& guid);

    uint16_t domain_id() const { return status_domain_id_; }

    uint32_t participant_id() const { return status_participant_id_; }

    uint32_t entity_id() const { return status_entity_id_; }

    pid_t pid() const { return status_pid_; }

 private:
    uint16_t status_domain_id_;
    uint32_t status_participant_id_;
    uint32_t status_entity_id_;
    pid_t status_pid_;
};

}  // namespace vbs

#endif  // VBS_STATUS_ENDPOINTCRASHSTATUS_HPP_
