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

#ifndef MEMBUF_SRC_CORE_ADAPTIVE_SCALE_ALGO_H
#define MEMBUF_SRC_CORE_ADAPTIVE_SCALE_ALGO_H

#include <stdint.h>
#include <array>
#include "ScaleAlgo.h"
#include "membuf/BufferPool.h"

namespace mbuf {

// 保存历史分配大小的数量
constexpr size_t kHisAllocSizeNum = 16;
constexpr size_t kHisAllocSizeIdxMask = kHisAllocSizeNum - 1;
// 触发 pool size 扩大的 pool size 使用率阈值
constexpr double kPoolSizeScaleUpThld = 0.5;
// 触发 buf count 扩大的 buf count 使用率阈值
constexpr double kBufCountScaleUpThld = 0.5;

struct RecommendSizeTable {
    // 根据文档 https://li.feishu.cn/docx/BjdbdPw2FoQxg0xO0bjcsYSXngg 给出的 5 个档位进行推荐
    static bool RecommendSize(size_t alloc_size, const AllocCfg& alloc_cfg, size_t& new_buf_count,
                              size_t& new_alloc_pool_size);
};

class AdaptiveScaleAlgo : public ScaleAlgo {
 public:
    /**
   * @brief Construct a new Adaptive Scale Algo object
   * 
   * @param init_alloc_pool_size 初始配置的 pool_size
   * @param init_buf_count 初始配置的 max_allocation
   */
    AdaptiveScaleAlgo(AllocCfg alloc_config, size_t init_alloc_pool_size, size_t init_buf_count);
    virtual ~AdaptiveScaleAlgo() {};
    AdaptiveScaleAlgo(const AdaptiveScaleAlgo&) = delete;
    AdaptiveScaleAlgo& operator=(const AdaptiveScaleAlgo&) = delete;
    AdaptiveScaleAlgo(AdaptiveScaleAlgo&&) = delete;
    AdaptiveScaleAlgo& operator=(AdaptiveScaleAlgo&&) = delete;

    void UpdateAllocSize(size_t alloc_size) override;
    void UpdatePoolStatus(const PoolStatus& status) override;
    bool ScaleUp(size_t& new_buf_count, size_t& new_alloc_pool_size) override;
    bool ScaleDown(size_t& new_buf_count, size_t& new_alloc_pool_size) override;

 protected:
    size_t CurAllocSize() const;

 private:
    const AllocCfg alloc_cfg_;
    const size_t init_alloc_pool_size_;
    const size_t init_buf_count_;
    size_t alloc_num_;
    std::array<size_t, kHisAllocSizeNum> his_alloc_size_;
    size_t his_alloc_size_sum_;
    uint64_t alloc_size_steady_duration_;
    PoolStatus pool_status_;
    double pool_size_usage_;
    double buf_count_usage_;
    uint64_t low_usage_begin_;
    uint64_t low_usage_duration_;
};
}  // namespace mbuf
#endif  // MEMBUF_SRC_CORE_ADAPTIVE_SCALE_ALGO_H
