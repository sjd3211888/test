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

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <chrono>
#include <elog/Log.hpp>
#include "ping/VbsPublisher.hpp"
#include "ping/VbsSubscriber.hpp"
#include "cmdlineCom.hpp"
#include <getopt.h>

enum EntityType { PUBLISHER, SUBSCRIBER, NONE };

static void Help() {
    std::cout << "please use: ping -h" << std::endl;
}

static void print_warning(std::string type, const char* opt);

static void printUsage();

void pingCli(const std::vector<std::string>& params) {
    if (params.size() == 0) {
        Help();
        return;
    }

    int argc = params.size();
    std::vector<char*> argv(argc + 1);

    for (size_t i = 0; i < params.size(); ++i) {
        // strdup 动态分配内存用于存储字符串副本
        argv[i] = strdup(params[i].c_str());
    }
    argv[argc] = nullptr;

    int32_t domain = -1;
    int32_t count = 0;
    long sleep = 100;
    std::string topic_name {};
    std::string type_name {};
    std::string type_xml {};
    std::string qos_path {};
    std::string participant_qos {};
    std::string entity_qos {};
    std::string json_path {};
    EntityType type = NONE;
    std::string rtt;
    std::string rtt_xml {};
    bool data_show_on_opt {true};

    if (!strcmp(argv[0], "publisher")) {
        type = EntityType::PUBLISHER;
    } else if (!strcmp(argv[0], "subscriber")) {
        type = EntityType::SUBSCRIBER;
    } else {
        printUsage();
        freeargv(argv);
        return;
    }

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},           {"verbosity", required_argument, 0, 'v'},
        {"domain", required_argument, 0, 'd'},   {"topic", required_argument, 0, 't'},
        {"xml", required_argument, 0, 1},        {"type", required_argument, 0, 2},
        {"qos", required_argument, 0, 3},        {"participant", required_argument, 0, 4},
        {"entity", required_argument, 0, 5},     {"rtt", required_argument, 0, 6},
        {"rtt_xml", required_argument, 0, 7},    {"samples", required_argument, 0, 's'},
        {"interval", required_argument, 0, 'i'}, {"json", required_argument, 0, 8},
        {"data_show", no_argument, 0, 9},        {0, 0, 0, 0}};

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc - 1, argv.data() + 1, "hv:d:t:s:i:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                printUsage();
                return;
            case 'v':
                if (!strcmp(optarg, "Info")) {
                    vbsutil::elog::Log::SetVerbosity(vbsutil::elog::Log::Info);
                } else if (!strcmp(optarg, "Warning")) {
                    vbsutil::elog::Log::SetVerbosity(vbsutil::elog::Log::Warning);
                } else if (!strcmp(optarg, "Error")) {
                    vbsutil::elog::Log::SetVerbosity(vbsutil::elog::Log::Error);
                }
                break;
            case 'd':
                domain = std::atoi(optarg);
                break;
            case 't':
                topic_name = optarg;
                break;
            case 's':
                count = std::atoi(optarg);
                break;
            case 'i':
                if (type == EntityType::PUBLISHER) {
                    sleep = std::atoi(optarg);
                } else {
                    print_warning("publisher", optarg);
                }
                break;
            case 1:
                type_xml = optarg;
                break;
            case 2:
                type_name = optarg;
                break;
            case 3:
                qos_path = optarg;
                break;
            case 4:
                participant_qos = optarg;
                break;
            case 5:
                entity_qos = optarg;
                break;
            case 6:
                rtt = optarg;
                break;
            case 7:
                rtt_xml = optarg;
                break;
            case 8:
                json_path = optarg;
                break;
            case 9:
                data_show_on_opt = true;
                break;
            case '?':
                std::cerr << "ERROR: " << optarg << " is not a valid argument." << std::endl;
                break;
        }
    }

    if (type_name.empty() != type_xml.empty()) {
        std::cerr << "ERROR: type_name must used with xml_path or idl_path." << std::endl;
        printUsage();
        freeargv(argv);
        return;
    }

    if (qos_path.empty() != participant_qos.empty()) {
        std::cerr << "ERROR: qos_path must used with participant_qos." << std::endl;
        printUsage();
        freeargv(argv);
        return;
    }

    if (qos_path.empty() != entity_qos.empty()) {
        std::cerr << "ERROR: qos_path must used with entity qos." << std::endl;
        printUsage();
        freeargv(argv);
        return;
    }

    if (topic_name.empty()) {
        topic_name = "VBS_PING_TOPIC";
        std::cout << "use default topic: VBS_PING_TOPIC" << std::endl;
    }

    if (rtt == "true") {
        topic_name = "VBS_PING_RTT_TEST";
        type_xml = "rtt_test.xml";
        if (rtt_xml == "small") {
            type_name = "rtt_test_small";
        } else if (rtt_xml == "normal") {
            type_name = "rtt_test_normal";
        } else if (rtt_xml == "big") {
            type_name = "rtt_test_big";
        } else {
            type_name = "rtt_test_normal";
            std::cout << "rtt default type: rtt_test_normal" << std::endl;
        }

        if (count == 0) {
            count = 100;
        }
    }

    std::shared_ptr<vbstoolsdk::VbsPublisher> publisher;
    std::shared_ptr<vbstoolsdk::VbsSubscriber> subscriber;
    if (type == EntityType::PUBLISHER) {
        if (qos_path.empty()) {
            qos_path = "publisher.xml";
            participant_qos = "test_xml_pub.xml_profile";
            entity_qos = "test_writer_prof";
        }
        publisher = std::make_shared<vbstoolsdk::VbsPublisher>(qos_path, participant_qos, entity_qos, topic_name,
                                                               domain, type_xml, type_name, json_path);
        if (type_xml.empty()) {
            for (uint8_t i = 5; i > 0; i--) {
                std::cout << "wait 5 seconds to find entity" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        publisher->run(static_cast<uint32_t>(count), static_cast<uint32_t>(sleep));
    } else if (type == EntityType::SUBSCRIBER) {
        if (qos_path.empty()) {
            qos_path = "subscriber.xml";
            participant_qos = "test_xml_sub.xml_profile";
            entity_qos = "test_reader_prof";
        }
        subscriber =
            std::make_shared<vbstoolsdk::VbsSubscriber>(qos_path, participant_qos, entity_qos, topic_name, domain,
                                                        data_show_on_opt, type_xml, type_name, json_path);
        if (type_xml.empty()) {
            for (uint8_t i = 5; i > 0; i--) {
                std::cout << "wait 5 seconds to find entity" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        subscriber->run(static_cast<uint32_t>(count));
    }

    for (;;) {
        if (type == EntityType::PUBLISHER) {
            if (publisher->get_stop_status()) {
                break;
            }
        } else if (type == EntityType::SUBSCRIBER) {
            if (subscriber->get_stop_status()) {
                break;
            }
        } else {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    freeargv(argv);
    return;
}

void printUsage() {
    std::cout << "Usage: HelloWorldExample <publisher|subscriber|monitor>" << std::endl;
    std::cout << "\nGeneral options:" << std::endl;
    std::cout << "  -h      \t--help  \t\tProduce help message." << std::endl;
    std::cout << "  -v      \t--verbosity  \t\tsets the verbosity level. Info/Warning/Error" << std::endl;
    std::cout << "  -d <id> \t--domain=<id>  \t\tDDS domain ID (Default: 0)." << std::endl;
    std::cout << "  -t \t\t--topic  \t\tsets the topic name." << std::endl;
    std::cout << "     \t\t--xml  \t\t\tsets the xml path. must be used with type_name" << std::endl;
    std::cout << "     \t\t--type  \t\tsets the type name. must be used with xml_path or idl_path" << std::endl;
    std::cout << "     \t\t--qos  \t\t\tsets the qos path. must be used with participant and (writer or reader)"
              << std::endl;
    std::cout << "     \t\t--participant  \t\tsets the participant qos profile name. must be used with qos and (writer "
                 "or reader)"
              << std::endl;
    std::cout << "     \t\t--entity  \t\tsets the entity qos profile name. must be used with qos and participant"
              << std::endl;
    std::cout << "     \t\t--rtt  \t\t\tenable rtt function. true/false" << std::endl;
    std::cout << "     \t\t--rtt_xml   \t\tsets the rtt xml. small/normal/big. must be used with rtt." << std::endl;
    std::cout << "\nPublisher options:" << std::endl;
    std::cout << "  -s <num> \t--samples=<num>  \tNumber of samples to send (Default: 0 => infinite samples)."
              << std::endl;
    std::cout << "  -i <num> \t--interval=<num>  \tTime between samples in milliseconds (Default: 100)." << std::endl;
    std::cout << "     \t\t--json  \t\tsets the json file. used to set data" << std::endl;
    std::cout << "\nSubscriber options:" << std::endl;
    std::cout << "  -s <num> \t--samples=<num>  \tNumber of samples to wait for (Default: 0 => infinite samples)."
              << std::endl;
    std::cout << "     \t\t--data_show  \t\tShow data on console (true/false flag)." << std::endl;
}

void print_warning(std::string type, const char* opt) {
    std::cerr << "WARNING: " << opt << " is a " << type << " option, ignoring argument." << std::endl;
}
