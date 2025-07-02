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

#ifndef VBS_STATUS_DEADLINEMISSEDSTATUS_HPP_
#define VBS_STATUS_DEADLINEMISSEDSTATUS_HPP_

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
class DeadlineMissedStatus;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSOfferedDeadlineMissedStatus = evbs::edds::dds::DeadlineMissedStatus;
using InstanceHandle_t = vbsutil::xmlparser::InstanceHandle_t;

class DeadlineMissedStatus {
 public:
    DeadlineMissedStatus();

    explicit DeadlineMissedStatus(const EVBSOfferedDeadlineMissedStatus& info);

    ~DeadlineMissedStatus() {}

    uint32_t total_count() const { return status_total_count_; }

    uint32_t total_count_change() const { return status_total_count_change_; }

    uint16_t domain_id() const { return status_domain_id_; }

    uint32_t participant_id() const { return status_participant_id_; }

    uint32_t entity_id() const { return status_entity_id_; }

 private:
    /**
     * Total cumulative number of offered deadline periods elapsed during which a writer
     * incremented by 1
     */
    uint32_t status_total_count_;
    /**
     * The change in total_count since the last time the listener was called or the status
     * was read
     */
    uint32_t status_total_count_change_;

    /**
     * Handle to the last instance missing the deadline
     */
    std::shared_ptr<InstanceHandle_t> status_last_instance_handle_;
    uint16_t status_domain_id_ = static_cast<uint16_t>(0);
    uint32_t status_participant_id_ = static_cast<uint32_t>(0);
    uint32_t status_entity_id_ = static_cast<uint32_t>(0);

    void init();
};

typedef DeadlineMissedStatus OfferedDeadlineMissedStatus;
typedef DeadlineMissedStatus RequestedDeadlineMissedStatus;

}  // namespace vbs

#endif  // VBS_STATUS_DEADLINEMISSEDSTATUS_HPP_
