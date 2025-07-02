/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "context/QosManager.hpp"
#include "context/NexusContext.hpp"
#include "context/MessageTraits.hpp"
#include "utils/QosConverters.hpp"
#include "ertps/xmlparser/XMLProfileManager.h"

using evbs::ertps::xmlparser::XMLProfileManager;
using vbsutil::xmlparser::XMLP_ret;

namespace vbs {

QosManager* QosManager::Instance() {
    return NexusContext::get_context().get_qos_manager();
}

QosManager::QosManager() {}

QosManager::~QosManager() {
    clear_participant_attribute();
    clear_topic_attribute();
    clear_publisher_attribute();
    clear_subscriber_attribute();

    clear_participant_qos();
    clear_topic_qos();
    clear_publisher_qos();
    clear_subscriber_qos();

    clear_topic();
    clear_writer();
    clear_reader();
}

bool QosManager::load_xml(const std::string& xml_file) {
    {
        std::lock_guard<std::mutex> lock(mtx_xml_file_);
        if (xml_files_.find(xml_file) != xml_files_.end()) {
            return true;
        }
        if (XMLProfileManager::loadXMLFile(xml_file) != XMLP_ret::XML_OK) {
            return false;
        }
        xml_files_.insert(xml_file);

        std::unique_ptr<evbs::ertps::ParticipantAttributes> attr(new evbs::ertps::ParticipantAttributes);
        XMLProfileManager::getDefaultParticipantAttributes(*attr);
        vbs::utils::set_qos_from_attributes(default_participant_qos_, attr->rtps);

        std::unique_ptr<PublisherAttributes> pub_attr(new PublisherAttributes);
        XMLProfileManager::getDefaultPublisherAttributes(*pub_attr);
        vbs::utils::set_qos_from_attributes(default_writer_qos_, *pub_attr);

        std::unique_ptr<SubscriberAttributes> sub_attr(new SubscriberAttributes);
        XMLProfileManager::getDefaultSubscriberAttributes(*sub_attr);
        vbs::utils::set_qos_from_attributes(default_reader_qos_, *sub_attr);

        std::unique_ptr<TopicAttributes> top_attr(new TopicAttributes);
        XMLProfileManager::getDefaultTopicAttributes(*top_attr);
        vbs::utils::set_qos_from_attributes(default_topic_qos_, *top_attr);
    }
    bool expect = true;
    if (is_init_load_.compare_exchange_strong(expect, false) == false) {
        clear_participant_attribute();
        clear_topic_attribute();
        clear_publisher_attribute();
        clear_subscriber_attribute();

        clear_participant_qos();
        clear_topic_qos();
        clear_publisher_qos();
        clear_subscriber_qos();
    }

    return true;
}

void QosManager::clear_xml() {
    std::lock_guard<std::mutex> lock(mtx_xml_file_);
    xml_files_.clear();
}

const std::shared_ptr<evbs::ertps::ParticipantAttributes> QosManager::get_participant_xml_attribute(
    const std::string& profile) {
    {
        std::shared_lock lock(mtx_participant_attr_);
        if (participant_attributes_.find(profile) != participant_attributes_.end()) {
            return participant_attributes_[profile];
        }
    }

    return create_participant_attribute(profile);
}

const std::shared_ptr<TopicAttributes> QosManager::get_topic_xml_attribute(const std::string& profile) {
    {
        std::shared_lock lock(mtx_topic_attr_);
        if (topic_attributes_.find(profile) != topic_attributes_.end()) {
            return topic_attributes_[profile];
        }
    }

    return create_topic_attribute(profile);
}

const std::shared_ptr<PublisherAttributes> QosManager::get_publisher_xml_attribute(const std::string& profile) {
    {
        std::shared_lock lock(mtx_publisher_attr_);
        if (publisher_attributes_.find(profile) != publisher_attributes_.end()) {
            return publisher_attributes_[profile];
        }
    }

    return create_publisher_attribute(profile);
}

const std::shared_ptr<SubscriberAttributes> QosManager::get_subscriber_xml_attribute(const std::string& profile) {
    {
        std::shared_lock lock(mtx_subscriber_attr_);
        if (subscriber_attributes_.find(profile) != subscriber_attributes_.end()) {
            return subscriber_attributes_[profile];
        }
    }

    return create_subscriber_attribute(profile);
}

const std::shared_ptr<XmlDomainParticipantQos> QosManager::get_participant_xml_qos(const std::string& profile) {
    {
        std::shared_lock lock(mtx_participant_qos_);
        if (participant_qos_.find(profile) != participant_qos_.end()) {
            return participant_qos_[profile];
        }
    }

    return create_participant_qos(profile);
}

const std::shared_ptr<XmlTopicQos> QosManager::get_topic_xml_qos(const std::string& profile) {
    {
        std::shared_lock lock(mtx_topic_qos_);
        if (topic_qos_.find(profile) != topic_qos_.end()) {
            return topic_qos_[profile];
        }
    }

    return create_topic_qos(profile);
}

const std::shared_ptr<XmlDataWriterQos> QosManager::get_writer_xml_qos(const std::string& profile) {
    {
        std::shared_lock lock(mtx_writer_qos_);
        if (writer_qos_.find(profile) != writer_qos_.end()) {
            return writer_qos_[profile];
        }
    }

    return create_publisher_qos(profile);
}

const std::shared_ptr<XmlDataReaderQos> QosManager::get_reader_xml_qos(const std::string& profile) {
    {
        std::shared_lock lock(mtx_reader_qos_);
        if (reader_qos_.find(profile) != reader_qos_.end()) {
            return reader_qos_[profile];
        }
    }

    return create_subscriber_qos(profile);
}

void QosManager::add_topic(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                           const std::string& topic_type, TopicImpl* topic) {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    std::stringstream ss;
    ss << guid;
    size_t key = std::hash<std::string>()(ss.str() + topic_name + topic_type);
    if (topic_map_.find(key) != topic_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_EXIST)
                << "topic has created : " << guid << " topic_name : " << topic_name << " type : " << topic_type);
        return;
    }
    topic_map_[key] = topic;
}

void QosManager::add_writer(uint16_t id, DataWriterImpl* writer) {
    std::lock_guard<std::mutex> lock(mtx_writer_);
    if (writer_map_.find(id) != writer_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_EXIST) << "writer has created : " << id);
        return;
    }
    writer_map_[id] = writer;
}

void QosManager::add_reader(uint16_t id, DataReaderImpl* reader) {
    std::lock_guard<std::mutex> lock(mtx_reader_);
    if (reader_map_.find(id) != reader_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_EXIST) << "reader has created : " << id);
        return;
    }
    reader_map_[id] = reader;
}

void QosManager::remove_topic(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                              const std::string& topic_type) {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    std::stringstream ss;
    ss << guid;
    size_t key = std::hash<std::string>()(ss.str() + topic_name + topic_type);
    if (topic_map_.find(key) != topic_map_.end()) {
        topic_map_.erase(key);
    }
}

void QosManager::remove_writer(uint16_t id) {
    std::lock_guard<std::mutex> lock(mtx_writer_);
    if (writer_map_.find(id) != writer_map_.end()) {
        writer_map_.erase(id);
    }
}

void QosManager::remove_reader(uint16_t id) {
    std::lock_guard<std::mutex> lock(mtx_reader_);
    if (reader_map_.find(id) != reader_map_.end()) {
        reader_map_.erase(id);
    }
}

bool QosManager::set_topic_running_qos(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                                       const std::string& topic_type, const TopicQosImpl& topic_qos) {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    std::stringstream ss;
    ss << guid;
    size_t key = std::hash<std::string>()(ss.str() + topic_name + topic_type);
    if (topic_map_.find(key) == topic_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << "topic not found : " << guid << " topic_name : " << topic_name << " type : " << topic_type);
        return false;
    }
    return vbs::SetQos(*(topic_map_[key]), topic_qos);
}

bool QosManager::set_writer_running_qos(uint16_t id, const DataWriterQosImpl& writer_qos) {
    std::lock_guard<std::mutex> lock(mtx_writer_);
    if (writer_map_.find(id) == writer_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "witer not found : " << id);
        return false;
    }
    return vbs::SetQos(*(writer_map_[id]), writer_qos);
}

bool QosManager::set_reader_running_qos(uint16_t id, const DataReaderQosImpl& reader_qos) {
    std::lock_guard<std::mutex> lock(mtx_reader_);
    if (reader_map_.find(id) == reader_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "Reader not found : " << id);
        return false;
    }
    return vbs::SetQos(*(reader_map_[id]), reader_qos);
}

bool QosManager::get_topic_running_qos(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                                       const std::string& topic_type, TopicQosImpl& topic_qos) {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    std::stringstream ss;
    ss << guid;
    size_t key = std::hash<std::string>()(ss.str() + topic_name + topic_type);
    if (topic_map_.find(key) == topic_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << "topic not found : " << guid << " topic_name : " << topic_name << " type : " << topic_type);
        return false;
    }
    return vbs::GetQos(*(topic_map_[key]), topic_qos);
}

bool QosManager::get_writer_running_qos(uint16_t id, DataWriterQosImpl& writer_qos) {
    std::lock_guard<std::mutex> lock(mtx_writer_);
    if (writer_map_.find(id) == writer_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "witer not found : " << id);
        return false;
    }
    return vbs::GetQos(*(writer_map_[id]), writer_qos);
}

bool QosManager::get_reader_running_qos(uint16_t id, DataReaderQosImpl& reader_qos) {
    std::lock_guard<std::mutex> lock(mtx_reader_);
    if (reader_map_.find(id) == reader_map_.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "Reader not found : " << id);
        return false;
    }

    return vbs::GetQos(*(reader_map_[id]), reader_qos);
}

const std::shared_ptr<evbs::ertps::ParticipantAttributes> QosManager::create_participant_attribute(
    const std::string& profile) {
    std::unique_lock lock(mtx_participant_attr_);
    if (participant_attributes_.find(profile) != participant_attributes_.end()) {
        return participant_attributes_[profile];
    }
    auto attr = std::make_shared<evbs::ertps::ParticipantAttributes>();
    if (XMLP_ret::XML_OK == XMLProfileManager::fillParticipantAttributes(profile, *attr)) {
        participant_attributes_.emplace(profile, std::move(attr));
    } else {
        return nullptr;
    }

    return participant_attributes_[profile];
}

void QosManager::clear_participant_attribute() {
    std::unique_lock lock(mtx_participant_attr_);
    participant_attributes_.clear();
}

const std::shared_ptr<TopicAttributes> QosManager::create_topic_attribute(const std::string& profile) {
    std::unique_lock lock(mtx_topic_attr_);
    if (topic_attributes_.find(profile) != topic_attributes_.end()) {
        return topic_attributes_[profile];
    }
    auto attr = std::make_shared<TopicAttributes>();
    if (XMLP_ret::XML_OK == XMLProfileManager::fillTopicAttributes(profile, *attr)) {
        topic_attributes_.emplace(profile, std::move(attr));
    } else {
        return nullptr;
    }

    return topic_attributes_[profile];
}

void QosManager::clear_topic_attribute() {
    std::unique_lock lock(mtx_topic_attr_);
    topic_attributes_.clear();
}

const std::shared_ptr<PublisherAttributes> QosManager::create_publisher_attribute(const std::string& profile) {
    std::unique_lock lock(mtx_publisher_attr_);
    if (publisher_attributes_.find(profile) != publisher_attributes_.end()) {
        return publisher_attributes_[profile];
    }
    auto attr = std::make_shared<PublisherAttributes>();
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile, *attr)) {
        publisher_attributes_.emplace(profile, std::move(attr));
    } else {
        return nullptr;
    }

    return publisher_attributes_[profile];
}

void QosManager::clear_publisher_attribute() {
    std::unique_lock lock(mtx_publisher_attr_);
    publisher_attributes_.clear();
}

const std::shared_ptr<SubscriberAttributes> QosManager::create_subscriber_attribute(const std::string& profile) {
    std::unique_lock lock(mtx_subscriber_attr_);
    if (subscriber_attributes_.find(profile) != subscriber_attributes_.end()) {
        return subscriber_attributes_[profile];
    }
    auto attr = std::make_shared<SubscriberAttributes>();
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile, *attr)) {
        subscriber_attributes_.emplace(profile, std::move(attr));
    } else {
        return nullptr;
    }

    return subscriber_attributes_[profile];
}

void QosManager::clear_subscriber_attribute() {
    std::unique_lock lock(mtx_subscriber_attr_);
    subscriber_attributes_.clear();
}

const std::shared_ptr<XmlDomainParticipantQos> QosManager::create_participant_qos(const std::string& profile) {
    std::unique_lock lock(mtx_participant_qos_);
    if (participant_qos_.find(profile) != participant_qos_.end()) {
        return participant_qos_[profile];
    }

    auto attr = get_participant_xml_attribute(profile);
    if (attr == nullptr) {
        return nullptr;
    }
    std::shared_ptr<XmlDomainParticipantQos> qos = std::make_shared<XmlDomainParticipantQos>();
    *qos = default_participant_qos_;
    vbs::utils::set_qos_from_attributes(*qos, attr->rtps);
    participant_qos_.emplace(profile, std::move(qos));

    return participant_qos_[profile];
}

void QosManager::clear_participant_qos() {
    std::unique_lock lock(mtx_participant_qos_);
    participant_qos_.clear();
}

const std::shared_ptr<XmlTopicQos> QosManager::create_topic_qos(const std::string& profile) {
    std::unique_lock lock(mtx_topic_qos_);
    if (topic_qos_.find(profile) != topic_qos_.end()) {
        return topic_qos_[profile];
    }

    auto attr = get_topic_xml_attribute(profile);
    if (attr == nullptr) {
        return nullptr;
    }
    std::shared_ptr<XmlTopicQos> qos = std::make_shared<XmlTopicQos>();
    *qos = default_topic_qos_;
    vbs::utils::set_qos_from_attributes(*qos, *attr);
    topic_qos_.emplace(profile, std::move(qos));

    return topic_qos_[profile];
}

void QosManager::clear_topic_qos() {
    std::unique_lock lock(mtx_topic_qos_);
    topic_qos_.clear();
}

const std::shared_ptr<XmlDataWriterQos> QosManager::create_publisher_qos(const std::string& profile) {
    std::unique_lock lock(mtx_writer_qos_);
    if (writer_qos_.find(profile) != writer_qos_.end()) {
        return writer_qos_[profile];
    }

    auto attr = get_publisher_xml_attribute(profile);
    if (attr == nullptr) {
        return nullptr;
    }
    std::shared_ptr<XmlDataWriterQos> qos = std::make_shared<XmlDataWriterQos>();
    *qos = default_writer_qos_;
    vbs::utils::set_qos_from_attributes(*qos, *attr);
    writer_qos_.emplace(profile, std::move(qos));

    return writer_qos_[profile];
}

void QosManager::clear_publisher_qos() {
    std::unique_lock lock(mtx_writer_qos_);
    writer_qos_.clear();
}

const std::shared_ptr<XmlDataReaderQos> QosManager::create_subscriber_qos(const std::string& profile) {
    std::unique_lock lock(mtx_reader_qos_);
    if (reader_qos_.find(profile) != reader_qos_.end()) {
        return reader_qos_[profile];
    }

    auto attr = get_subscriber_xml_attribute(profile);
    if (attr == nullptr) {
        return nullptr;
    }
    std::shared_ptr<XmlDataReaderQos> qos = std::make_shared<XmlDataReaderQos>();
    *qos = default_reader_qos_;
    vbs::utils::set_qos_from_attributes(*qos, *attr);
    reader_qos_.emplace(profile, std::move(qos));

    return reader_qos_[profile];
}

void QosManager::clear_subscriber_qos() {
    std::unique_lock lock(mtx_reader_qos_);
    reader_qos_.clear();
}

void QosManager::clear_topic() {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    topic_map_.clear();
}

void QosManager::clear_writer() {
    std::lock_guard<std::mutex> lock(mtx_writer_);
    writer_map_.clear();
}

void QosManager::clear_reader() {
    std::lock_guard<std::mutex> lock(mtx_reader_);
    reader_map_.clear();
}

}  // namespace vbs
