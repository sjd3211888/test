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

#include <iostream>
#include <sstream>
#include "CommandController.hpp"

namespace vbscmd {

CommandController::CommandController(std::vector<DomainConfig>& domain_configs)
    : stop_(false), domain_configs_(domain_configs) {
    for (const auto& config : domain_configs_) {
        std::shared_ptr<vbstoolsdk::VbsDiscoverer> discoverer = std::make_shared<vbstoolsdk::VbsDiscoverer>(
            config.domain_id, config.xml_file_name, config.participant_profile, false);
        discoverers_.push_back(discoverer);
    }

    command_map_.emplace("p", std::bind(&CommandController::participants_command_, this, std::placeholders::_1));
    command_map_.emplace("participant",
                         std::bind(&CommandController::participants_command_, this, std::placeholders::_1));
    command_map_.emplace("w", std::bind(&CommandController::writers_command_, this, std::placeholders::_1));
    command_map_.emplace("writer", std::bind(&CommandController::writers_command_, this, std::placeholders::_1));
    command_map_.emplace("r", std::bind(&CommandController::readers_command_, this, std::placeholders::_1));
    command_map_.emplace("reader", std::bind(&CommandController::readers_command_, this, std::placeholders::_1));
    command_map_.emplace("t", std::bind(&CommandController::topics_command_, this, std::placeholders::_1));
    command_map_.emplace("topic", std::bind(&CommandController::topics_command_, this, std::placeholders::_1));
    command_map_.emplace("h", std::bind(&CommandController::print_command_, this, std::placeholders::_1));
    command_map_.emplace("help", std::bind(&CommandController::print_command_, this, std::placeholders::_1));
    command_map_.emplace("e", std::bind(&CommandController::exit_command_, this, std::placeholders::_1));
    command_map_.emplace("exit", std::bind(&CommandController::exit_command_, this, std::placeholders::_1));
    command_map_.emplace("q", std::bind(&CommandController::exit_command_, this, std::placeholders::_1));
    command_map_.emplace("quit", std::bind(&CommandController::exit_command_, this, std::placeholders::_1));
}

void CommandController::run() {
    std::string input;
    show_domains_();
    while (!stop_) {
        std::cout << "Insert a command: " << std::endl;
        std::cout << ">> ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            process_args_(input);
        }
    }
}

void CommandController::process_args_(const std::string& input) {
    std::istringstream iss(input);
    std::string param;
    std::vector<std::string> params;
    while (iss >> param) {
        params.emplace_back(param);
    }
    auto it = command_map_.find(params.at(0));
    if (it != command_map_.end()) {
        it->second(params);
    }
}

void CommandController::participants_command_(const std::vector<std::string>& args) {
    (void)args;
    for (uint8_t i = 0; i < discoverers_.size(); i++) {
        show_domain_(i);
        auto participants = discoverers_.at(i)->get_participants_list();
        for (auto participant : participants) {
            show_participant_(participant);
        }
    }
    show_separator_();
}

void CommandController::writers_command_(const std::vector<std::string>& args) {
    (void)args;
    for (uint8_t i = 0; i < discoverers_.size(); i++) {
        show_domain_(i);
        auto writers = discoverers_.at(i)->get_writers_list();
        for (auto writer : writers) {
            show_rw_(writer);
        }
    }
    show_separator_();
}

void CommandController::readers_command_(const std::vector<std::string>& args) {
    (void)args;
    for (uint8_t i = 0; i < discoverers_.size(); i++) {
        show_domain_(i);
        auto readers = discoverers_.at(i)->get_readers_list();
        for (auto reader : readers) {
            show_rw_(reader);
        }
    }
    show_separator_();
}

void CommandController::topics_command_(const std::vector<std::string>& args) {
    (void)args;
    for (uint8_t i = 0; i < discoverers_.size(); i++) {
        show_domain_(i);
        auto topics = discoverers_.at(i)->get_topics_list();
        for (auto topic : topics) {
            show_topic_(topic);
        }
    }
    show_separator_();
}

void CommandController::print_command_(const std::vector<std::string>& args) {
    (void)args;
    std::cout << "p/participant : show participant info" << std::endl;
    std::cout << "w/writer      : show writer info" << std::endl;
    std::cout << "r/reader      : show reader info" << std::endl;
    std::cout << "t/topic       : show topic info" << std::endl;
    std::cout << "h/help        : show help" << std::endl;
    std::cout << "e/exit/q/quit : exit" << std::endl;
}

void CommandController::exit_command_(const std::vector<std::string>& args) {
    (void)args;
    stop_ = true;
}

void CommandController::show_participant_(vbstoolsdk::BaseDiscoverer::ParticipantInfo& participant) {
    std::cout << "- name: " << participant.participant_name << std::endl;
    std::cout << "  guid: " << participant.guid << std::endl;
}

void CommandController::show_rw_(vbstoolsdk::BaseDiscoverer::RWInfo& rw) {
    std::cout << "- guid : " << rw.guid << std::endl;
    std::cout << "  topic: " << rw.topic_name << std::endl;
    std::cout << "  type : " << rw.type_name << std::endl;
}

void CommandController::show_topic_(vbstoolsdk::BaseDiscoverer::TopicInfo& topic) {
    std::cout << "- topic         : " << topic.topic_name << std::endl;
    std::cout << "  type          : " << topic.type_name << std::endl;
    std::cout << "  type_discoverd: " << std::boolalpha << topic.type_discovered << std::endl;
    std::cout << "  rate          : " << topic.rate << " Hz" << std::endl;
    if (topic.writers.size() != 0) {
        std::cout << "  writers       :" << std::endl;
        for (auto guid : topic.writers) {
            std::cout << "    - " << guid << std::endl;
        }
    }
    if (topic.readers.size() != 0) {
        std::cout << "  readers       :" << std::endl;
        for (auto guid : topic.readers) {
            std::cout << "    - " << guid << std::endl;
        }
    }
}

void CommandController::show_separator_() {
    std::cout << "------------------------------" << std::endl;
}

void CommandController::show_domain_(uint8_t i) {
    std::cout << "domain: " << domain_configs_.at(i).domain_id << std::endl;
    show_separator_();
}

void CommandController::show_domains_() {
    std::cout << "Welcome use vbs spy!" << std::endl;
    std::cout << "domains: ";
    for (auto domain_config : domain_configs_) {
        std::cout << domain_config.domain_id << " ";
    }
    std::cout << std::endl;
    show_separator_();
}

}  // namespace vbscmd
