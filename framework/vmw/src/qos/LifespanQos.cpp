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

#include "vbs/qos/VbsLifespanQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

using Duration_t = evbs::ertps::Duration_t;

VbsLifespanQos::VbsLifespanQos() : dds_life_span_qos_(new EvbsLifespanQosPolicy) {}

VbsLifespanQos::VbsLifespanQos(const VbsLifespanQos& qos) : dds_life_span_qos_(new EvbsLifespanQosPolicy) {
    *this = qos;
}

VbsLifespanQos::VbsLifespanQos(VbsLifespanQos&& qos) : dds_life_span_qos_(new EvbsLifespanQosPolicy) {
    *this = std::move(qos);
}

VbsLifespanQos& VbsLifespanQos::operator=(const VbsLifespanQos& b) {
    *dds_life_span_qos_ = *b.dds_life_span_qos_;
    return *this;
}

VbsLifespanQos& VbsLifespanQos::operator=(VbsLifespanQos&& b) = default;

bool VbsLifespanQos::operator==(const VbsLifespanQos& qos) const {
    return (*dds_life_span_qos_ == *qos.dds_life_span_qos_);
}

void VbsLifespanQos::clear() {
    VbsLifespanQos reset = VbsLifespanQos();
    std::swap(*this, reset);
}

int64_t VbsLifespanQos::duration() const {
    return dds_life_span_qos_->duration.to_ns();
}

void VbsLifespanQos::duration(const int64_t& new_duration) {
    dds_life_span_qos_->duration = Duration_t(static_cast<double>(new_duration));
}

void VbsLifespanQos::set_dds_lifespan_qos(const EvbsLifespanQosPolicy& new_dds_life_span_qos) {
    *dds_life_span_qos_ = new_dds_life_span_qos;
}

void VbsLifespanQos::get_dds_lifespan_qos(EvbsLifespanQosPolicy& new_dds_life_span_qos) const {
    new_dds_life_span_qos = *dds_life_span_qos_;
}

}  // namespace vbs
