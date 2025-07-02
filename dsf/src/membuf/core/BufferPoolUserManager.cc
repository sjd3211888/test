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

#include <mutex>
#include "membuf/BufferPoolUserManager.h"
#include "BufferNode.h"
#include "multi_reader/BufferNode.h"
#include "membuf/BufferDesc.h"
#include "BufRefRecord.h"
#include "multi_reader/BufRefRecord.h"
#include "membuf/ShareBuffer.h"
#include "shm_heap_wrapper.h"
#include "DsfLog.hpp"
#include "membuf/MembufMonitor.h"
#include "BufferPoolOwner.h"
#include "ScaleAlgo.h"

namespace mbuf {
BufferPoolUserManager::BufferPoolUserManager() {}

void BufferPoolUserManager::RegisterBufferPool(const BufferPool& pool) {
    std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
    bufferPoolMap.emplace(pool.GetID(), pool);
}

PoolParam BufferPoolUserManager::OpenPoolShm(const PoolID id) {
    PoolParam param {false, id};
    const int ret = shm_buf_open_with_uuid(id, &param.fd);
    if (ret < 0) { // 可能返回29 (ESPIPE, 不拦截ESPIPE)
        DSF_LOG_ERROR_T(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_OPEN_ERR,
                        "open buffer pool error,ret:" << ret << ",pool id:" << id << ",fd:" << param.fd);
        param.valid = false;
        return param;
    }
    if (ret == AUTHENTICATION_FAILED) {
        DSF_LOG_INFO(DSF_MEMBUF,
                     "open buffer pool authentication failed,ret:" << ret << ",pool id:" << id << ",fd:" << param.fd);
    }
    param.shareMemPtr = shm_reader_mmap(param.fd);
    if (param.shareMemPtr) {
        param.allocateMode = *reinterpret_cast<AllocateMode*>(param.shareMemPtr);
        if (param.allocateMode == MultiReader) {
            const auto& bufRefRecord = *static_cast<multiReader::BufRefRecord*>(param.shareMemPtr);
            param.poolSize = bufRefRecord.GetPoolSize();
            param.bufCount = bufRefRecord.GetBufCount();
            param.bufMaxSize = bufRefRecord.GetBufMaxSize();
        } else {
            const auto& bufRefRecord = *static_cast<BufRefRecord*>(param.shareMemPtr);
            param.poolSize = bufRefRecord.GetPoolSize();
            param.bufCount = bufRefRecord.GetBufCount();
            param.bufMaxSize = bufRefRecord.GetBufMaxSize();
        }
        param.valid = true;
    } else {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_MMAP_ERR,
                      "param.shareMemPtr is nullptr:" << param.shareMemPtr);
        param.valid = false;
    }
    return param;
}

template <typename BufferNodeType, typename BufRefRecordType>
BufferPtr BufferPoolUserManager::RebuildBufferInternal(const BufferDesc& desc, BufferPool* bufferPool,
                                                       bool multiReader) {

    auto* bufferNode = reinterpret_cast<BufferNodeType*>(bufferPool->GetBufferNode(desc.GetNodeOffset()));
    auto pool_id = desc.GetPoolID();
    if (!bufferNode->OnUsing(desc.GetUserID())) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_REBUILD_BUFFER_ON_USING_ERR,
                      "buffer node ptr:" << &bufferNode << ",pool id:" << pool_id << ",user id:" << desc.GetUserID()
                                         << ",on using failed");
        std::stringstream ss;
        ss << "buffer node ptr:" << &bufferNode << ",pool id:" << pool_id
           << ",refState:" << bufferNode->GetState(desc.GetUserID()) << ",offset:" << bufferNode->GetOffset()
           << ",length:" << bufferNode->GetSize() << ",recycleNums:" << bufferNode->GetRecycleNums()
           << ",RebuildBuffer OnUsing failed expect old state:1,";
        bufferNode->GetState(ss, desc.GetUserID());
        MembufMonitor monitor {bufferPool->GetConfig().allocateMode};
        monitor.PrintBufRefRecord(bufferPool->GetShareMemHead(), ss);
        monitor.PrintBufferNode(bufferPool->GetShareMemHead(), ss, bufferNode->GetOffset());
        monitor.LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
        return nullptr;
    }

    if (!desc.IsValid(bufferNode->GetRecycleNums())) {
        DSF_LOG_WARNING(BufferPoolUserManager,
                        "IsValid failed! pool id:"
                            << pool_id << " ,buffer node bufferNodeRecycleNums:" << bufferNode->GetRecycleNums()
                            << ",desc recycle num:" << desc.GetRecycleNums()
                            << ",ret code:" << ReturnCode_t::RETCODE_MEMBUF_REBUILD_BUFFER_RECYCLE_NUM_INVALID);
        auto ret = bufferNode->OnRevertToPending(desc.GetUserID());
        if (reinterpret_cast<BufRefRecordType*>(bufferPool->GetRefRecord())->GetEnableDfx() ==
            MembufDfxLevel::MonitorLevel2) {
            std::stringstream ss;
            ss << "IsValid failed! pool id:" << pool_id
               << ",buffer node bufferNodeRecycleNums:" << bufferNode->GetRecycleNums()
               << ",desc recycle num:" << desc.GetRecycleNums() << ",onReleased ret:" << ret << ",";
            bufferNode->GetState(ss, desc.GetUserID());
            MembufMonitor::LogEachLine(vbsutil::elog::Log::Kind::Info, ss);
        }
        return nullptr;
    }

    DSF_LOG_DEBUG(BufferPoolUserManager,
                  "RebuildBuffer success,desc:" << desc << ",buffer node offset:" << bufferPool->GetOffset(bufferNode)
                                                << ",buffer node recycleNums:" << bufferNode->GetRecycleNums());

    return std::make_shared<ShareBuffer>(bufferNode, bufferPool->GetShareMemHead() + bufferNode->GetOffset(),
                                         bufferNode->GetSize(), desc.GetUserID(), multiReader);
}

BufferPtr BufferPoolUserManager::RebuildBuffer(const BufferDesc& desc) {
  std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
  const auto bufferPool = OpenPoolNTS(desc.GetPoolID());
  if (!bufferPool) {
      DSF_LOG_ERROR_KEY_T(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_OPEN_POOL_ERR, std::to_string(desc.GetUserID()),
                          "open buf desc:" << desc << " bufferPool is nullptr:");
      return nullptr;
  }
  if (bufferPool->GetConfig().allocateMode == MultiReader) {
    DSF_LOG_DEBUG(DSF_BUFFERPOOLUSERMANAGER,
                  "MultiReader open buf desc:" << desc << " allocMode:" << bufferPool->GetConfig().allocateMode);
    return RebuildBufferInternal<multiReader::BufferNode, multiReader::BufRefRecord>(desc, bufferPool, true);
  } else {
    DSF_LOG_DEBUG(DSF_BUFFERPOOLUSERMANAGER,
                  "open buf desc:" << desc << " allocMode:" << bufferPool->GetConfig().allocateMode);
    return RebuildBufferInternal<BufferNode, BufRefRecord>(desc, bufferPool, false);
  }
}

void BufferPoolUserManager::ClosePool() {
    // TODO 目前实现没有考虑生命周期，在Buffer还在使用的过程中（比如用户长时间持有一个Buffer）
    // 调用这个函数会触发coredump 参见：FUNCTEST_MEMMBUF_030
    // 解决方案：
    // 1. 现有方案 在正常析构时候加入判断没有Buffer在使用再关闭，但是实现起来比较困难
    // 2. 不主动触发close，在所有使用者都结束后由驱动释放（需要驱动支持）
    std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
    auto it = bufferPoolMap.begin();
    while (it != bufferPoolMap.end()) {
        if (it->second.GetFd() < 0) {
            ++it;
            continue;
        }
        (void)shm_unmap(it->second.GetConfig().shareMemPtr, it->second.GetConfig().poolSize);
        (void)shm_buf_close(it->second.GetFd());
        ++it;
    }
    bufferPoolMap.clear();
}

BufferPool* BufferPoolUserManager::OpenPool(PoolID id) {
  std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
  return OpenPoolNTS(id);
}

BufferPoolUserManager::~BufferPoolUserManager() {
    ClosePool();
}

bool BufferPoolUserManager::OnPeerCrash(PID) {
    return true;
}

bool BufferPoolUserManager::IsPoolFree(const PoolID poolId) const {
    auto iter = bufferPoolMap.find(poolId);
    if (iter == bufferPoolMap.end()) {
        return false;
    }
    auto bufferPool = iter->second;
    return bufferPool.IsAllUserFree();
}

bool BufferPoolUserManager::IsPoolNotUsing(const PoolID poolId) const {
    auto iter = bufferPoolMap.find(poolId);
    if (iter == bufferPoolMap.end()) {
        return false;
    }
    auto bufferPool = iter->second;
    return bufferPool.IsAllUserNotUsing();
}

bool BufferPoolUserManager::PoolCanBeFreed(PoolMapIterator iter) const {
    auto bufferPool = iter->second;
    if (bufferPool.GetRecoverMode() == RecoverMode::Force) {
        return bufferPool.IsAllUserNotUsing();
    }
    return bufferPool.IsAllUserFree();
}

bool BufferPoolUserManager::FreePool(const PoolID poolId, bool& stillOpen) {
    stillOpen = false;
    std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
    auto iter = bufferPoolMap.find(poolId);
    if (iter == bufferPoolMap.end()) {
        // 如果该pool不是该reader打开的会进入这里，属于正常逻辑
        DSF_LOG_DEBUG(BufferPoolUserManager, "pool id:" << poolId << " not in bufferPoolMap");
        return false;
    }

    if (iter->second.GetFd() < 0) {
        DSF_LOG_WARNING(BufferPoolUserManager, "pool id:" << poolId << " fd:" << iter->second.GetFd() << " < 0");
        stillOpen = false;
        return false;
    }

    if (PoolCanBeFreed(iter)) {
        const int unmapRet = shm_unmap(iter->second.GetConfig().shareMemPtr, iter->second.GetConfig().poolSize);
        if (unmapRet < 0) {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_UNMAP_ERR,
                          "pool id:" << poolId << " ptr:" << iter->second.GetConfig().shareMemPtr
                                     << " pool size:" << iter->second.GetConfig().poolSize
                                     << " fd:" << iter->second.GetFd() << " unmap ret:" << unmapRet);
        }
        const int closeRet = shm_buf_close(iter->second.GetFd());
        if (closeRet < 0) {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_BUFFER_CLOSE_FAILED,
                          "pool id:" << poolId << " ptr:" << iter->second.GetConfig().shareMemPtr
                                     << " pool size:" << iter->second.GetConfig().poolSize
                                     << " fd:" << iter->second.GetFd() << " close ret:" << closeRet);
        }
        DSF_LOG_INFO(BufferPoolUserManager, "pool id:" << poolId << " ptr:" << iter->second.GetConfig().shareMemPtr
                                                       << " pool size:" << iter->second.GetConfig().poolSize
                                                       << " fd:" << iter->second.GetFd() << " unmap ret:" << unmapRet
                                                       << " close ret:" << closeRet);
        bufferPoolMap.erase(iter);
        stillOpen = false;
        return true;
    } else {
        stillOpen = true;
        return false;
    }
}

bool BufferPoolUserManager::IsInBufferPoolMap(PoolID id) {
    std::lock_guard<std::mutex> lk(mtxBufferPoolMap);
    return bufferPoolMap.find(id) != bufferPoolMap.end();
}

bool BufferPoolUserManager::DumpMembuf(PoolID id, void** buf, size_t& len) {
    const auto bufferPool = OpenPool(id);
    if (!bufferPool) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_OPEN_POOL_ERR,
                      "open buffer pool id:" << id << " bufferPool is nullptr:");
        return false;
    }
    *buf = bufferPool->GetShareMemHead();
    len = bufferPool->GetConfig().poolSize;
    return true;
}

BufferPool* BufferPoolUserManager::OpenPoolNTS(PoolID id) {
  auto iter = bufferPoolMap.find(id);
  if (iter == bufferPoolMap.end()) {
    auto param = OpenPoolShm(id);
    if (param.valid) {
      param.recoverMode = this->recoverMode;  // reader best effort 对应 RecoverMode::Force
      auto it = bufferPoolMap.emplace(id, BufferPool {param, param.fd, false});
      if (it.second) {
        return &(it.first->second);
      } else {
          DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_EMPLACE_POOL_ERR,
                        "emplace pool id" << id << " failed!");
          return nullptr;
      }
    } else {
        DSF_LOG_ERROR_T(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_MMAP_ERR,
                        "shm_reader_mmap failed!,uuid:" << id << ",can not open unimem!");
        return nullptr;
    }
  } else {
    return &(iter->second);
  }
}

}  // namespace mbuf
