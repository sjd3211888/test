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

#include "vbs/qos/VbsRTPSReliableWriterQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"

namespace vbs {

VbsRTPSReliableWriterQos::VbsRTPSReliableWriterQos()
    : dds_rtps_reliability_writer_qos_(new EvbsRTPSReliableWriterQos) {}

VbsRTPSReliableWriterQos::VbsRTPSReliableWriterQos(const VbsRTPSReliableWriterQos& qos)
    : dds_rtps_reliability_writer_qos_(new EvbsRTPSReliableWriterQos) {
    *this = qos;
}

VbsRTPSReliableWriterQos::VbsRTPSReliableWriterQos(VbsRTPSReliableWriterQos&& qos)
    : dds_rtps_reliability_writer_qos_(new EvbsRTPSReliableWriterQos) {
    *this = std::move(qos);
}

VbsRTPSReliableWriterQos& VbsRTPSReliableWriterQos::operator=(const VbsRTPSReliableWriterQos& b) {
    *dds_rtps_reliability_writer_qos_ = *b.dds_rtps_reliability_writer_qos_;
    return *this;
}

VbsRTPSReliableWriterQos& VbsRTPSReliableWriterQos::operator=(VbsRTPSReliableWriterQos&& b) = default;

bool VbsRTPSReliableWriterQos::operator==(const VbsRTPSReliableWriterQos& qos) const {
    return (*dds_rtps_reliability_writer_qos_ == *qos.dds_rtps_reliability_writer_qos_);
}

void VbsRTPSReliableWriterQos::clear() {
    VbsRTPSReliableWriterQos reset = VbsRTPSReliableWriterQos();
    std::swap(*this, reset);
}

EvbsWriterTimes VbsRTPSReliableWriterQos::times() const {
    return dds_rtps_reliability_writer_qos_->times;
}

EvbsDisablePositiveACKsQosPolicy VbsRTPSReliableWriterQos::disable_positive_acks() const {
    return dds_rtps_reliability_writer_qos_->disable_positive_acks;
}

bool VbsRTPSReliableWriterQos::disable_heartbeat_piggyback() const {
    return dds_rtps_reliability_writer_qos_->disable_heartbeat_piggyback;
}

void VbsRTPSReliableWriterQos::disable_heartbeat_piggyback(const bool& new_heartbeat) {
    dds_rtps_reliability_writer_qos_->disable_heartbeat_piggyback = new_heartbeat;
}

int32_t VbsRTPSReliableWriterQos::heartbeat_per_max_samples() const {
    return dds_rtps_reliability_writer_qos_->heartbeat_per_max_samples;
}

void VbsRTPSReliableWriterQos::heartbeat_per_max_samples(const int32_t& new_heartbeat) {
    dds_rtps_reliability_writer_qos_->heartbeat_per_max_samples = new_heartbeat;
}

void VbsRTPSReliableWriterQos::set_dds_rtps_reliability_writer_qos(
    const EvbsRTPSReliableWriterQos& new_dds_rtps_reliability_writer_qos) {
    *dds_rtps_reliability_writer_qos_ = new_dds_rtps_reliability_writer_qos;
}

void VbsRTPSReliableWriterQos::get_dds_rtps_reliability_writer_qos(
    EvbsRTPSReliableWriterQos& new_dds_rtps_reliability_writer_qos) const {
    new_dds_rtps_reliability_writer_qos = *dds_rtps_reliability_writer_qos_;
}

}  // namespace vbs
