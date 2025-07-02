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

#ifndef INCLUDE_QOS_TOPICQOSIMPL_HPP_
#define INCLUDE_QOS_TOPICQOSIMPL_HPP_

#include "vbs/qos/VbsDurabilityQos.hpp"
#include "vbs/qos/VbsDeadLineQos.hpp"
#include "vbs/qos/VbsLiveLinessQos.hpp"
#include "vbs/qos/VbsReliabilityQos.hpp"
#include "vbs/qos/VbsHistoryQos.hpp"
#include "vbs/qos/VbsResourceLimitsQos.hpp"
#include "vbs/qos/VbsOwnershipQos.hpp"
#include "vbs/qos/VbsLifespanQos.hpp"
#include "vbs/qos/VbsRTPSReliableReaderQos.hpp"
#include "vbs/qos/VbsRTPSEndpointQos.hpp"
#include "vbs/qos/VbsReaderResourceLimitsQos.hpp"
#include "vbs/qos/VbsE2EProtectionQos.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"

namespace vbs {
class TopicQosImpl {
 public:
    /**
     * @brief Constructor
     */
    TopicQosImpl() {
        dds_is_durability_from_xml_ = false;
        dds_is_deadline_from_xml_ = false;
        dds_is_reliability_from_xml_ = false;
        dds_is_history_from_xml_ = false;
        dds_is_resource_from_xml_ = false;
        dds_is_lifespan_from_xml_ = false;
        dds_is_ownership_from_xml_ = false;
    }

    explicit TopicQosImpl(const TopicQosImpl& b)
        : dds_is_durability_from_xml_(b.dds_is_durability_from_xml_),
          dds_is_deadline_from_xml_(b.dds_is_deadline_from_xml_),
          dds_is_reliability_from_xml_(b.dds_is_reliability_from_xml_),
          dds_is_history_from_xml_(b.dds_is_history_from_xml_),
          dds_is_resource_from_xml_(b.dds_is_resource_from_xml_),
          dds_is_lifespan_from_xml_(b.dds_is_lifespan_from_xml_) {
        topic_durability_ = b.topic_durability_;
        topic_deadline_ = b.topic_deadline_;
        topic_liveliness_ = b.topic_liveliness_;
        topic_reliability_ = b.topic_reliability_;
        topic_history_ = b.topic_history_;
        topic_resource_limits_ = b.topic_resource_limits_;
        topic_lifespan_ = b.topic_lifespan_;
        topic_ownership_ = b.topic_ownership_;
        dds_is_ownership_from_xml_ = b.dds_is_ownership_from_xml_;
    }

    TopicQosImpl& operator=(const TopicQosImpl& b) {
        dds_is_durability_from_xml_ = b.dds_is_durability_from_xml_;
        dds_is_deadline_from_xml_ = b.dds_is_deadline_from_xml_;
        dds_is_reliability_from_xml_ = b.dds_is_reliability_from_xml_;
        dds_is_history_from_xml_ = b.dds_is_history_from_xml_;
        dds_is_resource_from_xml_ = b.dds_is_resource_from_xml_;
        dds_is_lifespan_from_xml_ = b.dds_is_lifespan_from_xml_;
        topic_durability_ = b.topic_durability_;
        topic_deadline_ = b.topic_deadline_;
        topic_liveliness_ = b.topic_liveliness_;
        topic_reliability_ = b.topic_reliability_;
        topic_history_ = b.topic_history_;
        topic_resource_limits_ = b.topic_resource_limits_;
        topic_lifespan_ = b.topic_lifespan_;
        topic_ownership_ = b.topic_ownership_;
        dds_is_ownership_from_xml_ = b.dds_is_ownership_from_xml_;
        return *this;
    }

    const VbsDurabilityQos& durability() const { return topic_durability_; }

    VbsDurabilityQos& durability() { return topic_durability_; }

    void durability(const VbsDurabilityQos& new_durability) { topic_durability_ = new_durability; }

    const bool& durability_from_xml() const { return dds_is_durability_from_xml_; }

    void durability_from_xml(const bool& new_durability_from_xml) {
        dds_is_durability_from_xml_ = new_durability_from_xml;
    }

    const VbsDeadLineQos& deadline() const { return topic_deadline_; }

    VbsDeadLineQos& deadline() { return topic_deadline_; }

    void deadline(const VbsDeadLineQos& new_deadline) { topic_deadline_ = new_deadline; }

    const bool& deadline_from_xml() const { return dds_is_deadline_from_xml_; }

    void deadline_from_xml(const bool& new_deadline_from_xml) { dds_is_deadline_from_xml_ = new_deadline_from_xml; }

    const VbsLiveLinessQos& liveliness() const { return topic_liveliness_; }

    VbsLiveLinessQos& liveliness() { return topic_liveliness_; }

    void liveliness(const VbsLiveLinessQos& new_liveliness) { topic_liveliness_ = new_liveliness; }

    const VbsReliabilityQos& reliability() const { return topic_reliability_; }

    VbsReliabilityQos& reliability() { return topic_reliability_; }

    void reliability(const VbsReliabilityQos& new_reliability) { topic_reliability_ = new_reliability; }

    const bool& reliability_from_xml() const { return dds_is_reliability_from_xml_; }

    void reliability_from_xml(const bool& new_reliability_from_xml) {
        dds_is_reliability_from_xml_ = new_reliability_from_xml;
    }

    const VbsHistoryQos& history() const { return topic_history_; }

    VbsHistoryQos& history() { return topic_history_; }

    void history(const VbsHistoryQos& new_history) { topic_history_ = new_history; }

    const bool& history_from_xml() const { return dds_is_history_from_xml_; }

    void history_from_xml(const bool& new_history_from_xml) { dds_is_history_from_xml_ = new_history_from_xml; }

    const VbsResourceLimitsQos& resource_limits() const { return topic_resource_limits_; }

    VbsResourceLimitsQos& resource_limits() { return topic_resource_limits_; }

    void resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
        topic_resource_limits_ = new_resource_limits;
    }

    const bool& resource_from_xml() const { return dds_is_resource_from_xml_; }

    void resource_from_xml(const bool& new_resource_from_xml) { dds_is_resource_from_xml_ = new_resource_from_xml; }

    const VbsLifespanQos& lifespan() const { return topic_lifespan_; }

    VbsLifespanQos& lifespan() { return topic_lifespan_; }

    void lifespan(const VbsLifespanQos& new_lifespan) { topic_lifespan_ = new_lifespan; }

    const bool& lifespan_from_xml() const { return dds_is_lifespan_from_xml_; }

    void lifespan_from_xml(const bool& new_lifespan_from_xml) { dds_is_lifespan_from_xml_ = new_lifespan_from_xml; }

    const VbsOwnershipQos& ownership() const { return topic_ownership_; }

    VbsOwnershipQos& ownership() { return topic_ownership_; }

    void ownership(const VbsOwnershipQos& new_ownership) { topic_ownership_ = new_ownership; }

    const bool& ownership_from_xml() const { return dds_is_ownership_from_xml_; }

    void ownership_from_xml(const bool& new_ownership_from_xml) { dds_is_ownership_from_xml_ = new_ownership_from_xml; }

    bool operator==(const TopicQosImpl& b) const {
        return (this->durability() == b.durability()) && (this->deadline() == b.deadline()) &&
               (this->liveliness() == b.liveliness()) && (this->reliability() == b.reliability()) &&
               (this->history() == b.history()) && (this->resource_limits() == b.resource_limits()) &&
               (this->lifespan() == b.lifespan()) && (this->ownership() == b.ownership());
    }

    void dds_topic_qos(const vbsutil::xmlparser::TopicQos& local_topic_qos) {
        topic_durability_.set_dds_durability_qos(local_topic_qos.durability());
        dds_is_durability_from_xml_ = local_topic_qos.durability_from_xml();
        topic_deadline_.set_dds_deadline_qos(local_topic_qos.deadline());
        dds_is_deadline_from_xml_ = local_topic_qos.deadline_from_xml();
        topic_liveliness_.set_dds_liveliness_qos(local_topic_qos.liveliness());
        topic_reliability_.set_dds_reliability_qos(local_topic_qos.reliability());
        dds_is_reliability_from_xml_ = local_topic_qos.reliability_from_xml();
        topic_history_.set_dds_history_qos(local_topic_qos.history());
        dds_is_history_from_xml_ = local_topic_qos.history_from_xml();
        topic_resource_limits_.set_dds_resource_limit_qos(local_topic_qos.resource_limits());
        dds_is_resource_from_xml_ = local_topic_qos.resource_from_xml();
        topic_lifespan_.set_dds_lifespan_qos(local_topic_qos.lifespan());
        dds_is_lifespan_from_xml_ = local_topic_qos.lifespan_from_xml();
        topic_ownership_.set_dds_ownership_qos(local_topic_qos.ownership());
        dds_is_ownership_from_xml_ = local_topic_qos.ownership_from_xml();
    }

    vbsutil::xmlparser::TopicQos get_dds_topic_qos() const {
        vbsutil::xmlparser::TopicQos local_dds_topic_qos;
        topic_durability_.get_dds_durability_qos(local_dds_topic_qos.durability());
        local_dds_topic_qos.durability_from_xml(dds_is_durability_from_xml_);
        topic_deadline_.get_dds_deadline_qos(local_dds_topic_qos.deadline());
        local_dds_topic_qos.deadline_from_xml(dds_is_deadline_from_xml_);
        topic_liveliness_.get_dds_liveliness_qos(local_dds_topic_qos.liveliness());
        topic_reliability_.get_dds_reliability_qos(local_dds_topic_qos.reliability());
        local_dds_topic_qos.reliability_from_xml(dds_is_reliability_from_xml_);
        topic_history_.get_dds_history_qos(local_dds_topic_qos.history());
        local_dds_topic_qos.history_from_xml(dds_is_history_from_xml_);
        topic_resource_limits_.get_dds_resource_limit_qos(local_dds_topic_qos.resource_limits());
        local_dds_topic_qos.resource_from_xml(dds_is_resource_from_xml_);
        topic_lifespan_.get_dds_lifespan_qos(local_dds_topic_qos.lifespan());
        local_dds_topic_qos.lifespan_from_xml(dds_is_lifespan_from_xml_);
        topic_ownership_.get_dds_ownership_qos(local_dds_topic_qos.ownership());
        local_dds_topic_qos.ownership_from_xml(dds_is_ownership_from_xml_);
        return local_dds_topic_qos;
    }

 private:
    //! Durability Qos, implemented in the library.
    VbsDurabilityQos topic_durability_;
    bool dds_is_durability_from_xml_;

    //! Deadline Qos, implemented in the library.
    VbsDeadLineQos topic_deadline_;
    bool dds_is_deadline_from_xml_;

    //! Liveliness Qos, implemented in the library.
    VbsLiveLinessQos topic_liveliness_;

    //! Reliability Qos, implemented in the library.
    VbsReliabilityQos topic_reliability_;
    bool dds_is_reliability_from_xml_;

    //! History Qos, implemented in the library.
    VbsHistoryQos topic_history_;
    bool dds_is_history_from_xml_;

    //! Resource Limits Qos, implemented in the library.
    VbsResourceLimitsQos topic_resource_limits_;
    bool dds_is_resource_from_xml_;

    //! Lifespan Qos, implemented in the library.
    VbsLifespanQos topic_lifespan_;
    bool dds_is_lifespan_from_xml_;

    //! Ownership Qos, NOT implemented in the library.
    VbsOwnershipQos topic_ownership_;
    bool dds_is_ownership_from_xml_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_TOPICQOSIMPL_HPP_
