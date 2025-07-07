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

#include <agent/PipeInstance.hpp>
#include <agent/types/TopicPubSubType.hpp>
#include <agent/EndpointConfig.hpp>

#include <domain/DomainParticipantFactoryInner.hpp>
#include <domain/qos/DomainParticipantQosInner.hpp>
#include <core/LoanableSequence.hpp>
#include <transport/UDPv4TransportDescriptor.h>

namespace discovery_server {
namespace agent {

using namespace evbs::edds;
using namespace evbs::edds::dds;

PipeInstance::PipeInstance() : dds_reader_listener_(this), pipe_listener_(this) {}

PipeInstance& PipeInstance::getInstance() {
    static PipeInstance instance;
    return instance;
}

bool PipeInstance::create(const CLIParser::server_config& server_config) {
    config::mvbs_profile* profile = config::GetAgentProfile();

    const config::participant_attr* ptcp_attr = profile->ptcp_attr;

    if (ptcp_attr == nullptr) {
        elogWarning(AGENT, "Agent participant is empty.");
        return false;
    }

    if (ptcp_attr->composite_reader_num == 0 && ptcp_attr->composite_writer_num == 0) {
        elogWarning(AGENT, "Agent composite reader/writer is empty.");
        return false;
    }

    vbs::DomainParticipantQosInner pqos;
    pqos.name(ptcp_attr->name);
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = {ptcp_attr->discovery.lease_duration.seconds,
                                                                   ptcp_attr->discovery.lease_duration.nanosec};
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = {
        ptcp_attr->discovery.lease_duration_announce_period.seconds,
        ptcp_attr->discovery.lease_duration_announce_period.nanosec};
    pqos.wire_protocol().builtin.discovery_config.initial_announcements.count =
        ptcp_attr->discovery.initial_announce_count;
    pqos.wire_protocol().builtin.discovery_config.initial_announcements.period = {
        ptcp_attr->discovery.initial_announce_period.seconds, ptcp_attr->discovery.initial_announce_period.nanosec};
    pqos.transport().use_builtin_transports = false;
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::SIMPLE;
    if (!server_config.whitelist.empty()) {
        auto descriptor = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
        vbsutil::xmlparser::WhiteListAddr w_addr = {server_config.whitelist, 65535};
        descriptor->interfaceWhiteList.emplace_back(w_addr);
        pqos.transport().user_transports.push_back(descriptor);
    }
    std::unique_lock<std::mutex> lock(mutex_);
    participant_ = vbs::DomainParticipantFactoryInner::get_instance()->create_participant(profile->domain_id, pqos);
    if (participant_ == nullptr) {
        elogError(AGENT, RetCode_t::RETCODE_ERROR, "Agent participant create fail.");
        return false;
    }

    for (int i = 0; i < ptcp_attr->composite_writer_num; i++) {
        create_pipe_writer(&ptcp_attr->composite_writer_attrs[i]);
    }

    for (int i = 0; i < ptcp_attr->composite_reader_num; i++) {
        create_pipe_reader(&ptcp_attr->composite_reader_attrs[i]);
    }

    elogInfo(AGENT, "MVBS QOS Version: " << config::GetMvbsQosVersion());
    return true;
}

void PipeInstance::clear() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (participant_) {
        participant_->delete_contained_entities();
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant_);
    }
    dds_topics_.clear();
    dds_readers_.clear();
    dds_writers_.clear();
}

vbs::TopicImpl* PipeInstance::get_topic(vbs::DomainParticipantImpl* partticipant, const std::string& topic_name,
                                        const std::string& type_name) {
    if (dds_topics_.find(topic_name) != dds_topics_.end()) {
        return dds_topics_[topic_name];
    }

    vbs::TopicImpl* topic = partticipant->create_topic(topic_name, type_name, vbsutil::xmlparser::getDefaultTopicQos());
    if (!topic) {
        return nullptr;
    }
    dds_topics_[topic_name] = topic;
    return topic;
}

vbs::DataReaderImpl* PipeInstance::create_sub_writer(vbs::Subscriber* subscriber,
                                                     const config::composite_endpoint_attr& composite_attr,
                                                     const config::sub_endpoint_attr& sub_endpoint_attr) {
    vbs::TypeSupport type_support(new TopicPubSubType {false});
    type_support->setName(sub_endpoint_attr.type_name.c_str());
    if (participant_->register_type(type_support) != ReturnCode_t::RETCODE_OK) {
        return nullptr;
    }

    vbs::TopicImpl* topic = get_topic(participant_, sub_endpoint_attr.topic_name, sub_endpoint_attr.type_name);
    if (topic == nullptr) {
        return nullptr;
    }

    DataReaderQos qos;
    qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS | vbsutil::xmlparser::IpcMode::DISABLE_INTER_HOST);
    qos_convert(qos, composite_attr);
    return subscriber->create_datareader(topic, qos, &dds_reader_listener_);
}

vbs::DataWriterImpl* PipeInstance::create_sub_reader(vbs::Publisher* publisher,
                                                     const config::composite_endpoint_attr& composite_attr,
                                                     const config::sub_endpoint_attr& sub_endpoint_attr) {
    vbs::TypeSupport type_support(new TopicPubSubType {false});
    type_support->setName(sub_endpoint_attr.type_name.c_str());
    if (participant_->register_type(type_support) != ReturnCode_t::RETCODE_OK) {
        return nullptr;
    }
    vbs::TopicImpl* topic = get_topic(participant_, sub_endpoint_attr.topic_name, sub_endpoint_attr.type_name);
    if (topic == nullptr) {
        return nullptr;
    }

    DataWriterQos qos;
    qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS | vbsutil::xmlparser::IpcMode::DISABLE_INTER_HOST);
    qos_convert(qos, composite_attr);
    return publisher->create_datawriter(topic, qos);
}

void PipeInstance::qos_convert(DataWriterQos& writer_qos, const config::composite_endpoint_attr& composite_attr) {
    writer_qos.reliability().kind =
        static_cast<vbsutil::xmlparser::ReliabilityQosPolicyKind>(composite_attr.qos.reliability_kind);
    writer_qos.reliability().max_blocking_time = {composite_attr.reliability.max_blocking_time.seconds,
                                                  composite_attr.reliability.max_blocking_time.nanosec};
    writer_qos.durability().kind =
        static_cast<vbsutil::xmlparser::DurabilityQosPolicyKind_t>(composite_attr.qos.durability_kind);
    writer_qos.ownership().kind =
        static_cast<vbsutil::xmlparser::OwnershipQosPolicyKind>(composite_attr.qos.ownership_kind);
    writer_qos.liveliness().kind =
        static_cast<vbsutil::xmlparser::LivelinessQosPolicyKind>(composite_attr.qos.liveliness_kind);
    writer_qos.liveliness().lease_duration = {composite_attr.liveliness.lease_duration.seconds,
                                              composite_attr.liveliness.lease_duration.nanosec};
    writer_qos.liveliness().announcement_period = {composite_attr.liveliness.announcement_period.seconds,
                                                   composite_attr.liveliness.announcement_period.nanosec};
    writer_qos.history().kind = static_cast<vbsutil::xmlparser::HistoryQosPolicyKind>(composite_attr.qos.history_kind);
    writer_qos.history().depth = static_cast<int32_t>(composite_attr.history.depth);
    writer_qos.ownership_strength().value = composite_attr.ownership_strength.value;
    writer_qos.resource_limits().max_samples = composite_attr.resource_limit.max_samples;
    writer_qos.resource_limits().max_samples_per_instance = composite_attr.resource_limit.max_samples_per_instance;
    writer_qos.resource_limits().max_instances = composite_attr.resource_limit.max_instances;
    writer_qos.lifespan().duration = {composite_attr.lifespan.duration.seconds,
                                      composite_attr.lifespan.duration.nanosec};
    writer_qos.e2e_protection().enabled = composite_attr.e2e.e2e_protection;
    writer_qos.e2e_protection().MinDataLength = composite_attr.e2e.e2e_p04_min_data_length;
    writer_qos.e2e_protection().MaxDataLength = composite_attr.e2e.e2e_p04_max_data_length;
    writer_qos.e2e_protection().MaxDeltaCounter = composite_attr.e2e.e2e_p04_max_delta_counter;
    writer_qos.deadline().period = {composite_attr.deadline.duration.seconds, composite_attr.deadline.duration.nanosec};
}

void PipeInstance::qos_convert(DataReaderQos& qos, const config::composite_endpoint_attr& composite_attr) {
    qos.reliability().kind =
        static_cast<vbsutil::xmlparser::ReliabilityQosPolicyKind>(composite_attr.qos.reliability_kind);
    qos.reliability().max_blocking_time = {composite_attr.reliability.max_blocking_time.seconds,
                                           composite_attr.reliability.max_blocking_time.nanosec};
    qos.durability().kind =
        static_cast<vbsutil::xmlparser::DurabilityQosPolicyKind_t>(composite_attr.qos.durability_kind);
    qos.ownership().kind = static_cast<vbsutil::xmlparser::OwnershipQosPolicyKind>(composite_attr.qos.ownership_kind);
    qos.liveliness().kind =
        static_cast<vbsutil::xmlparser::LivelinessQosPolicyKind>(composite_attr.qos.liveliness_kind);
    qos.liveliness().lease_duration = {composite_attr.liveliness.lease_duration.seconds,
                                       composite_attr.liveliness.lease_duration.nanosec};
    qos.liveliness().announcement_period = {composite_attr.liveliness.announcement_period.seconds,
                                            composite_attr.liveliness.announcement_period.nanosec};
    qos.history().kind = static_cast<vbsutil::xmlparser::HistoryQosPolicyKind>(composite_attr.qos.history_kind);
    qos.history().depth = static_cast<int32_t>(composite_attr.history.depth);
    qos.resource_limits().max_samples = composite_attr.resource_limit.max_samples;
    qos.resource_limits().max_samples_per_instance = composite_attr.resource_limit.max_samples_per_instance;
    qos.resource_limits().max_instances = composite_attr.resource_limit.max_instances;
    qos.lifespan().duration = {composite_attr.lifespan.duration.seconds, composite_attr.lifespan.duration.nanosec};
    qos.e2e_protection().enabled = composite_attr.e2e.e2e_protection;
    qos.e2e_protection().MinDataLength = composite_attr.e2e.e2e_p04_min_data_length;
    qos.e2e_protection().MaxDataLength = composite_attr.e2e.e2e_p04_max_data_length;
    qos.e2e_protection().MaxDeltaCounter = composite_attr.e2e.e2e_p04_max_delta_counter;
    qos.deadline().period = {composite_attr.deadline.duration.seconds, composite_attr.deadline.duration.nanosec};
}

PipeInstance::DDSReaderListener::DDSReaderListener(PipeInstance* pipe_obj) : pipe_obj_(pipe_obj) {}

void PipeInstance::DDSReaderListener::on_data_available(vbs::DataReaderImpl* reader) {
    const std::string& topic_name = reader->get_topicdescription()->get_name();
    if (pipe_obj_->pipe_writer_ == nullptr) {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET, topic_name,
                      "Pipe wirter not created to take data. topic " << topic_name);
        return;
    }
    auto it = pipe_obj_->dds_readers_.find(reader);
    if (it == pipe_obj_->dds_readers_.end()) {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET, topic_name,
                      "DDS reader in pipe not found. topic " << topic_name);
        return;
    }
    vbs::LoanableSequence<void*, std::true_type> data_values;
    SampleInfoSeq sample_infos;
    vbs::common::WriteParams writer_params;
    writer_params.topic_id(it->second);
    if (reader->take_buffer(data_values, sample_infos) == ReturnCode_t::RETCODE_OK) {
        for (LoanableCollection::size_type i = 0; i < data_values.length(); i++) {
            (void)pipe_obj_->pipe_writer_->write_buffer(&data_values[i], writer_params, sample_infos[i].payload_len);
        }
    } else {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_EXCEEDED_LIMIT, topic_name,
                      "Sub reader take fail. topic " << topic_name);
    }
    reader->return_loan(data_values, sample_infos);
}

bool PipeInstance::create_pipe_writer(const config::composite_writer_attr* writer_attr) {
    using namespace evbs::edds::dds;

    const config::composite_endpoint_attr& composite_attr = writer_attr->composite_ep_attr;
    if (composite_attr.topic_name.empty()) {
        elogError(AGENT, ReturnCode_t::RETCODE_BAD_PARAMETER, "composite topic name invalid.");
        return false;
    }

    if (writer_attr->sub_writer_ep_attr.empty()) {
        elogError(AGENT, ReturnCode_t::RETCODE_BAD_PARAMETER, "There`s no sub endpoint in composite writer.");
        return false;
    }

    vbs::TypeSupport type_support(new TopicPubSubType {false});
    type_support->setName(composite_attr.type_name.c_str());
    if (participant_->register_type(type_support) != ReturnCode_t::RETCODE_OK) {
        return false;
    }

    vbs::TopicImpl* pipe_topic = get_topic(participant_, composite_attr.topic_name, composite_attr.type_name);
    if (pipe_topic == nullptr) {
        return false;
    }

    vbs::Publisher* pipe_publisher = participant_->create_publisher(vbsutil::xmlparser::getDefaultPublisherQos());
    if (pipe_publisher == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    DataWriterQos pipe_qos;
    pipe_qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS);
    qos_convert(pipe_qos, composite_attr);
    vbs::DataWriterImpl* writer = pipe_publisher->create_datawriter(pipe_topic, pipe_qos, nullptr);
    if (writer == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    // for create sub readers
    vbs::Subscriber* sub_subscriber = participant_->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos());
    if (sub_subscriber == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    for (const config::sub_endpoint_attr& sub_writer_attr : writer_attr->sub_writer_ep_attr) {
        vbs::DataReaderImpl* dds_reader = create_sub_writer(sub_subscriber, composite_attr, sub_writer_attr);
        if (dds_reader == nullptr) {
            participant_->delete_contained_entities();
            dds_readers_.clear();
            return false;
        }
        dds_readers_.emplace(dds_reader, ENTITY_KEY_TO_TOPIC_ID(sub_writer_attr.entity_id));
    }
    pipe_writer_ = writer;
    return true;
}

PipeInstance::PipeReaderListener::PipeReaderListener(PipeInstance* pipe_obj) : pipe_obj_(pipe_obj) {}

void PipeInstance::PipeReaderListener::on_data_available(vbs::DataReaderImpl* reader) {
    const std::string& topic_name = reader->get_topicdescription()->get_name();
    vbs::LoanableSequence<void*, std::true_type> data_values;
    SampleInfoSeq sample_infos;
    vbs::common::WriteParams writer_params;
    auto writers_it = pipe_obj_->dds_writers_.find(reader);
    if (writers_it == pipe_obj_->dds_writers_.end()) {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_EXCEEDED_LIMIT, topic_name,
                      "Pipe topic:" << topic_name << "not found.");
        return;
    }
    if (reader->take_buffer(data_values, sample_infos) == ReturnCode_t::RETCODE_OK) {
        for (LoanableCollection::size_type i = 0; i < data_values.length(); i++) {
            uint32_t topic_id = sample_infos[i].topic_id;
            auto it = writers_it->second.find(topic_id);
            if (it == writers_it->second.end()) {
                elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_EXCEEDED_LIMIT, topic_name,
                              "Topic id:" << topic_id << " not found, Pipe topic:" << topic_name);
                continue;
            }
            (void)it->second->write_buffer(&data_values[i], writer_params, sample_infos[i].payload_len);
        }
    } else {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_EXCEEDED_LIMIT, topic_name,
                      "Sub reader take fail. topic " << topic_name);
    }
    reader->return_loan(data_values, sample_infos);
}

bool PipeInstance::create_pipe_reader(const config::composite_reader_attr* reader_attr) {
    using namespace evbs::edds::dds;

    const config::composite_endpoint_attr& composite_attr = reader_attr->composite_ep_attr;
    if (composite_attr.topic_name.empty()) {
        elogError(AGENT, ReturnCode_t::RETCODE_BAD_PARAMETER, "composite topic name invalid.");
        return false;
    }

    if (reader_attr->sub_reader_ep_attr.empty()) {
        elogError(AGENT, ReturnCode_t::RETCODE_BAD_PARAMETER, "There`s no sub endpoint in composite reader.");
        return false;
    }

    vbs::TypeSupport type_support(new TopicPubSubType {false});
    type_support->setName(composite_attr.type_name.c_str());
    if (participant_->register_type(type_support) != ReturnCode_t::RETCODE_OK) {
        return false;
    }

    vbs::TopicImpl* pipe_topic = get_topic(participant_, composite_attr.topic_name, composite_attr.type_name);
    if (pipe_topic == nullptr) {
        return false;
    }

    vbs::Subscriber* pipe_subcriber = participant_->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos());
    if (pipe_subcriber == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    DataReaderQos pipe_qos;
    pipe_qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS);
    qos_convert(pipe_qos, composite_attr);
    vbs::DataReaderImpl* reader = pipe_subcriber->create_datareader(pipe_topic, pipe_qos, &pipe_listener_);
    if (reader == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    // for create sub writers
    vbs::Publisher* sub_publisher = participant_->create_publisher(vbsutil::xmlparser::getDefaultPublisherQos());
    if (sub_publisher == nullptr) {
        participant_->delete_contained_entities();
        return false;
    }

    for (const config::sub_endpoint_attr& sub_reader_attr : reader_attr->sub_reader_ep_attr) {
        vbs::DataWriterImpl* dds_writer = create_sub_reader(sub_publisher, composite_attr, sub_reader_attr);
        if (dds_writer == nullptr) {
            participant_->delete_contained_entities();
            dds_readers_.clear();
            return false;
        }
        dds_writers_[reader].emplace(ENTITY_KEY_TO_TOPIC_ID(sub_reader_attr.entity_id), dds_writer);
    }
    pipe_reader_ = reader;
    return true;
}

}  // namespace agent
}  // namespace discovery_server
