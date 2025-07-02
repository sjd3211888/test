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

#include "vbs/qos/VbsOwnershipStrengthQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

VbsOwnershipStrengthQos::VbsOwnershipStrengthQos() : dds_ownership_strength_qos_(new EvbsOwnershipStrengthQosPolicy) {}

VbsOwnershipStrengthQos::VbsOwnershipStrengthQos(const VbsOwnershipStrengthQos& qos)
    : dds_ownership_strength_qos_(new EvbsOwnershipStrengthQosPolicy) {
    *this = qos;
}

VbsOwnershipStrengthQos::VbsOwnershipStrengthQos(VbsOwnershipStrengthQos&& qos)
    : dds_ownership_strength_qos_(new EvbsOwnershipStrengthQosPolicy) {
    *this = std::move(qos);
}

VbsOwnershipStrengthQos& VbsOwnershipStrengthQos::operator=(const VbsOwnershipStrengthQos& b) {
    *dds_ownership_strength_qos_ = *b.dds_ownership_strength_qos_;
    return *this;
}

VbsOwnershipStrengthQos& VbsOwnershipStrengthQos::operator=(VbsOwnershipStrengthQos&& b) = default;

bool VbsOwnershipStrengthQos::operator==(const VbsOwnershipStrengthQos& qos) const {
    return (*dds_ownership_strength_qos_ == *qos.dds_ownership_strength_qos_);
}

void VbsOwnershipStrengthQos::clear() {
    VbsOwnershipStrengthQos reset = VbsOwnershipStrengthQos();
    std::swap(*this, reset);
}

uint32_t VbsOwnershipStrengthQos::value() const {
    return dds_ownership_strength_qos_->value;
}

void VbsOwnershipStrengthQos::value(const uint32_t& new_value) {
    dds_ownership_strength_qos_->value = new_value;
}

void VbsOwnershipStrengthQos::set_dds_ownership_strength_qos(
    const EvbsOwnershipStrengthQosPolicy& new_dds_ownership_strength_qos) {
    *dds_ownership_strength_qos_ = new_dds_ownership_strength_qos;
}

void VbsOwnershipStrengthQos::get_dds_ownership_strength_qos(
    EvbsOwnershipStrengthQosPolicy& new_dds_ownership_strength_qos) const {
    new_dds_ownership_strength_qos = *dds_ownership_strength_qos_;
}

}  // namespace vbs
