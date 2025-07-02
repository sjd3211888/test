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

#include "vbs/qos/VbsPublishModeQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"

namespace vbs {

VbsPublishModeQos::VbsPublishModeQos() : dds_publishmode_qos_(new EvbsPublishModeQosPolicy) {}

VbsPublishModeQos::VbsPublishModeQos(const VbsPublishModeQos& qos)
    : dds_publishmode_qos_(new EvbsPublishModeQosPolicy) {
    *this = qos;
}

VbsPublishModeQos::VbsPublishModeQos(VbsPublishModeQos&& qos) : dds_publishmode_qos_(new EvbsPublishModeQosPolicy) {
    *this = std::move(qos);
}

VbsPublishModeQos& VbsPublishModeQos::operator=(const VbsPublishModeQos& b) {
    *dds_publishmode_qos_ = *b.dds_publishmode_qos_;
    return *this;
}

VbsPublishModeQos& VbsPublishModeQos::operator=(VbsPublishModeQos&& b) = default;

bool VbsPublishModeQos::operator==(const VbsPublishModeQos& qos) const {
    return (*dds_publishmode_qos_ == *qos.dds_publishmode_qos_);
}

void VbsPublishModeQos::clear() {
    VbsPublishModeQos reset = VbsPublishModeQos();
    std::swap(*this, reset);
}

std::string VbsPublishModeQos::flowname() const {
    return dds_publishmode_qos_->flowname;
}

void VbsPublishModeQos::flowname(const std::string& new_flowname) {
    dds_publishmode_qos_->flowname = new_flowname;
}

void VbsPublishModeQos::set_dds_publishmode_qos(const EvbsPublishModeQosPolicy& new_dds_publishmode_qos) {
    *dds_publishmode_qos_ = new_dds_publishmode_qos;
}

void VbsPublishModeQos::get_dds_publishmode_qos(EvbsPublishModeQosPolicy& new_dds_publishmode_qos) const {
    new_dds_publishmode_qos = *dds_publishmode_qos_;
}

}  // namespace vbs
