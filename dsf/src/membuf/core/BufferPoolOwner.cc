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

#include <fcntl.h>
#include <unistd.h>
#include "membuf/buffer_info.h"
#include "BufferPoolOwner.h"
#include "BufRefRecord.h"
#include "DsfLog.hpp"
#include "shm_heap_wrapper.h"
#include "FixSizeBufferAllocator.h"
#include "DynamicSizeBufferAllocator.h"

namespace mbuf {
BufferPoolOwner::BufferPoolOwner(const BufPoolCfg& cfg, const PID pid, const int fd, const bool isCreated,
                                 const std::string& name)
    : BufferPool(cfg, fd, isCreated), poolName(name) {
    if (cfg.allocateMode == Fixed) {
        allocator = std::make_unique<FixSizeBufferAllocator>(*this, isCreated);
    } else {
        allocator = std::make_unique<DynamicSizeBufferAllocator>(*this, isCreated);
    }
    if (cfg.allocateMode == MultiReader) {
        ConstructInternal<multiReader::BufRefRecord>(cfg, pid, isCreated);
    } else {
        ConstructInternal<BufRefRecord>(cfg, pid, isCreated);
    }
}

template <typename BufRefRecordType>
void BufferPoolOwner::ConstructInternal(const BufPoolCfg& cfg, const PID pid, bool isCreated) {
    if (!isCreated) {
        reinterpret_cast<BufRefRecordType*>(GetRefRecord())->OwnerCrash(*allocator);
        DSF_LOG_WARNING(BufferPoolOwner, "pid:" + std::to_string(::getpid()) + ",pool name:" + poolName +
                                             ",isCreated:" + std::to_string(isCreated) + ", pool already exists");
    }
    auto ownerId = reinterpret_cast<BufRefRecordType*>(GetRefRecord())->AddOwnerID(pid);
    if (ownerId.IsOwnerInvalid()) {
        throw std::runtime_error("pid:" + std::to_string(::getpid()) + ",pool name:" + poolName +
                                 ",isCreated:" + std::to_string(isCreated) +
                                 ",owner user id:" + std::to_string(ownerId) + " is invalid!!!");
    }
    reinterpret_cast<BufRefRecordType*>(GetRefRecord())->SetPoolSize(cfg.poolSize);
    reinterpret_cast<BufRefRecordType*>(GetRefRecord())->SetBufCount(cfg.bufCount);
    reinterpret_cast<BufRefRecordType*>(GetRefRecord())->SetBufMaxSize(cfg.bufMaxSize);
    reinterpret_cast<BufRefRecordType*>(GetRefRecord())->SetMemMode(cfg.allocateMode);
}

BufferPtr BufferPoolOwner::AllocBuffer(const std::size_t size) {
    if (0U == size) {
        DSF_LOG_WARNING(BufferPoolOwner, "AllocBuffer with size: 0.");
        return nullptr;
    }
    return allocator->Allocate(size);
}

PoolAllocStatus BufferPoolOwner::QueryAllocStatus() {
    PoolAllocStatus status = allocator->QueryAllocStatus();
    if (GetConfig().allocateMode == MultiReader) {
        UpdateAllocStatusInternal<multiReader::BufRefRecord>(status.used_pool_size_, status.used_buf_count_);
    } else {
        UpdateAllocStatusInternal<BufRefRecord>(status.used_pool_size_, status.used_buf_count_);
    }
    return status;
}

template <typename BufRefRecordType>
void BufferPoolOwner::UpdateAllocStatusInternal(size_t usedPoolSize, size_t usedBufCount) {
    auto& newBufRefRecord = *reinterpret_cast<BufRefRecordType*>(GetShareMemHead());
    newBufRefRecord.SetUsedPoolSize(usedPoolSize);
    newBufRefRecord.SetUsedBufCount(usedBufCount);
}

template <typename BufRefRecordType>
void BufferPoolOwner::InitRefRecordFromOldInternal(BufferPoolOwner& oldPool, size_t newPoolSize, size_t newBufCount) {
    std::memcpy(GetShareMemHead(), oldPool.GetShareMemHead(), sizeof(BufRefRecordType));
    auto& newBufRefRecord = *reinterpret_cast<BufRefRecordType*>(GetShareMemHead());
    newBufRefRecord.SetPoolSize(newPoolSize);
    newBufRefRecord.SetBufCount(newBufCount);
}

void BufferPoolOwner::InitRefRecordFromOld(BufferPoolOwner& oldPool, size_t newPoolSize, size_t newBufCount) {
    if (GetConfig().allocateMode == MultiReader) {
        InitRefRecordFromOldInternal<multiReader::BufRefRecord>(oldPool, newPoolSize, newBufCount);
    } else {
        InitRefRecordFromOldInternal<BufRefRecord>(oldPool, newPoolSize, newBufCount);
    }
}

bool BufferPoolOwner::OnPeerCrash(const PID pid) {
    if (GetConfig().allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufRefRecord*>(GetRefRecord())->UserCrash(pid, *allocator);
    } else {
        return reinterpret_cast<BufRefRecord*>(GetRefRecord())->UserCrash(pid, *allocator);
    }
}

UserID BufferPoolOwner::RegisterUser(const PID pid) const {
    if (GetConfig().allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufRefRecord*>(GetRefRecord())->AddUserID(pid);
    } else {
        return reinterpret_cast<BufRefRecord*>(GetRefRecord())->AddUserID(pid);
    }
}

void BufferPoolOwner::AddPeerId(const uint32_t peer_id) const {
    uint32_t uuid = 0;
    auto fd = GetFd();
    int ret = shm_buf_add_peer_pid(fd, peer_id, &uuid);
    DSF_LOG_INFO(BufferPoolOwner, "MbufPool AddPeerId " << ((ret == 0) ? "success" : "false") << ",pool name:"
                                                        << poolName << ",pool id:" << GetID() << ",fd:" << fd
                                                        << ",peer_id:" << peer_id << ",return uuid:" << uuid);
}

template <typename BufRefRecordType>
void BufferPoolOwner::AddPeerId(bool is_multi_reader) const {
    auto& BufRefRecord = *reinterpret_cast<BufRefRecordType*>(GetShareMemHead());
    int max_ref_count = 0;
    if (is_multi_reader) {
        max_ref_count = multiReader::MULTI_MAX_REF_COUNT;
    } else {
        max_ref_count = MAX_REF_COUNT;
    }
    for (int i = 1; i < max_ref_count; i++) {
        auto peer_id = static_cast<uint32_t>(BufRefRecord.GetUserPid(i));
        auto state = BufRefRecord.GetRefState(i);
        if (peer_id != 0 && state == 1) {
            AddPeerId(peer_id);
        }
    }
}

void BufferPoolOwner::AddPeerIdForNewPool() const {
    if (GetConfig().allocateMode == MultiReader) {
        AddPeerId<multiReader::BufRefRecord>(true);
    } else {
        AddPeerId<BufRefRecord>(false);
    }
}

bool BufferPoolOwner::UnRegisterUser(const UserID userId) const {
    if (GetConfig().allocateMode == MultiReader) {
        return reinterpret_cast<multiReader::BufRefRecord*>(GetRefRecord())->ResetUser(userId, *allocator);
    } else {
        return reinterpret_cast<BufRefRecord*>(GetRefRecord())->ResetUser(userId, *allocator);
    }
}

template uint64_t BufferPoolOwner::GetExtraSize<multiReader::BufRefRecord, multiReader::BufferNode>(
    size_t bufCount, size_t alignAddress);
template uint64_t BufferPoolOwner::GetExtraSize<BufRefRecord, BufferNode>(size_t bufCount, size_t alignAddress);

template <typename BufRefRecordType, typename BufferNodeType>
uint64_t BufferPoolOwner::GetExtraSize(const size_t bufCount, const size_t alignAddress) {
    // pool size must greater than result of align extra size
    const auto extraSize =
        sizeof(BufRefRecordType) + ReserveGapSize + (bufCount * sizeof(BufferNodeType)) + (ReserveGapSize / 8U);
    return DynamicSizeBufferAllocator::Align(extraSize, alignAddress);
}

bool BufferPoolOwner::CheckPoolParam(const std::string& name, size_t poolSize, size_t bufCount, size_t alignAddress,
                                     AllocateMode allocateMode) {
    // Check the parameters. If found invalid, return
    if ((0U == poolSize) || (0U == bufCount)) {
        DSF_LOG_ERROR(
            DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BAD_PARAMETER,
            "CreatePool name:" << name << " with size:" << poolSize << " or max_allocations:" << bufCount << " is 0: ");
        return false;
    }
    if (bufCount > kMaxAllocations) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_OUT_OF_RANGE,
                      "Allocation:" << bufCount << " overflows when creating buffer pool name: " << name);
        return false;
    }
    auto total_size = static_cast<uint64_t>(poolSize);
    if (allocateMode == MultiReader) {
        total_size += GetExtraSize<multiReader::BufRefRecord, multiReader::BufferNode>(bufCount, alignAddress);
    } else {
        total_size += GetExtraSize<BufRefRecord, BufferNode>(bufCount, alignAddress);
    }
    if (total_size > UINT32_MAX) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_OUT_OF_RANGE,
                      "Size:" << total_size << " overflows when creating buffer pool name: " << name);
        return false;
    }
    return true;
}

PoolParam BufferPoolOwner::CreatePoolShm(const std::string& name, const size_t poolSize, const size_t bufCount,
                                         const size_t bufMaxSize, const size_t alignAddress, const RecoverMode mode,
                                         const AllocateMode allocateMode, bool authentication) {

    PoolParam poolParam(poolSize, bufCount, bufMaxSize, alignAddress, mode, allocateMode);
    if (allocateMode == MultiReader) {
        poolParam.poolSize += GetExtraSize<multiReader::BufRefRecord, multiReader::BufferNode>(bufCount, alignAddress);
    } else {
        poolParam.poolSize += GetExtraSize<BufRefRecord, BufferNode>(bufCount, alignAddress);
    }
    int flags;
    if (authentication) {
        flags = O_CREAT | O_RDWR | UNIMEM_EXPORT_AUTHENTICATION_FLAG;
    } else {
        flags = O_CREAT | O_RDWR;
    }
    int32_t ret = shm_buf_open(name.c_str(), flags, 0644U, poolParam.poolSize);
    if (ret < 0) {
        DSF_LOG_WARNING(BufferPoolOwner, "create buffer pool name:"
                                             << name << ",pool size:" << poolSize << ",buf count:" << bufCount
                                             << ",buf max size:" << bufMaxSize << ",align address:" << alignAddress
                                             << " error,ret:" << ret << ",uuid:" << poolParam.poolId
                                             << ",fd:" << poolParam.fd << ",try to open");
        ret = shm_buf_open(name.c_str(), O_RDWR, 0644U, poolParam.poolSize);
        if (ret < 0) {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_OPEN_ERR,
                          "open buffer pool name:" << name << ",pool size:" << poolSize << ",buf count:" << bufCount
                                                   << ",buf max size:" << bufMaxSize
                                                   << ",align address:" << alignAddress << " error,ret:" << ret
                                                   << ",uuid:" << poolParam.poolId << ",fd:" << poolParam.fd
                                                   << ",open error");
            return poolParam;
        } else {
            poolParam.create = false;
        }
    } else {
        poolParam.create = true;
    }

    poolParam.fd = ret;
    int ret_uuid = shm_get_uuid(poolParam.fd, &poolParam.poolId);
    if (ret_uuid < 0) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_GET_UUID_ERR,
                      "shm_get_uuid error, pool name:" << name << ",ret uuid:" << ret_uuid
                                                       << ",pool param:" << poolParam.valid
                                                       << ",pool fd:" << poolParam.fd << ",uuid:" << poolParam.poolId);
        return poolParam;
    }

    poolParam.shareMemPtr = shm_mmap(poolParam.poolSize, poolParam.fd);
    poolParam.valid = (poolParam.shareMemPtr != nullptr);
    DSF_LOG_INFO(BufferPoolOwner, "ptr:" << poolParam.shareMemPtr << ",pool param:" << poolParam.valid
                                         << ",pool fd:" << poolParam.fd << ",pool name:" << name
                                         << ",uuid:" << poolParam.poolId << ",poolSize:" << poolParam.poolSize
                                         << ",bufCount:" << bufCount << ",authentication:" << authentication);
    return poolParam;
}

std::unique_ptr<BufferPoolOwner> BufferPoolOwner::CreateBufferPoolOwner(
    const std::string& name, const size_t poolSize, const size_t bufCount, const size_t bufMaxSize, const PID pid,
    const size_t alignAddress, const RecoverMode mode, AllocateMode allocateMode, bool authentication) {
    if (!CheckPoolParam(name, poolSize, bufCount, alignAddress, allocateMode)) {
        return nullptr;
    }
    const auto pool_shm =
        CreatePoolShm(name, poolSize, bufCount, bufMaxSize, alignAddress, mode, allocateMode, authentication);
    if (!pool_shm.valid) {
        return nullptr;
    }
    std::unique_ptr<BufferPoolOwner> bufferPoolOwner = nullptr;
    try {
        bufferPoolOwner = std::make_unique<BufferPoolOwner>(pool_shm, pid, pool_shm.fd, pool_shm.create, name);
    } catch (const std::exception& e) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_ERROR, "the error is: " << e.what());
    }
    return bufferPoolOwner;
}

BufferPoolOwner::~BufferPoolOwner() {
    DSF_LOG_DEBUG(BufferPoolOwner,
                  "pool name:" << poolName << ",pool id:" << GetID() << ",buffer pool owner fd:" << GetFd() << " dtor");
    if (GetFd() <= 0) {
        DSF_LOG_WARNING(BufferPoolOwner, "pool name:" << poolName << ",pool id:" << GetID()
                                                      << ",buffer pool owner fd:" << GetFd() << " <= 0");
        return;
    }
    const int ret = shm_unmap(GetConfig().shareMemPtr, GetConfig().poolSize);
    if (ret < 0) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_UNMAP_ERR,
                      "pool name:" << poolName << ",pool id:" << GetID() << ",fd:" << GetFd() << ",unmap ret:" << ret
                                   << ",shareMemPtr:" << GetConfig().shareMemPtr
                                   << ",poolSize:" << GetConfig().poolSize);
    }
    int closeRet = shm_buf_close(GetFd());
    if (closeRet < 0) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SHM_UNMAP_ERR,
                      "pool name:" << poolName << ",pool id:" << GetID() << ",fd:" << GetFd()
                                   << ",close ret:" << closeRet << ",shareMemPtr:" << GetConfig().shareMemPtr
                                   << ",poolSize:" << GetConfig().poolSize);
    }
    DSF_LOG_DEBUG(BufferPoolOwner,
                  "pool name:" << poolName << ",pool id:" << GetID() << ",fd:" << GetFd() << ",close ret:" << closeRet
                               << ",shareMemPtr:" << GetConfig().shareMemPtr << ",poolSize:" << GetConfig().poolSize);
}
template <typename BufferNodeType>
ReturnCode_t BufferPoolOwner::ProcessStateInMetaInternal(const BufferPtr& mbuf, UserID userId,
                                                         StateOperator stateOperator,
                                                         li::dsfdds::MultiModeChannel auto_shm_bigbuffer) {
  ReturnCode_t ret = ReturnCode_t::RETCODE_MEMBUF_OK;
  uint32_t buffer_info_size = 0U;
  mbuf::BufferDesc* p_buffer_info = nullptr;
  if ((auto_shm_bigbuffer & li::dsfdds::MultiModeChannel::SHM) !=
      li::dsfdds::MultiModeChannel::NONE)  // auto链路不用处理Meta状态
  {
      auto head_count = *static_cast<uint32_t*>(mbuf->data());
      buffer_info_size = head_count - 1;
      p_buffer_info = static_cast<mbuf::BufferDesc*>(
          static_cast<void*>(static_cast<uint8_t*>(mbuf->data()) + head_count * sizeof(uint32_t)));
  } else {
    buffer_info_size = *static_cast<uint32_t*>(mbuf->data());
    p_buffer_info =
        static_cast<mbuf::BufferDesc*>(static_cast<void*>(static_cast<uint8_t*>(mbuf->data()) + sizeof(uint32_t)));
  }
  for (uint32_t i = 0U; i < buffer_info_size; ++i) {
      // TODO
      // 这样写的前提是申请子mbuf的writer和申请meta buffer的writer是同一个，否则就需要根据子mbuf的desc信息找到自己的BufferPool而不是复用meta的BufferPool
      // 困难的选择：在连续的转发中，是否需要解包子mbuf，如果解包拷贝新数据到新的mbuf上效率上不如原样转发。
      // 如果不解包子mbuf 还是转发原来的子mbuf 则面临状态不好迁移（等待using->free）的问题和这里的写法要改变(这里要加入OpenPool的逻辑)
      // 如果不允许mbuf的多级传递则要改变下现有bigdata的测试程序，因为现有写法是逐级传递的。(结论：使用不允许跨进程传的方案)
      auto* bufferNode = reinterpret_cast<BufferNodeType*>(GetBufferNode(p_buffer_info->GetNodeOffset()));
      DSF_LOG_DEBUG(BufferPoolOwner, "pool name:" << poolName
                                                  << ",p_buffer_info recycle num:" << p_buffer_info->GetRecycleNums()
                                                  << ",buffer node recycle num:" << bufferNode->GetRecycleNums());
      if (!p_buffer_info->IsValid(bufferNode->GetRecycleNums())) {
          DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_RECYCLE_NUM_INVALID,
                        "pool name:" << poolName << ",sub buffer is not valid,BufferDesc RecycleNums:"
                                     << p_buffer_info->GetRecycleNums()
                                     << ",buffer node RecycleNums:" << bufferNode->GetRecycleNums());
          return ReturnCode_t::RETCODE_MEMBUF_SUBBUF_RECYCLE_NUM_INVALID;
      }
    switch (stateOperator) {
      case Ope_On_Pending: {
        if (!bufferNode->OnPending(userId)) {
          ret = ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_PENDING_FAILED;
          DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_PENDING_FAILED,
                        "pool name:" << poolName << ",sub buffer on pending failed, user id:" << userId
                                     << ",buffer node current state:" << bufferNode->QueryState(userId));
        }
        break;
      }
      case Ope_On_Free: {
        if (!bufferNode->OnExcept(userId)) {
          ret = ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_FREE_FAILED;
          DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_FREE_FAILED,
                        "pool name:" << poolName << ",sub buffer reset to free failed, user id:" << userId
                                     << ",buffer node current state:" << bufferNode->QueryState(userId));
        }
        break;
      }
      default: {
        ret = ReturnCode_t::RETCODE_MEMBUF_SUBBUF_UNKNOWN_OPERATE;
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_UNKNOWN_OPERATE,
                      "pool name:" << poolName << ",receive unknown count_operator: " << stateOperator);
        break;
      }
    }
    p_buffer_info++;
  }
  return ret;
}

ReturnCode_t BufferPoolOwner::ProcessStateInMeta(const BufferPtr& mbuf, UserID userId, StateOperator stateOperator,
                                                 li::dsfdds::MultiModeChannel auto_shm_bigbuffer) {
  if (mbuf == nullptr) {
      DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_INPUT_MBUF_NULL,
                    "pool name:" << poolName << ",input buffer is null, user id:" << userId
                                 << ",stateOperator:" << stateOperator);
      return ReturnCode_t::RETCODE_MEMBUF_INPUT_MBUF_NULL;
  }
  if (GetConfig().allocateMode == MultiReader) {
    return ProcessStateInMetaInternal<multiReader::BufferNode>(mbuf, userId, stateOperator, auto_shm_bigbuffer);
  } else {
    return ProcessStateInMetaInternal<BufferNode>(mbuf, userId, stateOperator, auto_shm_bigbuffer);
  }
}

bool BufferPoolOwner::IsMultiReader() const {
    return GetConfig().allocateMode == MultiReader;
}

}  // namespace mbuf
