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

#ifndef MEMBUF_INC_MEMBUF_MONITOR_H
#define MEMBUF_INC_MEMBUF_MONITOR_H
#include <ostream>
#include <istream>
#include <iomanip>
#include <atomic>
#include <cstdint>
#include <vector>
#include "membuf/BaseType.h"

namespace mbuf {

enum MembufDfxLevel : uint8_t {
    MonitorLevel0,  // 默认监测级别，只输出summary信息,包括alloc
    MonitorLevel1,  // 扩展监测级别1，在level0基础上额外输出统计信息
    MonitorLevel2,  // 扩展监测级别2，在level0基础上额外输出所有buffer node的详细信息
    MonitorMax
};

struct MembufStatisticsInfo {
    // size信息统计
    uint64_t totalSize;
    // 当前非Free状态(pending&using)的buffer node的size总和
    uint64_t usedSize;
    // 当前处于Free状态的buffer node的size总和
    uint64_t freeSize;
    // 最大连续free的size
    uint64_t maxConsecutiveFreeSize;

    // buffer node 状态统计信息
    uint64_t usedCount;  // usedCount 包含状态为using和pending
    uint64_t freeCount;  // 处于free状态的buffer node
    // 统计每个reader的处于pending的总个数
    std::vector<uint64_t> pendingCount;
    // 统计每个reader的处于using的总个数
    std::vector<uint64_t> usingCount;

    // 最大最小alloc size
    uint64_t maxAllocSize;
    uint64_t minAllocSize;
    // 已经分配的次数
    uint64_t allocCount;
    // 已用pool size
    uint64_t usedPoolSize;
    // 已用buf count
    uint64_t usedBufCount;
    friend std::ostream& operator<<(std::ostream& os, const MembufStatisticsInfo& info);
};

class MembufMonitor {
 public:
    explicit MembufMonitor(AllocateMode allocateMode);
    MembufMonitor(MembufMonitor const&) = default;
    MembufMonitor& operator=(MembufMonitor const&) = default;
    MembufMonitor(MembufMonitor&&) = default;
    MembufMonitor& operator=(MembufMonitor&&) = default;

    /**
   * 工具函数
   * @tparam T
   * @param os
   * @param element
   * @param width
   */
    template <class T>
    static void PrintElement(std::ostream& os, T element, const int& width) {
        std::ios_base::fmtflags format(os.flags());
        os << std::left << std::setw(width) << element;
        os.flags(format);
    }

    static void LogEachLine(int log_level, std::stringstream& ss);

    /**
   * dump 功能
   * @param ostream
   * @param poolId
   * @return
   */
    bool DumpMemBuf(std::ostream& ostream, uint32_t poolId);
    bool ParseDumpMemBuf(std::istream& istream, std::ostream& ostream);

    /**
   * 统计size和state信息
   * @param shareMemPtr
   * @param ostream
   */
    MembufStatisticsInfo StatisticsInfo(const void* shareMemPtr) const;
    void PrintStatisticsInfo(const void* shareMemPtr, std::ostream& ostream) const;

    /**
   * state详细信息输出
   * @param shareMemPtr
   * @param ostream
   */
    void PrintBufRefRecord(const void* shareMemPtr, std::ostream& ostream) const;
    void PrintTinyBufRefRecord(const void* shareMemPtr, std::ostream& ostream) const;
    void PrintBufferNode(const void* shareMemPtr, std::ostream& ostream, uint64_t offset = 0) const;
    static uint64_t Align(const uint64_t value, const uint64_t align) { return (value + (align - 1U)) & (~align + 1U); }

 protected:
    template <typename BufRefRecordType>
    void PrintBufRefRecordInternal(const void* shareMemPtr, std::ostream& ostream, uint16_t max_ref_count) const;
    template <typename BufRefRecordType>
    void PrintTinyBufRefRecordInternal(const void* shareMemPtr, std::ostream& ostream, uint16_t max_ref_count) const;
    template <typename BufferNodeType, typename BufRefRecordType>
    void PrintBufferNodeInternal(const void* shareMemPtr, std::ostream& ostream, uint16_t max_ref_count,
                                 uint64_t offset) const;
    template <typename BufferNodeType, typename BufRefRecordType>
    MembufStatisticsInfo StatisticsInfoInternal(const void* shareMemPtr, uint16_t max_ref_count) const;

 private:
    AllocateMode allocateMode;
};

}  // namespace mbuf

#endif  // MEMBUF_INC_MEMBUF_MONITOR_H
