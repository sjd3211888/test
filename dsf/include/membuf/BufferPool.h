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

#ifndef MEMBUF_INC_BUFFER_POOL_H
#define MEMBUF_INC_BUFFER_POOL_H

#include <type_traits>
#include <ostream>
#include "membuf/BaseType.h"
#include "membuf/BufferDesc.h"
#include "membuf/MembufMonitor.h"

namespace mbuf {
struct BufRefRecord;
struct BufferNode;

enum RecoverMode {
    Normal,
    Force,
};

struct BufPoolCfg {
    void* shareMemPtr;
    size_t poolSize;
    size_t bufCount;
    size_t bufMaxSize;
    size_t alignAddress;
    PoolID poolId;  // legal value range: 0x70000001 ~ 0x80000000
    RecoverMode recoverMode;
    AllocateMode allocateMode;
};

struct PoolParam : BufPoolCfg {
    int fd;
    bool create;
    bool valid;
    PoolParam(const size_t poolSize, const size_t bufCount, const size_t bufMaxSize, const size_t alignAddress,
              const RecoverMode mode, const AllocateMode alloc_mode = AllocateMode::Dynamic)
        : BufPoolCfg {nullptr, poolSize, bufCount, bufMaxSize, alignAddress, 0U, mode, alloc_mode},
          fd(-1),
          create(false),
          valid(false) {}
    PoolParam(const bool create, const PoolID id)
        : BufPoolCfg {nullptr, 0U, 0U, 0U, 0U, id, Normal, Dynamic}, fd(-1), create(create), valid(false) {}
};

struct BufferPool {
    explicit BufferPool(const BufPoolCfg& config, int fd, bool isCreated = true);
    void* GetBufferNode(BufferNodeOffset) const;

    PoolID GetID() const { return config.poolId; }
    BufferNodeOffset GetOffset(void* bufferNode) const;
    void* GetBufferHead() const;
    void* GetRefRecord() const;
    MemOffset GetDataStartOffset() const;
    size_t GetPoolSize() const;
    size_t GetBufCount() const { return config.bufCount; }
    uint8_t* GetShareMemHead() const { return reinterpret_cast<uint8_t*>(config.shareMemPtr); }

    RecoverMode GetRecoverMode() const { return config.recoverMode; }
    size_t GetRealPoolSize() const;
    size_t GetBufMaxSize() const { return config.bufMaxSize; }
    BufPoolCfg GetConfig() const { return config; }
    size_t GetAlignAddress() const { return config.alignAddress; }

    int GetFd() const { return fd; }
    // used for dfx
    void GetAllBufferNodeState(std::ostream& ostream) const;
    void GetStatisticsInfo(std::ostream& ostream) const;
    const MembufMonitor& GetMembufMonitor() const;

    bool IsAllUserNotUsing() const;
    bool IsAllUserFree() const;
    bool IsAllOwnerFree() const;

 private:
    void Reset() const;

 private:
    BufPoolCfg config;
    int fd;
    MembufMonitor monitor;
};

static_assert(std::is_standard_layout<BufferPool>::value, "BufferNode must be a trivial type");

struct AllocCfg {
    // 分配 pool size 的上限
    size_t poolSizeUpLimit_;
    // 单包最大 size
    size_t allocSizeLimit_;
    // pool 能分配的最大 buf count 上限
    size_t bufCountUpLimit_;
    // 触发 scale down 的 pool size 使用率阈值
    double poolSizeUsageScaleDownThld_;
    // 持续多少次分配保持 low usage 可以缩容
    uint64_t lowUsageDurationForScaleDown_;
    // 分配 size steady 容差
    double allocSizeSteadyTol_;
    // 持续多少次分配保持 分配 size 稳定可以缩容
    uint64_t allocSteadyDurationForScaleDown_;
    // scale down 目标 usage
    double scaleDownTargetUsage_;
    // 查询 pool status 的间隔分配次数
    uint64_t queryStatusInterval_;

    AllocCfg(size_t poolSizeUpLimit, size_t allocSizeLimit, size_t bufCountUpLimit, double poolSizeUsageScaleDownThld,
             uint64_t lowUsageDurationForScaleDown, double allocSizeSteadyTol, uint64_t allocSteadyDurationForScaleDown,
             double scaleDownTargetUsage, uint64_t queryStatusInterval)
        : poolSizeUpLimit_(poolSizeUpLimit),
          allocSizeLimit_(allocSizeLimit),
          bufCountUpLimit_(bufCountUpLimit),
          poolSizeUsageScaleDownThld_(poolSizeUsageScaleDownThld),
          lowUsageDurationForScaleDown_(lowUsageDurationForScaleDown),
          allocSizeSteadyTol_(allocSizeSteadyTol),
          allocSteadyDurationForScaleDown_(allocSteadyDurationForScaleDown),
          scaleDownTargetUsage_(scaleDownTargetUsage),
          queryStatusInterval_(queryStatusInterval) {}

    friend std::ostream& operator<<(std::ostream& os, const AllocCfg& obj) {
        return os << "poolSizeUpLimit_:" << obj.poolSizeUpLimit_ << " allocSizeLimit_:" << obj.allocSizeLimit_
                  << " bufCountUpLimit_:" << obj.bufCountUpLimit_
                  << " poolSizeUsageScaleDownThld_:" << obj.poolSizeUsageScaleDownThld_
                  << " lowUsageDurationForScaleDown_:" << obj.lowUsageDurationForScaleDown_
                  << " allocSizeSteadyTol_:" << obj.allocSizeSteadyTol_
                  << " allocSteadyDurationForScaleDown_:" << obj.allocSteadyDurationForScaleDown_
                  << " scaleDownTargetUsage_:" << obj.scaleDownTargetUsage_
                  << " queryStatusInterval_:" << obj.queryStatusInterval_;
    }
};
}  // namespace mbuf
#endif  // MEMBUF_INC_BUFFER_POOL_H
