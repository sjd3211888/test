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

#include "vbs/qos/VbsResourceLimitsQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsResourceLimitsQos::VbsResourceLimitsQos() : dds_resource_limit_qos_(new EvbsResourceLimitsQosPolicy) {}

VbsResourceLimitsQos::VbsResourceLimitsQos(const VbsResourceLimitsQos& qos)
    : dds_resource_limit_qos_(new EvbsResourceLimitsQosPolicy) {
    *this = qos;
}

VbsResourceLimitsQos::VbsResourceLimitsQos(VbsResourceLimitsQos&& qos)
    : dds_resource_limit_qos_(new EvbsResourceLimitsQosPolicy) {
    *this = std::move(qos);
}

VbsResourceLimitsQos& VbsResourceLimitsQos::operator=(const VbsResourceLimitsQos& b) {
    *dds_resource_limit_qos_ = *b.dds_resource_limit_qos_;
    return *this;
}

VbsResourceLimitsQos& VbsResourceLimitsQos::operator=(VbsResourceLimitsQos&& b) = default;

bool VbsResourceLimitsQos::operator==(const VbsResourceLimitsQos& qos) const {
    return (*dds_resource_limit_qos_ == *qos.dds_resource_limit_qos_);
}

void VbsResourceLimitsQos::clear() {
    VbsResourceLimitsQos reset = VbsResourceLimitsQos();
    std::swap(*this, reset);
}

int32_t VbsResourceLimitsQos::max_samples() const {
    return dds_resource_limit_qos_->max_samples;
}

void VbsResourceLimitsQos::max_samples(const int32_t& new_max_samples) {
    dds_resource_limit_qos_->max_samples = new_max_samples;
}

int32_t VbsResourceLimitsQos::max_instances() const {
    return dds_resource_limit_qos_->max_instances;
}

void VbsResourceLimitsQos::max_instances(const int32_t& new_max_instances) {
    dds_resource_limit_qos_->max_instances = new_max_instances;
}

int32_t VbsResourceLimitsQos::max_samples_per_instance() const {
    return dds_resource_limit_qos_->max_samples_per_instance;
}

void VbsResourceLimitsQos::max_samples_per_instance(const int32_t& new_mmax_samples_per_instance) {
    dds_resource_limit_qos_->max_samples_per_instance = new_mmax_samples_per_instance;
}

int32_t VbsResourceLimitsQos::allocated_samples() const {
    return dds_resource_limit_qos_->allocated_samples;
}

void VbsResourceLimitsQos::allocated_samples(const int32_t& new_allocated_samples) {
    dds_resource_limit_qos_->allocated_samples = new_allocated_samples;
}

int32_t VbsResourceLimitsQos::extra_samples() const {
    return dds_resource_limit_qos_->extra_samples;
}

void VbsResourceLimitsQos::extra_samples(const int32_t& new_extra_samples) {
    dds_resource_limit_qos_->extra_samples = new_extra_samples;
}

void VbsResourceLimitsQos::set_dds_resource_limit_qos(const EvbsResourceLimitsQosPolicy& new_dds_resource_limit_qos) {
    *dds_resource_limit_qos_ = new_dds_resource_limit_qos;
}

void VbsResourceLimitsQos::get_dds_resource_limit_qos(EvbsResourceLimitsQosPolicy& new_dds_resource_limit_qos) const {
    new_dds_resource_limit_qos = *dds_resource_limit_qos_;
}

}  // namespace vbs
