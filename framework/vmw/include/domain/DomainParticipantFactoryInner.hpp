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
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTFACTORY_HPP_
#define INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTFACTORY_HPP_

#include <map>
#include <memory>
#include <mutex>

#include "vbs/Utils.hpp"
#include "ertps/attributes/ParticipantAttributes.h"
#include "ertps/types/TypesBase.h"
#include "domain/qos/DomainParticipantQosInner.hpp"
#include "deps/core/status/StatusMask.hpp"

using evbs::ReturnCode_t;
using DomainId_t = uint32_t;

namespace vbs {
class DomainParticipantListenerInner;
class DomainParticipantListener;
class DomainParticipantImpl;
namespace common {
namespace detail {
class TopicPayloadPoolRegistry;
}  // namespace detail
}  // namespace common

/**
 * Class DomainParticipantFactoryInner
 *
 *  @ingroup EDDS_MODULE
 */
class DomainParticipantFactoryInner {
 public:
    /**
     * Returns the DomainParticipantFactoryInner singleton instance.
     *
     * @return A raw pointer to the DomainParticipantFactoryInner singleton instance.
     */
    RTPS_DllAPI static DomainParticipantFactoryInner* get_instance();

    /**
     * Returns the DomainParticipantFactoryInner singleton instance.
     *
     * @return A shared pointer to the DomainParticipantFactoryInner singleton instance.
     */
    RTPS_DllAPI static std::shared_ptr<DomainParticipantFactoryInner> get_shared_instance();

    /**
     * Create a Participant.
     *
     * @param domain_id Domain Id.
     * @param qos DomainParticipantQosInner Reference.
     * @param listener DomainParticipantListener Pointer (default: nullptr)
     * @param mask StatusMask Reference (default: all)
     * @return DomainParticipant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipantImpl* create_participant(
        DomainId_t domain_id, const vbs::DomainParticipantQosInner& qos,
        DomainParticipantListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true,
        DomainParticipantListener* user_listener = nullptr);

    /**
     * Create a Participant.
     *
     * @param domain_id Domain Id.
     * @param profile_name Participant profile name.
     * @param listener DomainParticipantListener Pointer (default: nullptr)
     * @param mask StatusMask Reference (default: all)
     * @return DomainParticipant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipantImpl* create_participant_with_profile(
        DomainId_t domain_id, const std::string& profile_name, DomainParticipantListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true,
        DomainParticipantListener* user_listener = nullptr);

    /**
     * Create a Participant.
     *
     * @param domain_id Domain Id.
     * @param qos Participant qos.
     * @param listener DomainParticipantListener Pointer (default: nullptr)
     * @param mask StatusMask Reference (default: all)
     * @return DomainParticipant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipantImpl* create_participant_with_profile(
        DomainId_t domain_id, const vbs::DomainParticipantQosInner& qos,
        DomainParticipantListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * Create a Participant.
     *
     * @param profile_name Participant profile name.
     * @param listener DomainParticipantListener Pointer (default: nullptr)
     * @param mask StatusMask Reference (default: all)
     * @return DomainParticipant pointer. (nullptr if not created.)
     */
    RTPS_DllAPI DomainParticipantImpl* create_participant_with_profile(
        const std::string& profile_name, DomainParticipantListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true,
        DomainParticipantListener* user_listener = nullptr);

    /**
     * This operation retrieves a previously created DomainParticipant belonging to specified
     * domain_id. If no such DomainParticipant exists, the operation will return 'nullptr'. If
     * multiple DomainParticipant entities belonging to that domain_id exist, then the operation
     * will return one of them. It is not specified which one.
     *
     * @param domain_id
     * @return previously created DomainParticipant within the specified domain
     */
    RTPS_DllAPI DomainParticipantImpl* lookup_participant(DomainId_t domain_id) const;

    /**
     * Returns all participants that belongs to the specified domain_id.
     *
     * @param domain_id
     * @return previously created DomainParticipants within the specified domain
     */
    RTPS_DllAPI std::vector<DomainParticipantImpl*> lookup_participants(DomainId_t domain_id) const;

    /**
     * @brief This operation retrieves the default value of the DomainParticipant QoS, that is, the
     * QoS policies which will be used for newly created DomainParticipant entities in the case
     * where the QoS policies are defaulted in the create_participant operation. The values
     * retrieved get_default_participant_qos will match the set of values specified on the last
     * successful call to set_default_participant_qos, or else, if the call was never made, the
     * default values.
     *
     * @param qos DomainParticipantQosInner where the qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_default_participant_qos(vbs::DomainParticipantQosInner& qos) const;

    /**
     * @brief This operation retrieves the default value of the DomainParticipant QoS, that is, the
     * QoS policies which will be used for newly created DomainParticipant entities in the case
     * where the QoS policies are defaulted in the create_participant operation. The values
     * retrieved get_default_participant_qos will match the set of values specified on the last
     * successful call to set_default_participant_qos, or else, if the call was never made, the
     * default values.
     *
     * @return A reference to the default DomainParticipantQosInner
     */
    RTPS_DllAPI const vbs::DomainParticipantQosInner& get_default_participant_qos() const;

    /**
     * @brief This operation sets a default value of the DomainParticipant QoS policies which will
     * be used for newly created DomainParticipant entities in the case where the QoS policies are
     * defaulted in the create_participant operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return INCONSISTENT_POLICY.
     *
     * The special value PARTICIPANT_QOS_DEFAULT may be passed to this operation to indicate that
     * the default QoS should be reset back to the initial values the factory would use, that is the
     * values that would be used if the set_default_participant_qos operation had never been called.
     *
     * @param qos DomainParticipantQosInner to be set
     * @return RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the
     * qos is changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_default_participant_qos(const vbs::DomainParticipantQosInner& qos);

    /**
     * Fills the DomainParticipantQosInner with the values of the XML profile.
     *
     * @param profile_name DomainParticipant profile name.
     * @param qos DomainParticipantQosInner object where the qos is returned.
     * @return RETCODE_OK if the profile exists. RETCODE_BAD_PARAMETER otherwise.
     */
    RTPS_DllAPI ReturnCode_t get_participant_qos_from_profile(const std::string& profile_name,
                                                              vbs::DomainParticipantQosInner& qos) const;

    /**
     * Remove a Participant and all associated publishers and subscribers.
     *
     * @param part Pointer to the participant.
     * @return RETCODE_PRECONDITION_NOT_MET if the participant has active entities, RETCODE_OK if
     * the participant is correctly deleted and RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t delete_participant(DomainParticipantImpl* part);

    /**
     * Remove a Participant and all associated entities.
     *
     * @param domain_id
     * @return RETCODE_PRECONDITION_NOT_MET if the participant has active entities cannot be
     * deleted, RETCODE_OK if the participant is correctly deleted and RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t delete_participant_and_entities(DomainId_t domain_id);
    /**
     * Load profiles from default XML file.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t load_profiles();

    /**
     * Load profiles from XML file.
     *
     * @param xml_profile_file XML profile file.
     * @return RETCODE_OK if it is correctly loaded, RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t load_XML_profiles_file(const std::string& xml_profile_file);

    /**
     * Load profiles from XML string.
     *
     * @param data buffer containing xml data.
     * @param length length of data
     * @return RETCODE_OK if it is correctly loaded, RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t load_XML_profiles_string(const char* data, size_t length);

    /**
     * Check the validity of the provided static discovery XML file
     *
     * @param xml_file xml file path
     * @return RETCODE_OK if the validation is successful, RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t check_xml_static_discovery(std::string& xml_file);

    /**
     * Get local vbs version in global discovery
     *
     * @return std::string local vbs version, example: VBS-1.2.0-000
     */
    RTPS_DllAPI static std::string get_vbs_version();

 protected:
    friend class DomainParticipantImpl;

    std::map<DomainId_t, std::vector<DomainParticipantImpl*>> participants_;

    DomainParticipantFactoryInner();

    virtual ~DomainParticipantFactoryInner();

    DomainParticipantFactoryInner(const DomainParticipantFactoryInner&) = delete;

    void operator=(const DomainParticipantFactoryInner&) = delete;

    void reset_default_participant_qos();

    void participant_has_been_deleted(DomainParticipantImpl* part);

    mutable std::mutex mtx_participants_;

    mutable bool default_xml_profiles_loaded;

    vbs::DomainParticipantQosInner default_participant_qos_;

    std::shared_ptr<vbs::common::detail::TopicPayloadPoolRegistry> topic_pool_;
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTFACTORY_HPP_
