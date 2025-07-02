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
#include <atomic>

#if !defined(_WIN32)
#include "core/Host.hpp"
#endif
#include "core/Guid.hpp"

namespace vbs {

VBSGuid::VBSGuid() {}

// ip + pid + num
uint16_t VBSGuid::generate() {
    static std::atomic<int> guid_num(0);
    guid_num++;
    memset(value, 0, 16);
#if !defined(_WIN32)
    static Host host;
    uint32_t host_id = host.id();
#else
    uint16_t host_id = 0x01;
#endif
    value[0] = 0x00U;
    value[1] = 0x00U;
    value[2] = static_cast<uint8_t>(host_id & 0xFF);
    value[3] = static_cast<uint8_t>((host_id >> 8) & 0xFF);
    value[4] = static_cast<uint8_t>((host_id >> 16) & 0xFF);
    value[5] = static_cast<uint8_t>((host_id >> 24) & 0xFF);

    int pid = vbs_get_pid_info();
    value[6] = static_cast<octet>(pid & 0xFF);
    value[7] = static_cast<octet>((pid >> 8) & 0xFF);
    value[8] = static_cast<octet>((pid >> 16) & 0xFF);
    value[9] = static_cast<octet>((pid >> 24) & 0xFF);

    value[10] = static_cast<octet>(guid_num & 0xFF);
    value[11] = static_cast<octet>((guid_num >> 8) & 0xFF);
    value[12] = static_cast<octet>((guid_num >> 16) & 0xFF);
    value[13] = static_cast<octet>((guid_num >> 24) & 0xFF);
    value[14] = 0x00U;
    value[15] = 0x00U;

    return guid();
}

uint16_t VBSGuid::guid() {
    uint16_t v = 0;
    vbsutil::MD5 md5;
    md5.update(value, 16);
    (void)md5.finalize();
    for (size_t i = 0U; i < sizeof(md5.digest); i += 2U) {
        v ^= ((md5.digest[i] << 8U) | md5.digest[i + 1]);
    }
    return v;
}

VBSGuid VBSGuid::operator=(const VBSGuid* id) {
    memcpy(this->value, id->value, 16);
    return *this;
}

int VBSGuid::vbs_get_pid_info() {
    int pid_id;
#if defined(_WIN32)
    pid_id = (uint32_t)_getpid();
#else
    pid_id = (uint32_t)getpid();
#endif
    return pid_id;
}

}  // namespace vbs
