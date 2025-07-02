// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// ------------------------------------------------------------------

#ifndef VBS_STATUS_MATCHEDSTATUS_HPP_
#define VBS_STATUS_MATCHEDSTATUS_HPP_

#include <cstdint>

namespace vbs {

class MatchedStatus {
 public:
    MatchedStatus() {
        status_total_count_ = 0;
        status_total_count_change_ = 0;
        status_current_count_ = 0;
        status_current_count_change_ = 0;
    }

    MatchedStatus(const MatchedStatus& status)
        : status_total_count_(status.status_total_count_),
          status_total_count_change_(status.status_total_count_change_),
          status_current_count_(status.status_current_count_),
          status_current_count_change_(status.status_current_count_change_) {}

    explicit MatchedStatus(const int32_t& total_count, const int32_t& total_count_change, const int32_t& current_count,
                           const int32_t& current_count_change)
        : status_total_count_(total_count),
          status_total_count_change_(total_count_change),
          status_current_count_(current_count),
          status_current_count_change_(current_count_change) {}

    MatchedStatus& operator=(const MatchedStatus& status) {
        status_total_count_ = status.status_total_count_;
        status_total_count_change_ = status.status_total_count_change_;
        status_current_count_ = status.status_current_count_;
        status_current_count_change_ = status.status_current_count_change_;
        return *this;
    }

    int32_t total_count() const { return status_total_count_; }

    int32_t total_count_change() const { return status_total_count_change_; }

    int32_t current_count() const { return status_current_count_; }

    int32_t current_count_change() const { return status_current_count_change_; }

 private:
    /**
     * Total cumulative count the concerned reader discovered a match with a writer reader
     */
    int32_t status_total_count_;
    /**
     * The change in total_count since the last time the listener was called or the status was read
     */
    int32_t status_total_count_change_;
    /**
     * The number of writers currently matched to the concerned reader
     */
    int32_t status_current_count_;
    /**
     * The change in current_count since the last time the listener was called or the status was read
     */
    int32_t status_current_count_change_;
};

}  // namespace vbs

#endif  // VBS_STATUS_MATCHEDSTATUS_HPP_
