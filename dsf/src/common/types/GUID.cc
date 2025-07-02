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

#include "common/types/GUID.h"
#include "common/types/TypesBase.h"
#include <net/if.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <unistd.h>
#include <mutex>
#include "DsfLog.hpp"

namespace li {
namespace dsfdds {

bool GuidPrefix_s::GenerateGuidPrefix(uint16_t participant_id, uint8_t did, GuidPrefix_s* guidPrefixS) {
    if (guidPrefixS == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                      "input guidPrefixS is nullptr, participant_id:" << participant_id << ", did:" << did);
        return false;
    }
    static uint8_t vendor_id[2] = {0x0F, 0x03};
    static uint8_t mac_address[3];
    static std::once_flag flag;
    static bool ret = false;
    auto func = [&](bool& ret) {
        ifaddrs* if_list = nullptr;
        ifaddrs* ifa = nullptr;

        getifaddrs(&if_list);
        for (ifa = if_list; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family == AF_PACKET &&
                !(ifa->ifa_flags & IFF_LOOPBACK)) {  // Select the first non loopback interface.
                auto sa = (struct sockaddr_ll*)ifa->ifa_addr;
                mac_address[0] = sa->sll_addr[3];
                mac_address[1] = sa->sll_addr[4];
                mac_address[2] = sa->sll_addr[5];
                break;
            }
        }
        if (if_list != nullptr) {
            freeifaddrs(if_list);
            ret = true;
            return true;
        }
        ret = false;
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ILLEGAL_OPERATION,
                      "getifaddrs  failed!" << " participant_id:" << participant_id << ", did:" << did);
        return false;
    };
    // mac地址获取无需每次都调用，仅仅执行一次就可以
    std::call_once(flag, func, std::ref(ret));
    if (ret) {
        const int pid = getpid();
        (void)memcpy(guidPrefixS->value, vendor_id, 2U);
        (void)memcpy(guidPrefixS->value + 2, mac_address, 3U);
        (void)memcpy(guidPrefixS->value + 5, &did, 1U);
        (void)memcpy(guidPrefixS->value + 6, &pid, 4U);
        (void)memcpy(guidPrefixS->value + 10, &participant_id, 2U);
    }
    return ret;
}

}  // namespace dsfdds
}  // namespace li
