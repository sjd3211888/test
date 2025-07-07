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

#include <agent/AgentInstance.hpp>
#include <agent/types/TopicPubSubType.hpp>

#include <domain/DomainParticipantFactoryInner.hpp>
#include <domain/qos/DomainParticipantQosInner.hpp>
#include <core/LoanableSequence.hpp>
#include <transport/UDPv4TransportDescriptor.h>
#include <discovery/database/DiscoveryDataBase.hpp>
#include <ertps/xmlparser/XMLProfileManager.h>

namespace discovery_server {
namespace agent {

using namespace evbs::edds;
using namespace evbs::edds::dds;

AgentInstance::AgentInstance() : reader_listener_(this), writer_listener_(this) {}

AgentInstance& AgentInstance::getInstance() {
    static AgentInstance instance;
    return instance;
}

bool AgentInstance::create(const CLIParser::server_config& server_config) {
    whitelist_ = server_config.whitelist;
    std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);
    agent_init_thread_ = std::thread(&AgentInstance::run, this);
    logInfo(AGENT, "Agent init done.");

    enable_inter_host_ = server_config.agent_inter_host;
    if (!server_config.agent_config_path.empty()) {
        evbs::ertps::xmlparser::XMLProfileManager::loadXMLFile(server_config.agent_config_path);
        allowed_topic_list_ = evbs::ertps::xmlparser::XMLParser::getAgentAllowedTopics();
        if (allowed_topic_list_.empty()) {
            elogError(AGENT, ReturnCode_t::RETCODE_ERROR, "Allowed topics for agent is empty.");
            return false;
        }
    }
    evbs::ertps::rtps::ddb::DiscoveryDataBase::register_agent_ep_match_notify(agent_ep_match_notify);

    return true;
}

void AgentInstance::update_discoveried_endpoint(
    std::unordered_map<std::string, std::vector<uint16_t>>& discoveried_endpoint, uint16_t domain_id,
    const std::string& topic_name) {
    auto it = discoveried_endpoint.find(topic_name);
    if (it != discoveried_endpoint.end()) {
        auto& domains = discoveried_endpoint[topic_name];
        if (std::find(domains.begin(), domains.end(), domain_id) == domains.end()) {
            domains.push_back(domain_id);
        }
    } else {
        std::vector<uint16_t> domains;
        domains.push_back(domain_id);
        discoveried_endpoint[topic_name] = domains;
    }
}

void AgentInstance::agent_ep_match_notify(uint16_t domain_id, const evbs::ertps::rtps::ddb::DiscoveryEndpointInfo& info,
                                          bool is_reader) {
    if (info.get_qos().m_agent_flag & AGENT_FLAG) {
        return;
    }
    const std::string& topic_name = info.topic();
    AgentInstance& inst = AgentInstance::getInstance();

    logDebug(AGENT, "New endpoint notify, reader " << is_reader << " agent flag "
                                                   << (uint32_t)info.get_qos().m_agent_flag << " topic " << topic_name
                                                   << " domain " << domain_id);

    std::unique_lock<vbs::shared_mutex> endpoint_lock(inst.endpoint_mutex_);
    if (inst.allowed_topic_list_.find(topic_name) == inst.allowed_topic_list_.end()) {
        return;
    }

    if (!info.type_name().empty() && inst.topic_type_names_.find(topic_name) == inst.topic_type_names_.end()) {
        inst.topic_type_names_[topic_name] = info.type_name();
    }

    TopicDomainkey qos_key(topic_name, domain_id);
    if (inst.qos_by_topic_.find(qos_key) == inst.qos_by_topic_.end()) {
        inst.qos_by_topic_[qos_key] = info.get_qos();
    }

    {
        std::unique_lock<std::mutex> lock(inst.event_mutex_);
        bool has_changed = true;
        bool is_from_mvbs = info.get_qos().m_agent_flag & 1;
        std::unordered_map<std::string, std::pair<std::vector<uint16_t>, std::vector<uint16_t>>>* created_info_ptr;
        std::unordered_map<std::string, std::vector<uint16_t>>* discovered_info_ptr;
        std::vector<uint16_t>* domains_ptr;
        if (is_from_mvbs) {
            if (is_reader) {
                created_info_ptr = &inst.created_vbs_writer_mvbs_reader_;
                discovered_info_ptr = &inst.discoveried_mvbs_readers_;
            } else {
                created_info_ptr = &inst.created_mvbs_writer_vbs_reader_;
                discovered_info_ptr = &inst.discoveried_mvbs_writers_;
            }
            domains_ptr = &(*created_info_ptr)[topic_name].second;
        } else {
            if (is_reader) {
                created_info_ptr = &inst.created_mvbs_writer_vbs_reader_;
                discovered_info_ptr = &inst.discoveried_vbs_readers_;
            } else {
                created_info_ptr = &inst.created_vbs_writer_mvbs_reader_;
                discovered_info_ptr = &inst.discoveried_vbs_writers_;
            }
            domains_ptr = &(*created_info_ptr)[topic_name].first;
        }

        auto it = (*created_info_ptr).find(topic_name);
        if (it == (*created_info_ptr).end()) {
            update_discoveried_endpoint(*discovered_info_ptr, domain_id, topic_name);
            (*created_info_ptr)[topic_name] = {{}, {domain_id}};
        } else {
            if (std::find((*domains_ptr).begin(), (*domains_ptr).end(), domain_id) == (*domains_ptr).end()) {
                update_discoveried_endpoint(*discovered_info_ptr, domain_id, topic_name);
                (*domains_ptr).push_back(domain_id);
            } else {
                has_changed = false;
            }
        }
        if ((*created_info_ptr)[topic_name].first.empty() || (*created_info_ptr)[topic_name].second.empty()) {
            has_changed = false;
        }

        if (has_changed) {
            inst.has_changed_ = true;
            inst.cv_.notify_one();
        }
    }
}

void AgentInstance::delete_endpoint() {
    for (auto it : writer_to_delete_) {
        auto ptcp_it = participant_by_domain_.find(it.first);
        if (ptcp_it != participant_by_domain_.end()) {
            logDebug(AGENT, "Delete agent writer " << it.second->get_topic()->get_name() << " domain " << it.first
                                                   << " agent flag "
                                                   << (uint32_t)it.second->get_dds_qos().agent_flag());
            ptcp_it->second.publisher->delete_datawriter(it.second);
        }
    }
    writer_to_delete_.clear();

    for (auto it : reader_to_delete_) {
        auto ptcp_it = participant_by_domain_.find(it.first);
        if (ptcp_it != participant_by_domain_.end()) {
            logDebug(AGENT, "Delete agent reader " << it.second->get_topicdescription()->get_name() << " domain "
                                                   << it.first << " agent flag "
                                                   << (uint32_t)it.second->get_dds_qos().agent_flag());
            ptcp_it->second.subscriber->delete_datareader(it.second);
        }
    }
    reader_to_delete_.clear();
}

// 节点的创建和删除线程
void AgentInstance::run() {
#if defined(__unix__)
    (void)pthread_setname_np(pthread_self(), "VBSAgent");
#endif  // defined(__unix__)

    bool found = false;
    bool is_mvbs_writer_vbs_reader = false;
    std::string topic_name;
    std::vector<uint16_t> mvbs_domains;
    std::vector<uint16_t> vbs_domains;
    std::unique_lock<std::mutex> lock(event_mutex_);
    while (!stop_.load()) {
        cv_.wait(lock, [&]() { return stop_.load() || has_changed_; });
        while (!stop_.load()) {
            for (auto mvbs_it = discoveried_mvbs_readers_.begin(); mvbs_it != discoveried_mvbs_readers_.end();) {
                found = true;
                topic_name = mvbs_it->first;
                mvbs_domains = mvbs_it->second;
                mvbs_it = discoveried_mvbs_readers_.erase(mvbs_it);
                break;
            }

            for (auto vbs_it = discoveried_vbs_writers_.begin(); vbs_it != discoveried_vbs_writers_.end();) {
                found = true;
                topic_name = vbs_it->first;
                vbs_domains = vbs_it->second;
                vbs_it = discoveried_vbs_writers_.erase(vbs_it);
                break;
            }

            for (auto mvbs_it = discoveried_mvbs_writers_.begin(); mvbs_it != discoveried_mvbs_writers_.end();) {
                found = true;
                topic_name = mvbs_it->first;
                mvbs_domains.insert(mvbs_domains.end(), mvbs_it->second.begin(), mvbs_it->second.end());
                mvbs_it = discoveried_mvbs_writers_.erase(mvbs_it);
                is_mvbs_writer_vbs_reader = true;
                break;
            }

            for (auto vbs_it = discoveried_vbs_readers_.begin(); vbs_it != discoveried_vbs_readers_.end();) {
                found = true;
                topic_name = vbs_it->first;
                vbs_domains.insert(vbs_domains.end(), vbs_it->second.begin(), vbs_it->second.end());
                vbs_it = discoveried_vbs_readers_.erase(vbs_it);
                is_mvbs_writer_vbs_reader = true;
                break;
            }

            if (!found) {
                has_changed_ = false;
                break;
            }

            lock.unlock();

            if (is_mvbs_writer_vbs_reader) {
                create_agent_for_mvbs_writer_vbs_reader(topic_name, mvbs_domains, vbs_domains);
            } else {
                create_agent_for_vbs_writer_mvbs_reader(topic_name, mvbs_domains, vbs_domains);
            }

            found = false;
            is_mvbs_writer_vbs_reader = false;
            mvbs_domains.clear();
            vbs_domains.clear();
            lock.lock();
        }

        lock.unlock();
        std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);
        lock.lock();
        delete_endpoint();
    }
}

void AgentInstance::clear() {
    {
        std::vector<vbs::DomainParticipantImpl*> participants_to_clean;
        {
            std::unique_lock<vbs::shared_mutex> lock(endpoint_mutex_);
            participants_to_clean.reserve(participant_by_domain_.size());
            for (auto& [key, value] : participant_by_domain_) {
                participants_to_clean.emplace_back(value.participant);
            }
            participant_by_domain_.clear();
        }

        for (auto* participant : participants_to_clean) {
            if (participant) {
                participant->delete_contained_entities();
                vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant);
            }
        }

        std::unique_lock<vbs::shared_mutex> lock(endpoint_mutex_);
        topic_type_names_.clear();
        agent_topics_.clear();

        stop_ = true;
        cv_.notify_one();
    }

    if (agent_init_thread_.joinable()) {
        agent_init_thread_.join();
    }

    {
        std::unique_lock<std::mutex> lock(event_mutex_);
        discoveried_vbs_writers_.clear();
        discoveried_vbs_readers_.clear();
        discoveried_mvbs_writers_.clear();
        discoveried_mvbs_readers_.clear();
    }
}

void AgentInstance::qos_convert(DataReaderQos& local_reader_qos, const ddb::EndpointQos& remote_reader_qos) {
    local_reader_qos.reliability(remote_reader_qos.m_reliability);
    local_reader_qos.durability(remote_reader_qos.m_durability);
    local_reader_qos.ownership(remote_reader_qos.m_ownership);
    local_reader_qos.liveliness().kind = remote_reader_qos.m_liveliness.kind;
    local_reader_qos.liveliness().lease_duration = remote_reader_qos.m_liveliness.lease_duration;
    if (local_reader_qos.liveliness().lease_duration < evbs::ertps::c_TimeInfinite) {
        local_reader_qos.liveliness().announcement_period.seconds =
            local_reader_qos.liveliness().lease_duration.seconds / 2;
        local_reader_qos.liveliness().announcement_period.nanosec =
            local_reader_qos.liveliness().lease_duration.nanosec / 2;
    }
    local_reader_qos.history(remote_reader_qos.m_history);
    local_reader_qos.resource_limits(remote_reader_qos.m_resourceLimits);
    local_reader_qos.e2e_protection(remote_reader_qos.m_e2eProtecion);
    local_reader_qos.deadline(remote_reader_qos.m_deadline);
}

void AgentInstance::qos_convert(DataWriterQos& local_reader_qos, const ddb::EndpointQos& remote_reader_qos) {
    local_reader_qos.reliability(remote_reader_qos.m_reliability);
    local_reader_qos.durability(remote_reader_qos.m_durability);
    local_reader_qos.ownership(remote_reader_qos.m_ownership);
    local_reader_qos.liveliness().kind = remote_reader_qos.m_liveliness.kind;
    local_reader_qos.liveliness().lease_duration = remote_reader_qos.m_liveliness.lease_duration;
    if (local_reader_qos.liveliness().lease_duration < evbs::ertps::c_TimeInfinite) {
        local_reader_qos.liveliness().announcement_period.seconds =
            local_reader_qos.liveliness().lease_duration.seconds / 2;
        local_reader_qos.liveliness().announcement_period.nanosec =
            local_reader_qos.liveliness().lease_duration.nanosec / 2;
    }
    local_reader_qos.history(remote_reader_qos.m_history);
    local_reader_qos.resource_limits(remote_reader_qos.m_resourceLimits);
    local_reader_qos.e2e_protection(remote_reader_qos.m_e2eProtecion);
    local_reader_qos.deadline(remote_reader_qos.m_deadline);
}

AgentInstance::DDSReaderListener::DDSReaderListener(AgentInstance* agent_obj) : agent_obj_(agent_obj) {}

void AgentInstance::DDSReaderListener::on_data_available(vbs::DataReaderImpl* reader) {
    const std::string& topic_name = reader->get_topicdescription()->get_name();

    vbs::shared_lock<vbs::shared_mutex> lock(agent_obj_->endpoint_mutex_);
    auto it = agent_obj_->writers_by_reader_.find(reader);
    if (it == agent_obj_->writers_by_reader_.end() || it->second.empty()) {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET, topic_name,
                      "DDS reader in agent not found. topic " << topic_name);
        return;
    }
    vbs::LoanableSequence<void*, std::true_type> data_values;
    SampleInfoSeq sample_infos;
    vbs::common::WriteParams params;
    if (reader->take_buffer(data_values, sample_infos) == ReturnCode_t::RETCODE_OK) {
        for (LoanableCollection::size_type i = 0; i < data_values.length(); i++) {
            for (auto& writer : it->second) {
                writer->write_buffer(&data_values[i], params, sample_infos[i].payload_len);
                logDebug(AGENT, "Send message, topic name " << topic_name << " writer " << writer);
            }
        }
    } else {
        elogErrorKeyT(AGENT, ReturnCode_t::RETCODE_EXCEEDED_LIMIT, topic_name,
                      "Reader take fail. topic " << topic_name);
    }
    reader->return_loan(data_values, sample_infos);
}

void AgentInstance::delete_matched_reader(vbs::DataReaderImpl* reader) {
    std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);

    const std::string& topic_name = reader->get_topicdescription()->get_name();
    if (reader_domain_.find(reader) == reader_domain_.end()) {
        logWarning(AGENT, "Reader not found to update. topic name " << topic_name);
        return;
    }

    // 匹配数为0时，需要更新所有的表项
    uint16_t domain_id = reader_domain_[reader];
    if (reader->get_dds_qos().agent_flag() & MVBS_FLAG) {
        if (agent_mvbs_readers_by_topic_.find(topic_name) != agent_mvbs_readers_by_topic_.end()) {
            auto& readers = agent_mvbs_readers_by_topic_[topic_name];
            readers.erase(std::remove(readers.begin(), readers.end(), reader), readers.end());
            logDebug(AGENT, "Delete match mvbs reader " << topic_name << " domain " << domain_id);
        }

        if (created_mvbs_writer_vbs_reader_.find(topic_name) != created_mvbs_writer_vbs_reader_.end()) {
            auto& writer_domains = created_mvbs_writer_vbs_reader_[topic_name].second;
            writer_domains.erase(std::remove(writer_domains.begin(), writer_domains.end(), domain_id),
                                 writer_domains.end());
            logDebug(AGENT, "Delete created mvbs reader " << topic_name << " domain " << domain_id);
        }
    } else {
        if (agent_vbs_readers_by_topic_.find(topic_name) != agent_vbs_readers_by_topic_.end()) {
            auto& readers = agent_vbs_readers_by_topic_[topic_name];
            readers.erase(std::remove(readers.begin(), readers.end(), reader), readers.end());
            logDebug(AGENT, "Delete match vbs reader " << topic_name << " domain " << domain_id);
        }

        if (created_vbs_writer_mvbs_reader_.find(topic_name) != created_vbs_writer_mvbs_reader_.end()) {
            auto& writer_domains = created_vbs_writer_mvbs_reader_[topic_name].first;
            writer_domains.erase(std::remove(writer_domains.begin(), writer_domains.end(), domain_id),
                                 writer_domains.end());
            logDebug(AGENT, "Delete created vbs reader " << topic_name << " domain " << domain_id);
        }
    }

    auto reader_it = writers_by_reader_.find(reader);
    if (reader_it != writers_by_reader_.end()) {
        writers_by_reader_.erase(reader_it);
        logDebug(AGENT, "Delete message vbs reader " << topic_name << " domain " << domain_id);
    }

    std::unique_lock<std::mutex> lock(event_mutex_);
    reader_to_delete_.emplace_back(std::make_pair(domain_id, reader));
    has_changed_ = true;
    cv_.notify_one();
}

void AgentInstance::DDSReaderListener::on_subscription_matched(vbs::DataReaderImpl* reader,
                                                               const SubscriptionMatchedStatus& info) {
    if (info.current_count != 0) {
        return;
    }
    agent_obj_->delete_matched_reader(reader);
}

void AgentInstance::delete_matched_writer(vbs::DataWriterImpl* writer) {
    std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);

    const std::string& topic_name = writer->get_topic()->get_name();
    if (writer_domain_.find(writer) == writer_domain_.end()) {
        logWarning(AGENT, "Writer not found to update. topic name " << topic_name);
        return;
    }

    // 匹配数为0时，需要更新所有的表项
    uint16_t domain_id = writer_domain_[writer];
    if (writer->get_dds_qos().agent_flag() & MVBS_FLAG) {
        if (agent_mvbs_writers_by_topic_.find(topic_name) != agent_mvbs_writers_by_topic_.end()) {
            auto& writers = agent_mvbs_writers_by_topic_[topic_name];
            writers.erase(std::remove(writers.begin(), writers.end(), writer), writers.end());
            logDebug(AGENT, "Delete match mvbs writer " << topic_name << " domain " << domain_id);
        }

        if (created_vbs_writer_mvbs_reader_.find(topic_name) != created_vbs_writer_mvbs_reader_.end()) {
            auto& reader_domains = created_vbs_writer_mvbs_reader_[topic_name].second;
            reader_domains.erase(std::remove(reader_domains.begin(), reader_domains.end(), domain_id),
                                 reader_domains.end());
            logDebug(AGENT, "Delete created mvbs writer " << topic_name << " domain " << domain_id);
        }
    } else {
        if (agent_vbs_writers_by_topic_.find(topic_name) != agent_vbs_writers_by_topic_.end()) {
            auto& writers = agent_vbs_writers_by_topic_[topic_name];
            writers.erase(std::remove(writers.begin(), writers.end(), writer), writers.end());
            logDebug(AGENT, "Delete match vbs writer " << topic_name << " domain " << domain_id);
        }

        if (created_mvbs_writer_vbs_reader_.find(topic_name) != created_mvbs_writer_vbs_reader_.end()) {
            auto& reader_domains = created_mvbs_writer_vbs_reader_[topic_name].first;
            reader_domains.erase(std::remove(reader_domains.begin(), reader_domains.end(), domain_id),
                                 reader_domains.end());
            logDebug(AGENT, "Delete created vbs writer " << topic_name << " domain " << domain_id);
        }
    }

    for (auto& it : writers_by_reader_) {
        auto& writers = it.second;
        writers.erase(std::remove(writers.begin(), writers.end(), writer), writers.end());
        logDebug(AGENT, "Delete message vbs reader " << topic_name << " domain " << domain_id);
    }

    std::unique_lock<std::mutex> lock(event_mutex_);
    writer_to_delete_.emplace_back(std::make_pair(domain_id, writer));
    has_changed_ = true;
    cv_.notify_one();
}

void AgentInstance::DDSWriterListener::on_publication_matched(vbs::DataWriterImpl* writer,
                                                              const PublicationMatchedStatus& info) {
    logDebug(AGENT, "Pub matched " << writer->get_topic()->get_name() << " writer " << writer << " change "
                                   << info.current_count_change);
    if (info.current_count != 0) {
        return;
    }
    agent_obj_->delete_matched_writer(writer);
}

vbs::TopicImpl* AgentInstance::get_topic(uint16_t domain_id, const std::string& topic_name) {
    TopicDomainkey key(topic_name, domain_id);
    if (agent_topics_.find(key) != agent_topics_.end()) {
        return agent_topics_[key];
    }

    std::string type_name;
    if (topic_type_names_.find(topic_name) != topic_type_names_.end()) {
        type_name = topic_type_names_[topic_name];
    }
    if (type_name.empty()) {
        elogError(AGENT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Type name of domain id " << domain_id << " topic " << topic_name << " not found.");
        return nullptr;
    }

    vbs::TypeSupport type_support(new TopicPubSubType {false});
    type_support->setName(type_name.c_str());

    auto it = participant_by_domain_.find(domain_id);
    if (it == participant_by_domain_.end()) {
        elogError(AGENT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Participant of domain id " << domain_id << " not created.");
        return nullptr;
    }
    vbs::DomainParticipantImpl* participant = participant_by_domain_[domain_id].participant;
    if (participant->register_type(type_support) != ReturnCode_t::RETCODE_OK) {
        return nullptr;
    }

    vbs::TopicImpl* topic = participant->create_topic(topic_name, type_name, vbsutil::xmlparser::getDefaultTopicQos());
    if (!topic) {
        return nullptr;
    }
    agent_topics_[key] = topic;
    return topic;
}

void AgentInstance::create_participant_endpoint(uint16_t domain_id) {
    vbs::DomainParticipantQosInner pqos;
    pqos.name("VBSAgentParticipant");
    pqos.transport().use_builtin_transports = false;
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::CLIENT;

    if (!whitelist_.empty()) {
        auto descriptor = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
        vbsutil::xmlparser::WhiteListAddr w_addr = {whitelist_, 65535};
        descriptor->interfaceWhiteList.emplace_back(w_addr);
        pqos.transport().user_transports.push_back(descriptor);
    }

    vbs::DomainParticipantImpl* participant =
        vbs::DomainParticipantFactoryInner::get_instance()->create_participant(domain_id, pqos, nullptr);
    if (participant == nullptr) {
        elogError(AGENT, RetCode_t::RETCODE_ERROR, "Agent participant create fail, domain id " << domain_id);
        return;
    }

    vbs::Publisher* publisher = participant->create_publisher(vbsutil::xmlparser::getDefaultPublisherQos());
    if (publisher == nullptr) {
        elogError(AGENT, RetCode_t::RETCODE_ERROR, "Agent publisher create fail, domain id " << domain_id);
        participant->delete_contained_entities();
        return;
    }

    vbs::Subscriber* subscriber = participant->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos());
    if (subscriber == nullptr) {
        elogError(AGENT, RetCode_t::RETCODE_ERROR, "Agent subscriber create fail, domain id " << domain_id);
        participant->delete_contained_entities();
        return;
    }

    participant_by_domain_[domain_id] = {participant, publisher, subscriber};
}

bool AgentInstance::create_agent_for_vbs_writer_mvbs_reader(const std::string& topic_name,
                                                            const std::vector<uint16_t>& mvbs_domains,
                                                            const std::vector<uint16_t>& vbs_domains) {
    std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);
    for (auto& domain_id : mvbs_domains) {
        TopicDomainkey key(topic_name, domain_id);
        if (participant_by_domain_.find(domain_id) == participant_by_domain_.end()) {
            create_participant_endpoint(domain_id);
        }

        vbs::TopicImpl* topic = get_topic(domain_id, topic_name);
        if (!topic) {
            return false;
        }

        vbs::Publisher* publisher = participant_by_domain_[domain_id].publisher;

        DataWriterQos writer_qos;
        writer_qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS);
        writer_qos.agent_flag(MVBS_FLAG | AGENT_FLAG);
        qos_convert(writer_qos, qos_by_topic_[key]);
        vbs::DataWriterImpl* mvbs_writer = publisher->create_datawriter(topic, writer_qos, &writer_listener_);
        if (mvbs_writer == nullptr) {
            return false;
        }
        logDebug(AGENT, "Create mvbs writer " << topic_name << " domain " << domain_id << " writer " << mvbs_writer);

        writer_domain_[mvbs_writer] = domain_id;
        if (agent_mvbs_writers_by_topic_.find(topic_name) == agent_mvbs_writers_by_topic_.end()) {
            agent_mvbs_writers_by_topic_[topic_name] = {mvbs_writer};
        } else {
            agent_mvbs_writers_by_topic_[topic_name].push_back(mvbs_writer);
        }

        if (agent_vbs_readers_by_topic_.find(topic_name) != agent_vbs_readers_by_topic_.end()) {
            for (auto reader : agent_vbs_readers_by_topic_[topic_name]) {
                if (writers_by_reader_.find(reader) == writers_by_reader_.end()) {
                    writers_by_reader_[reader] = {mvbs_writer};
                } else {
                    writers_by_reader_[reader].push_back(mvbs_writer);
                }
            }
        }
    }

    for (auto& domain_id : vbs_domains) {
        TopicDomainkey key(topic_name, domain_id);
        if (participant_by_domain_.find(domain_id) == participant_by_domain_.end()) {
            create_participant_endpoint(domain_id);
        }

        vbs::TopicImpl* topic = get_topic(domain_id, topic_name);
        if (!topic) {
            return false;
        }

        vbs::Subscriber* subscriber = participant_by_domain_[domain_id].subscriber;

        DataReaderQos reader_qos;
        uint8_t ipc_mode = vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS;
        if (!enable_inter_host_) {
            ipc_mode |= vbsutil::xmlparser::IpcMode::DISABLE_INTER_HOST;
        }
        reader_qos.agent_flag(AGENT_FLAG);
        reader_qos.ipc_mode(ipc_mode);
        qos_convert(reader_qos, qos_by_topic_[key]);
        vbs::DataReaderImpl* vbs_reader = subscriber->create_datareader(topic, reader_qos, &reader_listener_);
        if (vbs_reader == nullptr) {
            return false;
        }
        logDebug(AGENT, "Create vbs reader " << topic_name << " domain " << domain_id);

        reader_domain_[vbs_reader] = domain_id;
        if (agent_vbs_readers_by_topic_.find(topic_name) == agent_vbs_readers_by_topic_.end()) {
            agent_vbs_readers_by_topic_[topic_name] = {vbs_reader};
        } else {
            agent_vbs_readers_by_topic_[topic_name].push_back(vbs_reader);
        }

        if (agent_mvbs_writers_by_topic_.find(topic_name) != agent_mvbs_writers_by_topic_.end()) {
            writers_by_reader_[vbs_reader] = agent_mvbs_writers_by_topic_[topic_name];
        }
    }
    return true;
}

bool AgentInstance::create_agent_for_mvbs_writer_vbs_reader(const std::string& topic_name,
                                                            const std::vector<uint16_t>& mvbs_domains,
                                                            const std::vector<uint16_t>& vbs_domains) {
    std::unique_lock<vbs::shared_mutex> endpoint_lock(endpoint_mutex_);
    for (auto& domain_id : mvbs_domains) {
        TopicDomainkey key(topic_name, domain_id);
        if (participant_by_domain_.find(domain_id) == participant_by_domain_.end()) {
            create_participant_endpoint(domain_id);
        }

        vbs::TopicImpl* topic = get_topic(domain_id, topic_name);
        if (!topic) {
            return false;
        }

        vbs::Subscriber* subscriber = participant_by_domain_[domain_id].subscriber;

        DataReaderQos reader_qos;
        reader_qos.ipc_mode(vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS);
        reader_qos.agent_flag(MVBS_FLAG | AGENT_FLAG);
        qos_convert(reader_qos, qos_by_topic_[key]);
        vbs::DataReaderImpl* agent_reader = subscriber->create_datareader(topic, reader_qos, &reader_listener_);
        if (agent_reader == nullptr) {
            return false;
        }
        logDebug(AGENT, "Create mvbs reader " << topic_name << " domain " << domain_id);

        reader_domain_[agent_reader] = domain_id;
        if (agent_mvbs_readers_by_topic_.find(topic_name) == agent_mvbs_readers_by_topic_.end()) {
            agent_mvbs_readers_by_topic_[topic_name] = {agent_reader};
        } else {
            agent_mvbs_readers_by_topic_[topic_name].push_back(agent_reader);
        }

        if (agent_vbs_writers_by_topic_.find(topic_name) != agent_vbs_writers_by_topic_.end()) {
            writers_by_reader_[agent_reader] = agent_vbs_writers_by_topic_[topic_name];
        }
    }

    for (auto& domain_id : vbs_domains) {
        TopicDomainkey key(topic_name, domain_id);
        if (participant_by_domain_.find(domain_id) == participant_by_domain_.end()) {
            create_participant_endpoint(domain_id);
        }
        vbs::TopicImpl* topic = get_topic(domain_id, topic_name);
        if (!topic) {
            return false;
        }

        vbs::Publisher* publisher = participant_by_domain_[domain_id].publisher;

        DataWriterQos writer_qos;
        uint8_t ipc_mode = vbsutil::xmlparser::IpcMode::DISABLE_INTRA_PROCESS;
        if (!enable_inter_host_) {
            ipc_mode |= vbsutil::xmlparser::IpcMode::DISABLE_INTER_HOST;
        }
        writer_qos.agent_flag(AGENT_FLAG);
        writer_qos.ipc_mode(ipc_mode);
        qos_convert(writer_qos, qos_by_topic_[key]);
        vbs::DataWriterImpl* dds_writer = publisher->create_datawriter(topic, writer_qos, &writer_listener_);
        if (dds_writer == nullptr) {
            return false;
        }
        logDebug(AGENT, "Create vbs writer " << topic_name << " domain " << domain_id << " writer " << dds_writer);

        writer_domain_[dds_writer] = domain_id;
        if (agent_vbs_writers_by_topic_.find(topic_name) == agent_vbs_writers_by_topic_.end()) {
            agent_vbs_writers_by_topic_[topic_name] = {dds_writer};
        } else {
            agent_vbs_writers_by_topic_[topic_name].push_back(dds_writer);
        }

        if (agent_mvbs_readers_by_topic_.find(topic_name) != agent_mvbs_readers_by_topic_.end()) {
            for (auto reader : agent_mvbs_readers_by_topic_[topic_name]) {
                if (writers_by_reader_.find(reader) == writers_by_reader_.end()) {
                    writers_by_reader_[reader] = {dds_writer};
                } else {
                    writers_by_reader_[reader].push_back(dds_writer);
                }
            }
        }
    }
    return true;
}

}  // namespace agent
}  // namespace discovery_server
