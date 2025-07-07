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
#include "recorder/VbsRecorder.hpp"
#include "cmdlineCom.hpp"

static void Help() {
    std::cout << "Usage: " << std::endl;
    std::cout << "      "
              << "-t: record total time. default 60 seconds" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "      "
              << "record -t 60" << std::endl;
}

void recordCli(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        Help();
        return;
    }

    std::vector<char*> argv;
    argv.push_back(strdup("record"));

    for (size_t i = 0; i < params.size(); ++i) {
        argv.push_back(strdup(params[i].c_str()));
    }
    int argc = argv.size();

    int opt;
    uint32_t total_time = 60;
    if (argc > 1) {
        optind = 1;
        while ((opt = getopt(argc, argv.data(), "t:h")) != -1) {
            switch (opt) {
                case 't':
                    total_time = std::atoi(optarg);
                    break;
                case 'h':
                    Help();
                    freeargv(argv);
                    return;
            }
        }
    }
    std::shared_ptr<vbstoolsdk::VbsRecorder> recorder =
        std::make_shared<vbstoolsdk::VbsRecorder>("recorder.xml", "test_xml_sub.xml_profile", "test_reader_prof");
    int res = recorder->start();
    std::cout << "start record..., total_time(s):" << total_time << std::endl;
    if (res == 0) {
        freeargv(argv);
        return;
    }
    while (total_time) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        total_time--;
    }
    recorder->stop();
    freeargv(argv);
    return;
}
