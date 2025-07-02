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

#ifndef VBS_STATUS_BASESTATUS_HPP_
#define VBS_STATUS_BASESTATUS_HPP_

#include <cstdint>

namespace evbs {
namespace edds {
namespace dds {
class SampleLostStatus;
class BaseStatus;
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSSampleLostStatus = evbs::edds::dds::SampleLostStatus;
using EVBSLivelinessLostStatus = evbs::edds::dds::BaseStatus;

/**
 * @brief struct storing the base status
 */
class BaseStatus {
 public:
    BaseStatus();

    explicit BaseStatus(const EVBSSampleLostStatus& info);

    ~BaseStatus() {}

    int32_t total_count() const { return status_total_count_; }

    int32_t total_count_change() const { return status_total_count_change_; }

    int32_t last_status() const { return status_last_status_; }

 private:
    /**
     * Total cumulative count
     */
    int32_t status_total_count_;
    /**
     * Increment since the last time the status was read
     */
    int32_t status_total_count_change_;
    /**
     * Last status value
     */
    int32_t status_last_status_;
};

using SampleLostStatus = BaseStatus;
class LivelinessLostStatus {
 public:
    LivelinessLostStatus();

    explicit LivelinessLostStatus(const EVBSLivelinessLostStatus& info);

    ~LivelinessLostStatus() {}

    int32_t total_count() const { return status_total_count_; }

    int32_t total_count_change() const { return status_total_count_change_; }

 private:
    /**
     * Total cumulative count
     */
    int32_t status_total_count_;
    /**
     * Increment since the last time the status was read
     */
    int32_t status_total_count_change_;
};

}  // namespace vbs

#endif  // VBS_STATUS_BASESTATUS_HPP_
