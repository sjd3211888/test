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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_TOPIC_TOPICDESCRIPTION_HPP_
#define INCLUDE_EDDS_DDS_TOPIC_TOPICDESCRIPTION_HPP_

#include <string>

namespace vbs {

class DomainParticipantImpl;
class TopicDescriptionImpl;

/**
 * Class TopicDescription, represents the fact that both publications
 * and subscriptions are tied to a single data-type
 *
 * @ingroup EDDS_MODULE
 */
class TopicDescription {
 public:
    /**
     * Get the DomainParticipant to which the TopicDescription belongs.
     *
     * @return The DomainParticipant to which the TopicDescription belongs.
     */
    virtual vbs::DomainParticipantImpl* get_participant() const = 0;

    /**
     * Get the name used to create this TopicDescription.
     *
     * @return the name used to create this TopicDescription.
     */
    const std::string& get_name() const { return name_; }

    /**
     * Get the associated type name.
     *
     * @return the type name.
     */
    const std::string& get_type_name() const { return type_name_; }

    virtual vbs::TopicDescriptionImpl* get_impl() const = 0;

 protected:
    TopicDescription(const std::string& name, const std::string& type_name) : name_(name), type_name_(type_name) {}

    virtual ~TopicDescription() {}

 protected:
    //! Name that allows the TopicDescription to be retrieved locally
    std::string name_;

    //! Name that defines a unique resulting type for the publication or the subscription
    std::string type_name_;
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_TOPIC_TOPICDESCRIPTION_HPP_
