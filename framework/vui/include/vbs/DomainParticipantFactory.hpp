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
// feature: remove obv control
// feature: Enable trace and log control with udp
// feature: Support dump history TracerMsg to log
// feature: add api to get api version
// ------------------------------------------------------------------

#ifndef VBS_DOMAINPARTICIPANTFACTORY_HPP_
#define VBS_DOMAINPARTICIPANTFACTORY_HPP_

#include <map>
#include <string>
#include <memory>

#include "vbs/Global.hpp"
#include "vbs/DomainParticipant.hpp"
#include "VBSVersion.hpp"

namespace vbs {

class DomainParticipantFactory {
 public:
    /**
     * Create a DomainParticipant using specified XML file and profile name.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This static function initializes and returns a DomainParticipant by taking an XML configuration file name and a profile name as input.
     *
     * @param xml_file_name The name of the XML configuration file used for initializing the DomainParticipant.
     * @note len(xml_file_name) != 0 && len(xml_file_name) <= 255
     * @param profile_name The name of the profile within the XML configuration file to be applied.
     * @note len(profile_name) != 0
     * @param listener Pointer to the associated DomainParticipantListener. Default is nullptr, indicating no listener.
     * 
     * @return ::vbs::DomainParticipant* Pointer to the newly created DomainParticipant. If the creation fails or any other error occurs, nullptr is returned.
     */
    static ::vbs::DomainParticipant* create_participant(const std::string& xml_file_name,
                                                        const std::string& profile_name,
                                                        DomainParticipantListener* listener = nullptr);

    /**
     * Create a DomainParticipant using specified XML file and profile name.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This static function initializes and returns a DomainParticipant by taking an XML configuration file name and a profile name as input.
     *
     * @param did Domain id.
     * @param xml_file_name The name of the XML configuration file used for initializing the DomainParticipant.
     * @note len(xml_file_name) != 0 && len(xml_file_name) <= 255
     * @param profile_name The name of the profile within the XML configuration file to be applied.
     * @note len(profile_name) != 0
     * @param listener Pointer to the associated DomainParticipantListener. Default is nullptr, indicating no listener.
     * 
     * @return ::vbs::DomainParticipant* Pointer to the newly created DomainParticipant. If the creation fails or any other error occurs, nullptr is returned.
     */
    static ::vbs::DomainParticipant* create_participant(const DomainId_t& did, const std::string& xml_file_name,
                                                        const std::string& profile_name,
                                                        DomainParticipantListener* listener = nullptr);

    /**
     * Create a DomainParticipant using qos.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This static function initializes and returns a DomainParticipant by using a qos object as input.
     *
     * @param did Domain id.
     * @param domain_participant_qos DomainParticipant qos object.
     * @param listener Pointer to the associated DomainParticipantListener. Default is nullptr, indicating no listener.
     * 
     * @return ::vbs::DomainParticipant* Pointer to the newly created DomainParticipant. If the creation fails or any other error occurs, nullptr is returned.
     */
    static ::vbs::DomainParticipant* create_participant(const DomainId_t& did,
                                                        const DomainParticipantQos& domain_participant_qos,
                                                        DomainParticipantListener* listener = nullptr);

    /**
     * This static function get vbs version
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param vbs_module FWK or EVBS or DSF
     * @return vbs version
     */
    static std::string get_vbs_version(VbsVersionModule vbs_module = VbsVersionModule::FWK);

    /**
     * This static function get vbs api version
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @return vbs api version
     */
    static std::string get_vbs_api_version() { return VBS_FWK_API_VERSION; }
};

}  // namespace vbs

#endif  // VBS_DOMAINPARTICIPANTFACTORY_HPP_
