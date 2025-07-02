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

#include "vbs/qos/VbsDurabilityQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

VbsDurabilityQos::VbsDurabilityQos() : dds_durability_qos_(new EvbsDurabilityQosPolicy) {}

VbsDurabilityQos::VbsDurabilityQos(const VbsDurabilityQos& qos) : dds_durability_qos_(new EvbsDurabilityQosPolicy) {
    *this = qos;
}

VbsDurabilityQos::VbsDurabilityQos(VbsDurabilityQos&& qos) : dds_durability_qos_(new EvbsDurabilityQosPolicy) {
    *this = std::move(qos);
}

VbsDurabilityQos& VbsDurabilityQos::operator=(const VbsDurabilityQos& b) {
    *dds_durability_qos_ = *b.dds_durability_qos_;
    return *this;
}

VbsDurabilityQos& VbsDurabilityQos::operator=(VbsDurabilityQos&& b) = default;

bool VbsDurabilityQos::operator==(const VbsDurabilityQos& b) const {
    return (*dds_durability_qos_ == *b.dds_durability_qos_);
}

void VbsDurabilityQos::clear() {
    VbsDurabilityQos reset = VbsDurabilityQos();
    std::swap(*this, reset);
}

VbsDurabilityKind VbsDurabilityQos::kind() const {
    return dds_durability_qos_->durabilityKind();
}

void VbsDurabilityQos::kind(const VbsDurabilityKind& qos_durability) {
    return dds_durability_qos_->durabilityKind(qos_durability);
}

void VbsDurabilityQos::set_dds_durability_qos(const EvbsDurabilityQosPolicy& new_dds_durability_qos) {
    *dds_durability_qos_ = new_dds_durability_qos;
}

void VbsDurabilityQos::get_dds_durability_qos(EvbsDurabilityQosPolicy& new_dds_durability_qos) const {
    new_dds_durability_qos = *dds_durability_qos_;
}

}  // namespace vbs
