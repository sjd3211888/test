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

#ifndef MEMBUF_SRC_CORE_SCALE_ALGO_H
#define MEMBUF_SRC_CORE_SCALE_ALGO_H
#include <cstddef>
#include <vector>
namespace mbuf {

struct PoolStatus {
    size_t used_pool_size_;
    size_t used_buf_count_;
    size_t alloc_pool_size_;  // 创建 pool 时输入的 poolSize. 小于实际 size
    size_t buf_count_;
};

class ScaleAlgo {
 public:
    ScaleAlgo() {};
    virtual ~ScaleAlgo() {};
    /**
   * @brief 更新当前分配出去的 buf 的大小(每次分配时执行)
   * 
   * @param alloc_size 分配出去的 buf size
   */
    virtual void UpdateAllocSize(size_t alloc_size) = 0;

    /**
   * @brief 更新 pool 的使用状态, 每次查询 pool 的状态时执行
   * 
   * @param status 当前 pool 的使用状态
   */
    virtual void UpdatePoolStatus(const PoolStatus& status) = 0;

    /**
   * @brief 尝试扩容
   * 
   * @param[out] new_buf_count 扩容后的 buf_count
   * @param[out] new_alloc_pool_size 扩容后的 pool_size
   * @return true 可以扩容
   * @return false 无法扩容
   */
    virtual bool ScaleUp(size_t& new_buf_count, size_t& new_alloc_pool_size) = 0;

    /**
   * @brief 尝试缩容
   * 
   * @param[out] new_buf_count 缩容后的 buf_count
   * @param[out] new_alloc_pool_size 缩容后的 pool_size
   * @return true 可以扩容
   * @return false 无法扩容
   */
    virtual bool ScaleDown(size_t& new_buf_count, size_t& new_alloc_pool_size) = 0;
};
}  // namespace mbuf
#endif  // MEMBUF_SRC_CORE_SCALE_ALGO_H
