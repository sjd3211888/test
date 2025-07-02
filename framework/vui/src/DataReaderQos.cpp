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

#include "qos/DataReaderQosImpl.hpp"
#include "vbs/DataReaderQos.hpp"

namespace vbs {
DataReaderQos::DataReaderQos() {
    reader_qos_ptr_ = std::make_shared<vbs::DataReaderQosImpl>();
}

void DataReaderQos::set_instance(const std::shared_ptr<vbs::DataReaderQosImpl>& writer_qos_impl_ptr) {
    if (reader_qos_ptr_) {
        reader_qos_ptr_ = std::move(writer_qos_impl_ptr);
    }
}

std::shared_ptr<vbs::DataReaderQosImpl> DataReaderQos::get_instance() const {
    return reader_qos_ptr_;
}

DataReaderQos::~DataReaderQos() {}

bool DataReaderQos::operator==(const DataReaderQos& b) const {
    return (this->durability() == b.durability()) && (this->deadline() == b.deadline()) &&
           (this->liveliness() == b.liveliness()) && (this->reliability() == b.reliability()) &&
           (this->history() == b.history()) && (this->resource_limits() == b.resource_limits()) &&
           (this->lifespan() == b.lifespan()) && (this->ownership() == b.ownership());
}

const VbsDurabilityQos& DataReaderQos::durability() const {
    return reader_qos_ptr_->durability();
}

VbsDurabilityQos& DataReaderQos::durability() {
    return reader_qos_ptr_->durability();
}

void DataReaderQos::durability(const VbsDurabilityQos& new_durability) {
    reader_qos_ptr_->durability(new_durability);
}

const VbsDeadLineQos& DataReaderQos::deadline() const {
    return reader_qos_ptr_->deadline();
}

VbsDeadLineQos& DataReaderQos::deadline() {
    return reader_qos_ptr_->deadline();
}

void DataReaderQos::deadline(const VbsDeadLineQos& new_deadline) {
    reader_qos_ptr_->deadline(new_deadline);
}

const VbsLiveLinessQos& DataReaderQos::liveliness() const {
    return reader_qos_ptr_->liveliness();
}

VbsLiveLinessQos& DataReaderQos::liveliness() {
    return reader_qos_ptr_->liveliness();
}

void DataReaderQos::liveliness(const VbsLiveLinessQos& new_liveliness) {
    reader_qos_ptr_->liveliness(new_liveliness);
}

const VbsReliabilityQos& DataReaderQos::reliability() const {
    return reader_qos_ptr_->reliability();
}

VbsReliabilityQos& DataReaderQos::reliability() {
    return reader_qos_ptr_->reliability();
}

void DataReaderQos::reliability(const VbsReliabilityQos& new_reliability) {
    reader_qos_ptr_->reliability(new_reliability);
}

const VbsHistoryQos& DataReaderQos::history() const {
    return reader_qos_ptr_->history();
}

VbsHistoryQos& DataReaderQos::history() {
    return reader_qos_ptr_->history();
}

void DataReaderQos::history(const VbsHistoryQos& new_history) {
    reader_qos_ptr_->history(new_history);
}

const VbsResourceLimitsQos& DataReaderQos::resource_limits() const {
    return reader_qos_ptr_->resource_limits();
}

VbsResourceLimitsQos& DataReaderQos::resource_limits() {
    return reader_qos_ptr_->resource_limits();
}

void DataReaderQos::resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
    reader_qos_ptr_->resource_limits(new_resource_limits);
}

const VbsLifespanQos& DataReaderQos::lifespan() const {
    return reader_qos_ptr_->lifespan();
}

VbsLifespanQos& DataReaderQos::lifespan() {
    return reader_qos_ptr_->lifespan();
}

void DataReaderQos::lifespan(const VbsLifespanQos& new_lifespan) {
    reader_qos_ptr_->lifespan(new_lifespan);
}

const VbsOwnershipQos& DataReaderQos::ownership() const {
    return reader_qos_ptr_->ownership();
}

VbsOwnershipQos& DataReaderQos::ownership() {
    return reader_qos_ptr_->ownership();
}

void DataReaderQos::ownership(const VbsOwnershipQos& new_ownership) {
    reader_qos_ptr_->ownership(new_ownership);
}

const VbsE2EProtectionQos& DataReaderQos::e2e_protection() const {
    return reader_qos_ptr_->e2e_protection();
}

VbsE2EProtectionQos& DataReaderQos::e2e_protection() {
    return reader_qos_ptr_->e2e_protection();
}

void DataReaderQos::e2e_protection(const VbsE2EProtectionQos& new_reader_e2e_protection) {
    reader_qos_ptr_->e2e_protection(new_reader_e2e_protection);
}

}  // namespace vbs
