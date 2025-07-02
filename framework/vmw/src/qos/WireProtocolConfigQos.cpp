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

#include "vbs/qos/VbsWireProtocolConfigQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsWireProtocolConfigQos::VbsWireProtocolConfigQos() : dds_wire_protocol_config_qos_(new EvbsWireProtocolConfigQos) {}

VbsWireProtocolConfigQos::VbsWireProtocolConfigQos(const VbsWireProtocolConfigQos& qos)
    : dds_wire_protocol_config_qos_(new EvbsWireProtocolConfigQos) {
    *this = qos;
}

VbsWireProtocolConfigQos::VbsWireProtocolConfigQos(VbsWireProtocolConfigQos&& qos)
    : dds_wire_protocol_config_qos_(new EvbsWireProtocolConfigQos) {
    *this = std::move(qos);
}

VbsWireProtocolConfigQos::~VbsWireProtocolConfigQos() = default;

VbsWireProtocolConfigQos& VbsWireProtocolConfigQos::operator=(const VbsWireProtocolConfigQos& b) {
    *dds_wire_protocol_config_qos_ = *b.dds_wire_protocol_config_qos_;
    return *this;
}

VbsWireProtocolConfigQos& VbsWireProtocolConfigQos::operator=(VbsWireProtocolConfigQos&& b) = default;

bool VbsWireProtocolConfigQos::operator==(const VbsWireProtocolConfigQos& qos) const {
    return (*dds_wire_protocol_config_qos_ == *qos.dds_wire_protocol_config_qos_);
}

void VbsWireProtocolConfigQos::clear() {
    VbsWireProtocolConfigQos reset = VbsWireProtocolConfigQos();
    std::swap(*this, reset);
}

GuidPrefix_t VbsWireProtocolConfigQos::prefix() const {
    return dds_wire_protocol_config_qos_->prefix;
}

void VbsWireProtocolConfigQos::prefix(const GuidPrefix_t& prefix) {
    dds_wire_protocol_config_qos_->prefix = prefix;
}

int32_t VbsWireProtocolConfigQos::participant_id() const {
    return dds_wire_protocol_config_qos_->participant_id;
}

void VbsWireProtocolConfigQos::participant_id(int32_t id) {
    dds_wire_protocol_config_qos_->participant_id = id;
}

const EvbsBuiltinAttributes& VbsWireProtocolConfigQos::builtin() const {
    return dds_wire_protocol_config_qos_->builtin;
}

const EvbsPortParameters& VbsWireProtocolConfigQos::port() const {
    return dds_wire_protocol_config_qos_->port;
}

const EvbsLocatorList& VbsWireProtocolConfigQos::default_unicast_locator_list() const {
    return dds_wire_protocol_config_qos_->default_unicast_locator_list;
}

const EvbsLocatorList& VbsWireProtocolConfigQos::default_multicast_locator_list() const {
    return dds_wire_protocol_config_qos_->default_multicast_locator_list;
}

const EvbsExternalLocators& VbsWireProtocolConfigQos::default_external_unicast_locators() const {
    return dds_wire_protocol_config_qos_->default_external_unicast_locators;
}

bool VbsWireProtocolConfigQos::ignore_non_matching_locators() const {
    return dds_wire_protocol_config_qos_->ignore_non_matching_locators;
}

void VbsWireProtocolConfigQos::ignore_non_matching_locators(bool flag) const {
    dds_wire_protocol_config_qos_->ignore_non_matching_locators = flag;
}

void VbsWireProtocolConfigQos::set_dds_wire_protocol_config_qos(
    const EvbsWireProtocolConfigQos& new_dds_wire_protocol_config_qos) {
    *dds_wire_protocol_config_qos_ = new_dds_wire_protocol_config_qos;
}

void VbsWireProtocolConfigQos::get_dds_wire_protocol_config_qos(
    EvbsWireProtocolConfigQos& new_dds_wire_protocol_config_qos) const {
    new_dds_wire_protocol_config_qos = *dds_wire_protocol_config_qos_;
}

}  // namespace vbs
