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
// feature: change name NewusContext to NexusContext
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API
// 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: Reduce level of DomainParticipantListener
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// feature: Enable trace and log control with udp
// feature: VBS framework adapt to system test tool
// feature: separating interface and implementation about DomainParticipant
// feature: use Listeners and NexusUnits as singleton
// feature: add VBSFramework exception code
// feature: fwk adds a typename-less initialization topic interface
// ------------------------------------------------------------------

#include <variant>

#include "vbs/DataReader.hpp"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "domain/DomainParticipantImpl.hpp"
#include "vbs/status/StatusMask.hpp"
#include "vbs/DomainParticipant.hpp"
#include "vbs/ContentFilteredTopic.hpp"
#include "core/Macros.h"
#include "context/NexusUnits.hpp"
#include "context/NexusContext.hpp"
#include "context/EnvManager.hpp"
#include "context/QosManager.hpp"
#include "qos/DomainParticipantQosImpl.hpp"
#include "vbs/DataWriterListener.hpp"

namespace vbs {

DomainParticipant::~DomainParticipant() {
    set_listener(nullptr);
    if (domain_participant_ptr_) {
        del_participant_infos();
        del_content_filtered_topics();

        NexusUnits::get_unit().DeleteParticipant(domain_participant_ptr_);
        domain_participant_ptr_ = nullptr;
    }
}

DataWriter* DomainParticipant::create_data_writer(Topic* const local_topic, const std::string& profile_name,
                                                  DataWriterListener* const listener) {
    return create_data_writer(local_topic, profile_name, listener, StatusMask::all());
}

DataWriter* DomainParticipant::create_data_writer(Topic* const local_topic, const std::string& profile_name,
                                                  DataWriterListener* const listener, const StatusMask& mask) {
    DataWriter* tmp_data_writer_ptr = new DataWriter(this, local_topic, profile_name, listener, mask);
    if (tmp_data_writer_ptr != nullptr && tmp_data_writer_ptr->init_verify()) {
        return tmp_data_writer_ptr;
    }
    if (tmp_data_writer_ptr) {
        delete tmp_data_writer_ptr;
        tmp_data_writer_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "create writer failed " << profile_name);
    return nullptr;
}

DataWriter* DomainParticipant::create_data_writer(Topic* const local_topic, const DataWriterQos& writer_qos,
                                                  DataWriterListener* const listener) {
    return create_data_writer(local_topic, writer_qos, listener, StatusMask::all());
}

DataWriter* DomainParticipant::create_data_writer(Topic* const local_topic, const DataWriterQos& writer_qos,
                                                  DataWriterListener* const listener, const StatusMask& mask) {
    DataWriter* tmp_data_writer_ptr = new DataWriter(this, local_topic, writer_qos, listener, mask);
    if (tmp_data_writer_ptr != nullptr && tmp_data_writer_ptr->init_verify()) {
        return tmp_data_writer_ptr;
    }
    if (tmp_data_writer_ptr) {
        delete tmp_data_writer_ptr;
        tmp_data_writer_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "create writer failed with qos");
    return nullptr;
}

ReturnCode_t DomainParticipant::delete_data_writer(const DataWriter* writer) {
    if (writer) {
        delete writer;
        writer = nullptr;
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

DataReader* DomainParticipant::create_data_reader(Topic* const local_topic, const std::string& profile_name,
                                                  DataReaderListener* const listener) {
    return create_data_reader(local_topic, profile_name, listener, StatusMask::all());
}

DataReader* DomainParticipant::create_data_reader(Topic* const local_topic, const std::string& profile_name,
                                                  DataReaderListener* const listener, const StatusMask& mask) {
    return create_data_reader_helper(std::make_tuple(local_topic, nullptr), std::make_tuple(&profile_name, nullptr),
                                     listener, mask);
}

DataReader* DomainParticipant::create_data_reader(ContentFilteredTopic* const local_topic,
                                                  const std::string& profile_name, DataReaderListener* const listener) {
    return create_data_reader(local_topic, profile_name, listener, StatusMask::all());
}

DataReader* DomainParticipant::create_data_reader(ContentFilteredTopic* const local_topic,
                                                  const std::string& profile_name, DataReaderListener* const listener,
                                                  const StatusMask& mask) {
    return create_data_reader_helper(std::make_tuple(nullptr, local_topic), std::make_tuple(&profile_name, nullptr),
                                     listener, mask);
}

DataReader* DomainParticipant::create_data_reader(Topic* const local_topic, const DataReaderQos& reader_qos,
                                                  DataReaderListener* const listener) {
    return create_data_reader(local_topic, reader_qos, listener, StatusMask::all());
}

DataReader* DomainParticipant::create_data_reader(Topic* const local_topic, const DataReaderQos& reader_qos,
                                                  DataReaderListener* const listener, const StatusMask& mask) {
    return create_data_reader_helper(std::make_tuple(local_topic, nullptr), std::make_tuple(nullptr, &reader_qos),
                                     listener, mask);
}

DataReader* DomainParticipant::create_data_reader(ContentFilteredTopic* const local_topic,
                                                  const DataReaderQos& reader_qos, DataReaderListener* const listener) {
    return create_data_reader(local_topic, reader_qos, listener, StatusMask::all());
}

DataReader* DomainParticipant::create_data_reader(ContentFilteredTopic* const local_topic,
                                                  const DataReaderQos& reader_qos, DataReaderListener* const listener,
                                                  const StatusMask& mask) {
    return create_data_reader_helper(std::make_tuple(nullptr, local_topic), std::make_tuple(nullptr, &reader_qos),
                                     listener, mask);
}

ReturnCode_t DomainParticipant::delete_data_reader(const DataReader* reader) {
    if (reader) {
        delete reader;
        reader = nullptr;
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

Topic* DomainParticipant::create_topic(const std::string& topic_name, const std::string& type_name,
                                       TypeSupport const& local_type, const std::string& profile_name) {
    Topic* tmp_topic_ptr = new Topic(this, topic_name, type_name, local_type, profile_name);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(topic_name, type_name, tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Topic create failed, topic_name = " << topic_name << ", type_name = " << type_name
            << ", profile_name = " << profile_name);
    return nullptr;
}

Topic* DomainParticipant::create_topic(const std::string& topic_name, TypeSupport const& local_type,
                                       const std::string& profile_name) {
    Topic* tmp_topic_ptr = new Topic(this, topic_name, local_type.get_type_name(), local_type, profile_name);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(topic_name, local_type.get_type_name(), tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Topic create failed, topic_name = " << topic_name
            << ", local_type_name = " << local_type.get_type_name() << ", profile_name = " << profile_name);
    return nullptr;
}

Topic* DomainParticipant::create_topic(const std::string& topic_name, const std::string& type_name,
                                       TypeSupport const& local_type, const TopicQos& topic_qos) {
    Topic* tmp_topic_ptr = new Topic(this, topic_name, type_name, local_type, topic_qos);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(topic_name, type_name, tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Topic create failed, topic_name = " << topic_name << ", type_name = " << type_name);
    return nullptr;
}

Topic* DomainParticipant::create_topic(const std::string& topic_name, TypeSupport const& local_type,
                                       const TopicQos& topic_qos) {
    Topic* tmp_topic_ptr = new Topic(this, topic_name, local_type.get_type_name(), local_type, topic_qos);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(topic_name, local_type.get_type_name(), tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " Topic create failed, topic_name = " << topic_name
                                                             << ", local_type_name = " << local_type.get_type_name());
    return nullptr;
}

ContentFilteredTopic* DomainParticipant::create_contentfilteredtopic(
    const std::string& name, Topic* related_topic, const std::string& filter_expression,
    const std::vector<std::string>& expression_parameters) {
    ContentFilteredTopic* tmp_topic_ptr =
        new ContentFilteredTopic(this, name, related_topic, filter_expression, expression_parameters);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " ContentFilteredTopic create failed, topic_name = " << name);
    return nullptr;
}

Topic* DomainParticipant::create_dynamic_type_topic(const std::string& topic_name, const std::string& type_name,
                                                    VBSDynamicType& dyn_type, const std::string& profile_name) {
    Topic* tmp_topic_ptr = new Topic(this, topic_name, type_name, dyn_type, profile_name);
    if (tmp_topic_ptr != nullptr && tmp_topic_ptr->init_verify()) {
        add_topic(topic_name, type_name, tmp_topic_ptr);
        return tmp_topic_ptr;
    }
    if (tmp_topic_ptr) {
        delete tmp_topic_ptr;
        tmp_topic_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
            << " Dynamic topic create failed, topic_name = " << topic_name << ", type_name = " << type_name
            << ", profile_name = " << profile_name);
    return nullptr;
}

ReturnCode_t DomainParticipant::delete_topic(const Topic* topic) {
    remove_topic(topic);
    if (topic) {
        delete topic;
        topic = nullptr;
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

vbs::DomainParticipantImpl* DomainParticipant::get_instance() {
    return domain_participant_ptr_;
}

bool DomainParticipant::init_verify() {
    if (domain_participant_ptr_) {
        return true;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
    return false;
}

// TopicType
Topic* DomainParticipant::find_topic(const std::string& topic_name, const std::string& topic_type) {
    if (domain_participant_ptr_ == nullptr) {
        return nullptr;
    }
    return domain_participant_ptr_->find_topic(topic_name, topic_type);
}

void DomainParticipant::remove_topic(const Topic* topic_ptr) {
    if (domain_participant_ptr_ == nullptr) {
        return;
    }
    return domain_participant_ptr_->remove_topic(topic_ptr);
}

void DomainParticipant::add_topic(const std::string& topic_name, const std::string& topic_type, Topic* topic_ptr) {
    if (domain_participant_ptr_ == nullptr) {
        return;
    }
    return domain_participant_ptr_->add_topic(topic_name, topic_type, topic_ptr);
}

void DomainParticipant::del_participant_infos() {
    if (domain_participant_ptr_ == nullptr) {
        return;
    }
    return domain_participant_ptr_->del_participant_infos();
}

ReturnCode_t DomainParticipant::delete_topic(ContentFilteredTopic* topic) {
    if (domain_participant_ptr_) {
        domain_participant_ptr_->delete_topic(topic);
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

void DomainParticipant::add_topic(ContentFilteredTopic* topic_ptr) {
    if (domain_participant_ptr_) {
        domain_participant_ptr_->add_topic(topic_ptr);
    }
}

void DomainParticipant::del_content_filtered_topics() {
    if (domain_participant_ptr_) {
        domain_participant_ptr_->del_content_filtered_topics();
    }
}

void DomainParticipant::register_remote_topic(const std::string& topic_name) {
    if (domain_participant_ptr_ == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
        return;
    }
    domain_participant_ptr_->register_remote_topic(topic_name);
}

void DomainParticipant::register_remote_topic() {
    if (domain_participant_ptr_ == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
        return;
    }
    domain_participant_ptr_->register_remote_topic();
}

void DomainParticipant::unregister_remote_topic() {
    if (domain_participant_ptr_ == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
        return;
    }
    domain_participant_ptr_->unregister_remote_topic();
}

VBSDynamicType DomainParticipant::get_dynamic_type(const std::string& type_name) {
    VBSDynamicType dynamic_type;
    if (domain_participant_ptr_ != nullptr) {
        dynamic_type.dynamic_type_ =
            std::make_shared<DynamicType_ptr>(domain_participant_ptr_->get_dynamic_type(type_name));
        return dynamic_type;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
    return dynamic_type;
}

void DomainParticipant::delete_xml_instance() {
    evbs::ertps::xmlparser::XMLProfileManager::DeleteInstance();
    QosManager::Instance()->clear_xml();
}

void DomainParticipant::set_listener(DomainParticipantListener* listener) {
    if (domain_participant_ptr_ != nullptr) {
        domain_participant_ptr_->set_user_listener(listener);
    }
}

DomainId_t DomainParticipant::get_domain_id() {
    if (domain_participant_ptr_ == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " domain_participant_ptr_ unexpected nullptr");
        return UINT32_MAX;
    }
    return domain_participant_ptr_->get_domain_id();
}

void DomainParticipant::set_participant_lisener(DomainParticipantListener* listener) {
    if (domain_participant_ptr_ != nullptr) {
        domain_participant_ptr_->set_user_listener(listener);
    }
}

DataReader* DomainParticipant::create_data_reader_helper(std::tuple<Topic*, ContentFilteredTopic*> local_topic,
                                                         std::tuple<const std::string*, const DataReaderQos*> qos_info,
                                                         DataReaderListener* const listener, const StatusMask& mask) {
    DataReader* tmp_data_reader_ptr = nullptr;
    if (std::get<0>(local_topic) != nullptr) {
        if (std::get<0>(qos_info) != nullptr) {
            tmp_data_reader_ptr =
                new DataReader(this, std::get<0>(local_topic), *std::get<0>(qos_info), listener, mask);
        } else {
            tmp_data_reader_ptr =
                new DataReader(this, std::get<0>(local_topic), *std::get<1>(qos_info), listener, mask);
        }
    } else {
        if (std::get<0>(qos_info) != nullptr) {
            tmp_data_reader_ptr =
                new DataReader(this, std::get<1>(local_topic), *std::get<0>(qos_info), listener, mask);
        } else {
            tmp_data_reader_ptr =
                new DataReader(this, std::get<1>(local_topic), *std::get<1>(qos_info), listener, mask);
        }
    }

    if (tmp_data_reader_ptr != nullptr && tmp_data_reader_ptr->init_verify()) {
        return tmp_data_reader_ptr;
    }
    if (tmp_data_reader_ptr) {
        delete tmp_data_reader_ptr;
        tmp_data_reader_ptr = nullptr;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " DataReader create failed");
    return nullptr;
}

}  // namespace vbs
