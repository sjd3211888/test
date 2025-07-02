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

#include <cstring>
#include "membuf/BufferPool.h"
#include "BufRefRecord.h"
#include "multi_reader/BufRefRecord.h"
#include "BufferNode.h"
#include "multi_reader/BufferNode.h"
#include "DsfLog.hpp"
#include "membuf/MembufMonitor.h"

namespace mbuf {

BufferPool::BufferPool(const BufPoolCfg& config, int fd, bool isCreated)
    : config(config), fd(fd), monitor(config.allocateMode) {
    if (isCreated) {
        Reset();
    }
}

void BufferPool::Reset() const {
    auto* refRecord = GetRefRecord();
    if (config.allocateMode == MultiReader) {
        memset(refRecord, 0x0, sizeof(multiReader::BufRefRecord));
    } else {
        memset(refRecord, 0x0, sizeof(BufRefRecord));
    }

    const auto bufferNodeHead = GetBufferHead();
    if (config.allocateMode == MultiReader) {
        memset(bufferNodeHead, 0x0, sizeof(multiReader::BufferNode) * config.bufCount);
    } else {
        memset(bufferNodeHead, 0x0, sizeof(BufferNode) * config.bufCount);
    }
}

void* BufferPool::GetBufferHead() const {
    if (config.allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufferNode*>(reinterpret_cast<uint8_t*>(config.shareMemPtr) +
                                                          sizeof(multiReader::BufRefRecord) + ReserveGapSize);
    } else {
        return reinterpret_cast<BufferNode*>(reinterpret_cast<uint8_t*>(config.shareMemPtr) + sizeof(BufRefRecord) +
                                             ReserveGapSize);
    }
}

MemOffset BufferPool::GetDataStartOffset() const {
    uint64_t* dataStart;
    if (config.allocateMode == MultiReader) {
        dataStart =
            reinterpret_cast<uint64_t*>(reinterpret_cast<multiReader::BufferNode*>(GetBufferHead()) + config.bufCount) +
            (ReserveGapSize / 8U);
    } else {
        dataStart = reinterpret_cast<uint64_t*>(reinterpret_cast<BufferNode*>(GetBufferHead()) + config.bufCount) +
                    (ReserveGapSize / 8U);
    }
    return reinterpret_cast<uint64_t>(dataStart) - reinterpret_cast<uint64_t>(config.shareMemPtr);
}

size_t BufferPool::GetPoolSize() const {
    return config.poolSize;
}

void* BufferPool::GetRefRecord() const {
    if (config.allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufRefRecord*>(config.shareMemPtr);
    } else {
        return reinterpret_cast<BufRefRecord*>(config.shareMemPtr);
    }
}

BufferNodeOffset BufferPool::GetOffset(void* bufferNode) const {
    return reinterpret_cast<uint64_t>(bufferNode) - reinterpret_cast<uint64_t>(config.shareMemPtr);
}

void* BufferPool::GetBufferNode(BufferNodeOffset offset) const {
    if (config.allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufferNode*>(reinterpret_cast<uint64_t>(config.shareMemPtr) + offset);
    } else {
        return reinterpret_cast<BufferNode*>(reinterpret_cast<uint64_t>(config.shareMemPtr) + offset);
    }
}

size_t BufferPool::GetRealPoolSize() const {
    if (config.allocateMode == MultiReader) {
        return config.poolSize - (sizeof(multiReader::BufRefRecord) + ReserveGapSize + (config.bufCount * 64U));

    } else {
        return config.poolSize - (sizeof(BufRefRecord) + ReserveGapSize + (config.bufCount * 64U));
    }
}

void BufferPool::GetAllBufferNodeState(std::ostream& ostream) const {
    // 1. BufferRecord info
    monitor.PrintBufRefRecord(GetConfig().shareMemPtr, ostream);
    // 2. BufferNode info
    monitor.PrintBufferNode(GetConfig().shareMemPtr, ostream);
}

void BufferPool::GetStatisticsInfo(std::ostream& ostream) const {
    monitor.PrintStatisticsInfo(GetConfig().shareMemPtr, ostream);
}

const MembufMonitor& BufferPool::GetMembufMonitor() const {
    return monitor;
}

namespace {
template <typename BufferNodeType>
bool IsPoolAllUserFreeImpl(const BufferPool& bufferPool) {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    for (size_t i = 0; i < bufferPool.GetBufCount(); i++) {
        bool free = headPtr[i].IsAllUserFree();
        if (!free) {
            DSF_LOG_DEBUG(BufferPool, "pool id:" << bufferPool.GetID()
                                                 << ",buffer node offset:" << bufferPool.GetOffset(&headPtr[i])
                                                 << ",index:" << i << ",state:" << headPtr[i].PrintRefState()
                                                 << " is not all user RefNState::Free");
            return false;
        }
    }
    return true;
}

template <typename BufferNodeType>
bool IsPoolAllUserNotUsingImpl(const BufferPool& bufferPool) {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    for (size_t i = 0; i < bufferPool.GetBufCount(); i++) {
        bool notUsing = headPtr[i].IsAllUserNotUsing();
        if (!notUsing) {
            DSF_LOG_DEBUG(BufferPool, "pool id:" << bufferPool.GetID()
                                                 << ",buffer node offset:" << bufferPool.GetOffset(&headPtr[i])
                                                 << ",index:" << i << ",state:" << headPtr[i].PrintRefState()
                                                 << " is not all user not using");
            return false;
        }
    }
    return true;
}

template <typename BufferNodeType>
bool IsPoolAllOwnerFreeImpl(const BufferPool& bufferPool) {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    for (size_t i = 0; i < bufferPool.GetBufCount(); i++) {
        bool free = headPtr[i].IsAllOwnerFree();
        if (!free) {
            DSF_LOG_DEBUG(BufferPool, "pool id:" << bufferPool.GetID()
                                                 << ",buffer node offset:" << bufferPool.GetOffset(&headPtr[i])
                                                 << ",index:" << i << ",state:" << headPtr[i].PrintRefState()
                                                 << " is not all user RefNState::Free");
            return false;
        }
    }
    return true;
}

}  // namespace

bool BufferPool::IsAllUserFree() const {
    if (config.allocateMode == MultiReader) {
        return IsPoolAllUserFreeImpl<multiReader::BufferNode>(*this);
    } else {
        return IsPoolAllUserFreeImpl<BufferNode>(*this);
    }
}

bool BufferPool::IsAllUserNotUsing() const {
    if (config.allocateMode == MultiReader) {
        return IsPoolAllUserNotUsingImpl<multiReader::BufferNode>(*this);
    } else {
        return IsPoolAllUserNotUsingImpl<BufferNode>(*this);
    }
}

bool BufferPool::IsAllOwnerFree() const {
    if (config.allocateMode == MultiReader) {
        return IsPoolAllOwnerFreeImpl<multiReader::BufferNode>(*this);
    } else {
        return IsPoolAllOwnerFreeImpl<BufferNode>(*this);
    }
}

}  // namespace mbuf
