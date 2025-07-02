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

#include "FixSizeBufferAllocator.h"
#include "BufferNode.h"
#include "multi_reader/BufferNode.h"
#include "membuf/ShareBuffer.h"
#include "BufRefRecord.h"
#include "multi_reader/BufRefRecord.h"
#include <memory>
#include "membuf/MembufMonitor.h"

namespace mbuf {
FixSizeBufferAllocator::FixSizeBufferAllocator(const BufferPool& pool, bool isCreated)
    : bufferPool(pool), nodeAllocator(pool.GetBufCount()) {
    if (isCreated) {
        Init();
    } else {
        Rebuild();
    }
}
template <typename BufferNodeType>
void FixSizeBufferAllocator::InitInternal() {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    MemOffset dataStartOffset = bufferPool.GetDataStartOffset();
    size_t allocSize = 0U;
    for (size_t i = 0U; allocSize < bufferPool.GetRealPoolSize() && i < bufferPool.GetBufCount(); i++) {
        const MemOffset offset = dataStartOffset + i * bufferPool.GetBufMaxSize();
        headPtr[i].Init(offset, bufferPool.GetBufMaxSize());
        const auto nodeElem = nodeAllocator.New(&headPtr[i]);
        if (nodeElem == nullptr) {
            return;
        }
        freeNodes.push_back(*nodeElem);
        allocSize += reinterpret_cast<BufferNodeType*>(nodeElem->GetNode())->GetSize();
    }
}

void FixSizeBufferAllocator::Init() {
    std::lock_guard<std::mutex> lock(mtx);
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        InitInternal<multiReader::BufferNode>();
    } else {
        InitInternal<BufferNode>();
    }
}

template <typename BufferNodeType>
void FixSizeBufferAllocator::RebuildInternal() {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    for (size_t i = 0U; i < bufferPool.GetBufCount(); i++) {
        const auto nodeElem = nodeAllocator.New(&headPtr[i]);
        if (nodeElem == nullptr) {
            return;
        }
        if (reinterpret_cast<BufferNodeType*>(nodeElem->GetNode())->IsFree()) {
            freeNodes.push_back(*nodeElem);
        } else {
            usedNodes.push_back(*nodeElem);
        }
    }
}

void FixSizeBufferAllocator::Rebuild() {
    std::lock_guard<std::mutex> lock(mtx);
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        RebuildInternal<multiReader::BufferNode>();
    } else {
        RebuildInternal<BufferNode>();
    }
}

template <typename BufferNodeType, typename BufRefRecordType>
BufferPtr FixSizeBufferAllocator::AllocateInternal(std::size_t size) {
    std::lock_guard<std::mutex> lock(mtx);
    if (freeNodes.empty()) {
        Recycle();
    }
    if (freeNodes.empty()) {
        if (reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->GetEnableDfx() ==
            MembufDfxLevel::MonitorLevel1) {
            std::stringstream ss;
            ss << "freeNode empty,statistics info:" << std::endl;
            bufferPool.GetStatisticsInfo(ss);
            MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
        }

        if (reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->GetEnableDfx() ==
            MembufDfxLevel::MonitorLevel2) {
            std::stringstream ss;
            ss << "freeNode empty,detail info:" << std::endl;
            bufferPool.GetAllBufferNodeState(ss);
            MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
        }
        return nullptr;
    }
    auto nodeElem = freeNodes.pop_front();
    auto* bufferNode = reinterpret_cast<BufferNodeType*>(nodeElem->GetNode());
    bufferNode->OnPending(0U);
    // TODO check 0
    usedNodes.push_back(*nodeElem);
    bufferNode->IncRecycleNums();
    reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->UpdateAllocInfo(size);
    return std::make_shared<ShareBufferOwner>(bufferNode, bufferPool.GetShareMemHead() + bufferNode->GetOffset(), size,
                                              bufferPool.GetID(), bufferPool.GetOffset(bufferNode));
}

BufferPtr FixSizeBufferAllocator::Allocate(std::size_t size) {
    if (size > bufferPool.GetBufMaxSize()) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER,
                      "Allocate size:" << size << " > bufMaxSize:" << bufferPool.GetBufMaxSize());
        return nullptr;
    }
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        return AllocateInternal<multiReader::BufferNode, multiReader::BufRefRecord>(size);
    } else {
        return AllocateInternal<BufferNode, BufRefRecord>(size);
    }
}

// 优化方案：是只回收list尾部的free的节点
template <typename BufferNodeType>
void FixSizeBufferAllocator::RecycleInternal() {
    for (auto& nodeElem : usedNodes) {
        if (reinterpret_cast<BufferNodeType*>(nodeElem.GetNode())->IsFree()) {
            usedNodes.remove(nodeElem);
            freeNodes.push_back(nodeElem);
            break;
        } else if (bufferPool.GetRecoverMode() == RecoverMode::Force) {
            if (reinterpret_cast<BufferNodeType*>(nodeElem.GetNode())->OnRecycle()) {
                usedNodes.remove(nodeElem);
                freeNodes.push_back(nodeElem);
                break;
            }
        }
    }
}

void FixSizeBufferAllocator::Recycle() {
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        RecycleInternal<multiReader::BufferNode>();
    } else {
        RecycleInternal<BufferNode>();
    }
}

bool FixSizeBufferAllocator::HandleUserCrash(UserID userId) {
    for (auto& nodeElem : usedNodes) {
        if (bufferPool.GetConfig().allocateMode == MultiReader) {
            if (!reinterpret_cast<multiReader::BufferNode*>(nodeElem.GetNode())->OnExcept(userId)) {
                return false;
            }
        } else {
            if (!reinterpret_cast<BufferNode*>(nodeElem.GetNode())->OnExcept(userId)) {
                return false;
            }
        }
    }
    return true;
}

FixSizeBufferAllocator::~FixSizeBufferAllocator() {
    while (!freeNodes.empty()) {
        auto elem = freeNodes.pop_front();
        if (elem)
            nodeAllocator.Delete(*elem);
    }
    while (!usedNodes.empty()) {
        auto elem = usedNodes.pop_front();
        if (elem)
            nodeAllocator.Delete(*elem);
    }
}

}  // namespace mbuf
