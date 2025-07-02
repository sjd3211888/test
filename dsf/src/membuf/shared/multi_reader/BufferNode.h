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

#ifndef MEMBUF_SRC_SHARED_MULTI_READER_BUFFER_NODE_H
#define MEMBUF_SRC_SHARED_MULTI_READER_BUFFER_NODE_H

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <ostream>
#include <type_traits>
#include "membuf/BaseType.h"

namespace mbuf {
namespace multiReader {
using membuf_ret_t = li::dsfdds::ReturnCode_t;
constexpr uint32_t MaxMultiReader = 128U;
constexpr uint32_t RefStatusCount = MaxMultiReader / 32U;
enum RefNState : uint64_t {
    Free = 0,
    Pending,
    Using,
};

struct BufferNode {
    friend struct BufferPoolUserManager;
    bool OnPending(UserID userId);
    bool OnUsing(UserID userId);
    bool OnReleased(UserID userId);
    bool OnRevertToPending(UserID userId);
    bool OnExcept(UserID userId);
    uint64_t QueryState(UserID userId) const;
    bool OnRecycle();

    bool IsFree() const;
    bool IsAllUserFree() const;
    bool IsAllUserNotUsing() const;
    bool IsAllOwnerFree() const;
    void Reset();
    void Init(MemOffset, size_t);
    MemOffset GetOffset() const { return offset; }
    size_t GetSize() const { return length; }
    void IncRecycleNums() { recycleNums++; }
    uint64_t GetRecycleNums() const { return recycleNums; }
    void ReInit(MemOffset offset, size_t size) {
        this->offset = offset;
        this->length = size;
        this->recycleNums++;
    }

    // for dfx use
    void GetState(std::ostream& ostream, UserID id) const;
    uint64_t GetState(UserID id) const;
    std::string PrintRefState() const;

 private:
    mutable pthread_mutex_t refStateMutex;
    // 32*4-1 = 127
    std::atomic<uint64_t> refState[RefStatusCount];
    MemOffset offset;
    size_t length;
    volatile uint64_t recycleNums;
};

static_assert(std::is_trivial<BufferNode>::value, "BufferNode must be a trivial type");
}  // namespace multiReader
}  // namespace mbuf
#endif  // MEMBUF_SRC_SHARED_MULTI_READER_BUFFER_NODE_H
