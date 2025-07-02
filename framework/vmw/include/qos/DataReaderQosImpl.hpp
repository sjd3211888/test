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

#ifndef INCLUDE_QOS_DATAREADERQOSIMPL_HPP_
#define INCLUDE_QOS_DATAREADERQOSIMPL_HPP_

#include "core/Global.hpp"
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

namespace vbs {
class DataReaderQosImpl {
 public:
    /**
     * @brief Constructor
     */
    DataReaderQosImpl() {}

    const VbsDurabilityQos& durability() const { return reader_durability_; }

    VbsDurabilityQos& durability() { return reader_durability_; }

    void durability(const VbsDurabilityQos& new_durability) { reader_durability_ = new_durability; }

    const VbsDeadLineQos& deadline() const { return reader_deadline_; }

    VbsDeadLineQos& deadline() { return reader_deadline_; }

    void deadline(const VbsDeadLineQos& new_deadline) { reader_deadline_ = new_deadline; }

    const VbsLiveLinessQos& liveliness() const { return reader_liveliness_; }

    VbsLiveLinessQos& liveliness() { return reader_liveliness_; }

    void liveliness(const VbsLiveLinessQos& new_liveliness) { reader_liveliness_ = new_liveliness; }

    const VbsReliabilityQos& reliability() const { return reader_reliability_; }

    VbsReliabilityQos& reliability() { return reader_reliability_; }

    void reliability(const VbsReliabilityQos& new_reliability) { reader_reliability_ = new_reliability; }

    const VbsHistoryQos& history() const { return reader_history_; }

    VbsHistoryQos& history() { return reader_history_; }

    void history(const VbsHistoryQos& new_history) { reader_history_ = new_history; }

    const VbsResourceLimitsQos& resource_limits() const { return reader_resource_limits_; }

    VbsResourceLimitsQos& resource_limits() { return reader_resource_limits_; }

    void resource_limits(const VbsResourceLimitsQos& new_resource_limits) {
        reader_resource_limits_ = new_resource_limits;
    }

    const VbsOwnershipQos& ownership() const { return reader_ownership_; }

    VbsOwnershipQos& ownership() { return reader_ownership_; }

    void ownership(const VbsOwnershipQos& new_ownership) { reader_ownership_ = new_ownership; }

    const VbsLifespanQos& lifespan() const { return reader_lifespan_; }

    VbsLifespanQos& lifespan() { return reader_lifespan_; }

    void lifespan(const VbsLifespanQos& new_lifespan) { reader_lifespan_ = new_lifespan; }

    const VbsRTPSReliableReaderQos& reliable_reader_qos() const { return reader_reliable_reader_qos_; }

    VbsRTPSReliableReaderQos& reliable_reader_qos() { return reader_reliable_reader_qos_; }

    void reliable_reader_qos(const VbsRTPSReliableReaderQos& new_reliable_reader_qos) {
        reader_reliable_reader_qos_ = new_reliable_reader_qos;
    }

    const VbsRTPSEndpointQos& endpoint() const { return reader_endpoint_; }

    VbsRTPSEndpointQos& endpoint() { return reader_endpoint_; }

    void endpoint(const VbsRTPSEndpointQos& new_endpoint) { reader_endpoint_ = new_endpoint; }

    const VbsReaderResourceLimitsQos& reader_resource_limits() const { return reader_reader_resource_limits_; }

    VbsReaderResourceLimitsQos& reader_resource_limits() { return reader_reader_resource_limits_; }

    void reader_resource_limits(const VbsReaderResourceLimitsQos& new_resource_limits) {
        reader_reader_resource_limits_ = new_resource_limits;
    }

    const VbsE2EProtectionQos& e2e_protection() const { return reader_e2e_protection_; }

    VbsE2EProtectionQos& e2e_protection() { return reader_e2e_protection_; }

    void e2e_protection(const VbsE2EProtectionQos& new_e2e_protection) { reader_e2e_protection_ = new_e2e_protection; }

    bool operator==(const DataReaderQosImpl& b) const {
        return (durability() == b.durability()) && (deadline() == b.deadline()) && (liveliness() == b.liveliness()) &&
               (reliability() == b.reliability()) && (history() == b.history()) &&
               (resource_limits() == b.resource_limits()) && (ownership() == b.ownership()) &&
               (lifespan() == b.lifespan()) && (reliable_reader_qos() == b.reliable_reader_qos()) &&
               (endpoint() == b.endpoint()) && (reader_resource_limits() == b.reader_resource_limits()) &&
               (e2e_protection() == b.e2e_protection());
    }

    void dds_reader_qos(const VbsDDSDataReaderQos& local_dds_reader_qos) {
        reader_durability_.set_dds_durability_qos(local_dds_reader_qos.durability());
        reader_deadline_.set_dds_deadline_qos(local_dds_reader_qos.deadline());
        reader_liveliness_.set_dds_liveliness_qos(local_dds_reader_qos.liveliness());
        reader_reliability_.set_dds_reliability_qos(local_dds_reader_qos.reliability());
        reader_history_.set_dds_history_qos(local_dds_reader_qos.history());
        reader_resource_limits_.set_dds_resource_limit_qos(local_dds_reader_qos.resource_limits());
        reader_ownership_.set_dds_ownership_qos(local_dds_reader_qos.ownership());
        reader_lifespan_.set_dds_lifespan_qos(local_dds_reader_qos.lifespan());
        reader_reliable_reader_qos_.set_dds_rtps_reliable_reader_qos(local_dds_reader_qos.reliable_reader_qos());
        reader_endpoint_.set_dds_rtps_endpoint_qos(local_dds_reader_qos.endpoint());
        reader_reader_resource_limits_.set_dds_reader_resource_limits_qos(
            local_dds_reader_qos.reader_resource_limits());
        reader_e2e_protection_.set_dds_e2e_protection_qos(local_dds_reader_qos.e2e_protection());
    }

    VbsDDSDataReaderQos get_dds_reader_qos() const {
        VbsDDSDataReaderQos local_dds_reader_qos;
        reader_durability_.get_dds_durability_qos(local_dds_reader_qos.durability());
        reader_deadline_.get_dds_deadline_qos(local_dds_reader_qos.deadline());
        reader_liveliness_.get_dds_liveliness_qos(local_dds_reader_qos.liveliness());
        reader_reliability_.get_dds_reliability_qos(local_dds_reader_qos.reliability());
        reader_history_.get_dds_history_qos(local_dds_reader_qos.history());
        reader_resource_limits_.get_dds_resource_limit_qos(local_dds_reader_qos.resource_limits());
        reader_ownership_.get_dds_ownership_qos(local_dds_reader_qos.ownership());
        reader_lifespan_.get_dds_lifespan_qos(local_dds_reader_qos.lifespan());
        reader_reliable_reader_qos_.get_dds_rtps_reliable_reader_qos(local_dds_reader_qos.reliable_reader_qos());
        reader_endpoint_.get_dds_rtps_endpoint_qos(local_dds_reader_qos.endpoint());
        reader_reader_resource_limits_.get_dds_reader_resource_limits_qos(
            local_dds_reader_qos.reader_resource_limits());
        reader_e2e_protection_.get_dds_e2e_protection_qos(local_dds_reader_qos.e2e_protection());
        return local_dds_reader_qos;
    }

 private:
    //! Durability Qos, implemented in the library.
    VbsDurabilityQos reader_durability_;

    //! Deadline Qos, implemented in the library.
    VbsDeadLineQos reader_deadline_;

    //! Liveliness Qos, implemented in the library.
    VbsLiveLinessQos reader_liveliness_;

    //! Reliability Qos, implemented in the library.
    VbsReliabilityQos reader_reliability_;

    //! History Qos, implemented in the library.
    VbsHistoryQos reader_history_;

    //! Resource Limits Qos, implemented in the library.
    VbsResourceLimitsQos reader_resource_limits_;

    //! Ownership Qos, implemented in the library.
    VbsOwnershipQos reader_ownership_;

    //! Lifespan Qos (Extension).
    VbsLifespanQos reader_lifespan_;

    //! Reliable reader configuration (Extension)
    VbsRTPSReliableReaderQos reader_reliable_reader_qos_;

    //! Endpoint configuration (Extension)
    VbsRTPSEndpointQos reader_endpoint_;

    //! ReaderResourceLimitsQos
    VbsReaderResourceLimitsQos reader_reader_resource_limits_;

    //! E2EProtectionQosPolicy
    VbsE2EProtectionQos reader_e2e_protection_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_DATAREADERQOSIMPL_HPP_
