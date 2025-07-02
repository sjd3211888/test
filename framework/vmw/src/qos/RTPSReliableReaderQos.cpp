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

#include "vbs/qos/VbsRTPSReliableReaderQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsRTPSReliableReaderQos::VbsRTPSReliableReaderQos() : dds_rtps_reliable_reader_qos_(new EvbsRTPSReliableReaderQos) {}

VbsRTPSReliableReaderQos::VbsRTPSReliableReaderQos(const VbsRTPSReliableReaderQos& qos)
    : dds_rtps_reliable_reader_qos_(new EvbsRTPSReliableReaderQos) {
    *this = qos;
}

VbsRTPSReliableReaderQos::VbsRTPSReliableReaderQos(VbsRTPSReliableReaderQos&& qos)
    : dds_rtps_reliable_reader_qos_(new EvbsRTPSReliableReaderQos) {
    *this = std::move(qos);
}

VbsRTPSReliableReaderQos& VbsRTPSReliableReaderQos::operator=(const VbsRTPSReliableReaderQos& b) {
    *dds_rtps_reliable_reader_qos_ = *b.dds_rtps_reliable_reader_qos_;
    return *this;
}

VbsRTPSReliableReaderQos& VbsRTPSReliableReaderQos::operator=(VbsRTPSReliableReaderQos&& b) = default;

bool VbsRTPSReliableReaderQos::operator==(const VbsRTPSReliableReaderQos& qos) const {
    return (*dds_rtps_reliable_reader_qos_ == *qos.dds_rtps_reliable_reader_qos_);
}

void VbsRTPSReliableReaderQos::clear() {
    VbsRTPSReliableReaderQos reset = VbsRTPSReliableReaderQos();
    std::swap(*this, reset);
}

const EvbsReaderTimes& VbsRTPSReliableReaderQos::times() const {
    return dds_rtps_reliable_reader_qos_->times;
}

const EvbsDisablePositiveACKsQosPolicy& VbsRTPSReliableReaderQos::disable_positive_ACKs() const {
    return dds_rtps_reliable_reader_qos_->disable_positive_ACKs;
}

void VbsRTPSReliableReaderQos::set_dds_rtps_reliable_reader_qos(
    const EvbsRTPSReliableReaderQos& new_dds_rtps_reliable_reader_qos) {
    *dds_rtps_reliable_reader_qos_ = new_dds_rtps_reliable_reader_qos;
}

void VbsRTPSReliableReaderQos::get_dds_rtps_reliable_reader_qos(
    EvbsRTPSReliableReaderQos& new_dds_rtps_reliable_reader_qos) const {
    new_dds_rtps_reliable_reader_qos = *dds_rtps_reliable_reader_qos_;
}

}  // namespace vbs
