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

#include "vbs/qos/VbsReliabilityQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

using Duration_t = evbs::ertps::Duration_t;
using EvbsReliabilityQosPolicyKind = vbsutil::xmlparser::ReliabilityQosPolicyKind;

VbsReliabilityQos::VbsReliabilityQos()
    : vbs_max_blocking_time_(100000000), vbs_kind_(VbsReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS) {}

VbsReliabilityQos::VbsReliabilityQos(const VbsReliabilityQos& qos) = default;

VbsReliabilityQos::VbsReliabilityQos(VbsReliabilityQos&& qos) = default;

VbsReliabilityQos& VbsReliabilityQos::operator=(const VbsReliabilityQos& b) = default;

VbsReliabilityQos& VbsReliabilityQos::operator=(VbsReliabilityQos&& b) = default;

bool VbsReliabilityQos::operator==(const VbsReliabilityQos& qos) const {
    return ((vbs_max_blocking_time_ == qos.vbs_max_blocking_time_) && (vbs_kind_ == qos.vbs_kind_));
}

void VbsReliabilityQos::clear() {
    VbsReliabilityQos reset = VbsReliabilityQos();
    std::swap(*this, reset);
}

VbsReliabilityQosPolicyKind VbsReliabilityQos::kind() const {
    return vbs_kind_;
}

void VbsReliabilityQos::kind(const VbsReliabilityQosPolicyKind& new_kind) {
    vbs_kind_ = new_kind;
}

int64_t VbsReliabilityQos::max_blocking_time() const {
    return vbs_max_blocking_time_;
}

void VbsReliabilityQos::max_blocking_time(const int64_t& new_max_blocking_time) {
    vbs_max_blocking_time_ = new_max_blocking_time;
}

void VbsReliabilityQos::set_dds_reliability_qos(const EvbsReliabilityQosPolicy& new_dds_reliability_qos) {
    vbs_kind_ = static_cast<VbsReliabilityQosPolicyKind>(new_dds_reliability_qos.kind);
    vbs_max_blocking_time_ = new_dds_reliability_qos.max_blocking_time.to_ns();
}

void VbsReliabilityQos::get_dds_reliability_qos(EvbsReliabilityQosPolicy& new_dds_reliability_qos) const {
    new_dds_reliability_qos.kind = static_cast<EvbsReliabilityQosPolicyKind>(vbs_kind_);
    new_dds_reliability_qos.max_blocking_time = Duration_t(static_cast<double>(vbs_max_blocking_time_));
}

}  // namespace vbs
