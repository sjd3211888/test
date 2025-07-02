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

#ifndef MEMBUF_INC_BUFFER_POOL_USER_MANAGER_H
#define MEMBUF_INC_BUFFER_POOL_USER_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "membuf/BufferDesc.h"
#include "membuf/BufferPool.h"
#include "membuf/buffer.h"
namespace mbuf {
struct BufferPoolUserManager {
    friend class MembufMonitor;
    BufferPoolUserManager();
    BufferPoolUserManager(const BufferPoolUserManager& other) = delete;
    BufferPoolUserManager& operator=(const BufferPoolUserManager& other) = delete;
    BufferPoolUserManager(BufferPoolUserManager&& other) noexcept = delete;
    BufferPoolUserManager& operator=(BufferPoolUserManager&& other) noexcept = delete;
    BufferPtr RebuildBuffer(const BufferDesc&);
    void RegisterBufferPool(const BufferPool&);
    ~BufferPoolUserManager();
    void ClosePool();
    BufferPool* OpenPool(PoolID id);
    static bool OnPeerCrash(PID);

    /**
   * @brief 尝试释放打开的 pool
   *
   * @param id 释放的 pool id
   * @param stillOpen 调用之后该 pool 是否还被 this 持有
   * @return true 释放成功
   * @return false 释放失败
   */
    bool FreePool(PoolID id, bool& stillOpen);
    bool IsInBufferPoolMap(PoolID id);
    void SetRecoverMode(RecoverMode mode) { this->recoverMode = mode; }
    RecoverMode GetRecoverMode() const { return this->recoverMode; }

 private:
    static PoolParam OpenPoolShm(PoolID id);
    bool IsPoolFree(PoolID) const;
    // just for membuf monitor
    bool DumpMembuf(PoolID id, void** buf, size_t& len);
    template <typename BufferNodeType, typename BufRefRecordType>
    BufferPtr RebuildBufferInternal(const BufferDesc& desc, BufferPool* bufferPool, bool multiReader);
    BufferPool* OpenPoolNTS(PoolID id);

 private:
    std::mutex mtxBufferPoolMap;
    std::unordered_map<PoolID, BufferPool> bufferPoolMap;
    using PoolMapIterator = decltype(bufferPoolMap.begin());
    // best effort 模式对应的 pool 的 recover mode 是 force, 和 queue 的 overrite 是绑定的.
    // 该参数会影响判断 pool 是否能被释放
    RecoverMode recoverMode = RecoverMode::Normal;
    bool IsPoolNotUsing(PoolID) const;
    bool PoolCanBeFreed(PoolMapIterator poolMapIterator) const;
};
}  // namespace mbuf
#endif  // MEMBUF_INC_BUFFER_POOL_USER_MANAGER_H
