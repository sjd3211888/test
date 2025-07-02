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

#ifndef DSFDDS_MBUF_POOL_CRASH_STRATEGY_H
#define DSFDDS_MBUF_POOL_CRASH_STRATEGY_H
#include <memory>
#include <unordered_set>
#include "dsfdds/transport/pool_crash_strategy.h"

namespace li {
namespace dsfdds {
class MembufPoolCrashStrategy : public PoolCrashStrategy {
 public:
    explicit MembufPoolCrashStrategy(const std::shared_ptr<DiscoveryEndpoint>& discoveryEndpoint);
    virtual ~MembufPoolCrashStrategy();

    void UpdateCrashedMbufPools(const uint32_t uuid, const uint64_t pid) override;
    void UpdateUnmatchedMbufPools(const uint32_t uuid);
    void UpdateWriterRecycleMbufPools(const uint32_t uuid);

    /**
   * @brief 执行 pool 回收
   * !! 为了减少锁的范围, 在回收前后分别执行 pool 的读取和更新, 所以只能在一个线程里调用
   * 
   */
    void RecycleMbufPool();
    void CheckWriterStatusUpdatePoolIds();

 protected:
    void ReaderRecycle();
    void WriterRecycle();
    bool CheckRecycleReaderMbufDestructNTS(const MbufPoolUUID& uuid);
    bool CheckRecycleWriterMbufDestructNTS(const MbufPoolUUID& uuid);

 private:
    // key is uuid
    std::mutex mtx_reader_mbuf_pools_;
    std::unordered_set<MbufPoolUUID> reader_recycle_mbuf_pools_;
    std::mutex mtx_writer_mbuf_pools_;
    std::unordered_set<MbufPoolUUID> writer_recycle_mbuf_pools_;
};

}  // namespace dsfdds
}  // namespace li
#endif  // DSFDDS_MBUF_POOL_CRASH_STRATEGY_H
