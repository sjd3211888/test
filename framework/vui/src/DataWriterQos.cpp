// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#include "qos/DataWriterQosImpl.hpp"
#include "vbs/DataWriterQos.hpp"

namespace vbs {
DataWriterQos::DataWriterQos() {
    writer_qos_ptr_ = std::make_shared<vbs::DataWriterQosImpl>();
}

void DataWriterQos::set_instance(const std::shared_ptr<vbs::DataWriterQosImpl>& writer_qos_impl_ptr) {
    if (writer_qos_ptr_) {
        writer_qos_ptr_ = std::move(writer_qos_impl_ptr);
    }
}

std::shared_ptr<vbs::DataWriterQosImpl> DataWriterQos::get_instance() const {
    return writer_qos_ptr_;
}

DataWriterQos::~DataWriterQos() {}

bool DataWriterQos::operator==(const DataWriterQos& b) const {
    return (this->durability() == b.durability()) && (this->deadline() == b.deadline()) &&
           (this->liveliness() == b.liveliness()) && (this->reliability() == b.reliability()) &&
           (this->history() == b.history()) && (this->resource_limits() == b.resource_limits()) &&
           (this->lifespan() == b.lifespan()) && (this->ownership() == b.ownership()) &&
           (this->ownership_strength() == b.ownership_strength()) && (this->publish_mode() == b.publish_mode()) &&
           (this->e2e_protection() == b.e2e_protection());
}

const VbsDurabilityQos& DataWriterQos::durability() const {
    return writer_qos_ptr_->durability();
}

VbsDurabilityQos& DataWriterQos::durability() {
    return writer_qos_ptr_->durability();
}

void DataWriterQos::durability(const VbsDurabilityQos& new_durability) {
    writer_qos_ptr_->durability(new_durability);
}

const VbsDeadLineQos& DataWriterQos::deadline() const {
    return writer_qos_ptr_->deadline();
}

VbsDeadLineQos& DataWriterQos::deadline() {
    return writer_qos_ptr_->deadline();
}

void DataWriterQos::deadline(const VbsDeadLineQos& new_deadline) {
    writer_qos_ptr_->deadline(new_deadline);
}

const VbsLiveLinessQos& DataWriterQos::liveliness() const {
    return writer_qos_ptr_->liveliness();
}

VbsLiveLinessQos& DataWriterQos::liveliness() {
    return writer_qos_ptr_->liveliness();
}

void DataWriterQos::liveliness(const VbsLiveLinessQos& new_liveliness) {
    writer_qos_ptr_->liveliness(new_liveliness);
}

const VbsReliabilityQos& DataWriterQos::reliability() const {
    return writer_qos_ptr_->reliability();
}

VbsReliabilityQos& DataWriterQos::reliability() {
    return writer_qos_ptr_->reliability();
}

void DataWriterQos::reliability(const VbsReliabilityQos& new_reliability) {
    writer_qos_ptr_->reliability(new_reliability);
}

const VbsHistoryQos& DataWriterQos::history() const {
    return writer_qos_ptr_->history();
}

VbsHistoryQos& DataWriterQos::history() {
    return writer_qos_ptr_->history();
}

void DataWriterQos::history(const VbsHistoryQos& new_history) {
    writer_qos_ptr_->history(new_history);
}

const VbsResourceLimitsQos& DataWriterQos::resource_limits() const {
    return writer_qos_ptr_->resource_limits();
}

VbsResourceLimitsQos& DataWriterQos::resource_limits() {
    return writer_qos_ptr_->resource_limits();
}

void DataWriterQos::resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
    writer_qos_ptr_->resource_limits(new_resource_limits);
}

const VbsLifespanQos& DataWriterQos::lifespan() const {
    return writer_qos_ptr_->lifespan();
}

VbsLifespanQos& DataWriterQos::lifespan() {
    return writer_qos_ptr_->lifespan();
}

void DataWriterQos::lifespan(const VbsLifespanQos& new_lifespan) {
    writer_qos_ptr_->lifespan(new_lifespan);
}

const VbsOwnershipQos& DataWriterQos::ownership() const {
    return writer_qos_ptr_->ownership();
}

VbsOwnershipQos& DataWriterQos::ownership() {
    return writer_qos_ptr_->ownership();
}

void DataWriterQos::ownership(const VbsOwnershipQos& new_ownership) {
    writer_qos_ptr_->ownership(new_ownership);
}

const VbsOwnershipStrengthQos& DataWriterQos::ownership_strength() const {
    return writer_qos_ptr_->ownership_strength();
}

VbsOwnershipStrengthQos& DataWriterQos::ownership_strength() {
    return writer_qos_ptr_->ownership_strength();
}

void DataWriterQos::ownership_strength(const VbsOwnershipStrengthQos& new_ownershipstrength) {
    writer_qos_ptr_->ownership_strength(new_ownershipstrength);
}

const VbsPublishModeQos& DataWriterQos::publish_mode() const {
    return writer_qos_ptr_->publish_mode();
}

VbsPublishModeQos& DataWriterQos::publish_mode() {
    return writer_qos_ptr_->publish_mode();
}

void DataWriterQos::publish_mode(const VbsPublishModeQos& new_publish_mode) {
    writer_qos_ptr_->publish_mode(new_publish_mode);
}

const VbsE2EProtectionQos& DataWriterQos::e2e_protection() const {
    return writer_qos_ptr_->e2e_protection();
}

VbsE2EProtectionQos& DataWriterQos::e2e_protection() {
    return writer_qos_ptr_->e2e_protection();
}

void DataWriterQos::e2e_protection(const VbsE2EProtectionQos& new_writer_e2e_protection) {
    writer_qos_ptr_->e2e_protection(new_writer_e2e_protection);
}

}  // namespace vbs
