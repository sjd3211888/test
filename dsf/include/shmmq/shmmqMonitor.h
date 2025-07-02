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

#ifndef DSFDDS_SHMMQMONITOR_H
#define DSFDDS_SHMMQMONITOR_H
#include <shmmq/segment.h>
#include <ostream>
#include <istream>
#include <iomanip>
#include <atomic>
#include <cstdint>
#include <string>

namespace shmmq {

class ShmmqMonitor {
    friend class QueueSegment;

 public:
    static ShmmqMonitor& GetInstance();
    ShmmqMonitor(ShmmqMonitor const&) = delete;
    ShmmqMonitor& operator=(ShmmqMonitor const&) = delete;
    ShmmqMonitor(ShmmqMonitor&&) = delete;
    ShmmqMonitor& operator=(ShmmqMonitor&&) = delete;
    void PrintShmmqinfo(std::string queue_name, const SegmentStruct* const segment_info, std::ostream& ostream);

 private:
    ShmmqMonitor() = default;
    inline uint64_t IndexMod(uint64_t input, uint32_t length);
};
}  // namespace shmmq

#endif
