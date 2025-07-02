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

#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "domain/DomainParticipantFactoryInner.hpp"
#include "edds/rtps/participant/RTPSParticipant.h"
#include "edds/rtps/RTPSDomain.h"
#include "ertps/types/TypeObjectFactory.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "ertps/xmlparser/XMLEndpointParser.h"
#include "elog/Log.hpp"
#include "utils/QosConverters.hpp"
#include "history/TopicPayloadPoolRegistry.hpp"
#include "deps/common/SystemInfo.hpp"
#include "domain/DomainParticipantImpl.hpp"

using evbs::ertps::ParticipantAttributes;
using evbs::ertps::rtps::RTPSDomain;
using evbs::ertps::rtps::RTPSParticipant;
using evbs::ertps::xmlparser::XMLProfileManager;
using vbsutil::elog::Log;
using vbsutil::xmlparser::XMLP_ret;

namespace vbs {

DomainParticipantFactoryInner::DomainParticipantFactoryInner()
    : default_xml_profiles_loaded(false),
      default_participant_qos_(PARTICIPANT_QOS_DEFAULT),
      topic_pool_(vbs::common::TopicPayloadPoolRegistry::instance()) {}

DomainParticipantFactoryInner::~DomainParticipantFactoryInner() {
    {
        const std::lock_guard<std::mutex> guard(mtx_participants_);
        for (auto const& it : participants_) {
            for (const auto pit : it.second) {
                pit->batch_free_ = true;
                pit->disable();
                delete pit;
            }
        }
        participants_.clear();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    vbsutil::elog::Log::KillThread();
}

DomainParticipantFactoryInner* DomainParticipantFactoryInner::get_instance() {
    return get_shared_instance().get();
}

std::shared_ptr<DomainParticipantFactoryInner> DomainParticipantFactoryInner::get_shared_instance() {
    /*
     * The first time an interprocess synchronization object is created by boost, a singleton is
     * instantiated and its destructor is registered with std::atexit(&atexit_work).
     *
     * We need to ensure that the boost singleton is destroyed after the instance of
     * DomainParticipantFactoryInner, to ensure that the interprocess objects keep working until all the
     * participants are destroyed.
     *
     * We achieve this behavior by having an static instance of an auxiliary struct that
     * instantiates a synchronization object on the constructor, just to ensure that the boost
     * singleton is instantiated before the DomainParticipantFactoryInner.
     */
    struct AuxiliaryBoostFunctor {
        AuxiliaryBoostFunctor() {
            try {
                const boost::interprocess::interprocess_mutex mtx;
            } catch (const boost::interprocess::interprocess_exception& e) {
                std::cerr << e.what() << '\n';
            }
        }
    };
    static const AuxiliaryBoostFunctor boost_functor;

    // Note we need a custom deleter, since the destructor is protected.
    static const std::shared_ptr<DomainParticipantFactoryInner> instance(
        new DomainParticipantFactoryInner(), [](DomainParticipantFactoryInner* const p) { delete p; });
    return instance;
}

ReturnCode_t DomainParticipantFactoryInner::delete_participant(DomainParticipantImpl* part) {
    if (part != nullptr) {
        bool isFind = false;

        {
            const std::lock_guard<std::mutex> guard(mtx_participants_);
            if (part->has_active_entities()) {
                elogWarning(PARTICIPANT,
                            part->get_domain_id()
                                << " Participant delete fail. please delete pub/sub、writer/reader firsrtly.");
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
            }
            using VectorIt = std::map<DomainId_t, std::vector<DomainParticipantImpl*>>::iterator;
            VectorIt vit = participants_.find(part->get_domain_id());

            if (vit != participants_.end()) {
                using PartVectorIt = std::vector<DomainParticipantImpl*>::iterator;
                for (PartVectorIt pit = vit->second.begin(); pit != vit->second.end();) {
                    if (((*pit) == part) || ((*pit)->guid() == part->guid())) {
                        (*pit)->disable();
                        const PartVectorIt next_it = vit->second.erase(pit);
                        pit = next_it;
                        isFind = true;
                        break;
                    } else {
                        ++pit;
                    }
                }
                if (vit->second.empty()) {
                    (void)participants_.erase(vit);
                }
            }
        }

        if (isFind) {
            delete part;
            return ReturnCode_t::RETCODE_OK;
        }
        elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, part->get_domain_id() << " Participant delete fail.");
    }
    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DomainParticipantFactoryInner::delete_participant_and_entities(DomainId_t domain_id) {
    DomainParticipantImpl* part = nullptr;
    DomainParticipantImpl* del_part = nullptr;

    part = lookup_participant(domain_id);
    if (part != nullptr) {
        const std::lock_guard<std::mutex> guard(mtx_participants_);
        if (part->has_active_entities()) {
            if (RetCode_t(part->delete_contained_entities()()) != ReturnCode_t::RETCODE_OK) {
                elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR,
                          "Entities of domain id " << domain_id << " cannot be delete!");
                return ReturnCode_t::RETCODE_ERROR;
            }
        }

        using VectorIt = std::map<DomainId_t, std::vector<DomainParticipantImpl*>>::iterator;
        VectorIt vit = participants_.find(part->get_domain_id());

        if (vit != participants_.end()) {
            using PartVectorIt = std::vector<DomainParticipantImpl*>::iterator;
            for (PartVectorIt pit = vit->second.begin(); pit != vit->second.end();) {
                if (((*pit) == part) || ((*pit)->guid() == part->guid())) {
                    (*pit)->disable();
                    del_part = *pit;
                    const PartVectorIt next_it = vit->second.erase(pit);
                    pit = next_it;
                    break;
                } else {
                    ++pit;
                }
            }
            if (vit->second.empty()) {
                (void)participants_.erase(vit);
            }
        }
    }

    if (del_part != nullptr) {
        delete del_part;
        return ReturnCode_t::RETCODE_OK;
    }
    elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Domain id " << domain_id << " is not exist!");
    return ReturnCode_t::RETCODE_ERROR;
}

DomainParticipantImpl* DomainParticipantFactoryInner::create_participant(
    evbs::edds::dds::DomainId_t did, const vbs::DomainParticipantQosInner& qos, DomainParticipantListenerInner* listen,
    const evbs::edds::dds::StatusMask& mask, bool auto_enable, DomainParticipantListener* user_listener) {
    if (did > 160U) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Domain id " << did << " illegal, and limited to [0, 160].");
        return nullptr;
    }
    (void)load_profiles();

    const vbs::DomainParticipantQosInner& pqos = (&qos == &PARTICIPANT_QOS_DEFAULT) ? default_participant_qos_ : qos;

    logDebug(PARTICIPANT, "Participant creating, domain:" << did << " listener:" << (listen != nullptr)
                                                          << " mask:" << mask << " enable:" << auto_enable);

    DomainParticipantImpl* dom_part = new DomainParticipantImpl(did, pqos, mask, listen);

    pqos.exportParticipantQosInfo();
    {
        const std::lock_guard<std::mutex> guard(mtx_participants_);
        using VectorIt = std::map<DomainId_t, std::vector<DomainParticipantImpl*>>::iterator;
        VectorIt vector_it = participants_.find(did);

        if (vector_it == participants_.end()) {
            // Insert the vector
            std::vector<DomainParticipantImpl*> new_vector;
            auto pair_it = participants_.insert(std::make_pair(did, std::move(new_vector)));
            if (!pair_it.second || pair_it.first == participants_.end()) {
                elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Insert participants_ fail.");
                delete dom_part;
                return nullptr;
            }
            pair_it.first->second.push_back(dom_part);
        } else {
            vector_it->second.push_back(dom_part);
        }
    }

    dom_part->set_user_listener(user_listener);
    // auto enable
    if (auto_enable) {
        if (ReturnCode_t::RETCODE_OK != dom_part->enable()) {
            (void)delete_participant(dom_part);
            return nullptr;
        }
    }
    return dom_part;
}

DomainParticipantImpl* DomainParticipantFactoryInner::create_participant_with_profile(
    DomainId_t did, const std::string& profile_name, DomainParticipantListenerInner* listen,
    const evbs::edds::dds::StatusMask& mask, bool auto_enable, DomainParticipantListener* user_listener) {
    (void)load_profiles();

    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    evbs::ertps::ParticipantAttributes attr;
    DomainParticipantImpl* participant = nullptr;

    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr)) {
        vbs::DomainParticipantQosInner qos = default_participant_qos_;
        utils::set_qos_from_attributes(qos, attr.rtps);
        participant = create_participant(did, qos, listen, mask, auto_enable, user_listener);
    }
    if (participant != nullptr) {
        logInfo(PARTICIPANT, "Participant created, domain:" << did << " profile name:" << profile_name
                                                            << " guid:" << participant->guid());
    }
    return participant;
}

DomainParticipantImpl* DomainParticipantFactoryInner::create_participant_with_profile(
    DomainId_t did, const vbs::DomainParticipantQosInner& qos, DomainParticipantListenerInner* listen,
    const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    (void)load_profiles();

    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles

    DomainParticipantImpl* participant = nullptr;
    participant = create_participant(did, qos, listen, mask, auto_enable);
    if (participant != nullptr) {
        logInfo(PARTICIPANT, "Participant created, domain:" << did << " guid:" << participant->guid());
    }
    return participant;
}

DomainParticipantImpl* DomainParticipantFactoryInner::create_participant_with_profile(
    const std::string& profile_name, DomainParticipantListenerInner* listen, const evbs::edds::dds::StatusMask& mask,
    bool auto_enable, DomainParticipantListener* user_listener) {
    (void)load_profiles();

    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    evbs::ertps::ParticipantAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr)) {
        vbs::DomainParticipantQosInner qos = default_participant_qos_;
        utils::set_qos_from_attributes(qos, attr.rtps);
        return create_participant(attr.domainId, qos, listen, mask, auto_enable, user_listener);
    }

    return nullptr;
}

DomainParticipantImpl* DomainParticipantFactoryInner::lookup_participant(DomainId_t domain_id) const {
    std::lock_guard<std::mutex> guard(mtx_participants_);

    auto it = participants_.find(domain_id);
    if ((it != participants_.end()) && (it->second.size() > 0)) {
        return it->second.front();
    }

    return nullptr;
}

std::vector<DomainParticipantImpl*> DomainParticipantFactoryInner::lookup_participants(DomainId_t domain_id) const {
    std::lock_guard<std::mutex> guard(mtx_participants_);

    std::vector<DomainParticipantImpl*> result;
    auto it = participants_.find(domain_id);
    if (it != participants_.end()) {
        const std::vector<DomainParticipantImpl*>& v = it->second;
        for (auto pit = v.begin(); pit != v.end(); ++pit) {
            result.push_back((*pit));
        }
    }

    return result;
}

ReturnCode_t DomainParticipantFactoryInner::get_default_participant_qos(vbs::DomainParticipantQosInner& qos) const {
    qos = default_participant_qos_;
    return ReturnCode_t::RETCODE_OK;
}

const vbs::DomainParticipantQosInner& DomainParticipantFactoryInner::get_default_participant_qos() const {
    return default_participant_qos_;
}

ReturnCode_t DomainParticipantFactoryInner::set_default_participant_qos(const vbs::DomainParticipantQosInner& qos) {
    if (&qos == &PARTICIPANT_QOS_DEFAULT) {
        reset_default_participant_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = ReturnCode_t(DomainParticipantImpl::check_qos(qos)());
    if (!ret_val) {
        return ret_val;
    }
    (void)DomainParticipantImpl::set_qos(default_participant_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryInner::get_participant_qos_from_profile(
    const std::string& profile_name, vbs::DomainParticipantQosInner& qos) const {
    evbs::ertps::ParticipantAttributes attr;
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile_name, attr)) {
        qos = default_participant_qos_;
        utils::set_qos_from_attributes(qos, attr.rtps);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantFactoryInner::load_profiles() {
    const std::lock_guard<std::mutex> guard(mtx_participants_);
    if (false == default_xml_profiles_loaded) {
        (void)vbs::SystemInfo::set_environment_file();
        XMLProfileManager::loadDefaultXMLFile();
        // Only load profile once
        default_xml_profiles_loaded = true;

        // Only change default participant qos when not explicitly set by the user
        if (default_participant_qos_ == PARTICIPANT_QOS_DEFAULT) {
            reset_default_participant_qos();
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryInner::load_XML_profiles_file(const std::string& xml_profile_file) {
    if (XMLP_ret::XML_ERROR == XMLProfileManager::loadXMLFile(xml_profile_file)) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Problem loading XML file '" << xml_profile_file << "'");
        return ReturnCode_t::RETCODE_ERROR;
    }
    return ReturnCode_t::RETCODE_OK;
}
ReturnCode_t DomainParticipantFactoryInner::load_XML_profiles_string(const char* data, size_t length) {
    if (XMLP_ret::XML_ERROR == XMLProfileManager::loadXMLString(data, length)) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Problem loading XML string");
        return ReturnCode_t::RETCODE_ERROR;
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryInner::check_xml_static_discovery(std::string& xml_file) {
    evbs::ertps::xmlparser::XMLEndpointParser parser;
    if (XMLP_ret::XML_OK != parser.loadXMLFile(xml_file)) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Error parsing xml file");
        return ReturnCode_t::RETCODE_ERROR;
    }
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantFactoryInner::participant_has_been_deleted(DomainParticipantImpl* part) {
    std::lock_guard<std::mutex> guard(mtx_participants_);
    auto it = participants_.find(part->get_domain_id());
    if (it != participants_.end()) {
        for (auto pit = it->second.begin(); (pit != it->second.end());) {
            if (((*pit) == part) || ((*pit)->guid() == part->guid())) {
                pit = it->second.erase(pit);
            } else {
                ++pit;
            }
        }
        if (it->second.empty()) {
            (void)participants_.erase(it);
        }
    }
}

void DomainParticipantFactoryInner::reset_default_participant_qos() {
    // TODO (Miguel C): Change when we have full XML support for DDS QoS profiles
    (void)DomainParticipantImpl::set_qos(default_participant_qos_, PARTICIPANT_QOS_DEFAULT, true);
    if (true == default_xml_profiles_loaded) {
        evbs::ertps::ParticipantAttributes attr;
        XMLProfileManager::getDefaultParticipantAttributes(attr);
        utils::set_qos_from_attributes(default_participant_qos_, attr.rtps);
    }
}

std::string DomainParticipantFactoryInner::get_vbs_version() {
    return VBS_VERSION_VALUE;
}

} /* namespace vbs */
