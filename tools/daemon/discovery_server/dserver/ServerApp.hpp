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

#ifndef VBS_CPP_DISCOVERY_SERVER__SERVERAPP_HPP
#define VBS_CPP_DISCOVERY_SERVER__SERVERAPP_HPP

#include <condition_variable>

#include <edds/dds/builtin/topic/ParticipantBuiltinTopicData.hpp>
#include "domain/DomainParticipantImpl.hpp"
#include <domain/DomainParticipantListenerInner.hpp>
#include <deps/common/TypeSupport.hpp>
#include <erpc/discovery/RpcDiscoveryManager.h>
#include "topic/TopicImpl.hpp"

#include "Application.hpp"
#include "CLIParser.hpp"
#include "RpcDiscoveryPubSubTypes.h"
#include "RpcDataBase.hpp"

using namespace evbs::edds::dds;
using namespace vbs::rpc;

namespace discovery_server {
class ServerApp : public Application, public vbs::DomainParticipantListenerInner {
 public:
    ServerApp(const CLIParser::server_config& config);

    ~ServerApp();

    //! Publisher matched method
    void on_participant_discovery(vbs::DomainParticipantImpl*,
                                  evbs::ertps::rtps::ParticipantDiscoveryInfo&& info) override;

    //! Run publisher
    void run() override;

    //! Stop publisher
    void stop() override;

    void get_db_info();

 private:
    //! Return the current state of execution
    bool is_stopped();

    bool is_rpc_enable();

    bool is_agent_enable() { return is_agent_enable_; }

    bool use_agent_composite() { return use_agent_composite_; }

    vbs::DomainParticipantImpl* participant_;

    int16_t matched_;

    std::mutex mutex_;

    uint16_t timeout_;

    std::chrono::steady_clock::time_point start_time_;

    std::condition_variable cv_;

    std::atomic<bool> stop_;
    // RPC
    bool is_rpc_enable_;

    bool is_print_table_;

    bool is_agent_enable_;

    bool use_agent_composite_;

    vbs::TopicImpl* rpc_topic_;

    vbs::TypeSupport rpc_type_;

    std::shared_ptr<RpcDataBase> rpc_data_;

    std::shared_ptr<RpcDiscoveryManager> rpc_manager_;
};

}  // namespace discovery_server

#endif  // VBS_CPP_DISCOVERY_SERVER__SERVERAPP_HPP
