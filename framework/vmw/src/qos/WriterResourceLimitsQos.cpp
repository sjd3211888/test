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

#include "vbs/qos/VbsWriterResourceLimitsQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"

namespace vbs {

VbsWriterResourceLimitsQos::VbsWriterResourceLimitsQos()
    : dds_writer_resource_limits_qos_(new EvbsWriterResourceLimitsQos) {}

VbsWriterResourceLimitsQos::VbsWriterResourceLimitsQos(const VbsWriterResourceLimitsQos& qos)
    : dds_writer_resource_limits_qos_(new EvbsWriterResourceLimitsQos) {
    *this = qos;
}

VbsWriterResourceLimitsQos::VbsWriterResourceLimitsQos(VbsWriterResourceLimitsQos&& qos)
    : dds_writer_resource_limits_qos_(new EvbsWriterResourceLimitsQos) {
    *this = std::move(qos);
}

VbsWriterResourceLimitsQos& VbsWriterResourceLimitsQos::operator=(const VbsWriterResourceLimitsQos& b) {
    *dds_writer_resource_limits_qos_ = *b.dds_writer_resource_limits_qos_;
    return *this;
}

VbsWriterResourceLimitsQos& VbsWriterResourceLimitsQos::operator=(VbsWriterResourceLimitsQos&& b) = default;

bool VbsWriterResourceLimitsQos::operator==(const VbsWriterResourceLimitsQos& qos) const {
    return (*dds_writer_resource_limits_qos_ == *qos.dds_writer_resource_limits_qos_);
}

void VbsWriterResourceLimitsQos::clear() {
    VbsWriterResourceLimitsQos reset = VbsWriterResourceLimitsQos();
    std::swap(*this, reset);
}

const EvbsResourceLimitedContainerConfig& VbsWriterResourceLimitsQos::matched_subscriber_allocation() const {
    return dds_writer_resource_limits_qos_->matched_subscriber_allocation;
}

const EvbsResourceLimitedContainerConfig& VbsWriterResourceLimitsQos::reader_filters_allocation() const {
    return dds_writer_resource_limits_qos_->reader_filters_allocation;
}

void VbsWriterResourceLimitsQos::set_dds_writer_resource_limits_qos(
    const EvbsWriterResourceLimitsQos& new_dds_writer_resource_limits_qos) {
    *dds_writer_resource_limits_qos_ = new_dds_writer_resource_limits_qos;
}

void VbsWriterResourceLimitsQos::get_dds_writer_resource_limits_qos(
    EvbsWriterResourceLimitsQos& new_dds_writer_resource_limits_qos) const {
    new_dds_writer_resource_limits_qos = *dds_writer_resource_limits_qos_;
}

}  // namespace vbs
