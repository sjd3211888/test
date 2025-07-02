// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "qos/TopicQosImpl.hpp"
#include "vbs/TopicQos.hpp"

namespace vbs {
TopicQos::TopicQos() {
    topic_qos_ptr_ = std::make_shared<vbs::TopicQosImpl>();
}

void TopicQos::set_instance(const std::shared_ptr<vbs::TopicQosImpl>& topic_qos_impl_ptr) {
    if (topic_qos_ptr_) {
        topic_qos_ptr_ = std::move(topic_qos_impl_ptr);
    }
}

std::shared_ptr<vbs::TopicQosImpl> TopicQos::get_instance() const {
    return topic_qos_ptr_;
}

TopicQos::~TopicQos() {}

bool TopicQos::operator==(const TopicQos& b) const {
    return (this->durability() == b.durability()) && (this->deadline() == b.deadline()) &&
           (this->liveliness() == b.liveliness()) && (this->reliability() == b.reliability()) &&
           (this->history() == b.history()) && (this->resource_limits() == b.resource_limits()) &&
           (this->lifespan() == b.lifespan()) && (this->ownership() == b.ownership());
}

const VbsDurabilityQos& TopicQos::durability() const {
    return topic_qos_ptr_->durability();
}

VbsDurabilityQos& TopicQos::durability() {
    return topic_qos_ptr_->durability();
}

void TopicQos::durability(const VbsDurabilityQos& new_durability) {
    topic_qos_ptr_->durability(new_durability);
}

const VbsDeadLineQos& TopicQos::deadline() const {
    return topic_qos_ptr_->deadline();
}

VbsDeadLineQos& TopicQos::deadline() {
    return topic_qos_ptr_->deadline();
}

void TopicQos::deadline(const VbsDeadLineQos& new_deadline) {
    topic_qos_ptr_->deadline(new_deadline);
}

const VbsLiveLinessQos& TopicQos::liveliness() const {
    return topic_qos_ptr_->liveliness();
}

VbsLiveLinessQos& TopicQos::liveliness() {
    return topic_qos_ptr_->liveliness();
}

void TopicQos::liveliness(const VbsLiveLinessQos& new_liveliness) {
    topic_qos_ptr_->liveliness(new_liveliness);
}

const VbsReliabilityQos& TopicQos::reliability() const {
    return topic_qos_ptr_->reliability();
}

VbsReliabilityQos& TopicQos::reliability() {
    return topic_qos_ptr_->reliability();
}

void TopicQos::reliability(const VbsReliabilityQos& new_reliability) {
    topic_qos_ptr_->reliability(new_reliability);
}

const VbsHistoryQos& TopicQos::history() const {
    return topic_qos_ptr_->history();
}

VbsHistoryQos& TopicQos::history() {
    return topic_qos_ptr_->history();
}

void TopicQos::history(const VbsHistoryQos& new_history) {
    topic_qos_ptr_->history(new_history);
}

const VbsResourceLimitsQos& TopicQos::resource_limits() const {
    return topic_qos_ptr_->resource_limits();
}

VbsResourceLimitsQos& TopicQos::resource_limits() {
    return topic_qos_ptr_->resource_limits();
}

void TopicQos::resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
    topic_qos_ptr_->resource_limits(new_resource_limits);
}

const VbsLifespanQos& TopicQos::lifespan() const {
    return topic_qos_ptr_->lifespan();
}

VbsLifespanQos& TopicQos::lifespan() {
    return topic_qos_ptr_->lifespan();
}

void TopicQos::lifespan(const VbsLifespanQos& new_lifespan) {
    topic_qos_ptr_->lifespan(new_lifespan);
}

const VbsOwnershipQos& TopicQos::ownership() const {
    return topic_qos_ptr_->ownership();
}

VbsOwnershipQos& TopicQos::ownership() {
    return topic_qos_ptr_->ownership();
}

void TopicQos::ownership(const VbsOwnershipQos& new_ownership) {
    topic_qos_ptr_->ownership(new_ownership);
}

}  // namespace vbs
