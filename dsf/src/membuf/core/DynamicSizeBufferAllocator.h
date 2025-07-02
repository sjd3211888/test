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

#ifndef MBUF_SRC_CORE_DYNAMICSIZEBUFFERALLOCATOR_H
#define MBUF_SRC_CORE_DYNAMICSIZEBUFFERALLOCATOR_H

#include "BufferAllocator.h"
#include <mutex>
#include "Link.h"
#include "ObjectAlloctor.h"
#include "membuf/BufferPool.h"
#include "BufferNode.h"
#include "multi_reader/BufferNode.h"
#include "UserCrashHandler.h"
#include "NodeElem.h"
#include "BufferAllocatorBase.h"
#include "BufRefRecord.h"
#include "multi_reader/BufRefRecord.h"
#include <list>

namespace mbuf {

constexpr uint16_t SKIP_RECYCLE_TRY_COUNTS_MAX = 2U;
constexpr uint16_t SKIP_RECYCLE_NEED_FREE_NODES_MIN = 3U;

struct DynamicSizeBufferAllocator : BufferAllocatorBase {
    DynamicSizeBufferAllocator(const BufferPool& pool, bool isCreated = true);
    virtual ~DynamicSizeBufferAllocator();
    BufferPtr Allocate(std::size_t size) override;
    static uint64_t Align(const uint64_t value, const uint64_t align) { return (value + (align - 1U)) & (~align + 1U); }
    uint64_t GetAlignedAddress() const { return alignAddress_; }
    PoolAllocStatus QueryAllocStatus() override;

 private:
    bool HandleUserCrash(UserID) override;
    BufferPtr AllocBuffer(NodeElem& elem, uint64_t bufferOffset, size_t alignedSize);
    //bool FreeBufferIsEnough(MemOffset *, size_t);
    BufferPtr AllocateFromRecycle(size_t alignedSize);
    BufferPtr AllocateFromFreeNodes(size_t alignedSize);
    void Init();
    void Rebuild();

    NodeElem* UseNodesPop() {
        // By using the offset of the second node as the freePosition, the gaps
        // between nodes can also be released. If the offset of the first node
        // plus its size is used as the freePosition, the gaps between nodes cannot
        // be released in advance.
        NodeElem* node = usedNodes.pop_front();
        if (usedNodes.empty()) {
            freePosition_ = poolLength_;
            allocPosition_ = alignedDataStartOffset_;
            if (bufferPool.GetConfig().allocateMode == MultiReader) {
                reinterpret_cast<multiReader::BufRefRecord*>(bufferPool.GetRefRecord())
                    ->SetAllocPosition(allocPosition_);
            } else {
                reinterpret_cast<BufRefRecord*>(bufferPool.GetRefRecord())->SetAllocPosition(allocPosition_);
            }
        } else {
            if (bufferPool.GetConfig().allocateMode == MultiReader) {
                freePosition_ = reinterpret_cast<multiReader::BufferNode*>(usedNodes.front()->GetNode())->GetOffset();
            } else {
                freePosition_ = reinterpret_cast<BufferNode*>(usedNodes.front()->GetNode())->GetOffset();
            }
        }
        if (bufferPool.GetConfig().allocateMode == MultiReader) {
            reinterpret_cast<multiReader::BufRefRecord*>(bufferPool.GetRefRecord())->SetFreePosition(freePosition_);
        } else {
            reinterpret_cast<BufRefRecord*>(bufferPool.GetRefRecord())->SetFreePosition(freePosition_);
        }
        return node;
    }
    void UseNodesPush(NodeElem& elem) {
        usedNodes.push_back(elem);
        if (bufferPool.GetConfig().allocateMode == MultiReader) {
            allocPosition_ = reinterpret_cast<multiReader::BufferNode*>(elem.GetNode())->GetOffset() +
                             reinterpret_cast<multiReader::BufferNode*>(elem.GetNode())->GetSize();
            reinterpret_cast<multiReader::BufRefRecord*>(bufferPool.GetRefRecord())->SetAllocPosition(allocPosition_);
        } else {
            allocPosition_ = reinterpret_cast<BufferNode*>(elem.GetNode())->GetOffset() +
                             reinterpret_cast<BufferNode*>(elem.GetNode())->GetSize();
            reinterpret_cast<BufRefRecord*>(bufferPool.GetRefRecord())->SetAllocPosition(allocPosition_);
        }
    }
    bool FreeBufferIsEnough(MemOffset& offset, size_t alignedSize) const {
        if (allocPosition_ > freePosition_) {
            if (poolLength_ - allocPosition_ >= alignedSize) {
                offset = allocPosition_;
                return true;
            } else if (freePosition_ - alignedDataStartOffset_ >= alignedSize) {
                offset = alignedDataStartOffset_;
                return true;
            } else {
                return false;
            }
        } else {
            if (freePosition_ - allocPosition_ >= alignedSize) {
                offset = allocPosition_;
                return true;
            } else {
                return false;
            }
        }
    }
    template <typename BufferNodeType, typename BufRefRecordType>
    void InitInternal();
    template <typename BufferNodeType>
    void RebuildInternal();
    template <typename BufferNodeType>
    void SortUsedNodes(std::list<NodeElem*>& lists);
    template <typename BufferNodeType, typename BufRefRecordType>
    BufferPtr AllocateInternal(std::size_t size, std::size_t alignedSize);
    template <typename BufferNodeType>
    BufferPtr AllocBufferInternal(NodeElem& elem, uint64_t bufferOffset, size_t alignedSize, bool multiReader);
    template <typename BufferNodeType>
    PoolAllocStatus QueryStatusInternal();

 private:
    const BufferPool& bufferPool;
    ObjectAllocator<NodeElem> nodeAllocator;
    Link<NodeElem> freeNodes;
    Link<NodeElem> usedNodes;
    mutable std::mutex mtx;

    MemOffset alignedDataStartOffset_;
    uint64_t alignAddress_;
    size_t poolLength_;
    MemOffset allocPosition_;
    MemOffset freePosition_;
};
}  // namespace mbuf
#endif  // MBUF_SRC_CORE_DYNAMICSIZEBUFFERALLOCATOR_H
