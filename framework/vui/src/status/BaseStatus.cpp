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

#include "deps/core/status/SampleLostStatus.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "vbs/status/BaseStatus.hpp"

namespace vbs {

BaseStatus::BaseStatus() : status_total_count_(0), status_total_count_change_(0), status_last_status_(0) {}

BaseStatus::BaseStatus(const EVBSSampleLostStatus& info)
    : status_total_count_(info.total_count), status_total_count_change_(info.total_count_change) {
    status_last_status_ = static_cast<int32_t>(info.last_status);
}

LivelinessLostStatus::LivelinessLostStatus() : status_total_count_(0), status_total_count_change_(0) {}

LivelinessLostStatus::LivelinessLostStatus(const EVBSLivelinessLostStatus& info)
    : status_total_count_(info.total_count), status_total_count_change_(info.total_count_change) {}

}  // namespace vbs
