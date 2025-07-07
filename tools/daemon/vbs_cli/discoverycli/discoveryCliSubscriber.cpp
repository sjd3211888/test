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

#include "discoveryCli.h"
#include <ertps/attributes/ParticipantAttributes.h>
#include <ertps/attributes/SubscriberAttributes.h>
#include <sub/Subscriber.hpp>
#include <sub/SampleInfoInner.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include <domain/DomainParticipantFactoryInner.hpp>
#include <ertps/xmlparser/XMLProfileManager.h>
#include <ertps/types/DynamicDataHelper.hpp>
#include <ertps/types/DynamicDataFactory.h>
#include <transport/UDPv4TransportDescriptor.h>
#include <mutex>

using namespace evbs::edds::dds;
using evbs::ReturnCode_t;

HelloWorldSubscriber::HelloWorldSubscriber()
    : type_name_(""), mp_participant(nullptr), mp_subscriber(nullptr), m_listener(this) {}

bool HelloWorldSubscriber::init() {
    vbs::DomainParticipantQosInner pqos;
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::SIMPLE;
    pqos.name("VbsTypeDiscoveryCli");
    if (!whitelist_.empty()) {
        auto descriptor = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
        vbsutil::xmlparser::WhiteListAddr w_addr = {whitelist_, 65535};
        descriptor->interfaceWhiteList.emplace_back(w_addr);
        pqos.transport().user_transports.push_back(descriptor);
    }
    pqos.transport().use_builtin_transports = false;
    pqos.wire_protocol().builtin.lookup_config.use_client = true;
    pqos.wire_protocol().builtin.lookup_config.use_server = false;

    mp_participant =
        vbs::DomainParticipantFactoryInner::get_instance()->create_participant(domain_id_, pqos, &m_listener);

    if (mp_participant == nullptr) {
        return false;
    }
    if (mp_participant->enable() != ReturnCode_t::RETCODE_OK) {
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(mp_participant);
        return false;
    }

    mp_subscriber = mp_participant->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos(), nullptr);
    if (mp_subscriber == nullptr) {
        return false;
    }
    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber() {
    vbs::DomainParticipantFactoryInner::get_instance()->delete_participant_and_entities(domain_id_);
    topics_.clear();
    readers_.clear();
    datas_.clear();
}

void HelloWorldSubscriber::SubListener::on_subscription_matched(vbs::DataReaderImpl*,
                                                                const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1) {
        n_matched = info.total_count;
    } else if (info.current_count_change == -1) {
        n_matched = info.total_count;
    }
}

void HelloWorldSubscriber::SubListener::on_data_available(vbs::DataReaderImpl* reader) {
    auto dit = subscriber_->datas_.find(reader);
    if (dit != subscriber_->datas_.end()) {
        vbs::LoanableSequence<evbs::ertps::types::DynamicData, std::true_type> Loandatas;
        SampleInfoSeq infos;
        reader->take(Loandatas, infos);
        for (LoanableCollection::size_type i = 0; i < infos.length(); ++i) {
            evbs::ertps::types::DynamicType_ptr type = subscriber_->readers_[reader];
            this->n_samples++;
            if (subscriber_->show_data_) {
                evbs::ertps::types::DynamicDataHelper::print(&Loandatas[i]);
            }
            if (subscriber_->recv_check_) {
                std::cout << "Recv data check pass." << std::endl;
                subscriber_->exit_ = true;
                types_cv_.notify_one();
            }
        }
        topic_recv_maps_[reader->get_topicdescription()->get_name()] += infos.length();
        reader->return_loan(Loandatas, infos);
    }
}

void HelloWorldSubscriber::SubListener::on_type_discovery(vbs::DomainParticipantImpl* participant,
                                                          const std::string& type_name,
                                                          evbs::ertps::types::DynamicType_ptr dyn_type) {
    std::cout << "Discovered type: " << type_name << std::endl;
    if (type_discovery_maps_.find(type_name) != type_discovery_maps_.end()) {
        type_discovery_maps_[type_name] = true;
    } else {
        std::cout << "unknown type " << type_name << " discovery!" << std::endl;
        return;
    }
    vbs::TypeSupport m_type(new evbs::ertps::types::DynamicPubSubType(dyn_type));
    m_type.register_type(participant);
    for (auto& topic_name : type_topic_maps_[type_name]) {
        vbs::TopicImpl* topic =
            participant->create_topic(topic_name, type_name, vbsutil::xmlparser::getDefaultTopicQos());
        if (topic == nullptr) {
            std::cout << "type " << type_name << " topic " << topic_name << "create topic fail." << std::endl;
            continue;
        }

        vbs::DataReaderImpl* reader =
            subscriber_->mp_subscriber->create_datareader_with_profile(topic, "Liviz_reader_prof", this);
        if (reader == nullptr) {
            std::cout << "type " << type_name << " topic " << topic_name << "create reader fail." << std::endl;
            participant->delete_topic(topic);
            continue;
        }
        subscriber_->topics_[reader] = topic;
        subscriber_->readers_[reader] = dyn_type;
        evbs::ertps::types::DynamicData_ptr data(
            evbs::ertps::types::DynamicDataFactory::get_instance()->create_data(dyn_type));
        subscriber_->datas_[reader] = data;
    }
}

void HelloWorldSubscriber::run() {
    std::unique_lock<std::mutex> lock(m_listener.types_mx_);
    auto time_out = std::chrono::system_clock::now() + std::chrono::milliseconds(wait_ms_);
    m_listener.types_cv_.wait_until(lock, time_out, [this] { return exit_; });
    uint32_t success_count = 0;
    for (auto& it : m_listener.type_discovery_maps_) {
        std::cout << "Type " << it.first << " Discovery " << (it.second ? "true" : "false") << std::endl;
        std::cout << " Topic:" << std::endl;
        for (auto& topic_name : m_listener.type_topic_maps_[it.first]) {
            std::cout << "\t" << topic_name << " " << m_listener.topic_recv_maps_[topic_name] << "" << std::endl;
        }
        if (it.second) {
            success_count++;
        }
    }
    std::cout << "Total " << m_listener.type_discovery_maps_.size() << ", success " << success_count << ", fail "
              << m_listener.type_discovery_maps_.size() - success_count << std::endl;
}
