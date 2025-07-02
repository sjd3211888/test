// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: (security): Security enabled by default
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_DOMAIN_QOS_DOMAINPARTICIPANTQOS_HPP_
#define INCLUDE_EDDS_DDS_DOMAIN_QOS_DOMAINPARTICIPANTQOS_HPP_

#include <string>

#include "ertps/ertps_dll.h"
#include "edds/dds/core/policy/QosPolicies.hpp"
#include "edds/rtps/flowcontrol/FlowControllerDescriptor.hpp"

namespace vbs {

/**
 * Class DomainParticipantQosInner, contains all the possible Qos that can be set for a determined
 * participant. Please consult each of them to check for implementation details and default values.
 *
 * @ingroup EDDS_QOS_MODULE
 */
class DomainParticipantQosInner {
 public:
    /*!
     * User defined flow controllers to use alongside.
     *
     * @since 2.4.0
     */
    using FlowControllerDescriptorList = std::vector<std::shared_ptr<evbs::edds::rtps::FlowControllerDescriptor>>;

    /**
     * @brief Constructor
     */
    RTPS_DllAPI DomainParticipantQosInner();

    /**
     * @brief Destructor
     */
    RTPS_DllAPI virtual ~DomainParticipantQosInner() {}

    bool operator==(const DomainParticipantQosInner& b) const {
        return (this->allocation_ == b.allocation()) && (this->properties_ == b.properties()) &&
               (this->wire_protocol_ == b.wire_protocol()) && (this->transport_ == b.transport()) &&
               (this->name_ == b.name()) && (this->flow_controllers_ == b.flow_controllers());
    }

    void exportParticipantQosInfo() const;

    /**
     * Getter for ParticipantResourceLimitsQos
     *
     * @return ParticipantResourceLimitsQos reference
     */
    const evbs::edds::dds::ParticipantResourceLimitsQos& allocation() const { return allocation_; }

    /**
     * Getter for ParticipantResourceLimitsQos
     *
     * @return ParticipantResourceLimitsQos reference
     */
    evbs::edds::dds::ParticipantResourceLimitsQos& allocation() { return allocation_; }

    /**
     * Setter for ParticipantResourceLimitsQos
     *
     * @param allocation ParticipantResourceLimitsQos
     */
    void allocation(const evbs::edds::dds::ParticipantResourceLimitsQos& allocation) { allocation_ = allocation; }

    /**
     * Getter for PropertyPolicyQos
     *
     * @return PropertyPolicyQos reference
     */
    const evbs::edds::dds::PropertyPolicyQos& properties() const { return properties_; }

    /**
     * Getter for PropertyPolicyQos
     *
     * @return PropertyPolicyQos reference
     */
    evbs::edds::dds::PropertyPolicyQos& properties() { return properties_; }

    /**
     * Setter for PropertyPolicyQos
     *
     * @param properties PropertyPolicyQos
     */
    void properties(const evbs::edds::dds::PropertyPolicyQos& properties) { properties_ = properties; }

    /**
     * Getter for WireProtocolConfigQos
     *
     * @return WireProtocolConfigQos reference
     */
    const evbs::edds::dds::WireProtocolConfigQos& wire_protocol() const { return wire_protocol_; }

    /**
     * Getter for WireProtocolConfigQos
     *
     * @return WireProtocolConfigQos reference
     */
    evbs::edds::dds::WireProtocolConfigQos& wire_protocol() { return wire_protocol_; }

    /**
     * Setter for WireProtocolConfigQos
     *
     * @param wire_protocol WireProtocolConfigQos
     */
    void wire_protocol(const evbs::edds::dds::WireProtocolConfigQos& wire_protocol) { wire_protocol_ = wire_protocol; }

    /**
     * Getter for TransportConfigQos
     *
     * @return TransportConfigQos reference
     */
    const evbs::edds::dds::TransportConfigQos& transport() const { return transport_; }

    /**
     * Getter for TransportConfigQos
     *
     * @return TransportConfigQos reference
     */
    evbs::edds::dds::TransportConfigQos& transport() { return transport_; }

    /**
     * Setter for TransportConfigQos
     *
     * @param transport TransportConfigQos
     */
    void transport(const evbs::edds::dds::TransportConfigQos& transport) { transport_ = transport; }

    /**
     * Getter for the Participant name
     *
     * @return name
     */
    const evbs::ertps::string_255& name() const { return name_; }

    /**
     * Getter for the Participant name
     *
     * @return name
     */
    evbs::ertps::string_255& name() { return name_; }

    /**
     * Setter for the Participant name
     *
     * @param value New name to be set
     */
    void name(const evbs::ertps::string_255& value) { name_ = value; }

    /**
     * Getter for FlowControllerDescriptorList
     *
     * @return FlowControllerDescriptorList reference
     */
    FlowControllerDescriptorList& flow_controllers() { return flow_controllers_; }

    /**
     * Getter for FlowControllerDescriptorList
     *
     * @return FlowControllerDescriptorList reference
     */
    const FlowControllerDescriptorList& flow_controllers() const { return flow_controllers_; }

 private:
    //! Participant allocation limits
    evbs::edds::dds::ParticipantResourceLimitsQos allocation_;

    //! Property policies
    evbs::edds::dds::PropertyPolicyQos properties_;

    //! Wire Protocol options
    evbs::edds::dds::WireProtocolConfigQos wire_protocol_;

    //! Transport options
    evbs::edds::dds::TransportConfigQos transport_;

    //! Name of the participant.
    evbs::ertps::string_255 name_ = "RTPSParticipant";

    /*! User defined flow controller to use alongside.
     *
     *  @since 2.4.0
     */
    FlowControllerDescriptorList flow_controllers_;
};

inline std::ostream& operator<<(
    std::ostream& output,
    const std::vector<std::shared_ptr<evbs::edds::rtps::FlowControllerDescriptor>>& flow_controllers_) {

    for (const auto& flowController : flow_controllers_) {
        output << " flowController: " << flowController->flowname << " scheduler: " << (int)flowController->scheduler
               << " max_bytes_per_period: " << flowController->max_bytes_per_period
               << " period_ms: " << flowController->period_ms;
    }
    return output;
}

RTPS_DllAPI extern const DomainParticipantQosInner PARTICIPANT_QOS_DEFAULT;

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_DOMAIN_QOS_DOMAINPARTICIPANTQOS_HPP_
