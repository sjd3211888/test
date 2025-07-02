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

#include "vbs/qos/VbsRTPSEndpointQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsRTPSEndpointQos::VbsRTPSEndpointQos() : dds_rtps_endpoint_qos_(new EvbsRTPSEndpointQos) {}

VbsRTPSEndpointQos::VbsRTPSEndpointQos(const VbsRTPSEndpointQos& qos)
    : dds_rtps_endpoint_qos_(new EvbsRTPSEndpointQos) {
    *this = qos;
}

VbsRTPSEndpointQos::VbsRTPSEndpointQos(VbsRTPSEndpointQos&& qos) : dds_rtps_endpoint_qos_(new EvbsRTPSEndpointQos) {
    *this = std::move(qos);
}

VbsRTPSEndpointQos& VbsRTPSEndpointQos::operator=(const VbsRTPSEndpointQos& b) {
    *dds_rtps_endpoint_qos_ = *b.dds_rtps_endpoint_qos_;
    return *this;
}

VbsRTPSEndpointQos& VbsRTPSEndpointQos::operator=(VbsRTPSEndpointQos&& b) = default;

bool VbsRTPSEndpointQos::operator==(const VbsRTPSEndpointQos& b) const {
    return (*dds_rtps_endpoint_qos_ == *b.dds_rtps_endpoint_qos_);
}

int16_t VbsRTPSEndpointQos::user_defined_id() const {
    return dds_rtps_endpoint_qos_->user_defined_id;
}

void VbsRTPSEndpointQos::user_defined_id(const int16_t& new_d) {
    dds_rtps_endpoint_qos_->user_defined_id = new_d;
}

int16_t VbsRTPSEndpointQos::entity_id() const {
    return dds_rtps_endpoint_qos_->entity_id;
}

void VbsRTPSEndpointQos::entity_id(const int16_t& new_id) {
    dds_rtps_endpoint_qos_->entity_id = new_id;
}

const EvbsLocatorList& VbsRTPSEndpointQos::get_unicast_locator_list() const {
    return dds_rtps_endpoint_qos_->unicast_locator_list;
}

const EvbsLocatorList& VbsRTPSEndpointQos::get_multicast_locator_list() const {
    return dds_rtps_endpoint_qos_->multicast_locator_list;
}

const EvbsLocatorList& VbsRTPSEndpointQos::get_remote_locator_list() const {
    return dds_rtps_endpoint_qos_->remote_locator_list;
}

const EvbsExternalLocators& VbsRTPSEndpointQos::get_external_unicast_locators() const {
    return dds_rtps_endpoint_qos_->external_unicast_locators;
}

void VbsRTPSEndpointQos::set_dds_rtps_endpoint_qos(const EvbsRTPSEndpointQos& new_dds_rtps_endpoint_qos) {
    *dds_rtps_endpoint_qos_ = new_dds_rtps_endpoint_qos;
}

void VbsRTPSEndpointQos::get_dds_rtps_endpoint_qos(EvbsRTPSEndpointQos& new_dds_rtps_endpoint_qos) const {
    new_dds_rtps_endpoint_qos = *dds_rtps_endpoint_qos_;
}

}  // namespace vbs
