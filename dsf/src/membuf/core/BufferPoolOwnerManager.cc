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

#include "BufferPoolOwnerManager.h"

#include <unistd.h>
#include <mutex>
#include <exception>
#include "membuf/buffer.h"
#include "membuf/BufferPool.h"
#include "ScaleAlgo.h"
#include "AdaptiveScaleAlgo.h"
#include "BufferAllocatorBase.h"
#include "shm_heap_wrapper.h"
#include "DsfLog.hpp"

namespace mbuf {
BufferPoolOwnerManager::BufferPoolOwnerManager(const std::string& name, AllocCfg allocCfg, size_t poolSize,
                                               size_t bufCount, size_t bufMaxSize, size_t alignAddress,
                                               RecoverMode mode, AllocateMode allocateMode, bool expand)
    : mtxPoolOwners {},
      poolOwners {},
      poolName(name),
      allocCfg(allocCfg),
      curAllocPoolSize(poolSize),
      curBufCount(bufCount),
      curRealPoolSize(0),
      bufMaxSize(bufMaxSize),
      alignAddress(alignAddress),
      recoverMode(mode),
      allocateMode(allocateMode),
      enableExpand(expand),
      expandCount(0),
      shrinkCount(0),
      allocCount(0),
      prevQuery(0) {
    scaleAlgo = std::make_unique<AdaptiveScaleAlgo>(allocCfg, poolSize, bufCount);
}

std::unique_ptr<BufferPoolOwnerManager> BufferPoolOwnerManager::Create(const std::string& name, AllocCfg allocCfg,
                                                                       size_t poolSize, size_t bufCount,
                                                                       size_t bufMaxSize, PID pid, size_t alignAddress,
                                                                       RecoverMode mode, AllocateMode allocateMode,
                                                                       bool expand, bool authentication) {
    if (poolSize > allocCfg.poolSizeUpLimit_) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CONFIG_ERR,
                      "poolSize:" << poolSize << " limit:" << allocCfg.poolSizeUpLimit_ << " exceeds limit!");
        return nullptr;
    }
    auto pool = BufferPoolOwner::CreateBufferPoolOwner(name + "_" + std::to_string(0), poolSize, bufCount, bufMaxSize,
                                                       pid, alignAddress, mode, allocateMode, authentication);
    if (!pool) {
        return nullptr;
    }
    // 创建中会对齐 poolSize
    std::unique_ptr<BufferPoolOwnerManager> poolManager = std::make_unique<BufferPoolOwnerManager>(
        name, allocCfg, poolSize, bufCount, bufMaxSize, alignAddress, mode, allocateMode, expand);
    poolManager->curRealPoolSize = pool->GetPoolSize();
    std::lock_guard<std::mutex> lk(poolManager->mtxPoolOwners);
    poolManager->poolOwners.emplace_back(std::move(pool));

    return poolManager;
}

BufferPoolOwnerManager::~BufferPoolOwnerManager() {
    DSF_LOG_DEBUG(BufferPoolOwnerManager, "pool name:" << poolName << " dtor");
}

bool BufferPoolOwnerManager::OnPeerCrash(const PID pid) {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    auto ret = true;
    for (auto& it : poolOwners) {
        ret &= it->OnPeerCrash(pid);
    }
    return ret;
}

UserID BufferPoolOwnerManager::RegisterUser(const PID pid) const {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    // register only use new pool
    return poolOwners.back()->RegisterUser(pid);
}

void BufferPoolOwnerManager::AddPeerId(const uint32_t peer_id) const {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    return poolOwners.back()->AddPeerId(peer_id);
}

bool BufferPoolOwnerManager::UnRegisterUser(const UserID userId) const {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    auto ret = true;
    for (auto& it : poolOwners) {
        ret &= it->UnRegisterUser(userId);
    }
    return ret;
}

void BufferPoolOwnerManager::SetExpend(bool expand) {
    enableExpand = expand;
}

PoolID BufferPoolOwnerManager::GetID() const {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    return poolOwners.back()->GetID();
}

bool BufferPoolOwnerManager::IsMultiReader() const {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    return poolOwners.back()->GetConfig().allocateMode == MultiReader;
}

ReturnCode_t BufferPoolOwnerManager::ProcessStateInMeta(const BufferPtr& buffer, UserID userId,
                                                        StateOperator stateOperator,
                                                        li::dsfdds::MultiModeChannel auto_shm_bigbuffer) {
  std::lock_guard<std::mutex> lk(mtxPoolOwners);
  return poolOwners.back()->ProcessStateInMeta(buffer, userId, stateOperator, auto_shm_bigbuffer);
}

BufferPtr BufferPoolOwnerManager::AllocBuffer(std::size_t size, int64_t& poolID) {
    ++allocCount;
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    scaleAlgo->UpdateAllocSize(size);
    const auto& oldPool = poolOwners.back();
    auto ret = oldPool->AllocBuffer(size);
    if (ret == nullptr && enableExpand) {
        if (TryExpand(size, poolID)) {
            return poolOwners.back()->AllocBuffer(size);
        }
        // 扩容失败直接返回 ret
        return ret;
    }

    // 缩容
    if (allocCount >= prevQuery + allocCfg.queryStatusInterval_) {
        TryShrink(size, poolID);
    }

    return ret;
}

bool BufferPoolOwnerManager::TryExpand(std::size_t allocSize, int64_t& poolID) {
    auto& oldPool = poolOwners.back();
    ++expandCount;
    std::size_t newPoolAllocSize = 0;
    std::size_t newBufCount = 0;
    PoolAllocStatus allocStatus = oldPool->QueryAllocStatus();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    PoolStatus status {
        .used_pool_size_ = allocStatus.used_pool_size_,
        .used_buf_count_ = allocStatus.used_buf_count_,
        .alloc_pool_size_ = curAllocPoolSize,
        .buf_count_ = curBufCount,
    };
#pragma GCC diagnostic pop
    prevQuery = allocCount;
    scaleAlgo->UpdatePoolStatus(status);
    if (!scaleAlgo->ScaleUp(newBufCount, newPoolAllocSize)) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SCALE_UP_FAILED,
                      "Fail to expand. alloc buf size:" << allocSize << " old pool id:" << oldPool->GetID()
                                                        << " alloc size:" << curAllocPoolSize << " buf count:"
                                                        << curBufCount << " real size:" << oldPool->GetPoolSize());
        return false;
    }

    return CreateNewPool(oldPool, allocSize, poolID, newPoolAllocSize, newBufCount);
}

void BufferPoolOwnerManager::TryShrink(std::size_t allocSize, int64_t& poolID) {
    auto& oldPool = poolOwners.back();
    std::size_t newPoolAllocSize = 0;
    std::size_t newBufCount = 0;
    PoolAllocStatus allocStatus = oldPool->QueryAllocStatus();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    PoolStatus status {
        .used_pool_size_ = allocStatus.used_pool_size_,
        .used_buf_count_ = allocStatus.used_buf_count_,
        .alloc_pool_size_ = curAllocPoolSize,
        .buf_count_ = curBufCount,
    };
#pragma GCC diagnostic pop
    prevQuery = allocCount;
    scaleAlgo->UpdatePoolStatus(status);
    if (scaleAlgo->ScaleDown(newBufCount, newPoolAllocSize)) {
        ++shrinkCount;
        DSF_LOG_INFO(BufferPoolOwnerManager,
                     "shrink triggered, alloc count:" << allocCount << " shrink count:" << shrinkCount);
        if (CreateNewPool(oldPool, allocSize, poolID, newPoolAllocSize, newBufCount)) {
            DSF_LOG_INFO(BufferPoolOwnerManager,
                         "shrink success, alloc count:" << allocCount << " shrink count:" << shrinkCount);
        } else {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SCALE_UP_FAILED,
                          "shrink fail, alloc count:" << allocCount);
        }
    }
}

bool BufferPoolOwnerManager::CreateNewPool(std::unique_ptr<BufferPoolOwner>& oldPool, std::size_t allocSize,
                                           int64_t& poolID, std::size_t newPoolAllocSize, std::size_t newBufCount) {
    if (newPoolAllocSize > allocCfg.poolSizeUpLimit_ || newBufCount > allocCfg.bufCountUpLimit_) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CONFIG_ERR,
                      "newPoolAllocSize:" << newPoolAllocSize << " limit:" << allocCfg.poolSizeUpLimit_
                                          << " newBufCount:" << newBufCount << " limit:" << allocCfg.bufCountUpLimit_
                                          << " exceeds limit!");
        return false;
    }
    auto newPool = BufferPoolOwner::CreateBufferPoolOwner(
        poolName + "_ex" + std::to_string(expandCount) + "_sh" + std::to_string(shrinkCount), newPoolAllocSize,
        newBufCount, bufMaxSize, getpid(), alignAddress, recoverMode, allocateMode, true);
    if (!newPool) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CREATE_POOL_FAILED,
                      "Create new buf pool fail. "
                          << "old pool id:" << oldPool->GetID() << " alloc size:" << curAllocPoolSize
                          << " buf count:" << curBufCount << " real size:" << oldPool->GetPoolSize()
                          << ", new pool alloc size:" << newPoolAllocSize << " buf count:" << newBufCount);
        return false;
    }
    // 创建的时候内存对齐, 实际的 size 会比输入的 size 大
    size_t newPoolRealSize = newPool->GetPoolSize();
    newBufCount = newPool->GetBufCount();
    auto newPoolFd = newPool->GetFd();
    poolOwners.emplace_back(std::move(newPool));
    DSF_LOG_INFO(BufferPoolOwnerManager,
                 "Create buf pool success. Alloc buf size:"
                     << allocSize << ",OldPoolId:" << oldPool->GetID() << ",AllocSize:" << curAllocPoolSize
                     << ",BufCount:" << curBufCount << ",RealSize:" << curRealPoolSize << ".NewPoolId:"
                     << poolOwners.back()->GetID() << ",Fd:" << newPoolFd << ",AllocSize:" << newPoolAllocSize
                     << ",BufCount:" << newBufCount << ",RealSize:" << newPoolRealSize);
    curAllocPoolSize = newPoolAllocSize;
    curBufCount = newBufCount;
    curRealPoolSize = newPoolRealSize;
    poolOwners.back()->InitRefRecordFromOld(*oldPool, newPoolRealSize, newBufCount);
    poolOwners.back()->AddPeerIdForNewPool();
    poolID = oldPool->GetID();
    return true;
}

std::vector<mbuf::PoolID> BufferPoolOwnerManager::GetPoolIds() const {
    std::vector<mbuf::PoolID> ids {};
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    for (auto& it : poolOwners) {
        ids.emplace_back(it->GetID());
    }
    return ids;
}

BufferPoolOwner* BufferPoolOwnerManager::GetPool(PoolID poolId) {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    for (auto& it : poolOwners) {
        if (it->GetID() == poolId) {
            return it.get();
        }
    }
    return nullptr;
}

bool BufferPoolOwnerManager::FreePool(PoolID poolId) {
    std::lock_guard<std::mutex> lk(mtxPoolOwners);
    if (poolId == poolOwners.back()->GetID()) {
        DSF_LOG_DEBUG(BufferPoolOwnerManager, "Try to free pool being used! pool id " << poolId);
        return false;
    }
    for (auto it = poolOwners.begin(); it != poolOwners.end();) {
        if ((*it)->GetID() == poolId && (*it)->IsAllOwnerFree()) {
            DSF_LOG_DEBUG(BufferPoolOwnerManager, "free pool " << poolId << " size " << (*it)->GetPoolSize()
                                                               << " buf count " << (*it)->GetBufCount());
            it = poolOwners.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

}  // namespace mbuf
