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
// feature: Reduce level of DomainParticipantListener
// feature: use Listeners and NexusUnits as singleton
// feature: Add dynamic data type discovery
// ------------------------------------------------------------------

#ifndef INCLUDE_VBS_DOMAINPARTICIPANTLISTENER_HPP_
#define INCLUDE_VBS_DOMAINPARTICIPANTLISTENER_HPP_

#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class DomainParticipant;

class DynamicTypeInformation {
 public:
    DynamicTypeInformation();

    DynamicTypeInformation(const std::string& type_name, const std::string& topic_name)
        : type_name_(type_name), topic_name_(topic_name) {};

    ~DynamicTypeInformation();

    std::string get_type_name() const;

    std::string get_topic_name() const;

 private:
    std::string type_name_;
    std::string topic_name_;
};

class DomainParticipantListener {
 public:
    DomainParticipantListener();

    virtual ~DomainParticipantListener();

    virtual void on_type_info(DomainParticipant* participant, const std::string& type_name,
                              const DynamicTypeInformation& type_info) {

        (void)participant;
        (void)type_name;
        (void)type_info;
    }

    virtual void on_type_discovery(DomainParticipant* participant, const std::string& type_name, VBSDynamicType type) {

        (void)participant;
        (void)type_name;
        (void)type;
    }
};

}  // namespace vbs

#endif  // INCLUDE_VBS_DOMAINPARTICIPANTLISTENER_HPP_
