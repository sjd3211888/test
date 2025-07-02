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

#ifndef INCLUDE_QOS_DATAWRITERQOSIMPL_HPP_
#define INCLUDE_QOS_DATAWRITERQOSIMPL_HPP_

#include "core/Global.hpp"
#include "vbs/qos/VbsDurabilityQos.hpp"
#include "vbs/qos/VbsDeadLineQos.hpp"
#include "vbs/qos/VbsLiveLinessQos.hpp"
#include "vbs/qos/VbsReliabilityQos.hpp"
#include "vbs/qos/VbsHistoryQos.hpp"
#include "vbs/qos/VbsResourceLimitsQos.hpp"
#include "vbs/qos/VbsOwnershipQos.hpp"
#include "vbs/qos/VbsOwnershipStrengthQos.hpp"
#include "vbs/qos/VbsPublishModeQos.hpp"
#include "vbs/qos/VbsRTPSReliableWriterQos.hpp"
#include "vbs/qos/VbsWriterResourceLimitsQos.hpp"
#include "vbs/qos/VbsLifespanQos.hpp"
#include "vbs/qos/VbsRTPSReliableReaderQos.hpp"
#include "vbs/qos/VbsRTPSEndpointQos.hpp"
#include "vbs/qos/VbsReaderResourceLimitsQos.hpp"
#include "vbs/qos/VbsE2EProtectionQos.hpp"

namespace vbs {
class DataWriterQosImpl {
 public:
    /**
     * @brief Constructor
     */
    DataWriterQosImpl() {}

    const VbsDurabilityQos& durability() const { return writer_durability_; }

    VbsDurabilityQos& durability() { return writer_durability_; }

    void durability(const VbsDurabilityQos& new_durability) { writer_durability_ = new_durability; }

    const VbsDeadLineQos& deadline() const { return writer_deadline_; }

    VbsDeadLineQos& deadline() { return writer_deadline_; }

    void deadline(const VbsDeadLineQos& new_deadline) { writer_deadline_ = new_deadline; }

    const VbsLiveLinessQos& liveliness() const { return writer_liveliness_; }

    VbsLiveLinessQos& liveliness() { return writer_liveliness_; }

    void liveliness(const VbsLiveLinessQos& new_liveliness) { writer_liveliness_ = new_liveliness; }

    const VbsReliabilityQos& reliability() const { return writer_reliability_; }

    VbsReliabilityQos& reliability() { return writer_reliability_; }

    void reliability(const VbsReliabilityQos& new_reliability) { writer_reliability_ = new_reliability; }

    const VbsHistoryQos& history() const { return writer_history_; }

    VbsHistoryQos& history() { return writer_history_; }

    void history(const VbsHistoryQos& new_history) { writer_history_ = new_history; }

    const VbsResourceLimitsQos& resource_limits() const { return writer_resource_limits_; }

    VbsResourceLimitsQos& resource_limits() { return writer_resource_limits_; }

    void resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
        writer_resource_limits_ = new_resource_limits;
    }

    const VbsLifespanQos& lifespan() const { return writer_lifespan_; }

    VbsLifespanQos& lifespan() { return writer_lifespan_; }

    void lifespan(const VbsLifespanQos& new_lifespan) { writer_lifespan_ = new_lifespan; }

    const VbsOwnershipQos& ownership() const { return writer_ownership_; }

    VbsOwnershipQos& ownership() { return writer_ownership_; }

    void ownership(const VbsOwnershipQos& new_ownership) { writer_ownership_ = new_ownership; }

    const VbsOwnershipStrengthQos& ownership_strength() const { return writer_ownership_strength_; }

    VbsOwnershipStrengthQos& ownership_strength() { return writer_ownership_strength_; }

    void ownership_strength(const VbsOwnershipStrengthQos& new_ownershipstrength) {
        writer_ownership_strength_ = new_ownershipstrength;
    }

    const VbsPublishModeQos& publish_mode() const { return writer_publish_mode_; }

    VbsPublishModeQos& publish_mode() { return writer_publish_mode_; }

    void publish_mode(const VbsPublishModeQos& new_publish_mode) { writer_publish_mode_ = new_publish_mode; }

    const VbsRTPSReliableWriterQos& reliable_writer_qos() const { return writer_reliable_writer_qos_; }

    VbsRTPSReliableWriterQos& reliable_writer_qos() { return writer_reliable_writer_qos_; }

    void reliable_writer_qos(const VbsRTPSReliableWriterQos& new_reliable_writer_qos) {
        writer_reliable_writer_qos_ = new_reliable_writer_qos;
    }

    const VbsRTPSEndpointQos& endpoint() const { return writer_endpoint_; }

    VbsRTPSEndpointQos& endpoint() { return writer_endpoint_; }

    void endpoint(const VbsRTPSEndpointQos& new_writer_endpoint) { writer_endpoint_ = new_writer_endpoint; }

    const VbsWriterResourceLimitsQos& writer_resource_limits() const { return writer_writer_resource_limits_; }

    VbsWriterResourceLimitsQos& writer_resource_limits() { return writer_writer_resource_limits_; }

    void writer_resource_limits(const VbsWriterResourceLimitsQos& new_writer_resource_limits) {
        writer_writer_resource_limits_ = new_writer_resource_limits;
    }

    const VbsE2EProtectionQos& e2e_protection() const { return writer_e2e_protection_; }

    VbsE2EProtectionQos& e2e_protection() { return writer_e2e_protection_; }

    void e2e_protection(const VbsE2EProtectionQos& new_writer_e2e_protection) {
        writer_e2e_protection_ = new_writer_e2e_protection;
    }

    bool operator==(const DataWriterQosImpl& b) const {
        return (this->durability() == b.durability()) && (this->deadline() == b.deadline()) &&
               (this->liveliness() == b.liveliness()) && (this->reliability() == b.reliability()) &&
               (this->history() == b.history()) && (this->resource_limits() == b.resource_limits()) &&
               (this->lifespan() == b.lifespan()) && (this->ownership() == b.ownership()) &&
               (this->ownership_strength() == b.ownership_strength()) && (this->publish_mode() == b.publish_mode()) &&
               (this->reliable_writer_qos() == b.reliable_writer_qos()) && (this->endpoint() == b.endpoint()) &&
               (this->writer_resource_limits() == b.writer_resource_limits()) &&
               (this->e2e_protection() == b.e2e_protection());
    }

    void dds_writer_qos(const VbsDDSDataWriterQos& local_dds_writer_qos) {
        writer_durability_.set_dds_durability_qos(local_dds_writer_qos.durability());
        writer_deadline_.set_dds_deadline_qos(local_dds_writer_qos.deadline());
        writer_liveliness_.set_dds_liveliness_qos(local_dds_writer_qos.liveliness());
        writer_reliability_.set_dds_reliability_qos(local_dds_writer_qos.reliability());
        writer_history_.set_dds_history_qos(local_dds_writer_qos.history());
        writer_resource_limits_.set_dds_resource_limit_qos(local_dds_writer_qos.resource_limits());
        writer_lifespan_.set_dds_lifespan_qos(local_dds_writer_qos.lifespan());
        writer_ownership_.set_dds_ownership_qos(local_dds_writer_qos.ownership());
        writer_ownership_strength_.set_dds_ownership_strength_qos(local_dds_writer_qos.ownership_strength());
        writer_publish_mode_.set_dds_publishmode_qos(local_dds_writer_qos.publish_mode());
        writer_reliable_writer_qos_.set_dds_rtps_reliability_writer_qos(local_dds_writer_qos.reliable_writer_qos());
        writer_endpoint_.set_dds_rtps_endpoint_qos(local_dds_writer_qos.endpoint());
        writer_writer_resource_limits_.set_dds_writer_resource_limits_qos(
            local_dds_writer_qos.writer_resource_limits());
        writer_e2e_protection_.set_dds_e2e_protection_qos(local_dds_writer_qos.e2e_protection());
    }

    VbsDDSDataWriterQos get_dds_writer_qos() const {
        VbsDDSDataWriterQos local_dds_writer_qos;
        writer_durability_.get_dds_durability_qos(local_dds_writer_qos.durability());
        writer_deadline_.get_dds_deadline_qos(local_dds_writer_qos.deadline());
        writer_liveliness_.get_dds_liveliness_qos(local_dds_writer_qos.liveliness());
        writer_reliability_.get_dds_reliability_qos(local_dds_writer_qos.reliability());
        writer_history_.get_dds_history_qos(local_dds_writer_qos.history());
        writer_resource_limits_.get_dds_resource_limit_qos(local_dds_writer_qos.resource_limits());
        writer_lifespan_.get_dds_lifespan_qos(local_dds_writer_qos.lifespan());
        writer_ownership_.get_dds_ownership_qos(local_dds_writer_qos.ownership());
        writer_ownership_strength_.get_dds_ownership_strength_qos(local_dds_writer_qos.ownership_strength());
        writer_publish_mode_.get_dds_publishmode_qos(local_dds_writer_qos.publish_mode());
        writer_reliable_writer_qos_.get_dds_rtps_reliability_writer_qos(local_dds_writer_qos.reliable_writer_qos());
        writer_endpoint_.get_dds_rtps_endpoint_qos(local_dds_writer_qos.endpoint());
        writer_writer_resource_limits_.get_dds_writer_resource_limits_qos(
            local_dds_writer_qos.writer_resource_limits());
        writer_e2e_protection_.get_dds_e2e_protection_qos(local_dds_writer_qos.e2e_protection());
        return local_dds_writer_qos;
    }

 private:
    VbsDurabilityQos writer_durability_;

    VbsDeadLineQos writer_deadline_;

    VbsLiveLinessQos writer_liveliness_;

    //! Reliability Qos, implemented in the library.
    VbsReliabilityQos writer_reliability_;

    //! History Qos, implemented in the library.
    VbsHistoryQos writer_history_;

    //! Resource Limits Qos, implemented in the library.
    VbsResourceLimitsQos writer_resource_limits_;

    //! Lifespan Qos, implemented in the library.
    VbsLifespanQos writer_lifespan_;

    //! Ownership Qos, implemented in the library.
    VbsOwnershipQos writer_ownership_;

    //! Ownership Strength Qos, implemented in the library.
    VbsOwnershipStrengthQos writer_ownership_strength_;

    //! Publication Mode Qos, implemented in the library.
    VbsPublishModeQos writer_publish_mode_;

    //! RTPS Reliable Writer Qos
    VbsRTPSReliableWriterQos writer_reliable_writer_qos_;

    //! RTPS Endpoint Qos
    VbsRTPSEndpointQos writer_endpoint_;

    //! Writer Resource Limits Qos
    VbsWriterResourceLimitsQos writer_writer_resource_limits_;

    //! E2E Protection Qos
    VbsE2EProtectionQos writer_e2e_protection_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_DATAWRITERQOSIMPL_HPP_
