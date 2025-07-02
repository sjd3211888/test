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

#include "shmmq/shm_manager.h"
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#include "DsfLog.hpp"
#include "shm_heap_wrapper.h"
#include "common/types/TypesBase.h"

namespace shm {

void* ShmUniMem::ShmOpen(const char* name) {
    int fd = shm_buf_open(name, O_RDWR, 0644U, 0U);
    if (fd < 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_OPEN_ERR,
                      "get shm failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name);
        return nullptr;
    }

    /// fstat in unimem pagesize aglign
    struct stat file_attr;
    if (fstat(fd, &file_attr) < 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_FSTAT_ERR,
                      "fstat failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name);
        static_cast<void>(shm_buf_close(fd));
        return nullptr;
    }

    DSF_LOG_DEBUG(shmmq, "pid:" << getpid() << " open only shm done!");
    void* ret = ShmMMap(file_attr.st_size, fd);
    if (ret != nullptr) {
        static_cast<void>(memory_name_map_.insert(std::make_pair(name, fd)));
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_MMAP_ERR,
                      "ShmMMap failed:" << strerror(errno) << ",fd:" << fd << ",file_attr.st_size:" << file_attr.st_size
                                        << ",shm name:" << name);
    }
    return ret;
}

void* ShmUniMem::ShmCreate(const char* name, int32_t size, bool authentication) {
    int flags;
    if (authentication) {
        flags = O_CREAT | O_RDWR | UNIMEM_EXPORT_AUTHENTICATION_FLAG;
    } else {
        flags = O_CREAT | O_RDWR;
    }
    int fd = shm_buf_open(name, flags, 0644U, static_cast<uint64_t>(size));
    if (fd < 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_CREATE_ERR,
                      "create shm failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name << ",size:"
                                           << size << ",authentication:" << std::boolalpha << authentication);
        return nullptr;
    }

    void* ret = ShmMMap(size, fd);
    if (ret != nullptr) {
        static_cast<void>(memory_name_map_.insert(std::make_pair(name, fd)));
    } else {
        DSF_LOG_ERROR(
            DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_MMAP_ERR,
            "ShmMMap failed:" << strerror(errno) << ",fd:" << fd << ",the shm name is:" << name << ",size:" << size);
    }
    return ret;
}

void* ShmUniMem::ShmMMap(const int& size, const int& fd) {
    // attach p_managed_shm
    void* shm_ptr = mmap(nullptr, static_cast<uint64_t>(size), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == static_cast<void*>(MAP_FAILED)) {
        DSF_LOG_ERROR(
            DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_MMAP_ERR,
            "pid: " << getpid() << " attach shm failed:" << strerror(errno) << ",fd:" << fd << ",size:" << size);
        static_cast<void>(shm_buf_close(fd));
        return nullptr;
    }

    DSF_LOG_DEBUG(shmmq, "pid: " << getpid() << " mmap shm done!");
    return shm_ptr;
}

void ShmUniMem::ShmUnMap(void* shm_ptr, int32_t size) {
    if ((shm_ptr == nullptr) || (size <= 0)) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "ShmUnMap error, wrong param!, size: " << size);
        return;
    }

    if (munmap(shm_ptr, static_cast<uint64_t>(size)) < 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_UNMAP_ERR,
                      "ShmUnMap munmap error:" << strerror(errno) << ",shm_ptr:" << shm_ptr << ",size:" << size);
        return;
    }

    DSF_LOG_DEBUG(shmmq, "SHMUnMap done!");
    return;
}

void ShmUniMem::ShmClose(const char* name) {
    std::string str_name = name;
    if (memory_name_map_.find(str_name) == memory_name_map_.end()) {
        DSF_LOG_WARNING(shmmq, "can not find shm name in memory_name_map_, shm name: " << str_name);
        return;
    }
    int ret = shm_buf_close(memory_name_map_[str_name]);
    if (0 != ret) {
        DSF_LOG_ERROR(
            DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_CLOSE_ERR,
            "ShmClose failed fd is: " << memory_name_map_[str_name] << ",error is:" << ret << ",shm name:" << name);
    }
    static_cast<void>(memory_name_map_.erase(str_name));
    DSF_LOG_DEBUG(shmmq, "ShmClose name is " << str_name);
}

void ShmUniMem::ShmAddPeerId(const char* name, const uint32_t peer_id) {
    uint32_t uuid = 0;
    int fd = -1;
    auto it = memory_name_map_.find(name);
    if (it == memory_name_map_.end()) {
        DSF_LOG_ERROR(shmmq, vbsutil::elog::decToHex(queue_ret_t::RETCODE_SHMMQ_ERROR)
                                 << " " << "find in memory_name_map_ failed" << ".queue_name:" << name
                                 << ",peer_id:" << peer_id);
    } else {
        fd = it->second;
    }
    int ret = shm_buf_add_peer_pid(fd, peer_id, &uuid);
    DSF_LOG_INFO(shmmq, "Shmmq AddPeerId " << ((ret == 0) ? "success" : "false") << ",queue_name:" << name
                                           << ",peer_id:" << peer_id << ",return uuid:" << uuid);
}

int32_t ShmUniMem::shmGetDevFd() {
    return shm_heap_fd();
}

void* ShmUniMem::ShmOpen(uint32_t queueId) {
    int fd = -1;
    int ret = shm_buf_open_with_uuid(queueId, &fd);
    if (ret < 0 || fd < 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, li::dsfdds::ReturnCode_t::RETCODE_MEMBUF_SHM_OPEN_ERR,
                      "open queue error,ret:" << ret << ",queue id:" << queueId << ",fd:" << fd);
        return nullptr;
    }
    if (ret == AUTHENTICATION_FAILED) {
        DSF_LOG_INFO(ShmUniMem,
                     "open queue authentication failed,ret:" << ret << ",queue id:" << queueId << ",fd:" << fd);
    }
    auto* mapAddress = shm_reader_mmap(fd);
    if (mapAddress != nullptr) {
        static_cast<void>(memory_fd_map_.insert(std::make_pair(queueId, fd)));
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_MMAP_ERR,
                      "ShmMMap failed:" << strerror(errno) << ",fd:" << fd << ",queue id:" << queueId);
        return nullptr;
    }
    // TODO(query name)
    return mapAddress;
}

void* ShmUniMem::ShmOpen(const char* name, uint32_t& queue_id) {
    auto shmPtr = ShmUniMem::ShmOpen(name);
    int fd = -1;
    auto it = memory_name_map_.find(name);
    if (it == memory_name_map_.end()) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_OPEN_ERR,
                      "find in memory_name_map_ failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name
                                                         << ",queue id:" << queue_id);
    } else {
        fd = it->second;
    }

    auto ret_uid = shm_get_uuid(fd, &queue_id);
    if (ret_uid != 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_OPEN_ERR,
                      "shm_get_uuid failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name
                                             << ",ret:" << ret_uid << ",queue id:" << queue_id);
        return nullptr;
    }

    DSF_LOG_DEBUG(
        shmmq, "ShmOpen success, queue name" << name << ",fd:" << fd << ",ret:" << ret_uid << ",queue id:" << queue_id);
    return shmPtr;
}

void* ShmUniMem::ShmCreate(const char* name, int32_t size, uint32_t& queue_id, bool authentication) {
    auto shmPtr = ShmCreate(name, size, authentication);
    if (shmPtr == nullptr) {
        return nullptr;
    }

    int fd = -1;
    auto it = memory_name_map_.find(name);
    if (it == memory_name_map_.end()) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_CREATE_ERR,
                      "find in memory_name_map_ failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name
                                                         << ",queue id:" << queue_id);
        return nullptr;
    } else {
        fd = it->second;
    }

    auto ret_uid = shm_get_uuid(fd, &queue_id);
    if (ret_uid != 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_CREATE_ERR,
                      "shm_get_uuid failed:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name
                                             << ",ret:" << ret_uid << ",queue id:" << queue_id);
        return nullptr;
    }

    DSF_LOG_DEBUG(shmmq, "shm_get_uuid success:" << strerror(errno) << ",fd:" << fd << ",shm name:" << name
                                                 << ",ret:" << ret_uid << ",queue id:" << queue_id);

    return shmPtr;
}

void ShmUniMem::ShmClose(uint32_t queue_id) {
    auto it = memory_fd_map_.find(queue_id);
    if (it == memory_fd_map_.end()) {
        DSF_LOG_WARNING(shmmq, "can not find queue id:" << queue_id << " in memory_fd_map");
        return;
    }
    int ret = shm_buf_close(it->second);
    if (0 != ret) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SHM_CLOSE_ERR,
                      "ShmClose failed fd is: " << it->second << ",error is:" << ret << ",shm queue id:" << queue_id);
    }
    static_cast<void>(memory_fd_map_.erase(it));
    DSF_LOG_DEBUG(shmmq, "ShmClose queue id is " << queue_id);
}

}  // namespace shm
