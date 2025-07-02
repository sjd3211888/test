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
// feature: change name NewusContext to NexusContext
// feature: Enable trace and log control with udp
// feature: Support dump history TracerMsg to log
// feature: VBS framework adapt to system test tool
// feature: use macro to get timestamp of trace
// feature: add VBSFramework exception code
// feature: VBSFramework log collection
// ------------------------------------------------------------------

#include "vbs/DomainParticipantFactory.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/DomainParticipantFactoryInner.hpp"
#include "context/EnvManager.hpp"
#include "context/NexusContext.hpp"
#include "context/QosManager.hpp"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "deps/core/status/StatusMask.hpp"
#include "qos/DomainParticipantQosImpl.hpp"

namespace vbs {

vbs::DomainParticipant* DomainParticipantFactory::create_participant(const std::string& xml_file_name,
                                                                     const std::string& profile_name,
                                                                     DomainParticipantListener* listener) {
    if ((xml_file_name.length() == 0U) || (profile_name.length() == 0U)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Create particiant failed with xml:" << xml_file_name << ", profile: " << profile_name);
        return nullptr;
    }

    if (!QosManager::Instance()->load_xml(xml_file_name)) {
        return nullptr;
    }

    (void)QosManager::Instance()->get_participant_xml_qos(profile_name);

    auto inner_facotry = DomainParticipantFactoryInner::get_instance();
    DomainParticipantImpl* part_impl = inner_facotry->create_participant_with_profile(
        profile_name, nullptr, evbs::edds::dds::StatusMask::none(), false, listener);
    if (part_impl) {
        auto user_part = new vbs::DomainParticipant();
        user_part->domain_participant_ptr_ = part_impl;
        part_impl->set_user_participant(user_part);
        if (part_impl->enable() == vbsutil::elog::ReturnCode_t::RETCODE_OK) {
            vbs::NexusUnits::get_unit().InsertParticipant(part_impl);
            return user_part;
        }
        user_part->domain_participant_ptr_ = nullptr;
        part_impl->set_user_participant(nullptr);
        inner_facotry->delete_participant(part_impl);
        delete user_part;
    }

    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Create particiant failed with xml:" << xml_file_name << ", profile: " << profile_name);
    return nullptr;
}

vbs::DomainParticipant* DomainParticipantFactory::create_participant(const vbs::DomainId_t& did,
                                                                     const std::string& xml_file_name,
                                                                     const std::string& profile_name,
                                                                     DomainParticipantListener* listener) {
    if ((xml_file_name.length() == 0U) || (profile_name.length() == 0U)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Create particiant failed with xml:" << xml_file_name << ", profile: " << profile_name);
        return nullptr;
    }

    if (!QosManager::Instance()->load_xml(xml_file_name)) {
        return nullptr;
    }

    // util层已有qos管理结构，这里先临时给QosManager赋值，保证用例通过。
    (void)QosManager::Instance()->get_participant_xml_qos(profile_name);

    auto inner_facotry = DomainParticipantFactoryInner::get_instance();
    DomainParticipantImpl* part_impl = inner_facotry->create_participant_with_profile(
        did, profile_name, nullptr, evbs::edds::dds::StatusMask::none(), false, listener);
    if (part_impl) {
        auto user_part = new vbs::DomainParticipant();
        user_part->domain_participant_ptr_ = part_impl;
        part_impl->set_user_participant(user_part);
        if (part_impl->enable() == vbsutil::elog::ReturnCode_t::RETCODE_OK) {
            vbs::NexusUnits::get_unit().InsertParticipant(part_impl);
            return user_part;
        }
        user_part->domain_participant_ptr_ = nullptr;
        part_impl->set_user_participant(nullptr);
        inner_facotry->delete_participant(part_impl);
        delete user_part;
    }

    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Create particiant failed, xml_file_name = " << xml_file_name << ", profile_name = " << profile_name
            << ", did = " << did);
    return nullptr;
}

vbs::DomainParticipant* DomainParticipantFactory::create_participant(const DomainId_t& did,
                                                                     const DomainParticipantQos& domain_participant_qos,
                                                                     DomainParticipantListener* listener) {
    auto inner_facotry = DomainParticipantFactoryInner::get_instance();
    DomainParticipantImpl* part_impl =
        inner_facotry->create_participant(did, domain_participant_qos.get_instance()->get_participant_qos(), nullptr,
                                          evbs::edds::dds::StatusMask::none(), false, listener);
    if (part_impl) {
        auto user_part = new vbs::DomainParticipant();
        user_part->domain_participant_ptr_ = part_impl;
        part_impl->set_user_participant(user_part);
        if (part_impl->enable() == vbsutil::elog::ReturnCode_t::RETCODE_OK) {
            vbs::NexusUnits::get_unit().InsertParticipant(part_impl);
            return user_part;
        }
        user_part->domain_participant_ptr_ = nullptr;
        part_impl->set_user_participant(nullptr);
        inner_facotry->delete_participant(part_impl);
        delete user_part;
    }

    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " Create particiant failed"
                                                             << ", did = " << did);
    return nullptr;
}

std::string DomainParticipantFactory::get_vbs_version(VbsVersionModule vbs_module) {
    (void)vbs_module;
    return DomainParticipantFactoryInner::get_vbs_version();
}

}  // namespace vbs
