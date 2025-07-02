/*
 * Copyright 2010, Object Management Group, Inc.
 * Copyright 2010, PrismTech, Corp.
 * Copyright 2010, Real-Time Innovations, Inc.
 * Copyright 2019, Proyectos y Sistemas de Mantenimiento SL (eProsima).
 * All rights reserved.
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
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef VBS_STATUS_STATUSMASK_HPP_
#define VBS_STATUS_STATUSMASK_HPP_

#include <bitset>
#include <memory>

namespace evbs {
namespace edds {
namespace dds {
class StatusMask;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSStatusMask = evbs::edds::dds::StatusMask;
class StatusMask {
 public:
    StatusMask();

    StatusMask(const uint32_t& dds_mask, const uint32_t& dsf_mask);

    StatusMask(const StatusMask& mask);

    inline static StatusMask all() { return StatusMask(0x8000ffe7U, 0x8000007fU); }

    inline static StatusMask none() { return StatusMask(0U, 0U); }

 public:
    /**
     * Add given StatusMask bits into this StatusMask bitset.
     *
     * @return StatusMask this
     */
    StatusMask& operator<<(const StatusMask& mask);
    /**
     * Remove given StatusMask bits into this StatusMask bitset.
     *
     * @return StatusMask this
     */
    StatusMask& operator>>(const StatusMask& mask);

    /**
     * Get the StatusMask associated with dds::core::status::InconsistentTopicStatus
     *
     * @return StatusMask inconsistent_topic
     */
    static StatusMask inconsistent_topic();

    /**
     * Get the StatusMask associated with dds::core::status::OfferedDeadlineMissedStatus
     *
     * @return StatusMask offered_deadline_missed
     */
    static StatusMask offered_deadline_missed();

    /**
     * Get the StatusMask associated with dds::core::status::RequestedDeadlineMissedStatus
     *
     * @return StatusMask requested_deadline_missed
     */
    static StatusMask requested_deadline_missed();

    /**
     * Get the StatusMask associated with dds::core::status::OfferedIncompatibleQosStatus
     *
     * @return StatusMask offered_incompatible_qos
     */
    static StatusMask offered_incompatible_qos();

    /**
     * Get the StatusMask associated with dds::core::status::RequestedIncompatibleQosStatus
     *
     * @return StatusMask requested_incompatible_qos
     */
    static StatusMask requested_incompatible_qos();

    /**
     * Get the StatusMask associated with dds::core::status::SampleLostStatus
     *
     * @return StatusMask sample_lost
     */
    static StatusMask sample_lost();

    /**
     * Get the StatusMask associated with dds::core::status::SampleRejectedStatus
     *
     * @return StatusMask sample_rejected
     */
    static StatusMask sample_rejected();

    /**
     * Get the StatusMask associated with dds::core::status::data_on_readers
     *
     * @return StatusMask data_on_readers
     */
    static StatusMask data_on_readers();

    /**
     * get the statusmask associated with dds::core::status::data_available
     *
     * @return statusmask data_available
     */
    static StatusMask data_available();

    /**
     * Get the StatusMask associated with dds::core::status::LivelinessLostStatus
     *
     * @return StatusMask liveliness_lost
     */
    static StatusMask liveliness_lost();

    /**
     * Get the StatusMask associated with dds::core::status::LivelinessChangedStatus
     *
     * @return StatusMask liveliness_changed
     */
    static StatusMask liveliness_changed();

    /**
     * Get the statusmask associated with dds::core::status::PublicationMatchedStatus
     *
     * @return StatusMask publication_matched
     */
    static StatusMask publication_matched();

    /**
     * Get the statusmask associated with dds::core::status::SubscriptionMatchedStatus
     *
     * @return StatusMask subscription_matched
     */
    static StatusMask subscription_matched();

    /**
     * Get the statusmask associated with dds::core::status::E2EexceptionStatus
     *
     * @return StatusMask subscription_matched
     */
    static StatusMask e2e_expection();

    bool is_active(const StatusMask& status) const;

    std::shared_ptr<EVBSStatusMask> get_dds_status_mask() const { return dds_status_mask_; }

 private:
    std::shared_ptr<EVBSStatusMask> dds_status_mask_;
};

}  // namespace vbs

#endif  // VBS_STATUS_STATUSMASK_HPP_
