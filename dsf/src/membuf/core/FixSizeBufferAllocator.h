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

#ifndef MEMBUF_SRC_CORE_FIX_SIZE_BUFFER_ALLOCATOR_H
#define MEMBUF_SRC_CORE_FIX_SIZE_BUFFER_ALLOCATOR_H

#include <mutex>
#include "BufferAllocator.h"
#include "Link.h"
#include "ObjectAlloctor.h"
#include "membuf/BufferPool.h"
#include "UserCrashHandler.h"
#include "NodeElem.h"
#include "BufferAllocatorBase.h"

namespace mbuf {

struct FixSizeBufferAllocator : BufferAllocatorBase {
    explicit FixSizeBufferAllocator(const BufferPool& pool, bool isCreated = true);
    virtual ~FixSizeBufferAllocator();
    BufferPtr Allocate(std::size_t size) override;

 private:
    bool HandleUserCrash(UserID) override;
    void Recycle();
    void Init();
    void Rebuild();
    template <typename BufferNodeType>
    void InitInternal();
    template <typename BufferNodeType>
    void RebuildInternal();
    template <typename BufferNodeType, typename BufRefRecordType>
    BufferPtr AllocateInternal(std::size_t size);
    template <typename BufferNodeType>
    void RecycleInternal();

 private:
    const BufferPool& bufferPool;
    ObjectAllocator<NodeElem> nodeAllocator;
    Link<NodeElem> freeNodes;
    Link<NodeElem> usedNodes;
    mutable std::mutex mtx;
};
}  // namespace mbuf

#endif  // MEMBUF_SRC_CORE_FIX_SIZE_BUFFER_ALLOCATOR_H
