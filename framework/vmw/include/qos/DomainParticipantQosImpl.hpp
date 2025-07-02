// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef INCLUDE_QOS_DOMAINPARTICIPANTQOSIMPL_HPP_
#define INCLUDE_QOS_DOMAINPARTICIPANTQOSIMPL_HPP_

#include "ertps/ertps_dll.h"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "edds/rtps/flowcontrol/FlowControllerDescriptor.hpp"
#include "core/Global.hpp"
#include "vbs/qos/VbsWireProtocolConfigQos.hpp"
#include "vbs/qos/VbsTransportConfigQos.hpp"

namespace vbs {
class DomainParticipantQosImpl {
 public:
    /**
     * @brief Constructor
     */
    DomainParticipantQosImpl() {}

    bool operator==(const DomainParticipantQosImpl& b) const {
        return (this->wire_protocol_ == b.wire_protocol()) && (this->transport_ == b.transport()) &&
               (this->name_ == b.name());
    }

    void participant_qos(const DomainParticipantQosInner& local_participant_qos) {
        wire_protocol_.set_dds_wire_protocol_config_qos(local_participant_qos.wire_protocol());
        transport_.set_dds_transport_config_qos(local_participant_qos.transport());
        name_ = local_participant_qos.name();
    }

    DomainParticipantQosInner get_participant_qos() const {
        DomainParticipantQosInner local_participant_qos;
        wire_protocol_.get_dds_wire_protocol_config_qos(local_participant_qos.wire_protocol());
        transport_.get_dds_transport_config_qos(local_participant_qos.transport());
        local_participant_qos.name(name_);
        return local_participant_qos;
    }

    const VbsWireProtocolConfigQos& wire_protocol() const { return wire_protocol_; }

    VbsWireProtocolConfigQos& wire_protocol() { return wire_protocol_; }

    void wire_protocol(const VbsWireProtocolConfigQos& wire_protocol) { wire_protocol_ = wire_protocol; }

    const VbsTransportConfigQos& transport() const { return transport_; }

    VbsTransportConfigQos& transport() { return transport_; }

    void transport(const VbsTransportConfigQos& transport) { transport_ = transport; }

    const std::string& name() const { return name_; }

    std::string& name() { return name_; }

    void name(const std::string& value) { name_ = value; }

 private:
    //! Wire Protocol options
    VbsWireProtocolConfigQos wire_protocol_;

    //! Transport options
    VbsTransportConfigQos transport_;

    //! Name of the participant.
    std::string name_ = "RTPSParticipant";
};

}  // namespace vbs
#endif  // INCLUDE_QOS_DOMAINPARTICIPANTQOSIMPL_HPP_
