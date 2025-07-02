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

#include "shmmq/shmmq_profile.h"

#include "utils.h"

#include <climits>
#include <cstdint>

#include "DsfLog.hpp"
#include "shmmq/segment.h"

namespace shmmq {

ShmmqProfile& ShmmqProfile::getInstance() {
    static ShmmqProfile shmmq_profile;
    return shmmq_profile;
}

void ShmmqProfile::Init(ProfileStruct* profile) {
    this->profile_ = profile;
}

void ShmmqProfile::OnEnqueued() {
    ++profile_->msg_sent;
}

void ShmmqProfile::OnDequeued(const mbuf::BufferDescriptor* buf_desc) {
    uint64_t receive_timestamp_us =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::high_resolution_clock::now().time_since_epoch())
                                  .count() /
                              1000);
    uint64_t send_timestamp_us = static_cast<uint64_t>(buf_desc->timestamp() / 1000);
    uint32_t latency = 0;
    if (receive_timestamp_us > send_timestamp_us) {
        latency = static_cast<uint32_t>(receive_timestamp_us - send_timestamp_us);
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_PROFILE_TIMESTAMP_ERR,
                      "send_timestamp_us is greater then receive_timestamp_us, the send_timestamp_us is: "
                          << send_timestamp_us << ", the receive_timestamp_us is: " << receive_timestamp_us);
    }

    ++profile_->msg_received;  // test
    if (latency > 0U) {
        if (profile_->min_latency_us == 0U) {
            profile_->min_latency_us = latency;
        } else {
            if (latency < profile_->min_latency_us) {
                profile_->min_latency_us = latency;
            } else if (latency > profile_->max_latency_us) {
                profile_->max_latency_us = latency;
            } else {
                // do nothing
            }
        }
    }

    profile_->sum_latency_us += latency;
}

}  // namespace shmmq
