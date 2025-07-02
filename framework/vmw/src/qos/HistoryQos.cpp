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

#include "vbs/qos/VbsHistoryQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

VbsHistoryQos::VbsHistoryQos() : dds_history_qos_(new EvbsHistoryQosPolicy) {}

VbsHistoryQos::VbsHistoryQos(const VbsHistoryQos& qos) : dds_history_qos_(new EvbsHistoryQosPolicy) {
    *this = qos;
}

VbsHistoryQos::VbsHistoryQos(VbsHistoryQos&& qos) : dds_history_qos_(new EvbsHistoryQosPolicy) {
    *this = std::move(qos);
}

VbsHistoryQos& VbsHistoryQos::operator=(const VbsHistoryQos& b) {
    *dds_history_qos_ = *b.dds_history_qos_;
    return *this;
}

VbsHistoryQos& VbsHistoryQos::operator=(VbsHistoryQos&& b) = default;

bool VbsHistoryQos::operator==(const VbsHistoryQos& qos) const {
    return (*dds_history_qos_ == *qos.dds_history_qos_);
}

void VbsHistoryQos::clear() {
    VbsHistoryQos reset = VbsHistoryQos();
    std::swap(*this, reset);
}

VbsHistoryQosPolicyKind VbsHistoryQos::kind() const {
    return dds_history_qos_->kind;
}

void VbsHistoryQos::kind(const VbsHistoryQosPolicyKind& new_kind) {
    dds_history_qos_->kind = new_kind;
}

int32_t VbsHistoryQos::depth() const {
    return dds_history_qos_->depth;
}

void VbsHistoryQos::depth(const int32_t& new_depth) {
    dds_history_qos_->depth = new_depth;
}

void VbsHistoryQos::set_dds_history_qos(const EvbsHistoryQosPolicy& new_dds_history_qos) {
    *dds_history_qos_ = new_dds_history_qos;
}

void VbsHistoryQos::get_dds_history_qos(EvbsHistoryQosPolicy& new_dds_history_qos) const {
    new_dds_history_qos = *dds_history_qos_;
}

}  // namespace vbs
