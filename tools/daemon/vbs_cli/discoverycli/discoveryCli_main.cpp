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

#include "discoveryCli.h"

#include <elog/Log.hpp>
#include "cmdlineCom.hpp"
using vbsutil::elog::Log;

static void Help() {
    std::cout << "Example:" << std::endl;
    std::cout << "typediscovery -type HelloWorldTypeName -wait 10000 -logLevel 0 -domain 49 -check -data" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "-type      set type name to discovery" << std::endl;
    std::cout << "-wait      set discovery timeout value, ms" << std::endl;
    std::cout << "-logLevel  set log level. 0:error 2:warn 4:info 8:debug" << std::endl;
    std::cout << "-domain    set which domain to discovery" << std::endl;
    std::cout << "-check     finish process once receive a user message" << std::endl;
    std::cout << "-data      print user data" << std::endl;
    std::cout << "-whitelist set whitelist for participant" << std::endl;
}

void typeDiscoveryCli(const std::vector<std::string>& params) {
    if (checkParam(params) == false) {
        Help();
        return;
    }

    int argc = params.size();
    std::vector<char*> argv(argc);
    for (size_t i = 0; i < params.size(); ++i) {
        // strdup 动态分配内存用于存储字符串副本
        argv[i] = strdup(params[i].c_str());
    }

    HelloWorldSubscriber mysub;
    for (int32_t i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-type") == 0) {
            mysub.type_name_ = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-data") == 0) {
            mysub.show_data_ = true;
        } else if (strcmp(argv[i], "-wait") == 0) {
            mysub.wait_ms_ = static_cast<uint32_t>(std::atoi(argv[i + 1]));
            i++;
        } else if (strcmp(argv[i], "-logLevel") == 0) {
            mysub.log_level_ = static_cast<vbsutil::elog::Log::Kind>(std::atoi(argv[i + 1]));
            i++;
        } else if (strcmp(argv[i], "-domain") == 0) {
            mysub.domain_id_ = static_cast<uint32_t>(std::atoi(argv[i + 1]));
            i++;
        } else if (strcmp(argv[i], "-check") == 0) {
            mysub.recv_check_ = true;
        } else if (strcmp(argv[i], "-whitelist") == 0) {
            mysub.whitelist_ = argv[i + 1];
            i++;
        } else {
            std::cout << "unsupported param " << argv[i] << std::endl;
            std::cout << "Example:" << std::endl;
            std::cout << "typediscovery -type HelloWorldTypeName -wait 10000 -logLevel 0 -domain 49 -check -data"
                      << std::endl;
            std::cout << "Usage:" << std::endl;
            std::cout << "-type      set type name to discovery" << std::endl;
            std::cout << "-wait      set discovery timeout value, ms" << std::endl;
            std::cout << "-logLevel  set log level. 0:error 2:warn 4:info 8:debug" << std::endl;
            std::cout << "-domain    set which domain to discovery" << std::endl;
            std::cout << "-check     finish process once receive a user message" << std::endl;
            std::cout << "-data      print user data" << std::endl;
            std::cout << "-whitelist set whitelist for participant" << std::endl;
            freeargv(argv);
            return;
        }
    }
    std::cout << "Starting type " << mysub.type_name_ << " domain " << mysub.domain_id_ << std::endl;
    vbsutil::elog::Log::SetVerbosity(mysub.log_level_);

    if (mysub.init()) {
        mysub.run();
    } else {
        std::cout << "init fail. " << std::endl;
    }
    Log::Reset();
    freeargv(argv);
    return;
}
