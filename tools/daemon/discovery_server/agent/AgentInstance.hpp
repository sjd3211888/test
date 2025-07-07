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

#ifndef AGENT_INSTANCE_HPP
#define AGENT_INSTANCE_HPP

#include "CLIParser.hpp"

#include <agent/types/EndpointType.hpp>

#include "domain/DomainParticipantImpl.hpp"
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include "pub/DataWriterImpl.hpp"
#include <pub/Publisher.hpp>
#include <sub/DataReaderImpl.hpp>
#include <sub/Subscriber.hpp>
#include <domain/DomainParticipantListenerInner.hpp>
#include <deps/common/shared_mutex.hpp>

#include <thread>
#include <atomic>
#include <mutex>

#define ENTITY_KEY_TO_TOPIC_ID(key) (((key & 0xff0000) >> 16) | ((key & 0xff00)) | ((key & 0xff) << 16))

namespace discovery_server {
namespace agent {

using namespace evbs::ertps::rtps;
using namespace evbs::edds::dds;
using namespace vbsutil::xmlparser;
using TopicDomainkey = std::pair<std::string, uint16_t>;

#define MVBS_FLAG 1
#define AGENT_FLAG 2

/**
 * @brief   Singleton class to manage the launch process of a MicroXRCE-DDS Agent.
 */
class AgentInstance {
 private:
    /**
     * @brief   Default constructor.
     */
    AgentInstance();
    /**
     * @brief   AgentInstance class shall not be copy constructible.
     */
    AgentInstance(const AgentInstance&) = delete;

    AgentInstance(AgentInstance&&) = delete;

    /**
     * @brief   AgentInstance class shall not be copy assignable.
     */
    AgentInstance& operator=(const AgentInstance&) = delete;

    AgentInstance& operator=(AgentInstance&&) = delete;

    void delete_endpoint();

    void qos_convert(DataReaderQos& local_reader_qos, const ddb::EndpointQos& remote_reader_qos);

    void qos_convert(DataWriterQos& local_writer_qos, const ddb::EndpointQos& remote_writer_qos);

    bool create_agent_for_vbs_writer_mvbs_reader(const std::string& topic_name,
                                                 const std::vector<uint16_t>& mvbs_domains,
                                                 const std::vector<uint16_t>& vbs_domains);

    bool create_agent_for_mvbs_writer_vbs_reader(const std::string& topic_name,
                                                 const std::vector<uint16_t>& mvbs_domains,
                                                 const std::vector<uint16_t>& vbs_domains);

    void run();

    void create_participant_endpoint(uint16_t domain_id);

    vbs::TopicImpl* get_topic(uint16_t domain_id, const std::string& topic_name);

    static void update_discoveried_endpoint(
        std::unordered_map<std::string, std::vector<uint16_t>>& discoveried_endpoint, uint16_t domain_id,
        const std::string& topic_name);

    static void agent_ep_match_notify(uint16_t domain_id, const ddb::DiscoveryEndpointInfo& info, bool is_reader);

    void delete_matched_writer(vbs::DataWriterImpl* writer);

    void delete_matched_reader(vbs::DataReaderImpl* reader);

 public:
    static AgentInstance& getInstance();

    bool create(const CLIParser::server_config& server_config);

    void clear();

 private:
    bool enable_inter_host_ {false};
    bool has_changed_ {false};
    std::atomic<bool> stop_ {false};
    std::mutex event_mutex_;
    vbs::shared_mutex endpoint_mutex_;
    std::condition_variable cv_;
    std::thread agent_init_thread_;
    std::string whitelist_;
    std::set<std::string> allowed_topic_list_;

    // key: topic name, value: type name
    std::unordered_map<std::string, std::string> topic_type_names_;

    struct ParticipantEndpoint {
        vbs::DomainParticipantImpl* participant {nullptr};
        vbs::Publisher* publisher {nullptr};
        vbs::Subscriber* subscriber {nullptr};
    };

    // 用来保存已创建的topic
    std::map<TopicDomainkey, vbs::TopicImpl*> agent_topics_;
    std::map<std::string, std::vector<vbs::DataReaderImpl*>> agent_mvbs_readers_by_topic_;
    std::map<std::string, std::vector<vbs::DataReaderImpl*>> agent_vbs_readers_by_topic_;
    std::map<std::string, std::vector<vbs::DataWriterImpl*>> agent_mvbs_writers_by_topic_;
    std::map<std::string, std::vector<vbs::DataWriterImpl*>> agent_vbs_writers_by_topic_;
    std::map<TopicDomainkey, ddb::EndpointQos> qos_by_topic_;
    std::unordered_map<std::string, std::vector<uint16_t>> discoveried_vbs_writers_;
    std::unordered_map<std::string, std::vector<uint16_t>> discoveried_vbs_readers_;
    std::unordered_map<std::string, std::vector<uint16_t>> discoveried_mvbs_writers_;
    std::unordered_map<std::string, std::vector<uint16_t>> discoveried_mvbs_readers_;

    // key: topic name, value: [vbs domains, mvbs domains]
    // 用来保存m核发给a核时，某个topic下a核所有reader节点的domain、m核所有writer节点的domain
    std::unordered_map<std::string, std::pair<std::vector<uint16_t>, std::vector<uint16_t>>>
        created_mvbs_writer_vbs_reader_;

    // 用来保存a核发给m核时，某个topic下a核所有writer节点的domain、m核所有reader节点的domain
    std::unordered_map<std::string, std::pair<std::vector<uint16_t>, std::vector<uint16_t>>>
        created_vbs_writer_mvbs_reader_;

    // 用来保存每个reader需要转发的writer，通过topic_name关联，和domain无关
    std::unordered_map<vbs::DataReaderImpl*, std::vector<vbs::DataWriterImpl*>> writers_by_reader_;

    // 用来保存每个domain的participant、publisher、subscriber信息，较为固定
    std::unordered_map<uint16_t, ParticipantEndpoint> participant_by_domain_;

    std::unordered_map<vbs::DataWriterImpl*, uint16_t> writer_domain_;

    std::unordered_map<vbs::DataReaderImpl*, uint16_t> reader_domain_;

    std::list<std::pair<uint16_t, vbs::DataWriterImpl*>> writer_to_delete_;

    std::list<std::pair<uint16_t, vbs::DataReaderImpl*>> reader_to_delete_;

    class DDSReaderListener : public vbs::DataReaderListenerInner {
     public:
        DDSReaderListener(AgentInstance* agent_obj);

        DDSReaderListener() = delete;

        void on_data_available(vbs::DataReaderImpl* reader) override;

        void on_subscription_matched(vbs::DataReaderImpl* reader, const SubscriptionMatchedStatus& info) override;

        AgentInstance* agent_obj_;
    } reader_listener_;

    class DDSWriterListener : public vbs::DataWriterListenerInner {
     public:
        DDSWriterListener(AgentInstance* agent_obj) : agent_obj_(agent_obj) {}

        DDSWriterListener() = delete;

        void on_publication_matched(vbs::DataWriterImpl* writer, const PublicationMatchedStatus& info) override;

        AgentInstance* agent_obj_;
    } writer_listener_;
};
}  // namespace agent
}  // namespace discovery_server

#endif  // AGENT_INSTANCE_HPP
