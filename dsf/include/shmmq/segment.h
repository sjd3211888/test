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

#ifndef MESSAGE_QUEUE_SEGMENT_H
#define MESSAGE_QUEUE_SEGMENT_H

#include <stdint.h>
#include <semaphore.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include "shmmq/shmmq_profile.h"
#include "shmmq/shm_manager.h"
#include "common/types/TypesBase.h"
#include "common/utils/SequenceNumber.h"
#include "membuf/buffer.h"
#include "membuf/BufferDescriptor.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define _ALIGN_UP(addr) ((((addr) + PAGE_SIZE) - 1) & PAGE_MASK)
#define BLOCK_NUM_DEFAULT 16U

namespace shmmq {

using queue_ret_t = shm::queue_ret_t;
constexpr uint32_t MaxWriteCount = 128U;

typedef struct write_info {
    uint32_t write_pid;
    uint32_t write_tid;
    uint32_t entity_id;                    // writer entityid
    std::atomic_uint64_t send_count {0U};  // 每个writer发包数量
    std::atomic<bool> is_used_flag {false};
} WriteInfoStruct;

typedef struct state {
    std::atomic_uint64_t read_index;
    std::atomic_uint64_t write_index;
#ifdef ENABLE_DES
    union {
        // used in des wake up
        std::atomic_uint32_t event_cond;
        // used in sem wake up
        sem_t sem_cond;
    };
#else
    sem_t sem_cond;
#endif

    pthread_mutex_t multi_write_mtx;

    // statistic
    uint64_t create_ts;            // reader create time
    uint64_t alive_time;           // reader alive time
    uint64_t max_num_in_queue;     // 运行时实时记录write_idx-read_idx最大差值
    uint64_t max_num_in_queue_ts;  // 最大差值时刻的timestamps
    pid_t read_pid;
    pid_t read_tid;
    li::dsfdds::SequenceNumber_t reception_sequence_number;  // 该reader接收的包个数
    uint32_t entityId;                                       // reader entityId
    std::atomic_uint16_t write_count;                        // 与该reader匹配的writer数量
    WriteInfoStruct write_info[MaxWriteCount];               // 与该reader匹配的writer信息
} StateStruct;

typedef struct shm_conf {
    uint8_t queue_role;
    uint8_t queue_mode;
    uint8_t queue_authentication = 0U;
    uint8_t memory_mode;
    // shm的预留大小，方便reader writer间共享某些数据，默认值为0,单位是KB
    uint8_t reserved_size;
    uint32_t queue_size;
    uint32_t aligned_queue_size;
    uint32_t block_buf_size;
    uint32_t managed_shm_size;  // 对齐4K后的size
    uint32_t priority;
    uint64_t deadline;
#ifdef ENABLE_DES
    uint8_t wake_up_mode;
#endif
} ConfigStruct;

typedef struct segment {
    void* p_managed_shm {nullptr};
    struct shm_conf* p_config {nullptr};
    struct state* p_state {nullptr};
    struct profile* p_profile {nullptr};  // shmmq profile
    std::atomic_int* p_block {nullptr};   //  every block has its own lock
    void* p_buf {nullptr};                //  block start address
    void* p_reserved {nullptr};           //  reserved data address
} SegmentStruct;

typedef struct profile {
    bool enable;  // profile start/stop;

    uint64_t msg_sent;        // message sent count since profile start
    uint64_t msg_received;    // message received count since profile start
    uint64_t sum_latency_us;  // total latency time since profile start
    uint32_t max_latency_us;  // max latency time since profile start
    uint32_t min_latency_us;  // min latency time since profile start
    uint32_t enable_dfx;      // shmmq monitor info log start/stop;
} ProfileStruct;

class ShmmqProfile;
class QueueSegment {
 private:
    SegmentStruct* segment_ {nullptr};
    ConfigStruct config_ {};
    std::string queue_name_;
    uint32_t queue_id_;

 public:
    uint32_t GetQueueId() const;
    void SetQueueId(uint32_t queueId);
    void* GetReservedPtr() const;
#ifdef ENABLE_DES
    uint8_t GetWakeUpMode() const;
#endif

 private:
    uint32_t buf_desc_size_;
    bool is_open_only_;
    std::time_t timestamp_create_s;
    shm::ShmImp* shm_manager_ {nullptr};
    void SetConfig(const ConfigStruct& config);
    // init state in shm when create shm at the first time.
#ifdef ENABLE_DES
    queue_ret_t InitShmState(StateStruct* state);
#else
    static queue_ret_t InitShmState(StateStruct* state);
#endif
    // init config in shm，if open_only == true, will compare with config in shm
    queue_ret_t InitShmConfig(ConfigStruct* const shm_config, bool open_only);
    static void InitShmProfile(ProfileStruct* profile);
    queue_ret_t InitSegmentInternal();
    queue_ret_t InitSegmentOpenOnly();

    queue_ret_t RWCompetition(uint64_t& r_index);
    bool CompareShmConfig() const;
    void SetConfigFromSegment();

    static size_t RoundUp(size_t val) {
        if ((val & (val - 1U)) != 0U) {
            val |= val >> 1U;
            val |= val >> 2U;
            val |= val >> 4U;
            val |= val >> 8U;
            val |= val >> 16U;
            return val + 1U;
        } else {
            return (val == 0U) ? 1U : val;
        }
    }

    static uint32_t IndexMod(uint64_t input, uint32_t length) { return static_cast<uint32_t>(input) & (length - 1U); }

    // state 1024 block 1024 p_buf 1024*2,must align with 4K
    static uint64_t GetShmSize(uint32_t queue_size, uint32_t block_buf_size, uint32_t reserved_size) {
        uint32_t shm_size = sizeof(ConfigStruct) + sizeof(StateStruct) + sizeof(ProfileStruct) +
                            (queue_size * sizeof(std::atomic_int)) + (block_buf_size * queue_size) +
                            // the unit of reserved size is KB
                            (reserved_size * 1024U);
        return static_cast<uint64_t>(_ALIGN_UP(shm_size));
    }
    static ConfigStruct* GetShmConfigAddr(void* p_shm) { return reinterpret_cast<ConfigStruct*>(p_shm); }
    static StateStruct* GetShmStateAddr(void* p_shm) {
        return reinterpret_cast<StateStruct*>(reinterpret_cast<char*>(p_shm) + sizeof(ConfigStruct));
    }
    static ProfileStruct* GetShmProfileAddr(void* p_shm) {
        return reinterpret_cast<ProfileStruct*>(reinterpret_cast<char*>(p_shm) + sizeof(ConfigStruct) +
                                                sizeof(StateStruct));
    }
    static std::atomic_int* GetShmAtomicBlockAddr(void* p_shm) {
        return reinterpret_cast<std::atomic_int*>(reinterpret_cast<char*>(p_shm) + sizeof(ConfigStruct) +
                                                  sizeof(StateStruct) + sizeof(ProfileStruct));
    }
    static void* GetShmBufAddr(void* p_shm, uint32_t queue_size) {
        return reinterpret_cast<char*>(reinterpret_cast<char*>(p_shm) + sizeof(ConfigStruct) + sizeof(StateStruct) +
                                       sizeof(ProfileStruct) + (queue_size * sizeof(std::atomic_int)));
    }

    static void* GetShmReservedAddr(void* p_shm, uint32_t block_buf_size, uint32_t queue_size) {
        return reinterpret_cast<char*>(reinterpret_cast<char*>(p_shm) + sizeof(ConfigStruct) + sizeof(StateStruct) +
                                       sizeof(ProfileStruct) + (queue_size * sizeof(std::atomic_int)) +
                                       (block_buf_size * queue_size));
    }

 public:
    explicit QueueSegment(std::string queue_name, uint32_t queue_id);
    explicit QueueSegment(std::string queue_name, uint16_t block_size, uint32_t queue_id);
    ~QueueSegment();

    queue_ret_t InitSegment(const ConfigStruct& config);
    void AddPeerId(const uint32_t peer_id);

    queue_ret_t WaitMessage(uint32_t timeout_ms);
    queue_ret_t WriteNoBlock(const mbuf::BufferDescriptor& desc);
    queue_ret_t OverWriteNoBlock(const mbuf::BufferDescriptor& desc);

    queue_ret_t ReadBlock(mbuf::BufferDescriptor& desc, uint32_t timeout_ms);
    queue_ret_t ReadNoBlock(mbuf::BufferDescriptor& desc);

    queue_ret_t WriteNoBlock(const void* ptr);

    // timeout_ms: >=0 wait, <0 not wait
    queue_ret_t ReadNoPop(void* dst, int32_t timeout_ms);

    queue_ret_t Pop();

    bool IsQueueEmpty() const;
    bool IsQueueFull() const;
    const ConfigStruct GetConfig() const { return config_; }
    SegmentStruct* GetSegmentInfo() const { return segment_; }
    uint32_t Getsize() const { return config_.queue_size; }

 private:
    void ShmInit(void** shm_ptr);
    void SegmentCreate();
    queue_ret_t InitState(void* shm_ptr);
    queue_ret_t InitProfile(void* shm_ptr);
    queue_ret_t InitBlocks(void* shm_ptr);
    queue_ret_t EnqueueData(const mbuf::BufferDescriptor& desc) const;
    queue_ret_t HandleQueueFull(const mbuf::BufferDescriptor& desc, const uint32_t& blk_buf_size,
                                const uint32_t& try_idx) const;
    void UpdateIndexStatisticInfo(const mbuf::BufferDescriptor& desc) const;
};

}  // namespace shmmq

#endif  // MESSAGE_QUEUE_SEGMENT_H
