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

#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "common/types/TypesBase.h"

namespace shm {
using queue_ret_t = li::dsfdds::ReturnCode_t;
enum MemoryMode {
    KUNIMEM = 0,
    KPOSIX = 1,
};
class ShmImp {
 public:
    virtual void* ShmOpen(const char* name) = 0;
    virtual void* ShmCreate(const char* name, int32_t size, bool authentication = false) = 0;
    virtual void ShmAddPeerId(const char* name, const uint32_t peer_id) = 0;
    virtual void* ShmMMap(const int& size, const int& fd) = 0;
    virtual void ShmUnMap(void* shm_ptr, int32_t size) = 0;
    virtual void ShmClose(const char* name) = 0;
    virtual ~ShmImp() = default;
    // Use queue ID as the primary key for the operation queue
    virtual void* ShmOpen(uint32_t queueId) = 0;
    virtual void* ShmOpen(const char* name, uint32_t& queue_id) = 0;
    virtual void* ShmCreate(const char* name, int32_t size, uint32_t& queue_id, bool authentication = false) = 0;
    virtual void ShmClose(uint32_t queue_id) = 0;
};

class ShmUniMem : public ShmImp {
 public:
    void* ShmOpen(const char* name) override;
    void* ShmCreate(const char* name, int32_t size, bool authentication = false) override;
    void ShmAddPeerId(const char* name, const uint32_t peer_id) override;
    void ShmUnMap(void* shm_ptr, int32_t size) override;
    void ShmClose(const char* name) override;
    void* ShmMMap(const int& size, const int& fd) override;
    static int32_t shmGetDevFd();
    void* ShmOpen(uint32_t queueId) override;
    void* ShmOpen(const char* name, uint32_t& queue_id) override;
    void* ShmCreate(const char* name, int32_t size, uint32_t& queue_id, bool authentication = false) override;
    void ShmClose(uint32_t queue_id) override;

 private:
    std::unordered_map<std::string, int> memory_name_map_;
    std::unordered_map<uint32_t, int> memory_fd_map_;
};
}  // namespace shm
#endif
