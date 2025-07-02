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
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "core/Host.hpp"
#include "deps/common/md5.h"

namespace vbs {

Host::Host() {
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    void* tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    vbsutil::MD5 md5;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPV4
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::string ip(addressBuffer);
            if (ip == "127.0.0.1") {
                continue;
            }
            md5.update(ip.c_str(), ip.length());
            break;
        }
    }
    (void)md5.finalize();
    value = 0;
    for (size_t i = 0U; i < sizeof(md5.digest); i += 2U) {
        value ^= ((md5.digest[i] << 8U) | md5.digest[i + 1]);
    }
    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);
}

uint32_t Host::id() {
    return value;
}

}  // namespace vbs
