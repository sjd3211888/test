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

#include <memory>
#include <unistd.h>
#include <chrono>
#include "replayer/VbsDatReplayer.hpp"
#include <vector>
#include <iostream>
#include "cmdlineCom.hpp"

static void Help() {
    std::cout << "Usage: " << std::endl;
    std::cout << "      "
              << "-i: input dat file, can input multiple" << std::endl;
    std::cout << "      "
              << "-x: xml file name (default: replayer.xml)" << std::endl;
    std::cout << "      "
              << "-p: participant profile (default: test_xml_pub.xml_profile)" << std::endl;
    std::cout << "      "
              << "-w: writer profile (default: test_writer_prof)" << std::endl;
    std::cout << "      "
              << "-d: domain id (default: 0)" << std::endl;
    std::cout << "      "
              << "-r: rti flag (true/false, default: false)" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "      "
              << "replay -i a.dat b.dat -x replayer.xml -p participant.xml_profile -w writer.prof -d 0 -r true"
              << std::endl;
}

void replayCli(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        Help();
        return;
    }

    std::vector<char*> argv;
    argv.push_back(strdup("replay"));
    for (size_t i = 0; i < params.size(); ++i) {
        argv.push_back(strdup(params[i].c_str()));
    }
    int argc = argv.size();

    int opt;
    std::vector<std::string> dat_input;
    std::string xml_file_name = "replayer.xml";
    std::string participant_profile = "test_xml_pub.xml_profile";
    std::string writer_profile = "test_writer_prof";
    int domain_id = 0;
    bool rti_flag = true;
    if (argc > 1) {
        optind = 1;
        while ((opt = getopt(argc, argv.data(), "i:h")) != -1) {
            switch (opt) {
                case 'i':
                    dat_input.emplace_back(std::move(std::string(optarg)));
                    for (int i = optind; i < argc && argv[i][0] != '-'; ++i) {
                        dat_input.emplace_back(std::move(std::string(argv[i])));
                        ++optind;
                    }
                    break;
                case 'x':
                    xml_file_name = optarg;
                    break;
                case 'p':
                    participant_profile = optarg;
                    break;
                case 'w':
                    writer_profile = optarg;
                    break;
                case 'd':
                    domain_id = std::stoi(optarg);
                    break;
                case 'r':
                    rti_flag = std::string(optarg) == "true";
                    break;
                case 'h':
                    Help();
                    freeargv(argv);
                    return;
                default:
                    Help();
                    freeargv(argv);
                    return;
            }
        }
    }
    if (dat_input.empty()) {
        std::cerr << "No dat file input provided." << std::endl;
        Help();
        freeargv(argv);
        return;
    }

    for (auto dat : dat_input) {
        std::shared_ptr<vbstoolsdk::VbsDatReplayer> replayer = std::make_shared<vbstoolsdk::VbsDatReplayer>(
            dat, xml_file_name, participant_profile, writer_profile, domain_id, rti_flag);

        replayer->start();

        while (replayer->get_stop_status() != vbstoolsdk::VbsReplayer::ReplayStatus::STOP) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    freeargv(argv);
    return;
}
