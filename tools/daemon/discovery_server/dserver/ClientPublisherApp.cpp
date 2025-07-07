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

#include "ClientPublisherApp.hpp"

#include <condition_variable>
#include <stdexcept>

#include <domain/DomainParticipantFactoryInner.hpp>
#include "pub/DataWriterImpl.hpp"
#include <pub/Publisher.hpp>
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <xmlparser/qos/XMLPublisherQos.hpp>
#include <transport/UDPv4TransportDescriptor.h>

#include "ClientPubSubTypes.hpp"

using namespace evbs::edds::dds;
using namespace vbsutil::xmlparser;

namespace discovery_server {

ClientPublisherApp::ClientPublisherApp(const CLIParser::client_publisher_config& config)
    : participant_(nullptr),
      publisher_(nullptr),
      topic_(nullptr),
      writer_(nullptr),
      type_(new ClientPubSubType()),
      matched_(0),
      samples_(config.samples),
      stop_(false),
      period_ms_(config.interval) {
    // Set up the data type with initial values
    hello_.index(0);
    hello_.message("Hello world");

    // Configure Participant QoS
    vbs::DomainParticipantQosInner pqos;

    auto factory = vbs::DomainParticipantFactoryInner::get_instance();
    if (factory->load_profiles() == ReturnCode_t::RETCODE_OK) {
        pqos = factory->get_default_participant_qos();
        logDebug(DISCOVERY_SERVICE, "pqos properties:" << pqos.properties().properties());
    } else {
        throw std::runtime_error("load profiles failed");
    }

    pqos.name("DS-Client_pub");
    pqos.transport().use_builtin_transports = false;

    // Create DS locator
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
            pqos.name("DS-Pub-" + addressToString(locator));
        } else {
            throw std::runtime_error("Invalid Locator type");
        }
    }

    for (const Locator_t& locator : config.listening_list) {
        if (locator.kind == LOCATOR_KIND_UDPv4) {
            pqos.name("DS-Pub-" + std::to_string(static_cast<int>(locator.address[12])) + "." +
                      std::to_string(static_cast<int>(locator.address[13])) + "." +
                      std::to_string(static_cast<int>(locator.address[14])) + "." +
                      std::to_string(static_cast<int>(locator.address[15])));
            continue;
        } else if (locator.kind == LOCATOR_KIND_UDS) {
            pqos.name("DS-Pub-" + addressToString(locator));
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

    // Create Domainparticipant
    participant_ =
        vbs::DomainParticipantFactoryInner::get_instance()->create_participant(config.domain_id, pqos, nullptr);

    if (participant_ == nullptr) {
        throw std::runtime_error("Participant initialization failed");
    }

    elogInfo(DISCOVERY_SERVICE, "Publisher Participant " << pqos.name() << " domain-id(" << config.domain_id
                                                         << ") created with GUID " << participant_->guid()
                                                         << " connecting to server <" << config.connection_list
                                                         << "> ");

    // Regsiter type
    type_.register_type(participant_);

    // Create the publisher
    publisher_ = participant_->create_publisher(vbsutil::xmlparser::getDefaultPublisherQos(), nullptr);

    if (publisher_ == nullptr) {
        throw std::runtime_error("Publisher initialization failed");
    }

    // Create the topic
    topic_ =
        participant_->create_topic(config.topic_name, type_.get_type_name(), vbsutil::xmlparser::getDefaultTopicQos());

    if (topic_ == nullptr) {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create de data writer
    DataWriterQos wqos = vbsutil::xmlparser::getDefaultDataWriteQos();

    if (config.reliable) {
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    } else {
        wqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    }

    if (!config.transient_local) {
        wqos.durability().kind = VOLATILE_DURABILITY_QOS;
    }

    // So as not to overwriter the first sample
    // if we publish inmediately after the discovery
    // and the suscription is not prepared yet
    wqos.history().depth = 5;

    writer_ = publisher_->create_datawriter(topic_, wqos, this);

    if (writer_ == nullptr) {
        throw std::runtime_error("DataWriter initialization failed");
    }
}

ClientPublisherApp::~ClientPublisherApp() {
    if (nullptr != participant_) {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant_);
    }
}

void ClientPublisherApp::on_publication_matched(vbs::DataWriterImpl* /*writer*/, const PublicationMatchedStatus& info) {
    if (info.current_count_change == 1) {
        matched_ = static_cast<int16_t>(info.current_count);
        elogInfo(DISCOVERY_SERVICE, "Publisher matched.");
        cv_.notify_one();
    } else if (info.current_count_change == -1) {
        matched_ = static_cast<int16_t>(info.current_count);
        elogInfo(DISCOVERY_SERVICE, "Publisher unmatched.");
    } else {
        elogWarning(
            DISCOVERY_SERVICE,
            info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change");
    }
}

void ClientPublisherApp::run() {
    while (!is_stopped() && ((samples_ == 0) || (hello_.index() < samples_))) {
        if (publish()) {
            elogInfo(DISCOVERY_SERVICE,
                     "SENT Message: '" << hello_.message() << "' with index: '" << hello_.index() << "' ");
        }
        // Wait for period or stop event
        std::unique_lock<std::mutex> period_lock(mutex_);
        cv_.wait_for(period_lock, std::chrono::milliseconds(period_ms_), [&]() { return is_stopped(); });
    }
}

bool ClientPublisherApp::publish() {
    bool ret = false;
    // Wait for the data endpoints discovery
    std::unique_lock<std::mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]() {
        // at least one has been discovered
        return ((matched_ > 0) || is_stopped());
    });

    if (!is_stopped()) {
        hello_.index(hello_.index() + 1);
        ret = writer_->write(&hello_);
        if (!ret) {
            elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR,
                      "SEND Message: '" << hello_.message() << "' with index: '" << hello_.index() << "' FAIL!!!");
        }
    }
    return ret;
}

bool ClientPublisherApp::is_stopped() {
    return stop_.load();
}

void ClientPublisherApp::stop() {
    stop_.store(true);
    cv_.notify_one();
}

}  // namespace discovery_server
