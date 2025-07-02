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

#include "deps/core/status/LivelinessChangedStatus.hpp"
#include "edds/rtps/common/InstanceHandle.h"
#include "vbs/status/LivelinessChangedStatus.hpp"

namespace vbs {

LivelinessChangedStatus::LivelinessChangedStatus(const EVBSLivelinessChangedStatus& info)
    : status_alive_count_(info.alive_count),
      status_not_alive_count_(info.not_alive_count),
      status_alive_count_change_(info.alive_count_change),
      status_not_alive_count_change_(info.not_alive_count_change),
      status_last_publication_handle_(std::make_shared<InstanceHandle_t>(info.last_publication_handle)) {}

LivelinessChangedStatus::LivelinessChangedStatus()
    : status_alive_count_(0),
      status_not_alive_count_(0),
      status_alive_count_change_(0),
      status_not_alive_count_change_(0),
      status_last_publication_handle_(std::make_shared<InstanceHandle_t>()) {}

}  // namespace vbs
