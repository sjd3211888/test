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

#include "ServerApp.hpp"
#include "RpcDiscoveryPubSubTypes.h"
#include "RpcDataBase.hpp"

namespace vbs {
class TopicImpl;
}

using namespace evbs::edds::dds;
namespace discovery_server {

RpcDiscoveryMessage::RpcDiscoveryMessage() : is_local_(true) {}

RpcDiscoveryMessage::RpcDiscoveryMessage(RpcDiscovery& info, bool is_local_set) : RpcDiscovery(info) {
    is_local(is_local_set);
}

RpcDiscoveryMessage::~RpcDiscoveryMessage() {}

RpcDiscoverySub::RpcDiscoverySub(vbs::DomainParticipantImpl* participant, vbs::TopicImpl* topic)
    : subscriber_(nullptr), reader_(nullptr), received_samples_(0), stop_(false) {

    // Create the subscriber
    subscriber_ = participant->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos(), nullptr);

    if (subscriber_ == nullptr) {
        throw std::runtime_error("RPC Subscriber initialization failed");
    }

    // Create the data reader
    DataReaderQos rqos = vbsutil::xmlparser::getDefaultDataReaderQos();
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    rqos.history().kind = KEEP_ALL_HISTORY_QOS;

    reader_ = subscriber_->create_datareader(topic, rqos, this);

    if (reader_ == nullptr) {
        throw std::runtime_error("RPC DataReader initialization failed");
    }
}

RpcDiscoverySub::~RpcDiscoverySub() {}

void RpcDiscoverySub::delete_sub(vbs::DomainParticipantImpl* participant) {
    if (participant != nullptr) {
        if (subscriber_ != nullptr) {
            subscriber_->set_listener(nullptr);
            if (reader_ != nullptr) {
                subscriber_->delete_datareader(reader_);
            }
            participant->delete_subscriber(subscriber_);
        }
    }
}

void RpcDiscoverySub::on_subscription_matched(vbs::DataReaderImpl* /*reader*/, const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1) {
        elogInfo(DISCOVERY_SERVICE, "RPC Subscriber matched.");
    } else if (info.current_count_change == -1) {
        elogInfo(DISCOVERY_SERVICE, "RPC Subscriber unmatched.");
    } else {
        elogInfo(DISCOVERY_SERVICE,
                 info.current_count_change
                     << " is not a valid value for RPC SubscriptionMatchedStatus current count change");
    }
}

void RpcDiscoverySub::on_data_available(vbs::DataReaderImpl* reader) {
    vbs::SampleInfoInner info;
    while ((!is_stopped()) && (vbs::ReturnCode_t::RETCODE_OK == reader->take_next_sample(&msg_, &info))) {
        // TO DO: RPC try modify to contentfilter
        if (reader->guid().guidPrefix == iHandle2GUID(info.publication_handle).guidPrefix) {
            logDebug(DISCOVERY_SERVICE, "From same DServer guidPrefix: " << reader->guid().guidPrefix << "return.");
            continue;
        }
        if ((info.instance_state == vbs::ALIVE_INSTANCE_STATE) && info.valid_data) {
            received_samples_++;
            RpcDiscoveryMessage rev_info(msg_, false);
            // TO DO: RPC process rpc info
            if (msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) {
                (void)rpc_data_->RPC_insert_update(rev_info);
                logDebug(
                    DISCOVERY_SERVICE,
                    ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '" : "DELETE '")
                        << msg_.interface_name() << "' with '" << msg_.service_name() << " : " << msg_.hostid() << " : "
                        << msg_.ip_address_port() << " : " << msg_.transport_type() << "' RECEIVED");
            } else if (msg_.operating_mode() == OperatingMode::DELETE) {
                (void)rpc_data_->RPC_delete(rev_info);
                logDebug(
                    DISCOVERY_SERVICE,
                    ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '" : "DELETE '")
                        << msg_.interface_name() << "' with '" << msg_.service_name() << " : " << msg_.hostid() << " : "
                        << msg_.ip_address_port() << " : " << msg_.transport_type() << "' RECEIVED");
            } else {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR,
                          "Error " << ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '"
                                                                                                  : "DELETE '")
                                   << msg_.interface_name() << "' with '" << msg_.service_name() << " : "
                                   << msg_.hostid() << " : " << msg_.ip_address_port() << " : " << msg_.transport_type()
                                   << "' RECEIVED");
                return;
            }
        }
    }
}

void RpcDiscoverySub::run() {
    std::unique_lock<std::mutex> lck(terminate_cv_mtx_);
    terminate_cv_.wait(lck, [&] { return is_stopped(); });
}

bool RpcDiscoverySub::is_stopped() {
    return stop_.load();
}

void RpcDiscoverySub::stop() {
    stop_.store(true);
    terminate_cv_.notify_all();
}

RpcDiscoveryPub::RpcDiscoveryPub(vbs::DomainParticipantImpl* participant, vbs::TopicImpl* topic)
    : publisher_(nullptr), writer_(nullptr), samples_(0), stop_(false) {

    // Create the subscriber
    publisher_ = participant->create_publisher(vbsutil::xmlparser::getDefaultPublisherQos(), nullptr);

    if (publisher_ == nullptr) {
        throw std::runtime_error("RPC Publisher initialization failed");
    }

    // Create the data reader
    DataWriterQos wqos = vbsutil::xmlparser::getDefaultDataWriteQos();
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    wqos.history().kind = KEEP_ALL_HISTORY_QOS;

    writer_ = publisher_->create_datawriter(topic, wqos, this);

    if (writer_ == nullptr) {
        throw std::runtime_error("RPC DataWriter initialization failed");
    }
}

RpcDiscoveryPub::~RpcDiscoveryPub() {}

void RpcDiscoveryPub::delete_pub(vbs::DomainParticipantImpl* participant) {
    if (participant != nullptr) {
        if (publisher_ != nullptr) {
            publisher_->set_listener(nullptr);
            if (writer_ != nullptr) {
                publisher_->delete_datawriter(writer_);
            }
            participant->delete_publisher(publisher_);
        }
    }
}

void RpcDiscoveryPub::on_publication_matched(vbs::DataWriterImpl* /*writer*/, const PublicationMatchedStatus& info) {
    if (info.current_count_change == 1) {
        matched_ = static_cast<int16_t>(info.current_count);
        elogInfo(DISCOVERY_SERVICE, "RPC Publisher matched.");
        cv_.notify_one();
    } else if (info.current_count_change == -1) {
        matched_ = static_cast<int16_t>(info.current_count);
        elogInfo(DISCOVERY_SERVICE, "RPC Publisher unmatched.");
    } else {
        elogWarning(
            DISCOVERY_SERVICE,
            info.current_count_change << " is not a valid value for RPC PublicationMatchedStatus current count change");
    }
}

#if 0
// RPC TO DO: 此处仅为测试用途
void RpcDiscoveryPub::run() {
    while (!is_stopped()) {
        if (publish()) {
            elogInfo(DISCOVERY_SERVICE,
                     "SEND Message:" << ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '" : "DELETE '")
                     << msg_.service_name() << "' with '" << msg_.ip_address_port() << ":" << msg_.interface_name() << ":" << msg_.hostid() << "' ");
        }
        // Wait for period or stop event
        std::unique_lock<std::mutex> period_lock(mutex_);
        cv_.wait_for(period_lock, std::chrono::milliseconds(period_ms_), [&]() { return is_stopped(); });
    }
}

// RPC TO DO: 此处仅为测试用途
bool RpcDiscoveryPub::publish() {
    bool ret = true;
    static uint32_t test = 0;
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]() {
        // at least one has been discovered
        return ((matched_ > 0) || is_stopped());
    });
    if (!is_stopped()) {
        msg_.service_name("test" + std::to_string(test%10));
        msg_.ip_address_port("127.0.0.1:1234");
        msg_.interface_name("123");
        msg_.hostid(test%10);
        if ((test/10) > 1) {
            msg_.operating_mode(OperatingMode::DELETE);
        } else {
            msg_.operating_mode(OperatingMode::INSERT_OR_UPDATE);
        }
        ret = writer_->write(&msg_);
        test++;
    }
    if (!ret) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR,
                    "Error SEND Message:" << ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '" : "DELETE '")
                     << msg_.service_name() << "' with '" << msg_.ip_address_port() << ":" << msg_.interface_name() << ":" << msg_.hostid() << "' FAIL!!!");
    }
    return ret;
}
#endif

bool RpcDiscoveryPub::publish(RpcDiscoveryMessage& info) {
    bool ret = true;
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]() {
        // at least one has been discovered
        return ((matched_ > 0) || is_stopped());
    });

    if (!is_stopped()) {
        msg_.service_name(info.service_name());
        msg_.ip_address_port(info.ip_address_port());
        msg_.interface_name(info.interface_name());
        msg_.transport_type(info.transport_type());
        msg_.hostid(info.hostid());
        msg_.operating_mode(info.operating_mode());
        ret = writer_->write(&msg_);
    }
    if (!ret) {
        elogError(
            DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR,
            "Error SEND Message:" << ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '"
                                                                                                 : "DELETE '")
                                  << msg_.interface_name() << "' with '" << msg_.service_name() << " : "
                                  << msg_.hostid() << " : " << msg_.ip_address_port() << " : " << msg_.transport_type()
                                  << "' FAIL!!!");
    } else {
        elogInfo(DISCOVERY_SERVICE,
                 "SEND Message:" << ((msg_.operating_mode() == OperatingMode::INSERT_OR_UPDATE) ? "INSERT_OR_UPDATE '"
                                                                                                : "DELETE '")
                                 << msg_.interface_name() << "' with '" << msg_.service_name() << " : " << msg_.hostid()
                                 << " : " << msg_.ip_address_port() << " : " << msg_.transport_type() << "' ");
    }
    return ret;
}

bool RpcDiscoveryPub::is_stopped() {
    return stop_.load();
}

void RpcDiscoveryPub::stop() {
    stop_.store(true);
    cv_.notify_one();
}

RpcDataBase::RpcDataBase() {}

RpcDataBase::~RpcDataBase() {}

int32_t RpcDataBase::on_rpc_find(RpcFindMessage& find_info, RpcOfferMessage& offer_info) {
    bool ret = false;
    RpcDiscoveryMessage info;

    ret = RPC_select(find_info.interface_name, info);
    if (ret == false) {
        return -1;
    }

    offer_info.interface_name = info.interface_name();
    offer_info.service_name = info.service_name();
    offer_info.host_id = info.hostid();
    offer_info.ip = info.ip_address_port();
    offer_info.transport_type = info.transport_type();

    return 0;
}

int32_t RpcDataBase::on_rpc_offer(RpcOfferMessage& offer_info) {
    bool ret = false;
    RpcDiscoveryMessage info;

    info.interface_name() = offer_info.interface_name;
    info.service_name() = offer_info.service_name;
    info.hostid() = offer_info.host_id;
    info.ip_address_port() = offer_info.ip;
    info.transport_type() = offer_info.transport_type;

    ret = RPC_insert_update(info);
    if (ret == false) {
        return -1;
    }

    return 0;
}

int32_t RpcDataBase::on_rpc_stop_offer(RpcStopOfferMessage& stop_offer_info) {
    bool ret = false;
    RpcDiscoveryMessage info;

    info.interface_name() = stop_offer_info.interface_name;

    ret = RPC_delete(info);
    if (ret == false) {
        return -1;
    }

    return 0;
}

bool RpcDataBase::RPC_select(std::string& interface_name, RpcDiscoveryMessage& info) {
    if (interface_name.empty()) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR, "Select interface name is empty!!!");
        return false;
    }

    vbs::shared_lock<vbs::shared_mutex> disc_lock(mtx);

    auto itr = data_.find(interface_name);
    if (itr == data_.end()) {
        elogInfo(DISCOVERY_SERVICE, "Cannot find interface name: " << interface_name);
        return false;
    }

    info = itr->second;
    elogInfo(DISCOVERY_SERVICE, "find interface name: " << interface_name
                                                        << " discovery info: " << info.interface_name() << " : "
                                                        << info.service_name() << " : " << info.ip_address_port()
                                                        << " : " << info.transport_type() << " : " << info.hostid());
    info.operating_mode(OperatingMode::FIND);

    return true;
}

bool RpcDataBase::RPC_insert_update(RpcDiscoveryMessage& info) {
    if (info.interface_name().empty()) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR, "Insert interface name is empty!!!");
        return false;
    }

    {
        std::unique_lock<vbs::shared_mutex> disc_lock(mtx);
        auto itr = data_.find(info.interface_name());
        if (itr == data_.end()) {
            data_.insert(std::make_pair(info.interface_name(), info));
            elogInfo(DISCOVERY_SERVICE,
                     "Insert interface name: " << info.interface_name() << " discovery info: " << info.interface_name()
                                               << " : " << info.service_name() << " : " << info.ip_address_port()
                                               << " : " << info.transport_type() << " : " << info.hostid());
        } else if (itr != data_.end() && itr->second != info) {
            itr->second = info;
            elogInfo(DISCOVERY_SERVICE,
                     "Update interface name: " << info.service_name() << " discovery info: " << info.interface_name()
                                               << " : " << info.service_name() << " : " << info.ip_address_port()
                                               << " : " << info.transport_type() << " : " << info.hostid());
        } else {
            logDebug(DISCOVERY_SERVICE,
                     "Recieve interface name: " << info.service_name() << " discovery info: " << info.interface_name()
                                                << " : " << info.service_name() << " : " << info.ip_address_port()
                                                << " : " << info.transport_type() << " : " << info.hostid()
                                                << " don't need update.");
            return false;
        }
    }

    info.operating_mode(OperatingMode::INSERT_OR_UPDATE);

    if (info.is_local()) {
        rpc_pub->publish(info);
        elogInfo(DISCOVERY_SERVICE, "Synchronize INSERT_OR_UPDATE interface name: "
                                        << info.interface_name() << " discovery info: " << info.interface_name()
                                        << " : " << info.service_name() << " : " << info.ip_address_port() << " : "
                                        << info.transport_type() << " : " << info.hostid());
    }

    return true;
}

bool RpcDataBase::RPC_delete(RpcDiscoveryMessage& info) {
    if (info.interface_name().empty()) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR, "Delete interface name is empty!!!");
        return false;
    }

    {
        std::unique_lock<vbs::shared_mutex> disc_lock(mtx);
        auto itr = data_.find(info.interface_name());
        if (itr != data_.end()) {
            data_.erase(info.interface_name());
            elogInfo(DISCOVERY_SERVICE,
                     "Delete interface name: " << info.interface_name() << " discovery info: " << info.interface_name()
                                               << " : " << info.service_name() << " : " << info.ip_address_port()
                                               << " : " << info.transport_type() << " : " << info.hostid());
        } else {
            elogInfo(DISCOVERY_SERVICE,
                     "Cannot find interface name: " << info.interface_name() << " don't need delete.");
            return false;
        }
    }

    info.operating_mode(OperatingMode::DELETE);
    if (info.is_local()) {
        rpc_pub->publish(info);
        elogInfo(DISCOVERY_SERVICE, "Synchronize DELETE interface name: "
                                        << info.interface_name() << " discovery info: " << info.interface_name()
                                        << " : " << info.service_name() << " : " << info.ip_address_port() << " : "
                                        << info.transport_type() << " : " << info.hostid());
    }

    return true;
}

}  // namespace discovery_server
