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

#ifndef HELLOWORLDSUBSCRIBER_H_
#define HELLOWORLDSUBSCRIBER_H_

#include "domain/DomainParticipantImpl.hpp"
#include <domain/DomainParticipantListenerInner.hpp>
#include <sub/DataReaderImpl.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include <ertps/subscriber/SampleInfo.h>
#include <ertps/rtps/common/Types.h>

#include <ertps/types/TypeIdentifier.h>
#include <ertps/types/TypeObject.h>

#include <ertps/attributes/SubscriberAttributes.h>

#include <atomic>
#include <condition_variable>
#include <map>

class HelloWorldSubscriber {
 public:
    HelloWorldSubscriber();

    virtual ~HelloWorldSubscriber();

    //! Initialize the subscriber
    bool init();

    //! RUN the subscriber
    void run();

    std::string type_name_;
    uint32_t wait_ms_ = 20000;
    vbsutil::elog::Log::Kind log_level_ = vbsutil::elog::Log::Kind::Error;
    uint32_t domain_id_ = 49;  // 49: liviz domain id
    bool show_data_ = false;
    bool recv_check_ = false;
    std::string whitelist_;

 private:
    bool exit_ = false;
    vbs::DomainParticipantImpl* mp_participant;

    vbs::Subscriber* mp_subscriber;

    std::map<vbs::DataReaderImpl*, vbs::TopicImpl*> topics_;

    std::map<vbs::DataReaderImpl*, evbs::ertps::types::DynamicType_ptr> readers_;

    std::map<vbs::DataReaderImpl*, evbs::ertps::types::DynamicData_ptr> datas_;

 public:
    class SubListener : public vbs::DomainParticipantListenerInner {
     public:
        SubListener(HelloWorldSubscriber* sub) : n_matched(0), n_samples(0), subscriber_(sub) {}

        ~SubListener() override {}

        void on_subscriber_discovery(vbs::DomainParticipantImpl* participant,
                                     evbs::ertps::rtps::ReaderDiscoveryInfo&& info) override {
            auto type_name = info.info.typeName().to_string();
            auto topic_name = info.info.topicName().to_string();
            if (type_discovery_maps_.find(type_name) == type_discovery_maps_.end()) {
                type_discovery_maps_[type_name] = false;
                if (!subscriber_->type_name_.empty()) {
                    if (type_name.compare(subscriber_->type_name_) == 0) {
                        participant->register_remote_type(type_name);
                    }
                } else {
                    participant->register_remote_type(type_name);
                }
            }
            auto it = std::find(type_topic_maps_[type_name].begin(), type_topic_maps_[type_name].end(), topic_name);
            if (it == type_topic_maps_[type_name].end()) {
                topic_recv_maps_[topic_name] = 0;
                type_topic_maps_[type_name].push_back(topic_name);
            }
        }

        void on_publisher_discovery(vbs::DomainParticipantImpl* participant,
                                    evbs::ertps::rtps::WriterDiscoveryInfo&& info) override {
            auto type_name = info.info.typeName().to_string();
            auto topic_name = info.info.topicName().to_string();
            std::cout << "type " << type_name << " topic name " << topic_name << std::endl;
            if (type_discovery_maps_.find(type_name) == type_discovery_maps_.end()) {
                type_discovery_maps_[type_name] = false;
                if (!subscriber_->type_name_.empty()) {
                    if (type_name.compare(subscriber_->type_name_) == 0) {
                        participant->register_remote_type(type_name);
                    }
                } else {
                    participant->register_remote_type(type_name);
                }
            }
            auto it = std::find(type_topic_maps_[type_name].begin(), type_topic_maps_[type_name].end(), topic_name);
            if (it == type_topic_maps_[type_name].end()) {
                topic_recv_maps_[topic_name] = 0;
                type_topic_maps_[type_name].push_back(topic_name);
            }
        }

        void on_data_available(vbs::DataReaderImpl* reader) override;

        void on_subscription_matched(vbs::DataReaderImpl* reader,
                                     const evbs::edds::dds::SubscriptionMatchedStatus& info) override;

        void on_type_discovery(vbs::DomainParticipantImpl*, const std::string& type_name,
                               evbs::ertps::types::DynamicType_ptr dyn_type) override;

        void on_sample_lost(vbs::DataReaderImpl*, const SampleLostStatus& status) override {
            std::cout << "sample lost: " << status.total_count << " Last state: " << status.last_status
                      << " total_count_change: " << status.total_count_change << std::endl;
        }

        int n_matched;

        uint32_t n_samples;

        std::mutex types_mx_;

        std::condition_variable types_cv_;

        std::atomic<bool> reception_flag_ {false};

        HelloWorldSubscriber* subscriber_;

        std::map<std::string, bool> type_discovery_maps_;
        std::map<std::string, std::vector<std::string>> type_topic_maps_;
        std::map<std::string, uint32_t> topic_recv_maps_;
    } m_listener;
};

#endif /* HELLOWORLDSUBSCRIBER_H_ */
