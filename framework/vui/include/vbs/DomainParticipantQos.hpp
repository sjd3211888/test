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

#ifndef VBS_DOMAINPARTICIPANTQOS_HPP_
#define VBS_DOMAINPARTICIPANTQOS_HPP_

#include <memory>

#include "vbs/Global.hpp"

namespace vbs {

class DomainParticipantQosImpl;
class DomainParticipantQos {
 public:
    /**
    * @brief Constructor
    */
    DomainParticipantQos();

    /**
    * DomainParticipantQos deconstructor.
    */
    ~DomainParticipantQos();

    /**
    * Sets the instance for the DomainParticipant Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param writer_qos_impl_ptr Shared pointer to the DomainParticipantQosImpl instance to be set.
    */
    void set_instance(const std::shared_ptr<vbs::DomainParticipantQosImpl>& writer_qos_impl_ptr);

    /**
    * Gets the instance for the DomainParticipant Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    */
    std::shared_ptr<vbs::DomainParticipantQosImpl> get_instance() const;

    /**
    * Overload for the equality operator to compare two DomainParticipantQos objects.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param b The DomainParticipantQos object to compare against.
    * @return True if both DomainParticipantQos objects are equal, otherwise false.
    */
    bool operator==(const DomainParticipantQos& b) const;

    const VbsWireProtocolConfigQos& wire_protocol() const;

    VbsWireProtocolConfigQos& wire_protocol();

    void wire_protocol(const VbsWireProtocolConfigQos& wire_protocol);

    const VbsTransportConfigQos& transport() const;

    VbsTransportConfigQos& transport();

    void transport(const VbsTransportConfigQos& transport);

    const std::string& name() const;

    std::string& name();

    void name(const std::string& value);

 private:
    mutable std::shared_ptr<vbs::DomainParticipantQosImpl> participant_qos_ptr_;
};

}  // namespace vbs

#endif  // VBS_DOMAINPARTICIPANTQOS_HPP_
