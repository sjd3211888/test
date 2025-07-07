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

#include <unistd.h>
#include <vector>
#include <memory>
#include <chrono>
#include <iostream>
#include <getopt.h>
#include "CommandController.hpp"
#include "cmdlineCom.hpp"

static void Help() {
    std::cout << "Usage: " << std::endl;
    std::cout << "      "
              << "-d: input domain, can input multiple, default: 0" << std::endl;
    std::cout << "      "
              << "-xml: xml file name for domain (applies to next -d)" << std::endl;
    std::cout << "      "
              << "-profile: participant profile for domain (applies to next -d)" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "      "
              << "spy -d 0 -xml config.xml -profile profile -d 1 -xml config2.xml -profile profile2" << std::endl;
}

void spyCli(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        Help();
        return;
    }

    std::vector<char*> argv;
    argv.push_back(strdup("spy"));
    for (size_t i = 0; i < params.size(); ++i) {
        argv.push_back(strdup(params[i].c_str()));
    }
    int argc = argv.size();
    std::cout << params.size() << std::endl;
    std::cout << argc << std::endl;
    int opt;
    std::vector<vbscmd::DomainConfig> domain_configs;
    std::string xml_file_name;
    std::string participant_profile;

    struct option long_options[] = {{"domain", required_argument, 0, 'd'},
                                    {"xml", required_argument, 0, 'x'},
                                    {"profile", required_argument, 0, 'p'},
                                    {"help", no_argument, 0, 'h'},
                                    {0, 0, 0, 0}};

    if (argc > 1) {
        optind = 1;
        while ((opt = getopt_long(argc, argv.data(), "d:x:p:h", long_options, nullptr)) != -1) {

            switch (opt) {
                case 'd': {
                    uint32_t domain_id = std::stoul(optarg, nullptr, 10);
                    for (int i = optind; i < argc && argv[i][0] != '-'; ++i) {
                        domain_id = std::stoul(argv[i], nullptr, 10);
                        ++optind;
                    }
                    domain_configs.push_back({domain_id, xml_file_name, participant_profile});
                } break;
                case 'x':
                    xml_file_name = optarg;
                    break;
                case 'p':
                    participant_profile = optarg;
                    break;
                case 'h':
                    Help();
                    //freeargv(argv);
                    return;
                default:
                    break;
            }
        }
    }

    if (domain_configs.empty()) {
        domain_configs.push_back({0, xml_file_name, participant_profile});
    }

    std::shared_ptr<vbscmd::CommandController> controller = std::make_shared<vbscmd::CommandController>(domain_configs);

    for (uint8_t i = 5; i > 0; i--) {
        std::cout << "Wait " << i << " seconds to find entity..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    controller->run();
    //freeargv(argv);
    return;
}
