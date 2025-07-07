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

#ifndef COMMAND_CONTROLLER_HPP_
#define COMMAND_CONTROLLER_HPP_

#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <functional>
#include <discoverer/VbsDiscoverer.hpp>

namespace vbscmd {
struct DomainConfig {
    uint32_t domain_id;
    std::string xml_file_name;
    std::string participant_profile;
};

class CommandController {
 public:
    CommandController(std::vector<DomainConfig>& domains);
    ~CommandController() {}
    void run();

 private:
    void process_args_(const std::string& input);
    void participants_command_(const std::vector<std::string>& args);
    void writers_command_(const std::vector<std::string>& args);
    void readers_command_(const std::vector<std::string>& args);
    void topics_command_(const std::vector<std::string>& args);
    void print_command_(const std::vector<std::string>& args);
    void exit_command_(const std::vector<std::string>& args);
    void show_participant_(vbstoolsdk::BaseDiscoverer::ParticipantInfo& participant);
    void show_rw_(vbstoolsdk::BaseDiscoverer::RWInfo& rw);
    void show_topic_(vbstoolsdk::BaseDiscoverer::TopicInfo& topic);
    void show_separator_();
    void show_domain_(uint8_t i);
    void show_domains_();

    std::atomic_bool stop_;
    std::vector<DomainConfig> domain_configs_;
    std::vector<std::shared_ptr<vbstoolsdk::VbsDiscoverer>> discoverers_;
    std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> command_map_;
};

}  // namespace vbscmd

#endif
