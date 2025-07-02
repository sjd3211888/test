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

#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "edds/rtps/common/InstanceHandle.h"
#include "vbs/status/SubscriptionMatchedStatus.hpp"

namespace vbs {
SubscriptionMatchedStatus::SubscriptionMatchedStatus(const EVBSSubscriptionMatchedStatus& info)
    : status_match_status_(info.total_count, info.total_count_change, info.current_count, info.current_count_change),
      status_last_publication_handle_(std::make_shared<InstanceHandle_t>(info.last_publication_handle)) {}

SubscriptionMatchedStatus::SubscriptionMatchedStatus() {}

SubscriptionMatchedStatus::SubscriptionMatchedStatus(const SubscriptionMatchedStatus& status)
    : status_match_status_(status.status_match_status_),
      status_last_publication_handle_(status.status_last_publication_handle_) {}

}  // namespace vbs
