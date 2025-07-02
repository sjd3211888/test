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

#include "vbs/qos/VbsTransportConfigQos.hpp"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {

VbsTransportConfigQos::VbsTransportConfigQos() : dds_transport_config_qos_(new EvbsTransportConfigQos) {}

VbsTransportConfigQos::VbsTransportConfigQos(const VbsTransportConfigQos& qos)
    : dds_transport_config_qos_(new EvbsTransportConfigQos) {
    *this = qos;
}

VbsTransportConfigQos::VbsTransportConfigQos(VbsTransportConfigQos&& qos)
    : dds_transport_config_qos_(new EvbsTransportConfigQos) {
    *this = std::move(qos);
}

VbsTransportConfigQos::~VbsTransportConfigQos() = default;

VbsTransportConfigQos& VbsTransportConfigQos::operator=(const VbsTransportConfigQos& b) {
    *dds_transport_config_qos_ = *b.dds_transport_config_qos_;
    return *this;
}

VbsTransportConfigQos& VbsTransportConfigQos::operator=(VbsTransportConfigQos&& b) = default;

bool VbsTransportConfigQos::operator==(const VbsTransportConfigQos& qos) const {
    return (*dds_transport_config_qos_ == *qos.dds_transport_config_qos_);
}

void VbsTransportConfigQos::clear() {
    VbsTransportConfigQos reset = VbsTransportConfigQos();
    std::swap(*this, reset);
}

bool VbsTransportConfigQos::use_builtin_transports() const {
    return dds_transport_config_qos_->use_builtin_transports;
}

void VbsTransportConfigQos::use_builtin_transports(bool flag) {
    dds_transport_config_qos_->use_builtin_transports = flag;
}

TransportTypeKind VbsTransportConfigQos::transport_type() const {
    return dds_transport_config_qos_->transport_type;
}

void VbsTransportConfigQos::transport_type(TransportTypeKind kind) {
    dds_transport_config_qos_->transport_type = kind;
}

uint32_t VbsTransportConfigQos::send_socket_buffer_size() const {
    return dds_transport_config_qos_->send_socket_buffer_size;
}

void VbsTransportConfigQos::send_socket_buffer_size(uint32_t sz) {
    dds_transport_config_qos_->send_socket_buffer_size = sz;
}

uint32_t VbsTransportConfigQos::listen_socket_buffer_size() const {
    return dds_transport_config_qos_->listen_socket_buffer_size;
}

void VbsTransportConfigQos::listen_socket_buffer_size(uint32_t sz) {
    dds_transport_config_qos_->listen_socket_buffer_size = sz;
}

void VbsTransportConfigQos::set_dds_transport_config_qos(const EvbsTransportConfigQos& new_dds_transport_config_qos) {
    *dds_transport_config_qos_ = new_dds_transport_config_qos;
}

void VbsTransportConfigQos::get_dds_transport_config_qos(EvbsTransportConfigQos& new_dds_transport_config_qos) const {
    new_dds_transport_config_qos = *dds_transport_config_qos_;
}

}  // namespace vbs
