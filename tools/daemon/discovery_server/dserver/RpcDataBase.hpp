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

#ifndef VBS_CPP_RPC_DATA_BASE_HPP
#define VBS_CPP_RPC_DATA_BASE_HPP

#include "ServerApp.hpp"
#include "RpcDiscoveryPubSubTypes.h"
#include "RpcDiscovery.h"

#include <domain/DomainParticipantFactoryInner.hpp>
#include "pub/DataWriterImpl.hpp"
#include <pub/Publisher.hpp>
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <xmlparser/qos/XMLPublisherQos.hpp>
#include <deps/core/status/SubscriptionMatchedStatus.hpp>
#include <sub/DataReaderImpl.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include <xmlparser/qos/XMLSubscriberQos.hpp>
#include <sub/SampleInfoInner.hpp>
#include <sub/Subscriber.hpp>
#include <erpc/discovery/RpcDiscoveryMessages.h>
#include <deps/common/shared_mutex.hpp>

using namespace evbs::edds::dds;
using namespace evbs::ertps::rtps;
using namespace vbs::rpc;

namespace discovery_server {
class RpcDataBase;

class RpcDiscoveryMessage : public RpcDiscovery {
 public:
    RpcDiscoveryMessage();

    RpcDiscoveryMessage(RpcDiscovery& info, bool is_local_set);

    ~RpcDiscoveryMessage();

    bool is_local() const { return is_local_; }

    void is_local(bool is_local) { is_local_ = is_local; }

    RpcDiscoveryMessage& operator=(const RpcDiscoveryMessage& other) {
        if (this != &other) {
            RpcDiscovery::operator=(other);
        }
        return *this;
    }

    bool operator==(const RpcDiscoveryMessage& other) const {
        return RpcDiscovery::operator==(other) && (is_local() == other.is_local());
    }

    friend std::ostream& operator<<(std::ostream& os, const RpcDiscoveryMessage& obj) {
        os << static_cast<const RpcDiscovery&>(obj);
        return os;
    }

 private:
    bool is_local_;
};

class RpcDiscoveryPub : public vbs::DataWriterListenerInner {
 public:
    RpcDiscoveryPub(vbs::DomainParticipantImpl* participant, vbs::TopicImpl* topic);

    ~RpcDiscoveryPub();
    void delete_pub(vbs::DomainParticipantImpl* participant);

    RpcDiscoveryPub& operator=(const RpcDiscoveryPub& other) {
        if (this == &other) {
            return *this;
        }

        msg_ = other.msg_;
        publisher_ = other.publisher_;
        writer_ = other.writer_;
        matched_ = other.matched_;
        samples_ = other.samples_;
        stop_ = other.stop_.load();

        return *this;
    }

    //! Publisher matched method
    void on_publication_matched(vbs::DataWriterImpl* writer, const PublicationMatchedStatus& info) override;

    //! Run publisher
    void run();

    //! Stop publisher
    void stop();

    //! Publish a sample
    bool publish(RpcDiscoveryMessage& info);

 private:
    //! Return the current state of execution
    bool is_stopped();

    //! Publish a sample
    bool publish();

    RpcDiscovery msg_;

    vbs::Publisher* publisher_;

    vbs::DataWriterImpl* writer_;

    int16_t matched_;

    uint16_t samples_;

    std::mutex mutex_;

    std::condition_variable cv_;

    std::atomic<bool> stop_;

    const uint16_t period_ms_ {3000};  // in ms
};

class RpcDiscoverySub : public vbs::DataReaderListenerInner {
 public:
    RpcDiscoverySub(vbs::DomainParticipantImpl* participant, vbs::TopicImpl* topic);

    ~RpcDiscoverySub();

    void delete_sub(vbs::DomainParticipantImpl* participant);

    RpcDiscoverySub& operator=(const RpcDiscoverySub& other) {
        if (this == &other) {
            return *this;
        }

        msg_ = other.msg_;
        subscriber_ = other.subscriber_;
        reader_ = other.reader_;
        received_samples_ = other.received_samples_;
        stop_ = other.stop_.load();
        rpc_data_ = other.rpc_data_;

        return *this;
    }

    //! Subscription callback
    void on_data_available(vbs::DataReaderImpl* reader) override;

    //! Subscriber matched method
    void on_subscription_matched(vbs::DataReaderImpl* reader, const SubscriptionMatchedStatus& info) override;

    //! Run subscriber
    void run();

    //! Trigger the end of execution
    void stop();

    std::shared_ptr<RpcDataBase> get_rpc_data() { return rpc_data_; }

    void set_rpc_data(std::shared_ptr<RpcDataBase> rpc_data_set) { rpc_data_ = rpc_data_set; }

 private:
    //! Return the current state of execution
    bool is_stopped();

    RpcDiscovery msg_;

    vbs::Subscriber* subscriber_;

    vbs::DataReaderImpl* reader_;

    uint16_t received_samples_;

    std::atomic<bool> stop_;

    mutable std::mutex terminate_cv_mtx_;

    std::condition_variable terminate_cv_;

    std::shared_ptr<RpcDataBase> rpc_data_;
};

enum OperatingMode : uint16_t { INSERT_OR_UPDATE = 0, DELETE = 1, FIND = 2 };

class RpcDataBase {
 public:
    RpcDataBase();

    ~RpcDataBase();

    int32_t on_rpc_find(RpcFindMessage& find_info, RpcOfferMessage& offer_info);

    int32_t on_rpc_offer(RpcOfferMessage& offer_info);

    int32_t on_rpc_stop_offer(RpcStopOfferMessage& stop_offer_info);

    bool RPC_select(std::string& interface_name, RpcDiscoveryMessage& info);

    bool RPC_insert_update(RpcDiscoveryMessage& info);

    bool RPC_delete(RpcDiscoveryMessage& info);

    std::shared_ptr<RpcDiscoverySub> rpc_sub;

    std::shared_ptr<RpcDiscoveryPub> rpc_pub;

    // std::thread rpc_sub_thread;

    // RPC TO DO: 此处仅为测试用途
    // std::thread rpc_pub_thread;

    //! General mutex
    mutable vbs::shared_mutex mtx;

 private:
    std::map<std::string, RpcDiscoveryMessage> data_;
};

}  // namespace discovery_server

#endif  // VBS_CPP_RPC_DATA_BASE_HPP
