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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef VBS_STATUS_LIVELINESSCHANGEDSTATUS_HPP_
#define VBS_STATUS_LIVELINESSCHANGEDSTATUS_HPP_

#include <cstdint>
#include <memory>

namespace vbsutil {
namespace xmlparser {
struct InstanceHandle_t;
}
}  // namespace vbsutil

namespace evbs {
namespace edds {
namespace dds {
class LivelinessChangedStatus;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSLivelinessChangedStatus = evbs::edds::dds::LivelinessChangedStatus;
using InstanceHandle_t = vbsutil::xmlparser::InstanceHandle_t;

class LivelinessChangedStatus {
 public:
    explicit LivelinessChangedStatus(const EVBSLivelinessChangedStatus& info);

    LivelinessChangedStatus();

    int32_t alive_count() const { return status_alive_count_; }

    int32_t not_alive_count() const { return status_not_alive_count_; }

    int32_t alive_count_change() const { return status_alive_count_change_; }

    int32_t not_alive_count_change() const { return status_not_alive_count_change_; }

    std::shared_ptr<InstanceHandle_t> last_publication_handle() const { return status_last_publication_handle_; }

 private:
    /**
     * The total number of currently active publishers that write the topic read by the subscriber
     */
    int32_t status_alive_count_ = 0;
    /**
     * The total count of current publishers that write the topic read by the subscriber
     * that are no longer asserting their liveliness
     */
    int32_t status_not_alive_count_ = 0;
    /**
     * The change in the alive_count since the last time the listener was called or the
     * status was read
     */
    int32_t status_alive_count_change_ = 0;
    /**
     * The change in the not_alive_count since the last time the listener was called or the
     * status was read
     */
    int32_t status_not_alive_count_change_ = 0;
    /**
     * Handle to the last publisher whose change in liveliness caused this status to change
     */
    std::shared_ptr<InstanceHandle_t> status_last_publication_handle_;
};

}  // namespace vbs

#endif  // VBS_STATUS_LIVELINESSCHANGEDSTATUS_HPP_
