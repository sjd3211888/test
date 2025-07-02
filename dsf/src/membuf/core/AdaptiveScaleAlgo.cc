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

#include "AdaptiveScaleAlgo.h"
#include "BufferPoolOwner.h"
#include "DsfLog.hpp"

namespace mbuf {

// 根据文档 https://li.feishu.cn/docx/BjdbdPw2FoQxg0xO0bjcsYSXngg 给出的 5 个档位进行推荐
bool RecommendSizeTable::RecommendSize(size_t alloc_size, const AllocCfg& alloc_cfg, size_t& new_buf_count,
                                       size_t& new_alloc_pool_size) {
    if (alloc_size < 10 * 1024ul) {  // 10KB
        new_buf_count = 48ul;
        new_alloc_pool_size = 512 * 1024ul;  // 512KB
        return true;
    } else if (alloc_size < 42 * 1024ul) {  // 42KB
        new_buf_count = 48ul;
        new_alloc_pool_size = 2 * 1024 * 1024ul;  // 2MB
        return true;
    } else if (alloc_size < 170 * 1024ul) {  // 170KB
        new_buf_count = 48ul;
        new_alloc_pool_size = 8 * 1024 * 1024ul;  // 8MB
        return true;
    } else if (alloc_size < 1365 * 1024ul) {  // 1.3MB
        new_buf_count = 24ul;
        new_alloc_pool_size = 32 * 1024 * 1024ul;  // 32MB
        return true;
    } else if (alloc_size < 5461 * 1024ul) {  // 5.4MB
        new_buf_count = 24ul;
        new_alloc_pool_size = 128 * 1024 * 1024ul;
        return true;
    } else if (alloc_size <= alloc_cfg.allocSizeLimit_) {
        new_buf_count = 24ul;
        new_alloc_pool_size = alloc_cfg.poolSizeUpLimit_;
        return true;
    }
    return false;
}

AdaptiveScaleAlgo::AdaptiveScaleAlgo(AllocCfg alloc_cfg, size_t init_alloc_pool_size, size_t init_buf_count)
    : alloc_cfg_(alloc_cfg),
      init_alloc_pool_size_ {init_alloc_pool_size},
      init_buf_count_ {init_buf_count},
      alloc_num_ {0},
      his_alloc_size_ {},
      his_alloc_size_sum_(0),
      alloc_size_steady_duration_(0),
      pool_status_ {
          0,
          0,
          init_alloc_pool_size,
          init_buf_count,
      },
      pool_size_usage_ {0.0},
      buf_count_usage_ {0.0},
      low_usage_begin_ {0},
      low_usage_duration_ {0} {
    DSF_LOG_DEBUG(AdaptiveScaleAlgo, "alloc_cfg_:" << alloc_cfg_);
}

void AdaptiveScaleAlgo::UpdateAllocSize(size_t alloc_size) {
    size_t idx = alloc_num_ & kHisAllocSizeIdxMask;
    his_alloc_size_sum_ += alloc_size;
    his_alloc_size_sum_ -= his_alloc_size_[idx];
    his_alloc_size_[idx] = alloc_size;
    // 计算 alloc size 是否保持稳定
    double avg_alloc_size = 0.0;
    if (alloc_num_ < his_alloc_size_.size()) {
        avg_alloc_size = static_cast<double>(his_alloc_size_sum_) / static_cast<double>(alloc_num_ + 1);
    } else {
        avg_alloc_size = static_cast<double>(his_alloc_size_sum_) / static_cast<double>(his_alloc_size_.size());
    }

    double alloc_size_diff = 0.0;
    if (his_alloc_size_sum_ == 0) {  // 防止除 0
        alloc_size_diff = 0.0;
    } else {
        alloc_size_diff = std::abs((static_cast<double>(alloc_size) - avg_alloc_size) / avg_alloc_size);
    }
    if (alloc_size_diff < alloc_cfg_.allocSizeSteadyTol_) {
        ++alloc_size_steady_duration_;
    } else {
        alloc_size_steady_duration_ = 0;
    }
    alloc_num_++;
}

void AdaptiveScaleAlgo::UpdatePoolStatus(const PoolStatus& status) {
    double buf_count_usage = static_cast<double>(status.used_buf_count_) / static_cast<double>(status.buf_count_);
    double pool_size_usage = static_cast<double>(status.used_pool_size_) / static_cast<double>(status.alloc_pool_size_);
    // 利用保持低水平
    if (pool_size_usage >= alloc_cfg_.poolSizeUsageScaleDownThld_) {
        low_usage_begin_ = alloc_num_;
    }
    low_usage_duration_ = alloc_num_ - low_usage_begin_;
    pool_status_ = status;
    buf_count_usage_ = buf_count_usage;
    pool_size_usage_ = pool_size_usage;
    DSF_LOG_DEBUG(AdaptiveScaleAlgo, "pool_size_usage_:" << pool_size_usage_ << " buf_count_usage_:" << buf_count_usage_
                                                         << " low_usage_begin_" << low_usage_begin_
                                                         << " low_usage_duration_:" << low_usage_duration_
                                                         << " alloc_num_:" << alloc_num_);
}

bool AdaptiveScaleAlgo::ScaleUp(size_t& new_buf_count, size_t& new_alloc_pool_size) {
    size_t cur_alloc_size = CurAllocSize();
    // 如果 alloc_size 超过上限, 无法分配, 扩容失败
    if (cur_alloc_size > alloc_cfg_.allocSizeLimit_) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CONFIG_ERR,
                      "alloc buf size: " << cur_alloc_size << " exceeds limit:" << alloc_cfg_.allocSizeLimit_);
        return false;
    }

    DSF_LOG_DEBUG(AdaptiveScaleAlgo, "pool_size_usage_:" << pool_size_usage_ << " buf_count_usage_:" << buf_count_usage_
                                                         << " low_usage_begin_" << low_usage_begin_
                                                         << " low_usage_duration_:" << low_usage_duration_
                                                         << " alloc_num_:" << alloc_num_);
    // 根据 pool 状态分配
    new_alloc_pool_size = pool_status_.alloc_pool_size_;
    new_buf_count = pool_status_.buf_count_;

    if (buf_count_usage_ > kBufCountScaleUpThld) {
        new_buf_count *= 2;
    }
    if (pool_size_usage_ > kPoolSizeScaleUpThld) {
        new_alloc_pool_size *= 2;
    }

    // 查推荐表, 根据最近分配的最大 size
    size_t new_buf_count1 = 0, new_alloc_pool_size1 = 0;
    size_t max_his_alloc_size = *std::max_element(his_alloc_size_.begin(), his_alloc_size_.end());
    if (RecommendSizeTable::RecommendSize(max_his_alloc_size, alloc_cfg_, new_buf_count1, new_alloc_pool_size1)) {
        new_buf_count = std::max(new_buf_count, new_buf_count1);
        new_alloc_pool_size = std::max(new_alloc_pool_size, new_alloc_pool_size1);
    }

    // 不小于初始配置
    new_buf_count = std::max(new_buf_count, init_buf_count_);
    new_alloc_pool_size = std::max(new_alloc_pool_size, init_alloc_pool_size_);

    // 不超过上限
    new_buf_count = std::min(new_buf_count, alloc_cfg_.bufCountUpLimit_);
    new_alloc_pool_size = std::min(new_alloc_pool_size, alloc_cfg_.poolSizeUpLimit_);
    return true;
}

bool AdaptiveScaleAlgo::ScaleDown(size_t& new_buf_count, size_t& new_alloc_pool_size) {
    DSF_LOG_DEBUG(AdaptiveScaleAlgo,
                  "pool_size_usage_:" << pool_size_usage_ << " buf_count_usage_:" << buf_count_usage_
                                      << " low_usage_begin_" << low_usage_begin_
                                      << " low_usage_duration_:" << low_usage_duration_ << " alloc_num_:" << alloc_num_
                                      << " alloc_size_steady_duration_:" << alloc_size_steady_duration_
                                      << " cur alloc_pool_size_:" << pool_status_.alloc_pool_size_
                                      << " cur buf_count_:" << pool_status_.buf_count_);
    new_alloc_pool_size = pool_status_.alloc_pool_size_;
    new_buf_count = pool_status_.buf_count_;
    // 如果当前 pool alloc size, buf count 是初始值, 则不缩容
    if (new_alloc_pool_size == init_alloc_pool_size_ && new_buf_count == init_buf_count_) {
        return false;
    }

    // 条件1, 利用率持续低, 且利用率没有增长
    if (low_usage_duration_ < alloc_cfg_.lowUsageDurationForScaleDown_) {
        return false;
    }

    // 条件2, 分配的 size 持续稳定
    if (alloc_size_steady_duration_ < alloc_cfg_.allocSteadyDurationForScaleDown_) {
        return false;
    }

    // 按 target usage 缩容
    new_alloc_pool_size = static_cast<size_t>(static_cast<double>(new_alloc_pool_size) * pool_size_usage_ /
                                              alloc_cfg_.scaleDownTargetUsage_);
    new_buf_count =
        static_cast<size_t>(static_cast<double>(new_buf_count) * buf_count_usage_ / alloc_cfg_.scaleDownTargetUsage_);

    // 不小于初始配置
    new_buf_count = std::max(new_buf_count, init_buf_count_);
    new_alloc_pool_size = std::max(new_alloc_pool_size, init_alloc_pool_size_);

    // 不大于当前值
    new_buf_count = std::min(new_buf_count, pool_status_.buf_count_);
    new_alloc_pool_size = std::min(new_alloc_pool_size, pool_status_.alloc_pool_size_);

    // 不超过上限
    new_buf_count = std::min(new_buf_count, alloc_cfg_.bufCountUpLimit_);
    new_alloc_pool_size = std::min(new_alloc_pool_size, alloc_cfg_.poolSizeUpLimit_);

    // 如果最后结果与当前的 pool size, buf count 相同, 则不用缩容
    if (new_buf_count == pool_status_.buf_count_ && new_alloc_pool_size == pool_status_.alloc_pool_size_) {
        return false;
    }

    return true;
}

size_t AdaptiveScaleAlgo::CurAllocSize() const {
    if (his_alloc_size_.empty() || alloc_num_ < 1) {
        return 0;
    }
    return his_alloc_size_[(alloc_num_ - 1) & kHisAllocSizeIdxMask];
}

}  // namespace mbuf
