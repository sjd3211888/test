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

#include "dsfdds/transport/membuf_pool_crash_strategy.h"

namespace li {
namespace dsfdds {

MembufPoolCrashStrategy::MembufPoolCrashStrategy(const std::shared_ptr<DiscoveryEndpoint>& discoveryEndpoint)
    : PoolCrashStrategy(discoveryEndpoint),
      mtx_reader_mbuf_pools_ {},
      reader_recycle_mbuf_pools_ {},
      mtx_writer_mbuf_pools_ {},
      writer_recycle_mbuf_pools_ {} {}

MembufPoolCrashStrategy::~MembufPoolCrashStrategy() {}

void MembufPoolCrashStrategy::UpdateCrashedMbufPools(const uint32_t uuid, const uint64_t pid) {
    bool is_writer_crash = false;
    // 1. 检查异常的pid是writer还是reader
    {
        std::lock_guard<std::mutex> lk(discovery_endpoint_->mtx_local_register_reader_gs_);
        for (auto& it : discovery_endpoint_->local_register_readers_) {
            if (it->IsCrashPidInMatchedWriter(uuid, pid)) {
                // reader crash 无需处理, writer crash需要记录
                is_writer_crash = true;
                break;
            }
        }
    }
    // 2. 如果是writer crash 记录异常的mbuf，后续在合适的时机统一close mbuf pool
    if (is_writer_crash) {
        UpdateUnmatchedMbufPools(uuid);
    }
}

// 2.调用此函数前需要判断reader打开过才加入crashed_mbuf_pools_
void MembufPoolCrashStrategy::UpdateUnmatchedMbufPools(const uint32_t uuid) {
    // 添加到crashed_mbuf_pools_，后续在合适的时机统一close mbuf pool
    std::lock_guard<std::mutex> lk(mtx_reader_mbuf_pools_);
    reader_recycle_mbuf_pools_.emplace(uuid);
    DSF_LOG_DEBUG(MembufPoolCrashStrategy, "pool id:" << uuid << " is placed in reader_recycle_mbuf_pools_");
}

void MembufPoolCrashStrategy::UpdateWriterRecycleMbufPools(const uint32_t uuid) {
    std::lock_guard<std::mutex> lk(mtx_writer_mbuf_pools_);
    writer_recycle_mbuf_pools_.emplace(uuid);
    DSF_LOG_INFO(MembufPoolCrashStrategy, "pool id:" << uuid << " is placed in writer_recycle_mbuf_pools_");
}

void MembufPoolCrashStrategy::RecycleMbufPool() {
    ReaderRecycle();
    WriterRecycle();
}

void MembufPoolCrashStrategy::ReaderRecycle() {
    // 先锁 discovery_endpoint
    std::lock_guard<std::mutex> lk(discovery_endpoint_->mtx_local_register_reader_gs_);
    {
        std::lock_guard<std::mutex> lk(mtx_reader_mbuf_pools_);
        // 先进行 recycle mbuf pools 的回收
        DSF_LOG_DEBUG(MembufPoolCrashStrategy,
                      "reader recycle, reader_recycle_mbuf_pools_ size: " << reader_recycle_mbuf_pools_.size());
        auto recycle_mbuf_pool_it = reader_recycle_mbuf_pools_.begin();
        while (recycle_mbuf_pool_it != reader_recycle_mbuf_pools_.end()) {
            auto pool_id = *recycle_mbuf_pool_it;
            if (CheckRecycleReaderMbufDestructNTS(pool_id)) {
                recycle_mbuf_pool_it = reader_recycle_mbuf_pools_.erase(recycle_mbuf_pool_it);
            } else {
                ++recycle_mbuf_pool_it;
            }
        }
    }
}

bool MembufPoolCrashStrategy::CheckRecycleReaderMbufDestructNTS(const MbufPoolUUID& uuid) {
    bool canErase = true;
    for (auto& it : discovery_endpoint_->local_register_readers_) {
        bool stillOpen = false;
        bool freed = it->buffer_pool_user_manager_->FreePool(uuid, stillOpen);
        if (freed) {
            DSF_LOG_INFO(MembufPoolCrashStrategy,
                         "recycled pool id:" << uuid << " is freed by reader: " << it->get_guid());
        }
        if (stillOpen) {
            canErase = false;
        }
    }
    return canErase;
}

void MembufPoolCrashStrategy::WriterRecycle() {
    // 先锁 discovery_endpoint
    std::lock_guard<std::mutex> lk(discovery_endpoint_->mtx_local_register_writer_gs_);
    {
        std::lock_guard<std::mutex> lk(mtx_writer_mbuf_pools_);
        auto recycle_mbuf_pool_it = writer_recycle_mbuf_pools_.begin();
        while (recycle_mbuf_pool_it != writer_recycle_mbuf_pools_.end()) {
            auto pool_id = *recycle_mbuf_pool_it;
            if (CheckRecycleWriterMbufDestructNTS(pool_id)) {
                DSF_LOG_INFO(MembufPoolCrashStrategy, "recycled pool id:" << pool_id << " is freed by writer!");
                recycle_mbuf_pool_it = writer_recycle_mbuf_pools_.erase(recycle_mbuf_pool_it);
            } else {
                ++recycle_mbuf_pool_it;
            }
        }
    }
}

bool MembufPoolCrashStrategy::CheckRecycleWriterMbufDestructNTS(const MbufPoolUUID& uuid) {
    bool erased = false;
    for (auto& it : discovery_endpoint_->local_register_writers_) {
        if (it->GetPool()->FreePool(uuid)) {
            erased = true;
        }
    }
    return erased;
}

/**
 * Corner Cases：静态发现下writer crash时候reader未读取，后续正常读取，然后writer
 * 重启上线，这种场景会把旧的wp.second.pool_id_覆盖成新的，并且状态可能也不是unmatched
 * 导致旧的membuf pool没有机会被释放掉。
 */
void MembufPoolCrashStrategy::CheckWriterStatusUpdatePoolIds() {
    // 1. only check local reader unmatched or crashed status
    std::lock_guard<std::mutex> lk(discovery_endpoint_->mtx_local_register_reader_gs_);
    for (auto& it : discovery_endpoint_->local_register_readers_) {
        std::lock_guard<std::mutex> lock_guard(it->mtx_matched_writer_gs_);
        for (auto& wp : it->matched_writers_) {
            // 查询状态发现已经unmatched 并且 打开过 pool id，则加入异常处理流程
            if (wp.second.IsUnmatched() && it->buffer_pool_user_manager_->IsInBufferPoolMap(wp.second.pool_id_)) {
                UpdateUnmatchedMbufPools(wp.second.pool_id_);
            }
        }
    }
}

}  // namespace dsfdds
}  // namespace li
