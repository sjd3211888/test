// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: Code style modification
// ------------------------------------------------------------------

#include "ClientSubscriberApp.hpp"

#include <condition_variable>
#include <stdexcept>

#include <deps/core/status/SubscriptionMatchedStatus.hpp>
#include <domain/DomainParticipantFactoryInner.hpp>
#include <sub/DataReaderImpl.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include <xmlparser/qos/XMLSubscriberQos.hpp>
#include <sub/SampleInfoInner.hpp>
#include <sub/Subscriber.hpp>
#include <transport/UDPv4TransportDescriptor.h>

#include "CLIParser.hpp"
#include "Application.hpp"

#include "ClientPubSubTypes.hpp"

using namespace evbs::edds::dds;
using namespace vbsutil::xmlparser;

namespace discovery_server {

ClientSubscriberApp::ClientSubscriberApp(const CLIParser::client_subscriber_config& config)
    : participant_(nullptr),
      subscriber_(nullptr),
      topic_(nullptr),
      reader_(nullptr),
      type_(new ClientPubSubType()),
      samples_(config.samples),
      received_samples_(0),
      stop_(false) {
    vbs::DomainParticipantQosInner pqos;

    // export ERTPS_DEFAULT_PROFILES_FILE=security/secure_profile_encrypt.xml
    auto factory = vbs::DomainParticipantFactoryInner::get_instance();
    if (factory->load_profiles() == ReturnCode_t::RETCODE_OK) {
        pqos = factory->get_default_participant_qos();
        logDebug(DISCOVERY_SERVICE, "pqos properties:" << pqos.properties().properties());
    } else {
        throw std::runtime_error("load profiles failed");
    }

    pqos.name("DS-Client_sub");
    pqos.transport().use_builtin_transports = false;

    std::shared_ptr<vbs::transport::TransportDescriptorInterface> descriptor;

    // Add interfaceWhiteList
    for (const Locator_t& locator : config.connection_list) {
        if (locator.kind == LOCATOR_KIND_UDPv4) {
            static auto descriptor_tmp = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
            vbsutil::xmlparser::WhiteListAddr waddr;

            waddr = {evbs::ertps::rtps::IPLocator::toIPv4string(locator), 65535};
            descriptor_tmp->interfaceWhiteList.push_back(waddr);
            descriptor = descriptor_tmp;
        } else if (locator.kind == LOCATOR_KIND_UDS) {
            pqos.name("DS-Sub-" + addressToString(locator));
        } else {
            throw std::runtime_error("Invalid Locator type");
        }
    }

    for (const Locator_t& locator : config.listening_list) {
        if (locator.kind == LOCATOR_KIND_UDPv4) {
            pqos.name("DS-Sub-" + std::to_string(static_cast<int>(locator.address[12])) + "." +
                      std::to_string(static_cast<int>(locator.address[13])) + "." +
                      std::to_string(static_cast<int>(locator.address[14])) + "." +
                      std::to_string(static_cast<int>(locator.address[15])));
            continue;
        } else if (locator.kind == LOCATOR_KIND_UDS) {
            pqos.name("DS-Sub-" + addressToString(locator));
        } else {
            throw std::runtime_error("Invalid Locator type");
        }
    }
    // Set participant as DS CLIENT
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::CLIENT;

    // Add remote SERVER to CLIENT's list of SERVERs
    pqos.wire_protocol().builtin.discovery_config.m_DiscoveryServers = config.connection_list;

    // Set listening locator for uds
    if (!config.listening_list.empty()) {
        pqos.wire_protocol().builtin.metatrafficUnicastLocatorList = config.listening_list;
    }

    if (descriptor) {
        // Add descriptor
        pqos.transport().user_transports.push_back(descriptor);
    }

    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod.seconds = config.announcement;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod.nanosec = 0;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration.seconds = config.duration;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration.nanosec = 0;

    // Create the Domainparticipant
    participant_ = vbs::DomainParticipantFactoryInner::get_instance()->create_participant(
        config.domain_id, pqos, nullptr, StatusMask::all() >> StatusMask::data_on_readers());

    if (participant_ == nullptr) {
        throw std::runtime_error("Participant initialization failed");
    }

    elogInfo(DISCOVERY_SERVICE, "Subscriber Participant " << pqos.name() << " domain-id(" << config.domain_id
                                                          << " created with GUID " << participant_->guid()
                                                          << " connecting to server <" << config.connection_list
                                                          << "> ");

    // Register the type
    type_.register_type(participant_);

    // Create the subscriber
    subscriber_ = participant_->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos(), nullptr);

    if (subscriber_ == nullptr) {
        throw std::runtime_error("Subscriber initialization failed");
    }

    // Create the topic
    topic_ =
        participant_->create_topic(config.topic_name, type_.get_type_name(), vbsutil::xmlparser::getDefaultTopicQos());

    if (topic_ == nullptr) {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the data reader
    DataReaderQos rqos = vbsutil::xmlparser::getDefaultDataReaderQos();
    ;

    if (config.reliable) {
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    } else {
        rqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    }

    if (config.transient_local) {
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    }

    reader_ = subscriber_->create_datareader(topic_, rqos, this);

    if (reader_ == nullptr) {
        throw std::runtime_error("DataWriter initialization failed");
    }
}

ClientSubscriberApp::~ClientSubscriberApp() {
    if (nullptr != participant_) {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant_);
    }
}

void ClientSubscriberApp::on_subscription_matched(vbs::DataReaderImpl* /*reader*/,
                                                  const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1) {
        elogInfo(DISCOVERY_SERVICE, "Subscriber matched.");
    } else if (info.current_count_change == -1) {
        elogInfo(DISCOVERY_SERVICE, "Subscriber unmatched.");
    } else {
        elogInfo(DISCOVERY_SERVICE, info.current_count_change
                                        << " is not a valid value for SubscriptionMatchedStatus current count change");
    }
}

void ClientSubscriberApp::on_data_available(vbs::DataReaderImpl* reader) {
    vbs::SampleInfoInner info;
    while ((!is_stopped()) && (vbs::ReturnCode_t::RETCODE_OK == reader->take_next_sample(&hello_, &info))) {
        if ((info.instance_state == vbs::ALIVE_INSTANCE_STATE) && info.valid_data) {
            received_samples_++;
            // Print Hello world message data
            elogInfo(DISCOVERY_SERVICE,
                     "Message: '" << hello_.message() << "' with index: '" << hello_.index() << "' RECEIVED");
            if (samples_ > 0 && (received_samples_ >= samples_)) {
                stop();
            }
        }
    }
}

void ClientSubscriberApp::run() {
    std::unique_lock<std::mutex> lck(terminate_cv_mtx_);
    terminate_cv_.wait(lck, [&] { return is_stopped(); });
}

bool ClientSubscriberApp::is_stopped() {
    return stop_.load();
}

void ClientSubscriberApp::stop() {
    stop_.store(true);
    terminate_cv_.notify_all();
}

}  // namespace discovery_server
