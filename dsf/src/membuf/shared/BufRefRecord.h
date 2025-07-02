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

#ifndef MEMBUF_SRC_SHARED_BUF_REF_RECORD_H
#define MEMBUF_SRC_SHARED_BUF_REF_RECORD_H

#include <atomic>
#include <array>
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include "membuf/BaseType.h"
#include "UserCrashHandler.h"
namespace mbuf {
constexpr size_t ReserveGapSize = 64U;
struct BufRefRecord {
    bool IsRefBy(UserID userId) const;
    UserID AddUserID(const PID pid);
    UserID AddOwnerID(const PID pid);
    bool UserCrash(PID, UserCrashHandler&);
    void OwnerCrash(UserCrashHandler&);
    bool ResetUser(UserID userId, UserCrashHandler& userCrashHandler);
    size_t GetPoolSize() const { return poolSize; }
    size_t GetBufCount() const { return bufCount; }
    size_t GetBufMaxSize() const { return bufMaxSize; }
    size_t GetMemMode() const { return memMode; }
    void SetPoolSize(const size_t pool_size) { poolSize = pool_size; }
    void SetBufCount(const size_t buf_count) { bufCount = buf_count; }
    void SetBufMaxSize(const size_t buf_max_size) { bufMaxSize = buf_max_size; }
    void SetMemMode(const size_t mode) { memMode = mode; }
    void SetEnableDfx(uint8_t enable);
    uint8_t GetEnableDfx() const { return enableDfx.load(); }
    uint64_t GetAllocPosition() const;
    uint64_t GetFreePosition() const;
    void SetAllocPosition(uint64_t alloc_position) { BufRefRecord::allocPosition = alloc_position; }
    void SetFreePosition(uint64_t free_position) { BufRefRecord::freePosition = free_position; }
    uint64_t GetMaxAllocSize() const;
    uint64_t GetMinAllocSize() const;
    uint64_t GetAllocCount() const;
    uint64_t GetUsedPoolSize() const;
    uint64_t GetUsedBufCount() const;
    void UpdateAllocInfo(uint64_t size);
    PID GetUserPid(size_t i) const { return userPids.at(i).load(); }
    int32_t GetRefState(size_t i) const { return userStates.at(i).load(); }
    void SetUsedPoolSize(const size_t used_pool_size) { usedPoolSize = used_pool_size; }
    void SetUsedBufCount(const size_t used_buf_count) { usedBufCount = used_buf_count; }

 private:
    bool UserCrashHandle(UserID userId, UserCrashHandler& userCrashHandler);

 private:
    size_t memMode;
    enum RefState : int32_t { INVALID = 0, USING, CRASH };
    using RefNodeNew = std::atomic<PID>;
    using RefStateNew = std::atomic<RefState>;
    std::array<RefNodeNew, MAX_REF_COUNT> userPids;
    std::array<RefStateNew, MAX_REF_COUNT> userStates;
    // use for reader create pool
    size_t poolSize;
    size_t bufCount;
    size_t bufMaxSize;
    // use for debug and statistic
    std::atomic<uint8_t> enableDfx;
    uint64_t allocPosition;
    uint64_t freePosition;
    uint64_t maxAllocSize;
    uint64_t minAllocSize;
    uint64_t allocCount;
    uint64_t usedPoolSize;
    uint64_t usedBufCount;
};

static_assert(std::is_trivial<BufRefRecord>::value, "BufRefRecord must be a trivial type");
}  // namespace mbuf
#endif  // MEMBUF_SRC_SHARED_BUF_REF_RECORD_H
