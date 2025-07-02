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

#ifndef INCLUDE_CONTEXT_QOS_MANAGWE_H_
#define INCLUDE_CONTEXT_QOS_MANAGWE_H_

#include <atomic>
#include <memory>
#include <string>
#include <shared_mutex>
#include <unordered_map>

#include "ertps/attributes/ParticipantAttributes.h"
#include "ertps/attributes/PublisherAttributes.h"
#include "ertps/attributes/SubscriberAttributes.h"
#include "ertps/attributes/TopicAttributes.h"
#include "core/Global.hpp"
#include "core/Macros.h"
#include "pub/DataWriterImpl.hpp"
#include "sub/DataReaderImpl.hpp"
#include "topic/TopicImpl.hpp"
#include "domain/qos/DomainParticipantQosInner.hpp"

using XmlDomainParticipantQos = vbs::DomainParticipantQosInner;
using XmlTopicQos = vbsutil::xmlparser::TopicQos;
using XmlDataWriterQos = vbsutil::xmlparser::DataWriterQos;
using XmlDataReaderQos = vbsutil::xmlparser::DataReaderQos;

namespace vbs {

class TopicImpl;
class DataWriterImpl;
class DataReaderImpl;
class DataWriterQosImpl;

class QosManager {
 public:
    static QosManager* Instance();
    QosManager();
    virtual ~QosManager();

    bool load_xml(const std::string& xml_file);
    void clear_xml();

    const std::shared_ptr<evbs::ertps::ParticipantAttributes> get_participant_xml_attribute(const std::string& profile);
    const std::shared_ptr<TopicAttributes> get_topic_xml_attribute(const std::string& profile);
    const std::shared_ptr<PublisherAttributes> get_publisher_xml_attribute(const std::string& profile);
    const std::shared_ptr<SubscriberAttributes> get_subscriber_xml_attribute(const std::string& profile);

    const std::shared_ptr<XmlDomainParticipantQos> get_participant_xml_qos(const std::string& profile);
    const std::shared_ptr<XmlTopicQos> get_topic_xml_qos(const std::string& profile);
    const std::shared_ptr<XmlDataWriterQos> get_writer_xml_qos(const std::string& profile);
    const std::shared_ptr<XmlDataReaderQos> get_reader_xml_qos(const std::string& profile);

    void add_topic(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name, const std::string& topic_type,
                   TopicImpl* topic);
    void add_writer(uint16_t id, DataWriterImpl* writer);
    void add_reader(uint16_t id, DataReaderImpl* reader);

    void remove_topic(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                      const std::string& topic_type);
    void remove_writer(uint16_t id);
    void remove_reader(uint16_t id);

    bool set_topic_running_qos(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                               const std::string& topic_type, const TopicQosImpl& topic_qos);
    bool set_writer_running_qos(uint16_t id, const DataWriterQosImpl& writer_qos);
    bool set_reader_running_qos(uint16_t id, const DataReaderQosImpl& reader_qos);

    bool get_topic_running_qos(const vbsutil::xmlparser::GUID_t& guid, const std::string& topic_name,
                               const std::string& topic_type, TopicQosImpl& topic_qos);
    bool get_writer_running_qos(uint16_t id, DataWriterQosImpl& writer_qos);
    bool get_reader_running_qos(uint16_t id, DataReaderQosImpl& reader_qos);

 private:
    const std::shared_ptr<evbs::ertps::ParticipantAttributes> create_participant_attribute(const std::string& profile);
    void clear_participant_attribute();
    const std::shared_ptr<TopicAttributes> create_topic_attribute(const std::string& profile);
    void clear_topic_attribute();
    const std::shared_ptr<PublisherAttributes> create_publisher_attribute(const std::string& profile);
    void clear_publisher_attribute();
    const std::shared_ptr<SubscriberAttributes> create_subscriber_attribute(const std::string& profile);
    void clear_subscriber_attribute();

    const std::shared_ptr<XmlDomainParticipantQos> create_participant_qos(const std::string& profile);
    void clear_participant_qos();
    const std::shared_ptr<XmlTopicQos> create_topic_qos(const std::string& profile);
    void clear_topic_qos();
    const std::shared_ptr<XmlDataWriterQos> create_publisher_qos(const std::string& profile);
    void clear_publisher_qos();
    const std::shared_ptr<XmlDataReaderQos> create_subscriber_qos(const std::string& profile);
    void clear_subscriber_qos();

    void clear_topic();
    void clear_writer();
    void clear_reader();

    std::atomic<bool> is_init_load_ {true};
    XmlDomainParticipantQos default_participant_qos_;
    XmlTopicQos default_topic_qos_;
    XmlDataWriterQos default_writer_qos_;
    XmlDataReaderQos default_reader_qos_;
    std::mutex mtx_xml_file_;
    std::set<std::string> xml_files_;

    std::shared_mutex mtx_participant_attr_;
    std::shared_mutex mtx_topic_attr_;
    std::shared_mutex mtx_publisher_attr_;
    std::shared_mutex mtx_subscriber_attr_;
    std::unordered_map<std::string, std::shared_ptr<evbs::ertps::ParticipantAttributes>> participant_attributes_;
    std::unordered_map<std::string, std::shared_ptr<TopicAttributes>> topic_attributes_;
    std::unordered_map<std::string, std::shared_ptr<PublisherAttributes>> publisher_attributes_;
    std::unordered_map<std::string, std::shared_ptr<SubscriberAttributes>> subscriber_attributes_;

    std::shared_mutex mtx_participant_qos_;
    std::shared_mutex mtx_topic_qos_;
    std::shared_mutex mtx_reader_qos_;
    std::shared_mutex mtx_writer_qos_;
    std::unordered_map<std::string, std::shared_ptr<XmlDomainParticipantQos>> participant_qos_;
    std::unordered_map<std::string, std::shared_ptr<XmlTopicQos>> topic_qos_;
    std::unordered_map<std::string, std::shared_ptr<XmlDataReaderQos>> reader_qos_;
    std::unordered_map<std::string, std::shared_ptr<XmlDataWriterQos>> writer_qos_;

    std::mutex mtx_topic_;
    std::mutex mtx_writer_;
    std::mutex mtx_reader_;
    std::unordered_map<size_t, TopicImpl*> topic_map_;
    std::unordered_map<uint16_t, DataWriterImpl*> writer_map_;
    std::unordered_map<uint16_t, DataReaderImpl*> reader_map_;

    DISALLOW_COPY_AND_ASSIGN(QosManager)
};

}  // namespace vbs

#endif  // INCLUDE_CONTEXT_QOS_MANAGWE_H_
