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

#include "membuf/MembufMonitor.h"
#include <vector>
#include <algorithm>
#include "membuf/BufferPoolUserManager.h"
#include "BufRefRecord.h"
#include "BufferNode.h"
#include "multi_reader/BufRefRecord.h"
#include "multi_reader/BufferNode.h"
#include "DsfLog.hpp"

namespace mbuf {

#ifndef DSFDDS_MACRO_TO_STRING
#define DSFDDS_MACRO_TO_STRING
#define MAKE_STRING(x) #x
#define DSF_MACRO_TO_STRING(x) MAKE_STRING(x)
#endif /*DSFDDS_MACRO_TO_STRING*/

namespace {
constexpr size_t DefaultAlignAddress = 64U;
template <typename BufferNodeType, typename BufRefRecordType>
const BufferNodeType* GetBufferNodeHeader(const void* shareMemPtr) {
    return reinterpret_cast<const BufferNodeType*>(reinterpret_cast<const uint8_t*>(shareMemPtr) +
                                                   sizeof(BufRefRecordType) + ReserveGapSize);
}

template <typename BufferNodeType>
uint64_t GetBufferNodeOffset(const BufferNodeType* headPtr, size_t index) {
    auto bufNodeHeader = reinterpret_cast<const uint8_t*>(headPtr);
    bufNodeHeader = bufNodeHeader + sizeof(BufferNodeType) * index;
    auto* bufferNode = reinterpret_cast<const BufferNodeType*>(bufNodeHeader);
    return bufferNode->GetOffset();
}

template <typename BufferNodeType>
bool IsBufferNodeFree(const BufferNodeType* headPtr, size_t index) {
    auto bufNodeHeader = reinterpret_cast<const uint8_t*>(headPtr);
    bufNodeHeader = bufNodeHeader + sizeof(BufferNodeType) * index;
    auto* bufferNode = reinterpret_cast<const BufferNodeType*>(bufNodeHeader);
    return bufferNode->IsFree();
}

template <typename BufferNodeType>
size_t GetBufferNodeSize(const BufferNodeType* headPtr, size_t index) {
    auto bufNodeHeader = reinterpret_cast<const uint8_t*>(headPtr);
    bufNodeHeader = bufNodeHeader + sizeof(BufferNodeType) * index;
    auto* bufferNode = reinterpret_cast<const BufferNodeType*>(bufNodeHeader);
    return bufferNode->GetSize();
}

}  // namespace

void MembufMonitor::LogEachLine(int log_level, std::stringstream& ss) {
    // 将 stringstream 转换为 string
    std::string fullContent = ss.str();

    // 使用 istringstream 按行读取
    std::istringstream lineStream(fullContent);
    std::string line;

    while (std::getline(lineStream, line)) {
        switch (log_level) {
            case vbsutil::elog::Log::Error:
                DSF_LOG_ERROR(LogEachLine, line);
                break;
            case vbsutil::elog::Log::Warning:
                DSF_LOG_WARNING(LogEachLine, line);
                break;
            case vbsutil::elog::Log::Info:
                DSF_LOG_INFO(LogEachLine, line);
                break;
            case vbsutil::elog::Log::Debug:
                DSF_LOG_DEBUG(LogEachLine, line);
                break;
            case vbsutil::elog::Log::Event:
                DSF_LOG_EVENT(LogEachLine, line);
                break;
            default:
                DSF_LOG_INFO(LogEachLine, line);
                break;
        }
    }
}

MembufMonitor::MembufMonitor(AllocateMode allocateMode) : allocateMode(allocateMode) {}

bool MembufMonitor::DumpMemBuf(std::ostream& ostream, uint32_t poolId) {
    BufferPoolUserManager bufferPoolUserManager;
    std::size_t len = 0;
    void* buf = nullptr;
    if (bufferPoolUserManager.DumpMembuf(poolId, &buf, len)) {
        ostream.write(static_cast<const char*>(buf), len);
        bufferPoolUserManager.ClosePool();
        return true;
    }
    return false;
}

void mbuf::MembufMonitor::PrintBufRefRecord(const void* shareMemPtr, std::ostream& ostream) const {
    if (allocateMode == MultiReader) {
        PrintBufRefRecordInternal<multiReader::BufRefRecord>(shareMemPtr, ostream, mbuf::MAX_MULTI_REF_COUNT);
    } else {
        PrintBufRefRecordInternal<BufRefRecord>(shareMemPtr, ostream, mbuf::MAX_REF_COUNT);
    }
}

template <typename BufRefRecordType>
void mbuf::MembufMonitor::PrintBufRefRecordInternal(const void* shareMemPtr, std::ostream& ostream,
                                                    uint16_t max_ref_count) const {
    // 1. buffer pool summary info
    auto* bufRefRecord = reinterpret_cast<const BufRefRecordType*>(shareMemPtr);
    ostream << "Pool Summary:"
            << "poolSize=" << bufRefRecord->GetPoolSize() << ",bufCount=" << bufRefRecord->GetBufCount() << std::endl;

    // 2. BufferRecord info
    PrintElement(ostream, "UserPids: 0", 20);
    for (uint16_t i = 1; i < max_ref_count; i++) {
        PrintElement(ostream, i, 10);
    }
    ostream << std::endl;
    PrintElement(ostream, " ", 10);

    for (uint16_t i = 0; i < max_ref_count; i++) {
        PrintElement(ostream, bufRefRecord->GetUserPid(i), 10);
    }
    ostream << std::endl;

    PrintElement(ostream, "UserStates: 0", 22);
    for (uint16_t i = 1; i < max_ref_count; i++) {
        PrintElement(ostream, i, 10);
    }
    ostream << std::endl;
    PrintElement(ostream, "", 12);
    for (uint16_t i = 0; i < max_ref_count; i++) {
        auto userStates = bufRefRecord->GetRefState(i);
        switch (userStates) {
            case 0:
                PrintElement(ostream, "invalid", 10);
                break;
            case 1:
                PrintElement(ostream, "using", 10);
                break;
            case 2:
                PrintElement(ostream, "crash", 10);
                break;
            default:
                DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER,
                              "unknown userStates:" << userStates);
                break;
        }
    }
    ostream << std::endl;
}

void mbuf::MembufMonitor::PrintTinyBufRefRecord(const void* shareMemPtr, std::ostream& ostream) const {
    if (allocateMode == MultiReader) {
        PrintTinyBufRefRecordInternal<multiReader::BufRefRecord>(shareMemPtr, ostream, mbuf::MAX_MULTI_REF_COUNT);
    } else {
        PrintTinyBufRefRecordInternal<BufRefRecord>(shareMemPtr, ostream, mbuf::MAX_REF_COUNT);
    }
}

template <typename BufRefRecordType>
void mbuf::MembufMonitor::PrintTinyBufRefRecordInternal(const void* shareMemPtr, std::ostream& ostream,
                                                        uint16_t max_ref_count) const {
    // 1. buffer pool summary info
    auto* bufRefRecord = reinterpret_cast<const BufRefRecordType*>(shareMemPtr);
    ostream << "RefRecord info bufCount: " << bufRefRecord->GetBufCount();

    // 2. BufferRecord info
    ostream << ",UserPids:";
    for (uint16_t i = 0; i < max_ref_count; i++) {
        ostream << bufRefRecord->GetUserPid(i) << ",";
    }
    ostream << "UserStates:";
    for (uint16_t i = 0; i < max_ref_count; i++) {
        ostream << bufRefRecord->GetRefState(i) << ",";
    }
}

void mbuf::MembufMonitor::PrintBufferNode(const void* shareMemPtr, std::ostream& ostream, uint64_t offset) const {
    if (allocateMode == MultiReader) {
        PrintBufferNodeInternal<multiReader::BufferNode, multiReader::BufRefRecord>(shareMemPtr, ostream,
                                                                                    mbuf::MAX_MULTI_REF_COUNT, offset);
    } else {
        PrintBufferNodeInternal<BufferNode, BufRefRecord>(shareMemPtr, ostream, mbuf::MAX_REF_COUNT, offset);
    }
}

template <typename BufferNodeType, typename BufRefRecordType>
void mbuf::MembufMonitor::PrintBufferNodeInternal(const void* shareMemPtr, std::ostream& ostream,
                                                  uint16_t max_ref_count, uint64_t offset) const {
    // BufferNode info
    auto* headPtr = GetBufferNodeHeader<BufferNodeType, BufRefRecordType>(shareMemPtr);
    PrintElement(ostream, "RefState: 0", 18);
    for (uint16_t i = 1; i < max_ref_count; i++) {
        PrintElement(ostream, i, 8);
    }
    PrintElement(ostream, "offset", 10);
    PrintElement(ostream, "length", 10);
    PrintElement(ostream, "recycleNums", 10);

    ostream << std::endl;

    auto* bufRefRecord = reinterpret_cast<const BufRefRecordType*>(shareMemPtr);
    size_t bufCount = bufRefRecord->GetBufCount();
    for (size_t j = 0; j < bufCount; j++) {
        auto bufNodeHeader = reinterpret_cast<const uint8_t*>(headPtr);
        bufNodeHeader = bufNodeHeader + sizeof(BufferNodeType) * j;
        auto* bufferNode = reinterpret_cast<const BufferNodeType*>(bufNodeHeader);

        if ((bufferNode->GetOffset() != offset) && offset != 0) {
            continue;
        }

        PrintElement(ostream, " ", 10);
        for (uint16_t i = 0; i < max_ref_count; i++) {
            uint64_t extractedValue = bufferNode->GetState(i);
            switch (extractedValue) {
                case 0:
                    PrintElement(ostream, "free", 8);
                    break;
                case 1:
                    PrintElement(ostream, "pending", 8);
                    break;
                case 2:
                    PrintElement(ostream, "using", 8);
                    break;
                default:
                    DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER,
                                  "unknown state value:" << extractedValue);
                    break;
            }
        }

        PrintElement(ostream, bufferNode->GetOffset(), 10);
        PrintElement(ostream, bufferNode->GetSize(), 10);
        PrintElement(ostream, bufferNode->GetRecycleNums(), 10);
        ostream << std::endl;
    }
    ostream << std::endl;
}

bool mbuf::MembufMonitor::ParseDumpMemBuf(std::istream& istream, std::ostream& ostream) {
    // 0. get data from istream
    istream.seekg(0, std::istream::end);
    std::streamsize size = istream.tellg();
    istream.seekg(0, std::istream::beg);

    std::vector<char> buffer(size);
    if (!istream.read(buffer.data(), size)) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_STREAM_READ_ERR, "istream read failed");
    }

    void* shareMemPtr = buffer.data();
    // 1. bufRefRecord info
    PrintBufRefRecord(shareMemPtr, ostream);
    // 2. BufferNode info
    PrintBufferNode(shareMemPtr, ostream);
    // 3. todo buffer node data if need
    return true;
}

MembufStatisticsInfo MembufMonitor::StatisticsInfo(const void* shareMemPtr) const {
    if (allocateMode == MultiReader) {
        return StatisticsInfoInternal<multiReader::BufferNode, multiReader::BufRefRecord>(shareMemPtr,
                                                                                          mbuf::MAX_MULTI_REF_COUNT);
    }
    return StatisticsInfoInternal<BufferNode, BufRefRecord>(shareMemPtr, mbuf::MAX_REF_COUNT);
}

template <typename BufferNodeType, typename BufRefRecordType>
MembufStatisticsInfo MembufMonitor::StatisticsInfoInternal(const void* shareMemPtr, uint16_t max_ref_count) const {
    if (!shareMemPtr) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER, "shareMemPtr nullptr");
        return {};
    }

    MembufStatisticsInfo statisticsInfo {
        0U, 0U, 0U, 0U, 0U, 0U, std::vector<uint64_t>(max_ref_count, 0), std::vector<uint64_t>(max_ref_count, 0),
        0U, 0U, 0U, 0U, 0U};
    // BufferNode info
    auto* headPtr = GetBufferNodeHeader<BufferNodeType, BufRefRecordType>(shareMemPtr);

    auto* bufRefRecord = reinterpret_cast<const BufRefRecordType*>(shareMemPtr);
    statisticsInfo.totalSize = bufRefRecord->GetPoolSize();
    statisticsInfo.maxAllocSize = bufRefRecord->GetMaxAllocSize();
    statisticsInfo.minAllocSize = bufRefRecord->GetMinAllocSize();
    statisticsInfo.allocCount = bufRefRecord->GetAllocCount();
    statisticsInfo.usedPoolSize = bufRefRecord->GetUsedPoolSize();
    statisticsInfo.usedBufCount = bufRefRecord->GetUsedBufCount();

    uint64_t totalFreeSize = 0;
    size_t bufCount = bufRefRecord->GetBufCount();
    for (size_t j = 0; j < bufCount; j++) {
        auto bufNodeHeader = reinterpret_cast<const uint8_t*>(headPtr);
        bufNodeHeader = bufNodeHeader + sizeof(BufferNodeType) * j;
        auto* bufferNode = reinterpret_cast<const BufferNodeType*>(bufNodeHeader);
        if (bufferNode->IsFree()) {
            // Check for consecutive nodes with the same offset.
            auto next_it = j + 1;
            while (next_it < bufCount &&
                   GetBufferNodeOffset<BufferNodeType>(headPtr, next_it) ==
                       GetBufferNodeOffset<BufferNodeType>(headPtr, j) &&
                   IsBufferNodeFree(headPtr, next_it)) {
                j = next_it;
                ++next_it;
                ++statisticsInfo.freeCount;
            }
            // Add the length of the final free node with the same offset.
            if (IsBufferNodeFree(headPtr, j)) {
                totalFreeSize += GetBufferNodeSize(headPtr, j);
                // 更新最大连续free的size
                statisticsInfo.maxConsecutiveFreeSize = std::max(statisticsInfo.maxConsecutiveFreeSize, totalFreeSize);
            }
            ++statisticsInfo.freeCount;
        } else {
            ++statisticsInfo.usedCount;
            statisticsInfo.usedSize += bufferNode->GetSize();
            totalFreeSize = 0;
            // 统计每个reader的处于pending using的总个数
            for (size_t i = 0U; i < MAX_REF_COUNT; i++) {
                auto state = bufferNode->QueryState(i);
                if (state == RefNState::Pending) {
                    ++statisticsInfo.pendingCount[i];
                } else if (state == RefNState::Using) {
                    ++statisticsInfo.usingCount[i];
                }
            }
        }
    }

    // 因为只有分配出去的buffer node GetSize才能获取到准确值，所以这里freeSize使用总量减去usedSize,还需要去除管理结构占据的size
    uint64_t extra_size =
        sizeof(BufRefRecordType) + ReserveGapSize + bufCount * sizeof(BufferNodeType) + ReserveGapSize / 8;
    auto extra_size_aligned = Align(extra_size, DefaultAlignAddress);
    statisticsInfo.freeSize = bufRefRecord->GetPoolSize() - statisticsInfo.usedSize - extra_size_aligned;
    return statisticsInfo;
}

void MembufMonitor::PrintStatisticsInfo(const void* shareMemPtr, std::ostream& ostream) const {

    const MembufStatisticsInfo& info = StatisticsInfo(shareMemPtr);
    if (info.totalSize == 0) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER, "info.totalSize == 0");
        return;
    }
    ostream << "StatisticsInfo: " << std::endl;
    PrintElement(ostream, "total", 16);
    PrintElement(ostream, "used", 16);
    PrintElement(ostream, "free", 16);

    PrintElement(ostream, "usedCount", 16);
    PrintElement(ostream, "freeCount", 16);
    PrintElement(ostream, "maxAllocSize", 16);
    PrintElement(ostream, "minAllocSize", 16);
    PrintElement(ostream, "allocCount", 16);

    ostream << std::endl;

    // print size statistics
    PrintElement(ostream, info.totalSize, 16);
    PrintElement(ostream, info.usedSize, 16);
    PrintElement(ostream, info.freeSize, 16);
    // print state statistics
    PrintElement(ostream, info.usedCount, 16);
    PrintElement(ostream, info.freeCount, 16);
    // print alloc size statistics
    PrintElement(ostream, info.maxAllocSize, 16);
    PrintElement(ostream, info.minAllocSize, 16);
    PrintElement(ostream, info.allocCount, 16);

    ostream << std::endl;
}

std::ostream& operator<<(std::ostream& os, const MembufStatisticsInfo& info) {
    os << "totalSize: " << info.totalSize << " usedSize: " << info.usedSize << " freeSize: " << info.freeSize
       << " maxConsecutiveFreeSize: " << info.maxConsecutiveFreeSize << " usedCount: " << info.usedCount
       << " freeCount: " << info.freeCount << " ";

    os << "pendingCount: ";
    for (size_t i = 0; i < info.pendingCount.size(); ++i) {
        os << info.pendingCount[i] << (i < info.pendingCount.size() - 1 ? ", " : " ");
    }

    os << "usingCount: ";
    for (size_t i = 0; i < info.usingCount.size(); ++i) {
        os << info.usingCount[i] << (i < info.usingCount.size() - 1 ? ", " : " ");
    }

    os << "maxAllocSize: " << info.maxAllocSize << " minAllocSize: " << info.minAllocSize
       << " allocCount: " << info.allocCount;

    return os;
}

}  // namespace mbuf
