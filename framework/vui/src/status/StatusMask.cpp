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

#include <bitset>
#include <cstdint>

#include "deps/core/status/StatusMask.hpp"
#include "vbs/status/StatusMask.hpp"

namespace vbs {
StatusMask::StatusMask() : dds_status_mask_(std::make_shared<EVBSStatusMask>()) {}

StatusMask::StatusMask(const uint32_t& dds_mask, const uint32_t& dsf_mask)
    : dds_status_mask_(std::make_shared<EVBSStatusMask>(dds_mask)) {
    (void)dsf_mask;
}

StatusMask::StatusMask(const StatusMask& mask) {
    dds_status_mask_ = mask.dds_status_mask_;
}

StatusMask& StatusMask::operator<<(const StatusMask& mask) {
    *(this->dds_status_mask_) |= *(mask.dds_status_mask_);
    return *this;
}

StatusMask& StatusMask::operator>>(const StatusMask& mask) {
    *(this->dds_status_mask_) &= ~*(mask.dds_status_mask_);
    return *this;
}

StatusMask StatusMask::inconsistent_topic() {
    StatusMask status;
    if (status.dds_status_mask_ == nullptr)
        return status;
    *(status.dds_status_mask_) |= EVBSStatusMask::inconsistent_topic();
    return status;
}

StatusMask StatusMask::offered_deadline_missed() {
    StatusMask status;
    *(status.dds_status_mask_) |= EVBSStatusMask::offered_deadline_missed();
    return status;
}

StatusMask StatusMask::requested_deadline_missed() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::requested_deadline_missed();
    return status;
}

StatusMask StatusMask::offered_incompatible_qos() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::offered_incompatible_qos();
    return status;
}

StatusMask StatusMask::requested_incompatible_qos() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::requested_incompatible_qos();
    return status;
}

StatusMask StatusMask::sample_lost() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::sample_lost();
    return status;
}

StatusMask StatusMask::sample_rejected() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::sample_rejected();
    return status;
}

StatusMask StatusMask::data_on_readers() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::data_on_readers();
    return status;
}

StatusMask StatusMask::data_available() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::data_available();
    return status;
}

StatusMask StatusMask::liveliness_lost() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::liveliness_lost();
    return status;
}

StatusMask StatusMask::liveliness_changed() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::liveliness_changed();
    return status;
}

StatusMask StatusMask::publication_matched() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::publication_matched();
    return status;
}

StatusMask StatusMask::subscription_matched() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::subscription_matched();
    return status;
}

StatusMask StatusMask::e2e_expection() {
    StatusMask status;
    *(status.dds_status_mask_) << EVBSStatusMask::e2e_expection();
    return status;
}

bool StatusMask::is_active(const StatusMask& status) const {
    const bool is_status_dds = dds_status_mask_->is_active(*(status.dds_status_mask_));
    return is_status_dds;
}
}  // namespace vbs
