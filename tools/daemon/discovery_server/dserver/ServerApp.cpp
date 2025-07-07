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
// feature: add multi-domain support for discovery server
// feature: (discovery_server): add platform config and optimize defaults
// ------------------------------------------------------------------

#include "ServerApp.hpp"
#include "RpcDiscoveryPubSubTypes.h"
#include "RpcDataBase.hpp"
#include "agent/PipeInstance.hpp"
#include "agent/AgentInstance.hpp"

#include <condition_variable>
#include <stdexcept>

#include <domain/DomainParticipantFactoryInner.hpp>
#include "pub/DataWriterImpl.hpp"
#include <pub/Publisher.hpp>
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <xmlparser/qos/XMLPublisherQos.hpp>
#include <deps/core/status/SubscriptionMatchedStatus.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include <xmlparser/qos/XMLSubscriberQos.hpp>
#include <sub/SampleInfoInner.hpp>
#include <sub/Subscriber.hpp>
#include <transport/UDPv4TransportDescriptor.h>
#include <discovery/database/DiscoveryEndpointInfo.hpp>
#include <discovery/database/DiscoveryParticipantInfo.hpp>
#include <discovery/database/DiscoveryDataBase.hpp>
#include <deps/common/GuidPrefix_t.hpp>
#include <erpc/discovery/RpcDiscoveryManager.h>

using namespace evbs::ertps::rtps::ddb;

using namespace evbs::edds::dds;

namespace discovery_server {
ServerApp::ServerApp(const CLIParser::server_config& config)
    : participant_(nullptr),
      matched_(0),
      timeout_(config.timeout),
      start_time_(std::chrono::steady_clock::now()),
      stop_(false),
      is_rpc_enable_(config.server_mode == CLIParser::ServerModeKind::ALL_SERVER),
      is_print_table_(config.is_print_table),
      is_agent_enable_(config.server_mode == CLIParser::ServerModeKind::ALL_SERVER ||
                       config.server_mode == CLIParser::ServerModeKind::AGENT_SERVER),
      use_agent_composite_(config.use_agent_composite),
      rpc_type_(new RpcDiscoveryPubSubType()) {
    vbs::DomainParticipantQosInner pqos;

    auto factory = vbs::DomainParticipantFactoryInner::get_instance();
    if (factory->load_profiles() == ReturnCode_t::RETCODE_OK) {
        pqos = factory->get_default_participant_qos();
        logDebug(DISCOVERY_SERVICE, "pqos properties:" << pqos.properties().properties());
    } else {
        throw std::runtime_error("load profiles failed");
    }

    pqos.name("DS-Server");
    pqos.transport().use_builtin_transports = false;

    if (config.listening_list.empty()) {
        throw std::runtime_error("Invalid listening address");
    }

    if (config.is_also_client && config.connection_list.empty()) {
        throw std::runtime_error("Invalid connection address");
    }

    // Configure Listening address
    std::shared_ptr<vbs::transport::TransportDescriptorInterface> descriptor;

    auto descriptor_tmp = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
    descriptor = descriptor_tmp;
    // Add descriptor
    pqos.transport().user_transports.push_back(descriptor);

    for (const Locator_t& locator : config.listening_list) {
        if (locator.kind == LOCATOR_KIND_UDPv4) {
            pqos.name("DS-Server-" + std::to_string(static_cast<int>(locator.address[12])) + "." +
                      std::to_string(static_cast<int>(locator.address[13])) + "." +
                      std::to_string(static_cast<int>(locator.address[14])) + "." +
                      std::to_string(static_cast<int>(locator.address[15])));
            vbsutil::xmlparser::WhiteListAddr waddr;

            waddr = {evbs::ertps::rtps::IPLocator::toIPv4string(locator), 65535};
            descriptor_tmp->interfaceWhiteList.push_back(waddr);
            continue;
        } else if (locator.kind == LOCATOR_KIND_UDS) {
            // throw std::runtime_error("uds is under Development ...... ");
        } else {
            throw std::runtime_error("Invalid Locator type");
        }
    }
    if (config.is_also_client) {
        for (const Locator_t& locator : config.connection_list) {
            if (locator.kind == LOCATOR_KIND_UDPv4) {
                continue;
            } else if (locator.kind == LOCATOR_KIND_UDS) {
                elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_BAD_PARAMETER, "uds is under Development ...... ");
                // auto descriptor_tmp = std::make_shared<vbs::transport::UDPv4TransportDescriptor>();
                // descriptor = descriptor_tmp;
                // // Add descriptor
                // pqos.transport().user_transports.push_back(descriptor);
                throw std::runtime_error("uds is under Development ...... ");
            } else {
                throw std::runtime_error("Invalid Locator type");
            }
        }
    }
    // Set participant as SERVER or RPC
    if (config.is_backup) {
        pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::BACKUP;

        pqos.wire_protocol().prefix.value[0] = 0x0F;
        pqos.wire_protocol().prefix.value[1] = 0x01;
        pqos.wire_protocol().prefix.value[10] = 0x01;
        pqos.wire_protocol().prefix.value[11] = 0x01;
    } else {
        pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = evbs::ertps::rtps::DiscoveryProtocol::SERVER;
    }
    // Set SERVER's listening locator for PDP
    pqos.wire_protocol().builtin.metatrafficUnicastLocatorList = config.listening_list;

    if (config.is_compatible) {
        // TO DO:Set multicast address
        Locator_t multicast_locator;
        uint16_t port(7400);
        evbs::ertps::rtps::IPLocator::setIPv4(multicast_locator, "239.255.0.1");
        evbs::ertps::rtps::IPLocator::setPhysicalPort(multicast_locator, port);
        pqos.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(multicast_locator);
        pqos.wire_protocol().builtin.discovery_config.is_compatible = true;
    }

    // Configure Connection address
    if (config.is_also_client) {
        // Add remote SERVER to CLIENT's list of SERVERs
        pqos.wire_protocol().builtin.discovery_config.m_DiscoveryServers = config.connection_list;
    }

    // 设置 domain list
    if (!config.domain_list.empty()) {
        pqos.wire_protocol().builtin.discovery_config.domainIDs = config.domain_list;
        elogInfo(DISCOVERY_SERVICE, "Setting domain list with " << config.domain_list.size() << " domains");
        for (const auto& domain : config.domain_list) {
            elogInfo(DISCOVERY_SERVICE, "Domain ID: " << domain);
        }
    }

    // 设置 lease duration
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod.seconds = config.announcement;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod.nanosec = 0;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration.seconds = config.duration;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration.nanosec = 0;

    // Create Participant
    participant_ = vbs::DomainParticipantFactoryInner::get_instance()->create_participant(0, pqos, this);

    if (participant_ == nullptr) {
        throw std::runtime_error("Participant initialization failed");
    }

    if (config.is_also_client) {
        elogInfo(DISCOVERY_SERVICE, "Server Participant "
                                        << pqos.name() << " created with GUID " << participant_->guid()
                                        << " listening in address <" << config.listening_list << "> "
                                        << " connected to address <" << config.connection_list << "> ");
    } else {
        elogInfo(DISCOVERY_SERVICE, "Server Participant " << pqos.name() << " created with GUID "
                                                          << participant_->guid() << " listening in address <"
                                                          << config.listening_list << "> ");
    }

    if (is_rpc_enable()) {
        std::string path = "";

        // Register the type
        rpc_type_.register_type(participant_);
        // Create the topic
        rpc_topic_ = participant_->create_topic("rpc_server_topic", rpc_type_.get_type_name(),
                                                vbsutil::xmlparser::getDefaultTopicQos());

        if (rpc_topic_ == nullptr) {
            throw std::runtime_error("RPC Topic initialization failed");
        }
        // Create RPC database
        rpc_data_ = std::make_shared<RpcDataBase>();
        if (rpc_data_ == nullptr) {
            throw std::runtime_error("RPC database initialization failed");
        }
        rpc_data_->rpc_sub = std::make_shared<RpcDiscoverySub>(participant_, rpc_topic_);
        rpc_data_->rpc_pub = std::make_shared<RpcDiscoveryPub>(participant_, rpc_topic_);
        rpc_data_->rpc_sub->set_rpc_data(rpc_data_);

        for (auto locator : config.listening_list) {
            if (locator.kind == LOCATOR_KIND_UDS) {
                path = std::string(reinterpret_cast<const char*>(locator.address));
            }
        }

        rpc_manager_ = std::make_shared<RpcDiscoveryManager>();

        rpc_manager_->run(std::bind(&RpcDataBase::on_rpc_find, rpc_data_, std::placeholders::_1, std::placeholders::_2),
                          std::bind(&RpcDataBase::on_rpc_offer, rpc_data_, std::placeholders::_1),
                          std::bind(&RpcDataBase::on_rpc_stop_offer, rpc_data_, std::placeholders::_1), path);
    }

    if (is_agent_enable()) {
        if (use_agent_composite()) {
            discovery_server::agent::PipeInstance::getInstance().create(config);
        } else {
            discovery_server::agent::AgentInstance::getInstance().create(config);
        }
    }
}
void ServerApp::get_db_info() {
    bool ret = false;
    uint32_t writer_num = 0;
    uint32_t reader_num = 0;
    std::vector<vbsutil::xmlparser::GuidPrefix_t> prefix_list;
    ret = participant_->get_participants_in_db(prefix_list);
    if (!ret) {
        elogInfo(DISCOVERY_SERVICE, "prefix_list get failed! ");
        return;
    }

    for (auto prefix : prefix_list) {
        DiscoveryParticipantInfo part_info;
        // Get the participant info by guid_prefix
        ret = participant_->get_participant_info(prefix, part_info);
        if (ret) {
            if (is_print_table_) {
                elogInfo(DISCOVERY_SERVICE, "prefix: " << prefix << " part_info: " << part_info);
            }
            for (auto reader : part_info.readers()) {
                DiscoveryEndpointInfo reader_info;
                ret = participant_->get_reader_info(reader, reader_info);
                if (ret) {
                    if (!reader_info.is_virtual()) {
                        reader_num++;
                    }
                    if (is_print_table_) {
                        elogInfo(DISCOVERY_SERVICE, "reader_info: " << reader_info);
                    }
                }
            }
            for (auto writer : part_info.writers()) {
                DiscoveryEndpointInfo writer_info;
                ret = participant_->get_writer_info(writer, writer_info);
                if (ret) {
                    if (!writer_info.is_virtual()) {
                        writer_num++;
                    }
                    if (is_print_table_) {
                        elogInfo(DISCOVERY_SERVICE, "writer_info: " << writer_info);
                    }
                }
            }
        }
    }
    elogInfo(DISCOVERY_SERVICE, "participant num: " << prefix_list.size() << " writer num: " << writer_num
                                                    << " reader num: " << reader_num);
}

ServerApp::~ServerApp() {
    if (is_agent_enable()) {
        if (use_agent_composite()) {
            discovery_server::agent::PipeInstance::getInstance().clear();
        } else {
            discovery_server::agent::AgentInstance::getInstance().clear();
        }
    }

    if (is_rpc_enable()) {
        if (participant_ != nullptr) {
            if (rpc_data_ != nullptr && rpc_data_->rpc_sub != nullptr) {
                rpc_data_->rpc_sub->delete_sub(participant_);
            }
            if (rpc_data_ != nullptr && rpc_data_->rpc_pub != nullptr) {
                rpc_data_->rpc_pub->delete_pub(participant_);
            }
        }
        // Delete RPC database
        rpc_data_->rpc_pub.reset();
        rpc_data_->rpc_sub->get_rpc_data().reset();
        rpc_data_->rpc_sub.reset();
        rpc_data_.reset();

        rpc_manager_->stop();
    }

    get_db_info();

    if (nullptr != participant_) {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant_);
    }
}

void ServerApp::on_participant_discovery(vbs::DomainParticipantImpl*,
                                         evbs::ertps::rtps::ParticipantDiscoveryInfo&& info) {
    // static_cast<void>(should_be_ignored);
    if (info.status == evbs::ertps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT) {
        elogInfo(DISCOVERY_SERVICE, "Discovered Participant with GUID " << info.info.m_guid);
        ++matched_;
    } else if (info.status == evbs::ertps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT ||
               info.status == evbs::ertps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT) {
        elogInfo(DISCOVERY_SERVICE, "Dropped Participant with GUID " << info.info.m_guid);
        --matched_;
    }
}

void ServerApp::run() {
#if defined(__unix__)
    (void)pthread_setname_np(pthread_self(), "ServerApp");
#endif  // defined(__unix__)
    while (!is_stopped()) {
        // Wait for period or stop event
        std::unique_lock<std::mutex> period_lock(mutex_);
        // if (is_rpc_enable()) {
        //     // rpc_data_->rpc_sub_thread = std::thread(&RpcDiscoverySub::run, rpc_data_->rpc_sub);
        //     // RPC TO DO: 此处仅为测试用途
        //     rpc_data_->rpc_pub_thread = std::thread(&RpcDiscoveryPub::run, rpc_data_->rpc_pub);
        // }
        if (timeout_ != 0) {
            bool timeout = false;
            cv_.wait_for(period_lock, std::chrono::seconds(timeout_), [&]() {
                timeout =
                    ((std::chrono::steady_clock::now() - start_time_) >= std::chrono::milliseconds(timeout_ * 1000));
                return is_stopped() || timeout;
            });

            if (timeout) {
                stop();
            }
        } else {
            cv_.wait(period_lock, [&]() { return is_stopped(); });
        }
    }
}

bool ServerApp::is_stopped() {
    return stop_.load();
}

bool ServerApp::is_rpc_enable() {
    return is_rpc_enable_;
}

void ServerApp::stop() {
    if (is_agent_enable()) {
        discovery_server::agent::AgentInstance::getInstance().clear();
    }

    // if (is_rpc_enable()) {
    //     rpc_data_->rpc_pub->stop();
    //     rpc_data_->rpc_sub->stop();
    //     // if (rpc_data_->rpc_sub_thread.joinable()) {
    //     //     rpc_data_->rpc_sub_thread.join();
    //     // }
    //     // RPC TO DO: 此处仅为测试用途
    //     if (rpc_data_->rpc_pub_thread.joinable()) {
    //         rpc_data_->rpc_pub_thread.join();
    //     }
    // }

    stop_.store(true);
    cv_.notify_one();
}

}  // namespace discovery_server
