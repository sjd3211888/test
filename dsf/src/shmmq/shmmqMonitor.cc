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

#include <shmmq/shmmqMonitor.h>
#include <shmmq/message_queue.h>
#include <cstdint>

namespace shmmq {

ShmmqMonitor& ShmmqMonitor::GetInstance() {
    static ShmmqMonitor shmmqMonitor;
    return shmmqMonitor;
}

inline uint64_t ShmmqMonitor::IndexMod(uint64_t input, uint32_t length) {
    return input & (static_cast<uint64_t>(length - 1));
}

void ShmmqMonitor::PrintShmmqinfo(std::string queue_name, const SegmentStruct* const segment_info,
                                  std::ostream& ostream) {
    uint64_t write_idx = atomic_load(&segment_info->p_state->write_index);
    uint64_t read_idx = atomic_load(&segment_info->p_state->read_index);
    uint32_t q_free_size = 0;
    if ((write_idx - read_idx) <= segment_info->p_config->queue_size) {
        q_free_size = segment_info->p_config->queue_size - (write_idx - read_idx);
    }
    uint32_t aligned_queue_size = segment_info->p_config->aligned_queue_size;
    uint64_t write_idx_m = IndexMod(write_idx, aligned_queue_size);
    uint64_t read_idx_m = IndexMod(read_idx, aligned_queue_size);
    ostream << "queue_name: " << queue_name << ", write_idx_m " << write_idx_m << ", read_idx_m:" << read_idx_m
            << ", write_idx " << write_idx << ", read_idx:" << read_idx << ", q_free_size: " << q_free_size;
}
}  // namespace shmmq
