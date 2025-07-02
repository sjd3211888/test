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

#include "vbs/qos/VbsOwnershipQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

VbsOwnershipQos::VbsOwnershipQos() : dds_owner_ship_qos_(new EvbsOwnershipQosPolicy) {}

VbsOwnershipQos::VbsOwnershipQos(const VbsOwnershipQos& qos) : dds_owner_ship_qos_(new EvbsOwnershipQosPolicy) {
    *this = qos;
}

VbsOwnershipQos::VbsOwnershipQos(VbsOwnershipQos&& qos) : dds_owner_ship_qos_(new EvbsOwnershipQosPolicy) {
    *this = std::move(qos);
}

VbsOwnershipQos& VbsOwnershipQos::operator=(const VbsOwnershipQos& b) {
    *dds_owner_ship_qos_ = *b.dds_owner_ship_qos_;
    return *this;
}

VbsOwnershipQos& VbsOwnershipQos::operator=(VbsOwnershipQos&& b) = default;

bool VbsOwnershipQos::operator==(const VbsOwnershipQos& qos) const {
    return (*dds_owner_ship_qos_ == *qos.dds_owner_ship_qos_);
}

void VbsOwnershipQos::clear() {
    VbsOwnershipQos reset = VbsOwnershipQos();
    std::swap(*this, reset);
}

VbsOwnershipQosPolicyKind VbsOwnershipQos::kind() const {
    return dds_owner_ship_qos_->kind;
}

void VbsOwnershipQos::kind(const VbsOwnershipQosPolicyKind& new_kind) {
    dds_owner_ship_qos_->kind = new_kind;
}

void VbsOwnershipQos::set_dds_ownership_qos(const EvbsOwnershipQosPolicy& new_dds_owner_ship_qos) {
    *dds_owner_ship_qos_ = new_dds_owner_ship_qos;
}

void VbsOwnershipQos::get_dds_ownership_qos(EvbsOwnershipQosPolicy& new_dds_owner_ship_qos) const {
    new_dds_owner_ship_qos = *dds_owner_ship_qos_;
}

}  // namespace vbs
