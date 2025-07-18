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
// feature: listener code decoupling
// ------------------------------------------------------------------

#include <csignal>
#include <stdexcept>
#include <thread>

#include <domain/DomainParticipantFactoryInner.hpp>
#include <elog/log/Log.h>
#include <elog/FileConsumer.hpp>

#include "dserver/Application.hpp"
#include "CLIParser.hpp"

using vbsutil::elog::Log;
using namespace discovery_server;

std::function<void(int)> stop_app_handler;
void signal_handler(int signum) {
    stop_app_handler(signum);
}

int main(int argc, char** argv) {
    auto ret = EXIT_SUCCESS;
    CLIParser::ds_config config = CLIParser::parse_cli_options(argc, argv);
    uint16_t samples = 0;
    switch (config.entity) {
        case CLIParser::EntityKind::CLIENT_PUBLISHER:
            samples = config.pub_config.samples;
            break;
        case CLIParser::EntityKind::CLIENT_SUBSCRIBER:
            samples = config.sub_config.samples;
            break;
        default:
            break;
    }

    std::string app_name = CLIParser::parse_entity_kind(config.entity);
    std::shared_ptr<Application> app;

    try {
        app = Application::make_app(config);
    } catch (const std::runtime_error& e) {
        elogError(DISCOVERY_SERVICE, RetCode_t::RETCODE_ERROR, app_name << e.what());
        ret = EXIT_FAILURE;
    }

    if (EXIT_FAILURE != ret) {
        std::thread thread(&Application::run, app);

        if (samples == 0) {
            elogInfo(DISCOVERY_SERVICE,
                     app_name << " running. Please press Ctrl+C to stop the " << app_name << " at any time.");
        } else {
            elogInfo(DISCOVERY_SERVICE, app_name << " running for " << samples
                                                 << " samples. Please press Ctrl+C to stop the " << app_name
                                                 << " at any time.");
        }

        stop_app_handler = [&](int signum) {
            elogInfo(DISCOVERY_SERVICE, "\n" << CLIParser::parse_signal(signum) << " received, stopping execution.");
            app->stop();
        };

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
#ifndef _WIN32
        signal(SIGQUIT, signal_handler);
        signal(SIGHUP, signal_handler);
#endif  // _WIN32

        thread.join();
    }

    return ret;
}
