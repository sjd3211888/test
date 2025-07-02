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

#ifndef MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_H
#define MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_H

#include <memory>
#include <string>
#include "membuf/BufferPool.h"
#include "multi_reader/BufRefRecord.h"
#include "multi_reader/BufferNode.h"
#include "BufferAllocatorBase.h"
#include "common/types/TypesBase.h"

namespace mbuf {
enum StateOperator : int8_t {
    Ope_On_Pending,
    Ope_On_Free,
    Ope_On_Using,
};
constexpr uint32_t kMaxAllocations = 16384U;

class BufferAllocatorBase;

struct BufferPoolOwner : BufferPool {
    BufferPoolOwner(const BufPoolCfg&, PID, int fd, bool isCreated = true, const std::string& name = "");
    static std::unique_ptr<BufferPoolOwner> CreateBufferPoolOwner(const std::string& name, size_t poolSize,
                                                                  size_t bufCount, size_t bufMaxSize, PID pid,
                                                                  size_t alignAddress = 64U,
                                                                  RecoverMode mode = RecoverMode::Normal,
                                                                  AllocateMode allocateMode = AllocateMode::Dynamic,
                                                                  bool authentication = false);
    ~BufferPoolOwner();
    UserID RegisterUser(PID) const;
    void AddPeerId(const uint32_t peer_id) const;
    template <typename BufRefRecordType>
    void AddPeerId(bool is_multi_reader) const;
    void AddPeerIdForNewPool() const;
    bool UnRegisterUser(UserID) const;
    bool OnPeerCrash(PID);
    BufferPtr AllocBuffer(std::size_t size);
    ReturnCode_t ProcessStateInMeta(
        const BufferPtr& buffer, UserID userId, StateOperator stateOperator,
        li::dsfdds::MultiModeChannel auto_shm_bigbuffer = li::dsfdds::MultiModeChannel::NONE);
    bool IsMultiReader() const;
    PoolAllocStatus QueryAllocStatus();

    // copy ref record from old pool during expand
    void InitRefRecordFromOld(BufferPoolOwner& oldPool, size_t newPoolSize, size_t newBufCount);

 private:
    static PoolParam CreatePoolShm(const std::string& name, size_t poolSize, size_t bufCount, size_t bufMaxSize,
                                   size_t alignAddress, RecoverMode mode, AllocateMode allocateMode,
                                   bool authentication);
    template <typename BufRefRecordType, typename BufferNodeType>
    static uint64_t GetExtraSize(size_t bufCount, size_t alignAddress);
    static bool CheckPoolParam(const std::string& name, size_t poolSize, size_t bufCount, size_t alignAddress,
                               AllocateMode allocateMode);

    template <typename BufRefRecordType>
    void ConstructInternal(const BufPoolCfg& cfg, const PID pid, bool isCreated);
    template <typename BufferNodeType>
    ReturnCode_t ProcessStateInMetaInternal(const BufferPtr& buffer, UserID userId, StateOperator stateOperator,
                                            li::dsfdds::MultiModeChannel auto_shm_bigbuffer);
    template <typename BufRefRecordType>
    void InitRefRecordFromOldInternal(BufferPoolOwner& oldPool, size_t newPoolSize, size_t newBufCount);
    template <typename BufRefRecordType>
    void UpdateAllocStatusInternal(size_t usedPoolSize, size_t usedBufCount);

 private:
    // 可以配置FixSizeBufferAllocator和DynamicSizeBufferAllocator，支持2种算法
    std::unique_ptr<BufferAllocatorBase> allocator;
    std::string poolName;
};

extern template uint64_t BufferPoolOwner::GetExtraSize<multiReader::BufRefRecord, multiReader::BufferNode>(
    size_t bufCount, size_t alignAddress);
extern template uint64_t BufferPoolOwner::GetExtraSize<BufRefRecord, BufferNode>(size_t bufCount, size_t alignAddress);

}  // namespace mbuf
#endif  // MEMBUF_SRC_CORE_BUFFER_POOL_OWNER_H
