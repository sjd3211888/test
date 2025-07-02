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

#ifndef VBS_STATUS_SUBSCRIPTIONMATCHEDSTATUS_HPP_
#define VBS_STATUS_SUBSCRIPTIONMATCHEDSTATUS_HPP_

#include <cstdint>
#include <memory>

#include "vbs/status/MatchedStatus.hpp"

namespace vbsutil {
namespace xmlparser {
struct InstanceHandle_t;
}
}  // namespace vbsutil

namespace evbs {
namespace edds {
namespace dds {
class SubscriptionMatchedStatus;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSSubscriptionMatchedStatus = evbs::edds::dds::SubscriptionMatchedStatus;
using InstanceHandle_t = vbsutil::xmlparser::InstanceHandle_t;

class SubscriptionMatchedStatus {
 public:
    explicit SubscriptionMatchedStatus(const EVBSSubscriptionMatchedStatus& info);

    SubscriptionMatchedStatus();

    SubscriptionMatchedStatus(const SubscriptionMatchedStatus& status);

    SubscriptionMatchedStatus& operator=(const SubscriptionMatchedStatus& status) {
        status_match_status_ = status.status_match_status_;
        status_last_publication_handle_ = status.status_last_publication_handle_;
        return *this;
    }

    int32_t total_count() const { return status_match_status_.total_count(); }

    int32_t total_count_change() const { return status_match_status_.total_count_change(); }

    int32_t current_count() const { return status_match_status_.current_count(); }

    int32_t current_count_change() const { return status_match_status_.current_count_change(); }

 private:
    MatchedStatus status_match_status_;
    /**
     * Handle to the last writer that matched the reader causing the status change
     */
    std::shared_ptr<InstanceHandle_t> status_last_publication_handle_;
};

}  // namespace vbs

#endif  // VBS_STATUS_SUBSCRIPTIONMATCHEDSTATUS_HPP_
