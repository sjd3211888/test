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

#include "vbs/qos/VbsLiveLinessQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

using Duration_t = evbs::ertps::Duration_t;

VbsLiveLinessQos::VbsLiveLinessQos() : dds_liveliness_qos_(new EvbsLivelinessQosPolicy) {}

VbsLiveLinessQos::VbsLiveLinessQos(const VbsLiveLinessQos& qos) : dds_liveliness_qos_(new EvbsLivelinessQosPolicy) {
    *this = qos;
}

VbsLiveLinessQos::VbsLiveLinessQos(VbsLiveLinessQos&& qos) : dds_liveliness_qos_(new EvbsLivelinessQosPolicy) {
    *this = std::move(qos);
}

VbsLiveLinessQos& VbsLiveLinessQos::operator=(const VbsLiveLinessQos& b) {
    *dds_liveliness_qos_ = *b.dds_liveliness_qos_;
    return *this;
}

VbsLiveLinessQos& VbsLiveLinessQos::operator=(VbsLiveLinessQos&& b) = default;

bool VbsLiveLinessQos::operator==(const VbsLiveLinessQos& qos) const {
    return (*dds_liveliness_qos_ == *qos.dds_liveliness_qos_);
}

void VbsLiveLinessQos::clear() {
    VbsLiveLinessQos reset = VbsLiveLinessQos();
    std::swap(*this, reset);
}

VbsLivelinessQosPolicyKind VbsLiveLinessQos::kind() const {
    return dds_liveliness_qos_->kind;
}

void VbsLiveLinessQos::kind(const VbsLivelinessQosPolicyKind& new_kind) {
    dds_liveliness_qos_->kind = new_kind;
}

int64_t VbsLiveLinessQos::lease_duration() const {
    return dds_liveliness_qos_->lease_duration.to_ns();
}

void VbsLiveLinessQos::lease_duration(const int64_t& new_lease_duration) {
    dds_liveliness_qos_->lease_duration = Duration_t(static_cast<double>(new_lease_duration));
}

int64_t VbsLiveLinessQos::announcement_period() const {
    return dds_liveliness_qos_->announcement_period.to_ns();
}

void VbsLiveLinessQos::announcement_period(const int64_t& new_announcement_period) {
    dds_liveliness_qos_->announcement_period = Duration_t(static_cast<double>(new_announcement_period));
}

void VbsLiveLinessQos::set_dds_liveliness_qos(const EvbsLivelinessQosPolicy& new_dds_liveliness_qos) {
    *dds_liveliness_qos_ = new_dds_liveliness_qos;
}

void VbsLiveLinessQos::get_dds_liveliness_qos(EvbsLivelinessQosPolicy& new_dds_liveliness_qos) const {
    new_dds_liveliness_qos = *dds_liveliness_qos_;
}

}  // namespace vbs
