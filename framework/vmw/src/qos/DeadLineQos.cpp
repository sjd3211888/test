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

#include "vbs/qos/VbsDeadLineQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {
using Duration_t = evbs::ertps::Duration_t;

VbsDeadLineQos::VbsDeadLineQos() : dds_deadline_qos_(new EvbsDeadlineQosPolicy) {}

VbsDeadLineQos::VbsDeadLineQos(const VbsDeadLineQos& qos) : dds_deadline_qos_(new EvbsDeadlineQosPolicy) {
    *this = qos;
}

VbsDeadLineQos::VbsDeadLineQos(VbsDeadLineQos&& qos) : dds_deadline_qos_(new EvbsDeadlineQosPolicy) {
    *this = std::move(qos);
}

VbsDeadLineQos::~VbsDeadLineQos() = default;

VbsDeadLineQos& VbsDeadLineQos::operator=(const VbsDeadLineQos& b) {
    *dds_deadline_qos_ = *b.dds_deadline_qos_;
    return *this;
}

VbsDeadLineQos& VbsDeadLineQos::operator=(VbsDeadLineQos&& b) = default;

bool VbsDeadLineQos::operator==(const VbsDeadLineQos& qos) const {
    return (*dds_deadline_qos_ == *qos.dds_deadline_qos_);
}

void VbsDeadLineQos::clear() {
    VbsDeadLineQos reset = VbsDeadLineQos();
    std::swap(*this, reset);
}

void VbsDeadLineQos::period(uint64_t nsec) {
    dds_deadline_qos_->period = Duration_t(static_cast<double>(nsec));
}

void VbsDeadLineQos::period(int32_t sec, uint32_t nsec) {
    dds_deadline_qos_->period = Duration_t(sec, nsec);
}

uint64_t VbsDeadLineQos::period() const {
    return static_cast<uint64_t>(dds_deadline_qos_->period.to_ns());
}

void VbsDeadLineQos::set_dds_deadline_qos(const EvbsDeadlineQosPolicy& new_dds_deadline_qos) {
    *dds_deadline_qos_ = new_dds_deadline_qos;
}

void VbsDeadLineQos::get_dds_deadline_qos(EvbsDeadlineQosPolicy& new_dds_deadline_qos) const {
    new_dds_deadline_qos = *dds_deadline_qos_;
}

}  // namespace vbs
