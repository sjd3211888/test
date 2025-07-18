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

#include "Application.hpp"

#include "ClientPublisherApp.hpp"
#include "ClientSubscriberApp.hpp"
#include "CLIParser.hpp"
#include "ServerApp.hpp"

using namespace evbs::edds::dds;

namespace discovery_server {

//! Factory method to create a publisher or subscriber
std::shared_ptr<Application> Application::make_app(const CLIParser::ds_config& config) {
    std::shared_ptr<Application> entity;
    switch (config.entity) {
        case CLIParser::EntityKind::CLIENT_PUBLISHER:
            entity = std::make_shared<ClientPublisherApp>(config.pub_config);
            break;
        case CLIParser::EntityKind::CLIENT_SUBSCRIBER:
            entity = std::make_shared<ClientSubscriberApp>(config.sub_config);
            break;
        case CLIParser::EntityKind::SERVER:
            entity = std::make_shared<ServerApp>(config.srv_config);
            break;
        case CLIParser::EntityKind::UNDEFINED:
        default:
            throw std::runtime_error("Entity initialization failed");
            break;
    }
    return entity;
}

}  // namespace discovery_server
