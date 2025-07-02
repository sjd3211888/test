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

#include "qos/DomainParticipantQosImpl.hpp"
#include "vbs/DomainParticipantQos.hpp"

namespace vbs {
DomainParticipantQos::DomainParticipantQos() {
    participant_qos_ptr_ = std::make_shared<vbs::DomainParticipantQosImpl>();
}

void DomainParticipantQos::set_instance(
    const std::shared_ptr<vbs::DomainParticipantQosImpl>& participant_qos_impl_ptr) {
    if (participant_qos_ptr_) {
        participant_qos_ptr_ = std::move(participant_qos_impl_ptr);
    }
}

std::shared_ptr<vbs::DomainParticipantQosImpl> DomainParticipantQos::get_instance() const {
    return participant_qos_ptr_;
}

DomainParticipantQos::~DomainParticipantQos() {}

bool DomainParticipantQos::operator==(const DomainParticipantQos& b) const {
    return (this->wire_protocol() == b.wire_protocol()) && (this->transport() == b.transport()) &&
           (this->name() == b.name());
}

const VbsWireProtocolConfigQos& DomainParticipantQos::wire_protocol() const {
    return participant_qos_ptr_->wire_protocol();
}

VbsWireProtocolConfigQos& DomainParticipantQos::wire_protocol() {
    return participant_qos_ptr_->wire_protocol();
}

void DomainParticipantQos::wire_protocol(const VbsWireProtocolConfigQos& wire_protocol) {
    participant_qos_ptr_->wire_protocol(wire_protocol);
}

const VbsTransportConfigQos& DomainParticipantQos::transport() const {
    return participant_qos_ptr_->transport();
}

VbsTransportConfigQos& DomainParticipantQos::transport() {
    return participant_qos_ptr_->transport();
}

void DomainParticipantQos::transport(const VbsTransportConfigQos& transport) {
    participant_qos_ptr_->transport(transport);
}

const std::string& DomainParticipantQos::name() const {
    return participant_qos_ptr_->name();
}

std::string& DomainParticipantQos::name() {
    return participant_qos_ptr_->name();
}

void DomainParticipantQos::name(const std::string& value) {
    participant_qos_ptr_->name(value);
}

}  // namespace vbs
