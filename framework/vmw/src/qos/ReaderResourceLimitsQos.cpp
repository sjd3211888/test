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

#include "vbs/qos/VbsReaderResourceLimitsQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsReaderResourceLimitsQos::VbsReaderResourceLimitsQos()
    : dds_reader_resource_limits_qos_(new EvbsReaderResourceLimitsQos) {}

VbsReaderResourceLimitsQos::VbsReaderResourceLimitsQos(const VbsReaderResourceLimitsQos& qos)
    : dds_reader_resource_limits_qos_(new EvbsReaderResourceLimitsQos) {
    *this = qos;
}

VbsReaderResourceLimitsQos::VbsReaderResourceLimitsQos(VbsReaderResourceLimitsQos&& qos)
    : dds_reader_resource_limits_qos_(new EvbsReaderResourceLimitsQos) {
    *this = std::move(qos);
}

VbsReaderResourceLimitsQos& VbsReaderResourceLimitsQos::operator=(const VbsReaderResourceLimitsQos& b) {
    *dds_reader_resource_limits_qos_ = *b.dds_reader_resource_limits_qos_;
    return *this;
}

VbsReaderResourceLimitsQos& VbsReaderResourceLimitsQos::operator=(VbsReaderResourceLimitsQos&& b) = default;

bool VbsReaderResourceLimitsQos::operator==(const VbsReaderResourceLimitsQos& qos) const {
    return (*dds_reader_resource_limits_qos_ == *qos.dds_reader_resource_limits_qos_);
}

void VbsReaderResourceLimitsQos::clear() {
    VbsReaderResourceLimitsQos reset = VbsReaderResourceLimitsQos();
    std::swap(*this, reset);
}

const ResourceLimitedContainerConfig& VbsReaderResourceLimitsQos::matched_publisher_allocation() const {
    return dds_reader_resource_limits_qos_->matched_publisher_allocation;
}

const ResourceLimitedContainerConfig& VbsReaderResourceLimitsQos::sample_infos_allocation() const {
    return dds_reader_resource_limits_qos_->sample_infos_allocation;
}

const ResourceLimitedContainerConfig& VbsReaderResourceLimitsQos::outstanding_reads_allocation() const {
    return dds_reader_resource_limits_qos_->outstanding_reads_allocation;
}

int32_t VbsReaderResourceLimitsQos::max_samples_per_read() const {
    return dds_reader_resource_limits_qos_->max_samples_per_read;
}

void VbsReaderResourceLimitsQos::max_samples_per_read(const int32_t& new_sample) {
    dds_reader_resource_limits_qos_->max_samples_per_read = new_sample;
}

void VbsReaderResourceLimitsQos::set_dds_reader_resource_limits_qos(
    const EvbsReaderResourceLimitsQos& new_dds_reader_resource_limits_qos) {
    *dds_reader_resource_limits_qos_ = new_dds_reader_resource_limits_qos;
}

void VbsReaderResourceLimitsQos::get_dds_reader_resource_limits_qos(
    EvbsReaderResourceLimitsQos& new_dds_reader_resource_limits_qos) const {
    new_dds_reader_resource_limits_qos = *dds_reader_resource_limits_qos_;
}

}  // namespace vbs
