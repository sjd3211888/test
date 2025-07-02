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

#include <cstdint>

#include "deps/core/status/DeadlineMissedStatus.hpp"
#include "edds/rtps/common/InstanceHandle.h"
#include "vbs/status/DeadlineMissedStatus.hpp"

namespace vbs {

void DeadlineMissedStatus::init() {
    status_domain_id_ = static_cast<uint16_t>(0);
    status_participant_id_ = static_cast<uint32_t>(0);
    status_entity_id_ = static_cast<uint32_t>(0);
}

DeadlineMissedStatus::DeadlineMissedStatus()
    : status_total_count_(0),
      status_total_count_change_(0),
      status_last_instance_handle_(std::make_shared<InstanceHandle_t>()) {
    init();
}

DeadlineMissedStatus::DeadlineMissedStatus(const EVBSOfferedDeadlineMissedStatus& info)
    : status_total_count_(info.total_count),
      status_total_count_change_(info.total_count_change),
      status_last_instance_handle_(std::make_shared<InstanceHandle_t>(info.last_instance_handle)) {
    init();
}

}  // namespace vbs
