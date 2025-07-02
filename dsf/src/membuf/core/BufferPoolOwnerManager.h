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

#ifndef MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_MANAGER_H
#define MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_MANAGER_H

#include <mutex>
#include <deque>
#include "membuf/buffer.h"
#include "membuf/BufferPool.h"
#include "BufferPoolOwner.h"
#include "common/types/TypesBase.h"

namespace mbuf {

class BufferPoolOwner;
class ScaleAlgo;

class BufferPoolOwnerManager {
    friend class MembufMonitor;

 public:
    BufferPoolOwnerManager(const std::string& name, AllocCfg allocCfg, size_t poolSize, size_t bufCount,
                           size_t bufMaxSize, size_t alignAddress = 64U, RecoverMode mode = RecoverMode::Normal,
                           AllocateMode allocateMode = AllocateMode::Dynamic, bool expand = false);
    BufferPoolOwnerManager(const BufferPoolOwnerManager& other) = delete;
    BufferPoolOwnerManager& operator=(const BufferPoolOwnerManager& other) = delete;
    BufferPoolOwnerManager(BufferPoolOwnerManager&& other) noexcept = delete;
    BufferPoolOwnerManager& operator=(BufferPoolOwnerManager&& other) noexcept = delete;

    static std::unique_ptr<BufferPoolOwnerManager> Create(const std::string& name, AllocCfg allocCfg, size_t poolSize,
                                                          size_t bufCount, size_t bufMaxSize, PID pid,
                                                          size_t alignAddress = 64U,
                                                          RecoverMode mode = RecoverMode::Normal,
                                                          AllocateMode allocateMode = AllocateMode::Dynamic,
                                                          bool expand = false, bool authentication = false);
    ~BufferPoolOwnerManager();

    UserID RegisterUser(PID) const;
    void AddPeerId(const uint32_t peer_id) const;
    bool UnRegisterUser(UserID) const;
    bool OnPeerCrash(PID);
    BufferPtr AllocBuffer(std::size_t size, int64_t& poolID);
    ReturnCode_t ProcessStateInMeta(
        const BufferPtr& buffer, UserID userId, StateOperator stateOperator,
        li::dsfdds::MultiModeChannel auto_shm_bigbuffer = li::dsfdds::MultiModeChannel::NONE);
    bool IsMultiReader() const;
    void SetExpend(bool expand);
    PoolID GetID() const;
    std::vector<mbuf::PoolID> GetPoolIds() const;
    BufferPoolOwner* GetPool(PoolID poolId);
    bool FreePool(PoolID poolId);

 protected:
    bool TryExpand(std::size_t allocSize, int64_t& poolID);
    void TryShrink(std::size_t allocSize, int64_t& poolID);
    bool CreateNewPool(std::unique_ptr<BufferPoolOwner>& oldPool, std::size_t allocSize, int64_t& poolID,
                       std::size_t newPoolAllocSize, std::size_t newBufCount);

 private:
    mutable std::mutex mtxPoolOwners;
    std::deque<std::unique_ptr<BufferPoolOwner>> poolOwners;
    std::string poolName;
    AllocCfg allocCfg;
    std::size_t curAllocPoolSize;
    std::size_t curBufCount;
    std::size_t curRealPoolSize;
    std::size_t bufMaxSize;
    std::size_t alignAddress;
    RecoverMode recoverMode;
    AllocateMode allocateMode;
    // 1: enable expand pool 0: disable expand pool
    bool enableExpand;
    uint64_t expandCount;
    uint64_t shrinkCount;
    std::unique_ptr<ScaleAlgo> scaleAlgo;
    uint64_t allocCount;  // alloc buffer 总次数
    uint64_t prevQuery;   // 上次 query status 的 alloc 次数
};
}  // namespace mbuf
#endif  // MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_MANAGER_H
