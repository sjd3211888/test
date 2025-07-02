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

#include "vbs/qos/VbsE2EProtectionQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {
VbsE2EProtectionQos::VbsE2EProtectionQos() : dds_e2e_protection_qos_(new EvbsE2EProtectionQosPolicy) {}

VbsE2EProtectionQos::VbsE2EProtectionQos(const VbsE2EProtectionQos& qos)
    : dds_e2e_protection_qos_(new EvbsE2EProtectionQosPolicy) {
    *this = qos;
}

VbsE2EProtectionQos::VbsE2EProtectionQos(VbsE2EProtectionQos&& qos)
    : dds_e2e_protection_qos_(new EvbsE2EProtectionQosPolicy) {
    *this = std::move(qos);
}

VbsE2EProtectionQos& VbsE2EProtectionQos::operator=(const VbsE2EProtectionQos& b) {
    *dds_e2e_protection_qos_ = *b.dds_e2e_protection_qos_;
    return *this;
}

VbsE2EProtectionQos& VbsE2EProtectionQos::operator=(VbsE2EProtectionQos&& b) {
    dds_e2e_protection_qos_ = std::move(b.dds_e2e_protection_qos_);
    return *this;
}

bool VbsE2EProtectionQos::operator==(const VbsE2EProtectionQos& qos) const {
    return (*dds_e2e_protection_qos_ == *qos.dds_e2e_protection_qos_);
}

void VbsE2EProtectionQos::clear() {
    VbsE2EProtectionQos reset = VbsE2EProtectionQos();
    std::swap(*this, reset);
}

bool VbsE2EProtectionQos::enabled() const {
    return dds_e2e_protection_qos_->enabled;
}

void VbsE2EProtectionQos::enabled(const bool& new_enable) {
    dds_e2e_protection_qos_->enabled = new_enable;
}

uint16_t VbsE2EProtectionQos::MinDataLength() const {
    return dds_e2e_protection_qos_->MinDataLength;
}

void VbsE2EProtectionQos::MinDataLength(const uint16_t& new_length) {
    dds_e2e_protection_qos_->MinDataLength = new_length;
}

uint16_t VbsE2EProtectionQos::MaxDataLength() const {
    return dds_e2e_protection_qos_->MaxDataLength;
}

void VbsE2EProtectionQos::MaxDataLength(const uint16_t& new_length) {
    dds_e2e_protection_qos_->MaxDataLength = new_length;
}

uint16_t VbsE2EProtectionQos::MaxDeltaCounter() const {
    return dds_e2e_protection_qos_->MaxDeltaCounter;
}

void VbsE2EProtectionQos::MaxDeltaCounter(const uint16_t& new_counter) {
    dds_e2e_protection_qos_->MaxDeltaCounter = new_counter;
}

void VbsE2EProtectionQos::set_dds_e2e_protection_qos(const EvbsE2EProtectionQosPolicy& new_dds_e2e_protection_qos) {
    *dds_e2e_protection_qos_ = new_dds_e2e_protection_qos;
}

void VbsE2EProtectionQos::get_dds_e2e_protection_qos(EvbsE2EProtectionQosPolicy& new_dds_e2e_protection_qos) const {
    new_dds_e2e_protection_qos = *dds_e2e_protection_qos_;
}

}  // namespace vbs
