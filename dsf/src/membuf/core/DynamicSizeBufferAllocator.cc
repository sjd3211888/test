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

#include "DynamicSizeBufferAllocator.h"
#include "BufferNode.h"
#include "membuf/ShareBuffer.h"
#include "BufRefRecord.h"
#include "DsfLog.hpp"
#include "membuf/MembufMonitor.h"

namespace mbuf {

DynamicSizeBufferAllocator::DynamicSizeBufferAllocator(const BufferPool& pool, bool isCreated)
    : bufferPool(pool), nodeAllocator(pool.GetBufCount()), alignAddress_(pool.GetAlignAddress()) {
    alignedDataStartOffset_ = Align(pool.GetDataStartOffset(), alignAddress_);
    poolLength_ = pool.GetPoolSize() & (~alignAddress_ + 1U);
    if (isCreated) {
        Init();
    } else {
        Rebuild();
    }
}

template <typename BufferNodeType, typename BufRefRecordType>
void DynamicSizeBufferAllocator::InitInternal() {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    for (size_t i = 0U; i < bufferPool.GetBufCount(); i++) {
        headPtr[i].Init(0U, 0U);
        const auto nodeElem = nodeAllocator.New(&headPtr[i]);
        if (nodeElem == nullptr)
            return;
        freeNodes.push_back(*nodeElem);
    }
    allocPosition_ = alignedDataStartOffset_;
    freePosition_ = poolLength_;
    auto* refRecord = reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord());
    refRecord->SetAllocPosition(allocPosition_);
    refRecord->SetFreePosition(freePosition_);
}

void DynamicSizeBufferAllocator::Init() {
    std::lock_guard<std::mutex> lock(mtx);
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        InitInternal<multiReader::BufferNode, multiReader::BufRefRecord>();
    } else {
        InitInternal<BufferNode, BufRefRecord>();
    }
}

template <typename BufferNodeType>
void DynamicSizeBufferAllocator::RebuildInternal() {
    auto* headPtr = reinterpret_cast<BufferNodeType*>(bufferPool.GetBufferHead());
    std::list<NodeElem*> usedLists;
    for (size_t i = 0U; i < bufferPool.GetBufCount(); i++) {
        auto newElem = nodeAllocator.New(&headPtr[i]);
        if (newElem == nullptr) {
            return;
        }
        if (reinterpret_cast<BufferNodeType*>(newElem->GetNode())->IsFree()) {
            freeNodes.push_back(*newElem);
        } else {
            usedLists.push_back(newElem);
        }
    }
    SortUsedNodes<BufferNodeType>(usedLists);
}

void DynamicSizeBufferAllocator::Rebuild() {
    std::lock_guard<std::mutex> lock(mtx);
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        RebuildInternal<multiReader::BufferNode>();
    } else {
        RebuildInternal<BufferNode>();
    }
}

template <typename BufferNodeType>
void DynamicSizeBufferAllocator::SortUsedNodes(std::list<NodeElem*>& lists) {
    lists.sort([](NodeElem* first, NodeElem* second) {
        return reinterpret_cast<BufferNodeType*>(first->GetNode())->GetOffset() <
               reinterpret_cast<BufferNodeType*>(second->GetNode())->GetOffset();
    });

    if (lists.empty()) {
        allocPosition_ = alignedDataStartOffset_;
        freePosition_ = poolLength_;
    } else {
        // find head
        size_t max_free_buffer_size = reinterpret_cast<BufferNodeType*>(lists.front()->GetNode())->GetOffset() -
                                      alignedDataStartOffset_ + poolLength_ -
                                      reinterpret_cast<BufferNodeType*>(lists.back()->GetNode())->GetOffset() -
                                      reinterpret_cast<BufferNodeType*>(lists.back()->GetNode())->GetSize();
        size_t prev_offset;
        size_t find_head = 0U, index = 0U;
        for (const auto elem : lists) {
            if (index > 0U) {
                const size_t free_size = reinterpret_cast<BufferNodeType*>(elem->GetNode())->GetOffset() - prev_offset;
                if (max_free_buffer_size < free_size) {
                    find_head = index;
                    max_free_buffer_size = free_size;
                }
            }
            prev_offset = reinterpret_cast<BufferNodeType*>(elem->GetNode())->GetOffset() +
                          reinterpret_cast<BufferNodeType*>(elem->GetNode())->GetSize();
            index++;
            usedNodes.push_back(*elem);
        }
        for (size_t i = 0U; i < find_head; i++) {
            usedNodes.push_back(*usedNodes.pop_front());
        }
        freePosition_ = reinterpret_cast<BufferNodeType*>(usedNodes.front()->GetNode())->GetOffset();
        allocPosition_ = reinterpret_cast<BufferNodeType*>(usedNodes.back()->GetNode())->GetOffset() +
                         reinterpret_cast<BufferNodeType*>(usedNodes.back()->GetNode())->GetSize();
    }
}

template <typename BufferNodeType, typename BufRefRecordType>
BufferPtr DynamicSizeBufferAllocator::AllocateInternal(std::size_t size, std::size_t alignedSize) {

    std::lock_guard<std::mutex> lock(mtx);
    if (usedNodes.empty()) {
        const auto nodeElem = freeNodes.pop_front();
        if (nodeElem != nullptr) {
            reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->UpdateAllocInfo(size);
            auto ret = AllocBuffer(*nodeElem, alignedDataStartOffset_, alignedSize);
            DSF_LOG_DEBUG(DynamicSizeBufferAllocator,
                          "Allocate usedNodes empty, buf size:"
                              << size << ",alignedSize" << alignedSize
                              << ",buffer node offset:" << bufferPool.GetOffset(nodeElem->GetNode()) << ",recycleNums:"
                              << reinterpret_cast<BufferNodeType*>(nodeElem->GetNode())->GetRecycleNums());
            return ret;
        } else {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER, "nodeElem is nullptr!");
            return nullptr;
        }
    }
    size_t freeNodeCount = 0U;
    size_t pendingNodeCount = 0U;
    const size_t usedNodesListLen = usedNodes.size();
    size_t iterCnt = 0U;
    while (iterCnt < usedNodesListLen) {
        ++iterCnt;
        auto nodeElem = usedNodes.begin();
        // Firstly, when the front node of the usedNodes list can be released,
        // the optimization considers recycling the node in the usedNodes list.
        // if free buffer size is not enough, recycle next node of the useNodes
        // until the node cannot be released.
        if (reinterpret_cast<BufferNodeType*>(nodeElem->GetNode())->IsFree()) {
            ++freeNodeCount;
            auto buffer = AllocateFromRecycle(alignedSize);
            if (buffer) {
                reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->UpdateAllocInfo(size);
                DSF_LOG_DEBUG(
                    DynamicSizeBufferAllocator,
                    "Allocate from first free usedNodes, buf size:"
                        << size << ",alignedSize" << alignedSize << ",buffer node offset:"
                        << bufferPool.GetOffset(std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                        << ",recycleNums:"
                        << reinterpret_cast<BufferNodeType*>(
                               std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                               ->GetRecycleNums());
                return buffer;
            } else {
                continue;
            }
        }
        // Secondly, if the front node of the usedNodes list cannot be released,
        // try to take a node from the freeNodes list.
        {
            auto buffer = AllocateFromFreeNodes(alignedSize);
            if (buffer) {
                reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->UpdateAllocInfo(size);
                DSF_LOG_DEBUG(
                    DynamicSizeBufferAllocator,
                    "Allocate from freeNodes, buf size:"
                        << size << ",alignedSize" << alignedSize << ",buffer node offset:"
                        << bufferPool.GetOffset(std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                        << ",recycleNums:"
                        << reinterpret_cast<BufferNodeType*>(
                               std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                               ->GetRecycleNums());
                return buffer;
            }
        }
        // Thirdly, if the recover mode is force, try to forcibly recycle.
        if (bufferPool.GetRecoverMode() == RecoverMode::Force) {
            if (reinterpret_cast<BufferNodeType*>(nodeElem->GetNode())->OnRecycle()) {
                ++pendingNodeCount;
                auto buffer = AllocateFromRecycle(alignedSize);
                if (buffer) {
                    reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->UpdateAllocInfo(size);
                    DSF_LOG_DEBUG(DynamicSizeBufferAllocator,
                                  "Allocate from force recover, buf size:"
                                      << size << ",alignedSize" << alignedSize << ",buffer node offset:"
                                      << bufferPool.GetOffset(
                                             std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                                      << ",recycleNums:"
                                      << reinterpret_cast<BufferNodeType*>(
                                             std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetBufferNode())
                                             ->GetRecycleNums());
                    return buffer;
                } else {
                    continue;
                }
            }
        }
        // At last, the previous three steps fail due to insufficient free buffer size
        // or insufficient freeNodes, try to skip the busy node.
        UseNodesPush(*UseNodesPop());
    }

    if (reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->GetEnableDfx() ==
        MembufDfxLevel::MonitorLevel1) {
        std::stringstream ss;
        bufferPool.GetStatisticsInfo(ss);
        MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
    }

    if (reinterpret_cast<BufRefRecordType*>(bufferPool.GetRefRecord())->GetEnableDfx() ==
        MembufDfxLevel::MonitorLevel2) {
        std::stringstream ss;
        bufferPool.GetAllBufferNodeState(ss);
        MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
    }

    std::stringstream ss;
    auto info = bufferPool.GetMembufMonitor().StatisticsInfo(bufferPool.GetShareMemHead());
    bufferPool.GetMembufMonitor().PrintTinyBufRefRecord(bufferPool.GetShareMemHead(), ss);
    if (alignedSize <= info.freeSize && alignedSize <= info.maxConsecutiveFreeSize) {
        ss << "alloc algorithm failed,";
    } else {
        ss << "alloc no free buffer,";
    }
    ss << "allocate size:" << size << ",alignedSize:" << alignedSize << ",usedNodesListLen:" << usedNodesListLen
       << ",freeNodes size:" << freeNodes.size() << ",freeNodeCount:" << freeNodeCount
       << ",pendingNodeCount:" << pendingNodeCount << ",iterCnt:" << iterCnt << ",allocPosition:" << allocPosition_
       << ",freePosition:" << freePosition_ << ", pool statistics info:" << info;
    MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Debug, ss);
    return nullptr;
}
/**
 * allocate size for membuf pool
 * @param size user allocate size
 * @return buffer shared ptr
 */
BufferPtr DynamicSizeBufferAllocator::Allocate(std::size_t size) {
    const size_t alignedSize = Align(size, alignAddress_);

    if (alignedSize > bufferPool.GetBufMaxSize()) {
        DSF_LOG_DEBUG(DynamicSizeBufferAllocator, "pool id:" << bufferPool.GetID() << ",input size:" << size
                                                             << ",alignedSize:" << alignedSize
                                                             << " > buffer max size:" << bufferPool.GetBufMaxSize());
        return nullptr;
    }

    if (alignedSize > bufferPool.GetPoolSize()) {
        DSF_LOG_DEBUG(DynamicSizeBufferAllocator, "pool id:" << bufferPool.GetID() << ",input size:" << size
                                                             << ",alignedSize:" << alignedSize
                                                             << " > buffer pool size:" << bufferPool.GetPoolSize());
        return nullptr;
    }

    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        return AllocateInternal<multiReader::BufferNode, multiReader::BufRefRecord>(size, alignedSize);
    } else {
        return AllocateInternal<BufferNode, BufRefRecord>(size, alignedSize);
    }
}

template <typename BufferNodeType>
BufferPtr DynamicSizeBufferAllocator::AllocBufferInternal(NodeElem& elem, uint64_t bufferOffset, size_t alignedSize,
                                                          bool multiReader) {
    auto* bufferNode = reinterpret_cast<BufferNodeType*>(elem.GetNode());
    bufferNode->ReInit(bufferOffset, alignedSize);
    bufferNode->OnPending(0U);
    UseNodesPush(elem);
    return std::make_shared<ShareBufferOwner>(bufferNode, bufferPool.GetShareMemHead() + bufferNode->GetOffset(),
                                              alignedSize, bufferPool.GetID(), bufferPool.GetOffset(bufferNode),
                                              multiReader);
}

BufferPtr DynamicSizeBufferAllocator::AllocBuffer(NodeElem& elem, uint64_t bufferOffset, size_t alignedSize) {

    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        return AllocBufferInternal<multiReader::BufferNode>(elem, bufferOffset, alignedSize, true);
    } else {
        return AllocBufferInternal<BufferNode>(elem, bufferOffset, alignedSize, false);
    }
}

/**
 * recycle membuf buffer with node is free or in force recycle mode
 * @param alignedSize  user allocate size
 * @return buffer shared ptr
 */
BufferPtr DynamicSizeBufferAllocator::AllocateFromRecycle(size_t alignedSize) {
    const auto nodeElem = UseNodesPop();
    MemOffset bufferOffset = 0U;
    if (nodeElem == nullptr) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER, "UseNodes pop is nullptr!");
        return nullptr;
    }
    if (FreeBufferIsEnough(bufferOffset, alignedSize)) {
        return AllocBuffer(*nodeElem, bufferOffset, alignedSize);
    } else {
        freeNodes.push_back(*nodeElem);
        DSF_LOG_DEBUG(DynamicSizeBufferAllocator,
                      "no free buffer in recycle,alignedSize:" << alignedSize << ",allocPos:" << allocPosition_
                                                               << ",freePos:" << freePosition_);
        return nullptr;
    }
}

BufferPtr DynamicSizeBufferAllocator::AllocateFromFreeNodes(size_t alignedSize) {
    if (freeNodes.empty()) {
        return nullptr;
    } else {
        MemOffset bufferOffset = 0U;
        if (FreeBufferIsEnough(bufferOffset, alignedSize)) {
            const auto nodeElem = freeNodes.pop_front();
            return AllocBuffer(*nodeElem, bufferOffset, alignedSize);
        } else {
            DSF_LOG_DEBUG(DynamicSizeBufferAllocator,
                          "no free buffer in free nodes,alignedSize:" << alignedSize << ",allocPos:" << allocPosition_
                                                                      << ",freePos:" << freePosition_);
            return nullptr;
        }
    }
}

PoolAllocStatus DynamicSizeBufferAllocator::QueryAllocStatus() {
    if (bufferPool.GetConfig().allocateMode == MultiReader) {
        return QueryStatusInternal<multiReader::BufferNode>();
    } else {
        return QueryStatusInternal<BufferNode>();
    }
}

template <typename BufferNodeType>
PoolAllocStatus DynamicSizeBufferAllocator::QueryStatusInternal() {
    std::lock_guard<std::mutex> lock(mtx);
    PoolAllocStatus status {0, 0};

    for (auto& nodeElem : usedNodes) {
        if (!reinterpret_cast<BufferNodeType*>(nodeElem.GetNode())->IsFree()) {
            status.used_buf_count_++;
            status.used_pool_size_ += reinterpret_cast<BufferNodeType*>(nodeElem.GetNode())->GetSize();
        }
    }

    return status;
}

bool DynamicSizeBufferAllocator::HandleUserCrash(UserID userId) {
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

// fix LeakSanitizer: detected memory leaks
DynamicSizeBufferAllocator::~DynamicSizeBufferAllocator() {
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