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

#include "shmmq/segment.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cstdint>
#include <pthread.h>
#include <atomic>
#include <utility>

#include "DsfLog.hpp"
#include "shmmq/message_queue.h"
#include "utils.h"
#include "shmmq/shmmqMonitor.h"
#ifdef ENABLE_DES
#include "des_api.h"
#endif
#include <unistd.h>
#include <sys/syscall.h>

/*detect hold mutex process crash*/
#define ERR_HOLDMUTEX_CRASH 1

#define SEMAPHORE_LIMIT 2

namespace shmmq {

const int32_t RW_LOCK_FREE = 0;
const int32_t WRITE_EXCLUSIVE = -1;
const int32_t READ_EXCLUSIVE = 1;
const uint32_t RW_TIMEOUT_NS = 100000000U;  // 100 ms
const int32_t NS_PER_SECOND = 1000000000;
const uint32_t MS_PER_SECOND = 1000U;
const uint32_t NS_PER_MS = 1000000U;
const int32_t TV_NSEC_SECOND_THRESHOLD = 1000000000;

static inline int pthread_mutex_lock_private(pthread_mutex_t* ptr_mutex) {
    int ret = pthread_mutex_lock(ptr_mutex);
    if (EOWNERDEAD == ret) {
        DSF_LOG_WARNING(shmmq, "detect mutex owner process crash!");
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
        ret = pthread_mutex_consistent(ptr_mutex);
#else
        ret = 0;
#endif
        if (0 != ret) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION,
                          "pthread_mutex_consistent failed, ret value is: " << ret);
            return queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION;
        }
        DSF_LOG_WARNING(shmmq, "hold mutex crash, continue");
        return ERR_HOLDMUTEX_CRASH;
    } else if (0 != ret) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION,
                      "pthread_mutex_lock unexpectedly failed, ret value is: " << ret);
        return queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION;
    } else {
        // do nothing
    }
    return 0;
}

QueueSegment::QueueSegment(std::string queue_name, uint32_t queue_id) {
    buf_desc_size_ = sizeof(mbuf::BufferDescriptor);
    is_open_only_ = false;
    queue_name_ = std::move(queue_name);
    queue_id_ = queue_id;
    timestamp_create_s = Utils::GetTime_NS() / NS_PER_SECOND;
}

QueueSegment::QueueSegment(std::string queue_name, uint16_t block_size, uint32_t queue_id) {
    buf_desc_size_ = block_size;
    is_open_only_ = false;
    queue_name_ = std::move(queue_name);
    queue_id_ = queue_id;
    timestamp_create_s = Utils::GetTime_NS() / NS_PER_SECOND;
}

QueueSegment::~QueueSegment() {
    std::time_t timestamp_now_s = Utils::GetTime_NS() / NS_PER_SECOND;

    if ((segment_ != nullptr) && (segment_->p_state != nullptr)) {
        segment_->p_state->alive_time = static_cast<uint64_t>((timestamp_now_s - timestamp_create_s) / NS_PER_SECOND);
    }

    if ((segment_ != nullptr) && (segment_->p_managed_shm != nullptr)) {
        shm_manager_->ShmUnMap(segment_->p_managed_shm, static_cast<int32_t>(segment_->p_config->managed_shm_size));
    }
    if (nullptr != shm_manager_) {
        if (config_.queue_role == READER) {
            shm_manager_->ShmClose(queue_name_.c_str());
        } else {
            if (queue_id_ == 0 && !queue_name_.empty()) {
                shm_manager_->ShmClose(queue_name_.c_str());
            } else {
                shm_manager_->ShmClose(queue_id_);
            }
        }
    }

    if (segment_ != nullptr) {
        delete segment_;
    }

    if (nullptr != shm_manager_) {
        delete shm_manager_;
    }
    DSF_LOG_DEBUG(shmmq, "~QueueSegment.");
}

queue_ret_t QueueSegment::InitSegment(const ConfigStruct& config) {
    SetConfig(config);

    if (shm::KUNIMEM == config.memory_mode) {
        shm_manager_ = new shm::ShmUniMem();
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "Segment memory_mode is invalid, the memory_mode is: " << config.memory_mode);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    queue_ret_t ret = queue_ret_t::RETCODE_SHMMQ_SUCCESS;
    if (config.queue_mode == MODE_MONITOR) {
        ret = InitSegmentOpenOnly();
    } else {
        ret = InitSegmentInternal();
    }
    if (ret == queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        ShmmqProfile::getInstance().Init(segment_->p_profile);
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, ret(), "InitSegmentInternal or InitSegmentOpenOnly failed!, return value:" << ret());
    }
    return ret;
}

queue_ret_t QueueSegment::WriteNoBlock(const void* ptr) {
    if ((segment_ == nullptr) || (ptr == nullptr)
#ifdef ENABLE_DES
        || (GetWakeUpMode() == DES_WAKE_UP)
#endif
    ) {
#ifdef ENABLE_DES
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "WriteNoBlock segment:" << segment_ << ",ptr" << ptr << ",wake up mode:" << GetWakeUpMode()
                                              << " is invalid! ");
#else
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "WriteNoBlock segment or buffer is null! ");
#endif
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }
    // shmmq Segment config
    const uint32_t blk_buf_size = config_.block_buf_size;

    /// multi writer lock
    int ret_write_mutex = pthread_mutex_lock_private(&segment_->p_state->multi_write_mtx);
    if (ret_write_mutex == queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_LOCK_ERR,
                      "Compete mutex failed, ret_write_mutex: " << ret_write_mutex);
        return static_cast<shmmq::queue_ret_t>(ret_write_mutex);
    }

    // writing state
    uint64_t w_idx = atomic_load(&segment_->p_state->write_index);
    const uint32_t try_idx = IndexMod(w_idx, config_.aligned_queue_size);
    // queue not full
    if (!IsQueueFull()) {
        int sem_value = 0;

        void* dst = reinterpret_cast<char*>(segment_->p_buf) + (try_idx * blk_buf_size);
        static_cast<void>(memcpy(dst, ptr, static_cast<uint64_t>(buf_desc_size_)));

// read condition notify
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
        static_cast<void>(atomic_fetch_add(&segment_->p_state->write_index, 1U));
#else
        static_cast<void>(segment_->p_state->write_index.fetch_add(1U));
#endif
        auto sem_getvalue_ret = sem_getvalue(&segment_->p_state->sem_cond, &sem_value);
        if (sem_getvalue_ret != 0) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                          "WriteNoBlock sem_getvalue return value:" << sem_getvalue_ret << ", errno: " << errno);
        }

        if (sem_value < SEMAPHORE_LIMIT) {
            auto sem_post_ret = sem_post(&segment_->p_state->sem_cond);
            if (sem_post_ret != 0) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                              "WriteNoBlock sem_post return value:" << sem_post_ret << ", errno: " << errno);
            }
        }

        // update enqueue profile
        if (segment_->p_profile->enable) {
            ShmmqProfile::getInstance().OnEnqueued();
        }

        // unlock multi-write lock.
        static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));

        return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
    }

    static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));

    return queue_ret_t::RETCODE_SHMMQ_QUEUE_FULL;
}

void QueueSegment::AddPeerId(const uint32_t peer_id) {
    return shm_manager_->ShmAddPeerId(queue_name_.c_str(), peer_id);
}

queue_ret_t QueueSegment::WaitMessage(uint32_t timeout_ms) {
#ifdef ENABLE_DES
    if (GetWakeUpMode() == DES_WAKE_UP) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "WaitMessage API will not be supported in Des,timeout ms:" << timeout_ms << ",wake up mode:"
                                                                                 << GetWakeUpMode());
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    } else if (GetWakeUpMode() == PTHREAD_CONDITION_WAKE_UP) {
#endif
        if (IsQueueEmpty()) {
            struct timespec ts;
            static_cast<void>(clock_gettime(CLOCK_REALTIME, &ts));
            ts.tv_sec += timeout_ms / MS_PER_SECOND;
            ts.tv_nsec += static_cast<uint64_t>(timeout_ms % MS_PER_SECOND) * NS_PER_MS;
            if (ts.tv_nsec >= TV_NSEC_SECOND_THRESHOLD) {
                ts.tv_sec += 1;
                ts.tv_nsec -= TV_NSEC_SECOND_THRESHOLD;
            }

            // reader wait writer
            int ret = 0;
            while (atomic_load(&segment_->p_state->read_index) >= atomic_load(&segment_->p_state->write_index)) {
                if (timeout_ms > 0U) {
                    ret = sem_timedwait(&segment_->p_state->sem_cond, &ts);
                    if (ret != 0) {
                        if (errno == ETIMEDOUT) {
                            return Utils::ToShmmqErr(errno);
                        }

                        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                                      "sem_timedwait failed, ret:" << ret << ", errno: " << errno);
                        return Utils::ToShmmqErr(errno);
                    }
                } else {
                    // wait forever
                    ret = sem_wait(&segment_->p_state->sem_cond);
                    if (ret != 0) {
                        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                                      "sem_wait failed, ret:" << ret << ", errno: " << errno);
                        return Utils::ToShmmqErr(errno);
                    }
                }
            }
        }
        return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
#ifdef ENABLE_DES
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "unknown wake up mode:" << GetWakeUpMode());
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }
#endif
}

queue_ret_t QueueSegment::ReadNoPop(void* dst, int32_t timeout_ms) {
    if ((nullptr == dst)
#ifdef ENABLE_DES
        || (GetWakeUpMode() == DES_WAKE_UP)
#endif
    ) {
#ifdef ENABLE_DES
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "ReadNoPop segment:" << segment_ << ",dst" << dst << ",wake up mode:" << GetWakeUpMode()
                                           << ",timeout_ms:" << timeout_ms << " is invalid! ");
#else
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "ReadNoDequeue segment or buffer is null! ");
#endif
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }
    // shmmq segment
    const uint32_t blk_buf_size = config_.block_buf_size;
    // 1. if queue empty ,waiting.
    if (IsQueueEmpty()) {
        // queue empty ,timeout waiting on seg point
        if (timeout_ms < 0) {
            return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
        } else {
            if (WaitMessage(timeout_ms) != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
                return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
            }
        }
    }

    // 2.read-write confliction
    uint64_t try_reader_idx;
    queue_ret_t rw_ret = RWCompetition(try_reader_idx);
    if (rw_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_ERROR(DSF_SHMMQ, rw_ret(), "RWCompetition failed");
        return rw_ret;
    }
    // ref to : https://li.feishu.cn/wiki/TRmGwPe5qiWl9ykfr7Ac0TUTnHh#Y3LSdxEfKoDTg7xoqiscauDrnrd
    if (IsQueueEmpty()) {
        static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_reader_idx], 1));
        return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
    }

    // reading BufferDescriptor from shm
    void* src = reinterpret_cast<char*>(segment_->p_buf) + (try_reader_idx * blk_buf_size);
    static_cast<void>(memcpy(dst, src, static_cast<uint64_t>(blk_buf_size)));
    // release p_block
    static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_reader_idx], 1));
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::Pop() {
    // 1. if queue empty ,waiting.
    if (IsQueueEmpty()) {
        return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
    }
    // 2.read-write confliction
    uint64_t try_reader_idx;
    queue_ret_t rw_ret = RWCompetition(try_reader_idx);
    if (rw_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_ERROR(DSF_SHMMQ, rw_ret(), "RWCompetition failed");
        return rw_ret;
    }
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_add(&segment_->p_state->read_index, 1U));
#else
    static_cast<void>(segment_->p_state->read_index.fetch_add(1U));
#endif
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_reader_idx], 1U));
#else
    static_cast<void>(segment_->p_block[try_reader_idx].fetch_sub(1U));
#endif
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::WriteNoBlock(const mbuf::BufferDescriptor& desc) {
    if (segment_ == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "WriteNoBlock segment or buffer is null! ");
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    // shmmq Segment config
    const uint32_t blk_buf_size = config_.block_buf_size;

    /// multi writer lock
    int ret_write_mutex = pthread_mutex_lock_private(&segment_->p_state->multi_write_mtx);
    if (ret_write_mutex == queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_LOCK_ERR,
                      "Compete mutex failed, ret_write_mutex: " << ret_write_mutex);
        return static_cast<shmmq::queue_ret_t>(ret_write_mutex);
    }

    // writing state
    uint64_t w_idx = atomic_load(&segment_->p_state->write_index);
    const uint32_t try_idx = IndexMod(w_idx, config_.aligned_queue_size);
    // queue not full
    if (!IsQueueFull()) {
#ifdef ENABLE_DES
        uint8_t sem_value = 0;
#else
        int sem_value = 0;
#endif

        void* dst = reinterpret_cast<char*>(segment_->p_buf) + (try_idx * blk_buf_size);
        static_cast<void>(memcpy(dst, &desc, buf_desc_size_));

        // read condition notify
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
        static_cast<void>(atomic_fetch_add(&segment_->p_state->write_index, 1U));
#else
        static_cast<void>(segment_->p_state->write_index.fetch_add(1U));
#endif
#ifdef ENABLE_DES
        sem_value = atomic_load(&segment_->p_state->event_cond);
#else
        auto sem_getvalue_ret = sem_getvalue(&segment_->p_state->sem_cond, &sem_value);
        if (sem_getvalue_ret != 0) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                          "WriteNoBlock sem_getvalue ret value:" << sem_getvalue_ret << ", errno: " << errno);
        }
#endif
        if (sem_value < SEMAPHORE_LIMIT) {
#ifdef ENABLE_DES
            Des::DesEvent event {GetQueueId(), 0U};
            auto sem_post_ret = Des::SendEvent(event);
            static_cast<void>(atomic_fetch_add(&segment_->p_state->event_cond, 1U));
#else
            auto sem_post_ret = sem_post(&segment_->p_state->sem_cond);
#endif
            if (sem_post_ret != 0) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                              "WriteNoBlock event post ret value:" << sem_post_ret << ", errno: " << errno);
            }
        }

        // update enqueue profile
        if (segment_->p_profile->enable) {
            ShmmqProfile::getInstance().OnEnqueued();
        }

        // enable or disable shmmq real time monitor info log
        if (segment_->p_profile->enable_dfx) {
            std::stringstream ss;
            ss << "role: WRITER, ";
            ShmmqMonitor::GetInstance().PrintShmmqinfo(queue_name_, GetSegmentInfo(), ss);
            ss << ", before sem post sem_value:" << sem_value;
#ifdef ENABLE_DES
            auto sem_getvalue_ret_dfx = atomic_load(&segment_->p_state->event_cond);
#else
            auto sem_getvalue_ret_dfx = sem_getvalue(&segment_->p_state->sem_cond, &sem_value);
#endif
            if (sem_getvalue_ret_dfx != 0) {
                DSF_LOG_ERROR(
                    DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                    "WriteNoBlock sem_getvalue return value:" << sem_getvalue_ret_dfx << ", errno: " << errno);
            }
            ss << ", after sem post sem_value:" << sem_value;
            DSF_LOG_INFO(shmmq, "desc info: " << desc << ",  shmmq info: " << ss.str());
        }

        UpdateIndexStatisticInfo(desc);

        DSF_LOG_DEBUG(shmmq, "WriteNoBlock, desc:" << desc << ",w_idx:" << segment_->p_state->write_index
                                                   << ",r_idx:" << segment_->p_state->read_index
                                                   << ",queue id:" << queue_id_ << ",queue name:" << queue_name_);

        // unlock multi-write lock.
        static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));

        return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
    }

    static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));

    return queue_ret_t::RETCODE_SHMMQ_QUEUE_FULL;
}

queue_ret_t QueueSegment::OverWriteNoBlock(const mbuf::BufferDescriptor& desc) {
    if (segment_ == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "OverWriteNoBlock segment or buffer is null! ");
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }
    return EnqueueData(desc);
}

queue_ret_t QueueSegment::ReadBlock(mbuf::BufferDescriptor& desc, uint32_t timeout_ms) {
    // shmmq segment
    const uint32_t blk_buf_size = config_.block_buf_size;

    // 1. if queue empty ,waiting.
    if (IsQueueEmpty()) {
        // queue empty ,timeout waiting on seg point
        if (WaitMessage(timeout_ms) != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
            return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
        }
    }

    // 2.read-write confliction
    uint64_t try_reader_idx;
    queue_ret_t rw_ret = RWCompetition(try_reader_idx);
    if (rw_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_ERROR(DSF_SHMMQ, rw_ret(), "RWCompetition failed");
        return rw_ret;
    }

    // 3.start reading
    // reading BufferDescriptor from shm
    void* src = reinterpret_cast<char*>(segment_->p_buf) + (try_reader_idx * blk_buf_size);

    mbuf::BufferDescriptor* desc_src = reinterpret_cast<mbuf::BufferDescriptor*>(src);
    desc = *desc_src;
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_add(&segment_->p_state->read_index, 1U));
#else
    static_cast<void>(segment_->p_state->read_index.fetch_add(1U));
#endif
// release p_block
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_reader_idx], 1U));
#else
    static_cast<void>(segment_->p_block[try_reader_idx].fetch_sub(1U));
#endif
    // update dequeue profile
    if (segment_->p_profile->enable) {
        ShmmqProfile::getInstance().OnDequeued(desc_src);
    }

    // enable or disable shmmq real time monitor info log
    if (segment_->p_profile->enable_dfx) {
        std::stringstream ss;
        ss << "role: READER, ";
        ShmmqMonitor::GetInstance().PrintShmmqinfo(queue_name_, GetSegmentInfo(), ss);
        DSF_LOG_INFO(shmmq, "desc info: " << desc << ",  shmmq info: " << ss.str());
    }

    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::ReadNoBlock(mbuf::BufferDescriptor& desc) {
    // shmmq segment
    const uint32_t blk_buf_size = config_.block_buf_size;

    // 1. if queue empty , return.
    if (IsQueueEmpty()) {
#ifdef ENABLE_DES
        //改动原因参见：https://li.feishu.cn/docx/CCCNdUjdmoHJ5Vxdid3cYac5ndg#JY2qdr3shoXkPZxT5bbcQQednrd
        if (atomic_load(&segment_->p_state->event_cond) >= 1U) {
            static_cast<void>(atomic_fetch_sub(&segment_->p_state->event_cond, 1U));
        }
#endif
        return queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY;
    }

    // 2.read-write confliction
    uint64_t try_reader_idx;
    queue_ret_t rw_ret = RWCompetition(try_reader_idx);
    if (rw_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_ERROR(DSF_SHMMQ, rw_ret(), "RWCompetition failed");
        return rw_ret;
    }

    // 3.start reading
    // reading BufferDescriptor from shm
    void* src = reinterpret_cast<char*>(segment_->p_buf) + (try_reader_idx * blk_buf_size);
    mbuf::BufferDescriptor* desc_src = reinterpret_cast<mbuf::BufferDescriptor*>(src);
    desc = *desc_src;
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_add(&segment_->p_state->read_index, 1U));
#else
    static_cast<void>(segment_->p_state->read_index.fetch_add(1U));
#endif
// release p_block
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_reader_idx], 1U));
#else
    static_cast<void>(segment_->p_block[try_reader_idx].fetch_sub(1U));
#endif
    // update dequeue profile
    if (segment_->p_profile->enable) {
        ShmmqProfile::getInstance().OnDequeued(desc_src);
    }

    // enable or disable shmmq real time monitor info log
    if (segment_->p_profile->enable_dfx) {
        std::stringstream ss;
        ss << "role: READER, ";
        ShmmqMonitor::GetInstance().PrintShmmqinfo(queue_name_, GetSegmentInfo(), ss);
        DSF_LOG_INFO(shmmq, "desc info: " << desc << ",  shmmq info: " << ss.str());
    }

    DSF_LOG_DEBUG(shmmq, "ReadNoBlock, desc:" << desc << ",w_idx:" << segment_->p_state->write_index
                                              << ",r_idx:" << segment_->p_state->read_index << ",queue id:" << queue_id_
                                              << ",queue name:" << queue_name_);
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::EnqueueData(const mbuf::BufferDescriptor& desc) const {
    queue_ret_t ret = queue_ret_t::RETCODE_SHMMQ_SUCCESS;
    // shmmq Segment config
    const uint32_t blk_buf_size = config_.block_buf_size;

    // writing lock
    int ret_write_mutex = pthread_mutex_lock_private(&segment_->p_state->multi_write_mtx);
    if (ret_write_mutex == queue_ret_t::RETCODE_SHMMQ_MUTEX_EXCEPTION) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_LOCK_ERR,
                      "Compete mutex failed, ret_write_mutex: " << ret_write_mutex);
        return static_cast<shmmq::queue_ret_t>(ret_write_mutex);
    }
    uint64_t w_idx = atomic_load(&segment_->p_state->write_index);
    const uint32_t try_idx = IndexMod(w_idx, config_.aligned_queue_size);
    // if crash writer process hold p_block, then alive or restart
    // writer process need reset p_block;
    // potential problem is that if just one writer process do not
    // restart,then reader process can not get p_block lead to reader
    // can not read data in queue full
    if (ERR_HOLDMUTEX_CRASH == ret_write_mutex) {
        if (WRITE_EXCLUSIVE == atomic_load(&segment_->p_block[try_idx])) {
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
            static_cast<void>(atomic_fetch_add(&segment_->p_block[try_idx], 1));
#else
            static_cast<void>(segment_->p_block[try_idx].fetch_add(1U));
#endif
        }
    }
    // queue full
    if (IsQueueFull()) {
        ret = HandleQueueFull(desc, blk_buf_size, try_idx);
        if (ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
            return ret;
        }

    } else {
        void* dst = reinterpret_cast<char*>(segment_->p_buf) + (try_idx * blk_buf_size);
        static_cast<void>(memcpy(dst, &desc, buf_desc_size_));
    }

// read condition notify
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_add(&segment_->p_state->write_index, 1U));
#else
    static_cast<void>(segment_->p_state->write_index.fetch_add(1U));
#endif
#ifdef ENABLE_DES
    auto sem_value = atomic_load(&segment_->p_state->event_cond);
#else
    int sem_value = 0;
    auto sem_getvalue_ret = sem_getvalue(&segment_->p_state->sem_cond, &sem_value);
    if (sem_getvalue_ret != 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                      "OverWriteNoBlock sem_getvalue return value:" << sem_getvalue_ret << ", errno: " << errno);
    }
#endif
    if (sem_value < SEMAPHORE_LIMIT) {
#ifdef ENABLE_DES
        Des::DesEvent event {GetQueueId(), 0};
        auto sem_post_ret = Des::SendEvent(event);
        static_cast<void>(atomic_fetch_add(&segment_->p_state->event_cond, 1U));
#else
        auto sem_post_ret = sem_post(&segment_->p_state->sem_cond);
#endif
        if (sem_post_ret != 0) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                          "OverWriteNoBlock event post return value:" << sem_post_ret << ", errno: " << errno);
        }
    }
    // update enqueue profile
    if (segment_->p_profile->enable) {
        ShmmqProfile::getInstance().OnEnqueued();
    }

    // enable or disable shmmq real time monitor info log
    if (segment_->p_profile->enable_dfx) {
        std::stringstream ss;
        ss << "role: WRITER, ";
        ShmmqMonitor::GetInstance().PrintShmmqinfo(queue_name_, GetSegmentInfo(), ss);
        ss << ", before sem post sem_value:" << sem_value;
#ifdef ENABLE_DES
        auto sem_getvalue_ret_dfx = atomic_load(&segment_->p_state->event_cond);
#else
        auto sem_getvalue_ret_dfx = sem_getvalue(&segment_->p_state->sem_cond, &sem_value);
#endif
        if (sem_getvalue_ret_dfx != 0) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                          "WriteNoBlock sem_getvalue return value:" << sem_getvalue_ret_dfx << ", errno: " << errno);
        }
        ss << ", after sem post sem_value:" << sem_value;
        DSF_LOG_INFO(shmmq, "desc info: " << desc << ",  shmmq info: " << ss.str());
    }

    UpdateIndexStatisticInfo(desc);

    // unlock multi-write lock.
    static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));
    return ret;
}

queue_ret_t QueueSegment::HandleQueueFull(const mbuf::BufferDescriptor& desc, const uint32_t& blk_buf_size,
                                          const uint32_t& try_idx) const {
    int32_t rw_lock_free = RW_LOCK_FREE;
    uint64_t start_time_us = static_cast<uint64_t>(Utils::GetTime_NS());
    // if reader hoding the automic lock of current block, then waiting. while
    while (!atomic_compare_exchange_weak_explicit(
        &segment_->p_block[try_idx], &rw_lock_free, WRITE_EXCLUSIVE, std::memory_order_acq_rel,
        std::memory_order_relaxed)) {
        // DSF_LOG_WARNING(shmmq, "trying to get write permission."
        //                              << rw_lock_free << " try_idx " <<
        //                              try_idx);
        rw_lock_free = RW_LOCK_FREE;

        uint64_t duration = static_cast<uint64_t>(Utils::GetTime_NS()) - start_time_us;
        // time unit ns
        if (duration > RW_TIMEOUT_NS) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_TIME_OUT, "overwrite timeout: " << duration);
            static_cast<void>(pthread_mutex_unlock(&segment_->p_state->multi_write_mtx));
            return queue_ret_t::RETCODE_SHMMQ_TIME_OUT;
        }
    }

    void* dst = reinterpret_cast<char*>(segment_->p_buf) + (try_idx * blk_buf_size);

    // current block is overwrited, move reader index to next block;
    if ((atomic_load(&segment_->p_state->write_index) - atomic_load(&segment_->p_state->read_index)) ==
        config_.queue_size) {
        DSF_LOG_DEBUG(shmmq, "over write!!!, write index:"
                                 << atomic_load(&segment_->p_state->write_index) << ",write desc:" << desc
                                 << ",read index:" << atomic_load(&segment_->p_state->read_index)
                                 << ",override desc:" << *reinterpret_cast<mbuf::BufferDescriptor*>(dst));
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
        static_cast<void>(atomic_fetch_add(&segment_->p_state->read_index, 1U));
#else
        static_cast<void>(segment_->p_state->read_index.fetch_add(1U));
#endif
    }

    // memcpy after read_index++ prevent restart out of order
    static_cast<void>(memcpy(dst, &desc, buf_desc_size_));
// release p_block
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(atomic_fetch_add(&segment_->p_block[try_idx], 1));
#else
    static_cast<void>(segment_->p_block[try_idx].fetch_add(1U));
#endif
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::RWCompetition(uint64_t& r_index) {
    int32_t rw_lock_free = RW_LOCK_FREE;
    uint64_t start_time_ns = static_cast<uint64_t>(Utils::GetTime_NS());

    uint64_t read_idx_current = atomic_load(&segment_->p_state->read_index);
    uint32_t try_idx = IndexMod(read_idx_current, config_.aligned_queue_size);

    while (read_idx_current <= atomic_load(&segment_->p_state->read_index)) {
        // if writer hoding the automic lock of current block, then waiting.
        while (!atomic_compare_exchange_weak_explicit(&segment_->p_block[try_idx], &rw_lock_free, READ_EXCLUSIVE,
                                                      std::memory_order_acq_rel, std::memory_order_relaxed)) {
            // DSF_LOG_WARNING(shmmq, "trying to get read permission.");
            rw_lock_free = RW_LOCK_FREE;

            uint64_t duration_ns = Utils::GetTime_NS() - start_time_ns;
            // time unit ns
            if (duration_ns > RW_TIMEOUT_NS) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_TIME_OUT, " get lock time out!");
                return queue_ret_t::RETCODE_SHMMQ_TIME_OUT;
            }

            read_idx_current = atomic_load(&segment_->p_state->read_index);
            try_idx = IndexMod(read_idx_current, config_.aligned_queue_size);
        }

        // if read_idx_current equal with read_index , read block;
        // if read_idx_current not equal read_index, Writer has updated read_index
        // in overwrite mode during competition, we need to recompete next block
        // with writer
        if (read_idx_current == atomic_load(&segment_->p_state->read_index)) {
            r_index = try_idx;
            return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
        } else if (read_idx_current < atomic_load(&segment_->p_state->read_index)) {
            DSF_LOG_WARNING(shmmq, "writer moved reader index, re-competition!");
            static_cast<void>(atomic_fetch_sub(&segment_->p_block[try_idx], 1));
            read_idx_current = atomic_load(&segment_->p_state->read_index);
            try_idx = IndexMod(read_idx_current, config_.aligned_queue_size);
        } else {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                          "read_idx_current should never be larger than read_index!");
        }
    }
    return queue_ret_t::RETCODE_SHMMQ_INVALID_READER_INDEX;
}

bool QueueSegment::IsQueueFull() const {
    uint64_t r_idx = atomic_load(&segment_->p_state->read_index);
    uint64_t w_idx = atomic_load(&segment_->p_state->write_index);
    if (r_idx > w_idx) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_QUEUE_FULL_INDEX_ERR,
                      "IsQueueFull index error, r_idx:" << r_idx << ">w_idx:" << w_idx);
    }
    return (static_cast<int64_t>(w_idx - r_idx) >= config_.queue_size) ? true : false;
}

bool QueueSegment::IsQueueEmpty() const {
    uint64_t r_idx = atomic_load(&segment_->p_state->read_index);
    uint64_t w_idx = atomic_load(&segment_->p_state->write_index);
    if (r_idx > w_idx) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY_INDEX_ERR,
                      "IsQueueEmpty index error, r_idx:" << r_idx << ">w_idx:" << w_idx);
    }

    return (r_idx >= w_idx) ? true : false;
}

void QueueSegment::SetConfig(const ConfigStruct& config) {
    config_ = config;

    config_.aligned_queue_size = static_cast<uint32_t>(
        (config.queue_size == 0U) ? BLOCK_NUM_DEFAULT : RoundUp(static_cast<uint64_t>(config.queue_size)));
    if (config.block_buf_size != 0U) {
        buf_desc_size_ = config.block_buf_size;
    }
    config_.block_buf_size = buf_desc_size_;

    config_.memory_mode = config.memory_mode;
    config_.managed_shm_size =
        static_cast<uint32_t>(GetShmSize(config_.aligned_queue_size, config_.block_buf_size, config_.reserved_size));

    DSF_LOG_DEBUG(
        shmmq, "SetConfig: name:" << queue_name_ << " shm size: " << config_.managed_shm_size
                                  << " block buf size:" << config_.block_buf_size
                                  << " block num:" << config_.aligned_queue_size << " queue size:" << config_.queue_size
                                  << " role :" << static_cast<unsigned>(config_.queue_role)
                                  << " mode:" << static_cast<unsigned>(config_.queue_mode)
                                  << " authentication:" << static_cast<unsigned>(config_.queue_authentication)
                                  << " priority:" << static_cast<unsigned>(config_.priority) << " deadline:"
                                  << config_.deadline << " memory mode:" << static_cast<unsigned>(config_.memory_mode)
                                  << ",reserved_size:" << static_cast<unsigned>(config_.reserved_size) << " kB");
}

queue_ret_t QueueSegment::InitSegmentInternal() {
    void* shm_ptr = nullptr;
    ShmInit(&shm_ptr);

    if (shm_ptr == nullptr) {
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    SegmentCreate();

    if (segment_ == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS,
                      "shm segment ptr is nullptr, queue name: " << queue_name_);
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    auto ret = InitState(shm_ptr);
    if (ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        return ret;
    }

    ret = InitProfile(shm_ptr);
    if (ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        return ret;
    }

    ret = InitBlocks(shm_ptr);
    if (ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        return ret;
    }

    // get field p_buf
    segment_->p_buf = GetShmBufAddr(shm_ptr, segment_->p_config->aligned_queue_size);
    if (segment_->p_buf == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm p_buf ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    // get field p_reserved
    segment_->p_reserved =
        GetShmReservedAddr(shm_ptr, segment_->p_config->block_buf_size, segment_->p_config->aligned_queue_size);
    if (segment_->p_reserved == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm p_reserved ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::InitBlocks(void* shm_ptr) {  // get field p_block
    std::atomic_int* blocks = GetShmAtomicBlockAddr(shm_ptr);
    if (blocks == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm blocks ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_block = blocks;
    // init field p_block
    if (is_open_only_) {
        if (config_.queue_role == READER) {
            // restart reset p_block state
            // potential problem is that if just reader process do not
            // restart,then writer process can not get p_block lead to writer
            // can not write data in queue full
            for (uint32_t i = 0U; i < segment_->p_config->aligned_queue_size; ++i) {
                if (READ_EXCLUSIVE == atomic_load(&segment_->p_block[i])) {
                    static_cast<void>(atomic_fetch_sub(&segment_->p_block[i], 1));
                }
            }
        }
    } else {
        for (uint32_t i = 0U; i < segment_->p_config->aligned_queue_size; ++i) {
            std::atomic_init(&blocks[i], 0);
        }
    }
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::InitProfile(void* shm_ptr) {
    // get field p_profile
    segment_->p_profile = GetShmProfileAddr(shm_ptr);
    if (segment_->p_profile == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm profile ptr is null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    if (!is_open_only_) {
        InitShmProfile(segment_->p_profile);
    }
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::InitState(void* shm_ptr) {
    segment_->p_managed_shm = shm_ptr;
    // get field p_config
    ConfigStruct* config_ptr = GetShmConfigAddr(shm_ptr);

    if (config_ptr == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm config ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_config = config_ptr;
    // init config
    queue_ret_t conf_ret = InitShmConfig(segment_->p_config, is_open_only_);
    if (conf_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "InitShmConfig failed.");
        return conf_ret;
    }

    // create field p_state
    StateStruct* state_ptr = GetShmStateAddr(shm_ptr);

    if (state_ptr == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm state ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_state = state_ptr;
    /// init segment;
    if (!is_open_only_) {
        queue_ret_t state_ret = InitShmState(state_ptr);
        if (state_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "InitShmState failed.");
            return state_ret;
        }
    }
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

void QueueSegment::SegmentCreate() {
    segment_ = new (std::nothrow) SegmentStruct();
    if (segment_ == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_PUBSUB_CREATE_OBJECT_ERR,
                      "create Segment Failed, the queue name is:" << queue_name_);
    }
}

void QueueSegment::ShmInit(void** shm_ptr) {
    if (config_.queue_role == READER) {
        *shm_ptr = shm_manager_->ShmCreate(queue_name_.c_str(), static_cast<int32_t>(config_.managed_shm_size),
                                           queue_id_, (config_.queue_authentication == 0U ? false : true));
        if (*shm_ptr == nullptr) {
            *shm_ptr = shm_manager_->ShmOpen(queue_name_.c_str(), queue_id_);
            is_open_only_ = true;
        } else {
            is_open_only_ = false;
        }
    } else if (config_.queue_role == WRITER) {
        if (queue_id_ == 0 && !queue_name_.empty()) {
            // 针对服务发现的公共队列，cli队列等还是以name打开
            *shm_ptr = shm_manager_->ShmOpen(queue_name_.c_str());
        } else {
            *shm_ptr = shm_manager_->ShmOpen(queue_id_);
        }
        is_open_only_ = true;
    } else {
        // do nothing
    }
}

queue_ret_t QueueSegment::InitSegmentOpenOnly() {
    void* shm_ptr = shm_manager_->ShmOpen(queue_name_.c_str());
    if (shm_ptr == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    SegmentCreate();

    if (segment_ == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS,
                      "shm segment ptr is nullptr, the queue name is: " << queue_name_);
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    segment_->p_managed_shm = shm_ptr;
    // get field p_config
    ConfigStruct* config_ptr = GetShmConfigAddr(shm_ptr);

    if (config_ptr == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm config ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_config = config_ptr;

    // create field p_state
    StateStruct* state_ptr = GetShmStateAddr(shm_ptr);

    if (state_ptr == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm state ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_state = state_ptr;

    // get field p_profile
    segment_->p_profile = GetShmProfileAddr(shm_ptr);
    if (segment_->p_profile == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm profile ptr is null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }

    InitShmProfile(segment_->p_profile);

    // get field p_block
    std::atomic_int* blocks = GetShmAtomicBlockAddr(shm_ptr);
    if (blocks == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm blocks ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    segment_->p_block = blocks;
    // init field p_block
    for (uint32_t i = 0U; i < segment_->p_config->aligned_queue_size; ++i) {
        std::atomic_init(&blocks[i], 0);
    }

    // get field p_buf
    segment_->p_buf = GetShmBufAddr(shm_ptr, segment_->p_config->aligned_queue_size);
    if (segment_->p_buf == nullptr) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS, "shm p_buf ptr null!");
        return queue_ret_t::RETCODE_SHMMQ_BAD_ADDRESS;
    }
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::InitShmConfig(ConfigStruct* const shm_config, bool open_only) {
    if (!open_only) {
        shm_config->managed_shm_size = config_.managed_shm_size;
        shm_config->block_buf_size = config_.block_buf_size;
        shm_config->queue_size = config_.queue_size;
        shm_config->aligned_queue_size = config_.aligned_queue_size;
        shm_config->queue_mode = config_.queue_mode;
        shm_config->priority = config_.priority;
        shm_config->deadline = config_.deadline;
        shm_config->memory_mode = config_.memory_mode;
        shm_config->reserved_size = config_.reserved_size;
        shm_config->queue_authentication = config_.queue_authentication;
#ifdef ENABLE_DES
        shm_config->wake_up_mode = config_.wake_up_mode;
#endif
    } else {
        if (queue_id_ != 0 && config_.queue_role == WRITER) {
            SetConfigFromSegment();
        } else {
            if (!CompareShmConfig()) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                              "Reader attribute not match with config stored in shm"
                                  << ", queue_size:" << config_.queue_size << ", shm queue_size:"
                                  << shm_config->queue_size << ", aligned_queue_size:" << config_.aligned_queue_size
                                  << ", shm aligned_queue_size:" << shm_config->aligned_queue_size
                                  << ", queue_mode: " << static_cast<unsigned>(config_.queue_mode)
                                  << ", shm queue_mode: " << static_cast<unsigned>(shm_config->queue_mode)
                                  << ", authentication: " << static_cast<unsigned>(config_.queue_authentication)
                                  << ", shm authentication: " << static_cast<unsigned>(shm_config->queue_authentication)
                                  << ", priority:" << config_.priority << ", shm priority:" << shm_config->priority
                                  << ", deadline:" << config_.deadline << ", shm deadline:" << shm_config->deadline
                                  << ", block_buf_size:" << config_.block_buf_size
                                  << ", shm block_buf_size:" << shm_config->block_buf_size
#ifdef ENABLE_DES
                                  << ", wake_up_mode:" << static_cast<unsigned>(config_.wake_up_mode)
                                  << ", shm wake_up_mode:" << static_cast<unsigned>(shm_config->wake_up_mode)
#endif
                );
                return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
            }
        }
    }

    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

queue_ret_t QueueSegment::InitShmState(StateStruct* state) {
    pthread_mutexattr_t multi_write_mtx_attr;

    // multi-writers state
    int ret_multi_write_mux = 0;
    /* set multi-write mutex shared between processes */
    static_cast<void>(pthread_mutexattr_init(&multi_write_mtx_attr));
    static_cast<void>(pthread_mutexattr_setpshared(&multi_write_mtx_attr, PTHREAD_PROCESS_SHARED));
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(pthread_mutexattr_setrobust(&multi_write_mtx_attr, PTHREAD_MUTEX_ROBUST));
#endif
    ret_multi_write_mux = pthread_mutex_init(&state->multi_write_mtx, &multi_write_mtx_attr);

    if (ret_multi_write_mux != 0) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_MUTEX_LOCK_ERR,
                      "writer pthread retcode: " << strerror(errno));
        static_cast<void>(pthread_mutex_destroy(&state->multi_write_mtx));
        return Utils::ToShmmqErr(ret_multi_write_mux);
    }
#ifdef ENABLE_DES
    if (config_.wake_up_mode == PTHREAD_CONDITION_WAKE_UP) {
#endif
        // reader-writer state
        int ret_sem = 0;
        /* set semaphore shared between processes */
        ret_sem = sem_init(&state->sem_cond, 1, 0U);
        if (ret_sem != 0) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_SEM_OPERATION_ERR,
                          "reader semaphore init failed,return value:" << ret_sem << ",retcode: " << strerror(errno));
            static_cast<void>(sem_destroy(&state->sem_cond));
            return Utils::ToShmmqErr(ret_sem);
        }
        DSF_LOG_DEBUG(shmmq, "init semaphore done");
#ifdef ENABLE_DES
    } else if (config_.wake_up_mode == DES_WAKE_UP) {
        std::atomic_init(&state->event_cond, 0U);
        DSF_LOG_INFO(shmmq, "init event_cond done");
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "shmmq wake up mode:" << config_.wake_up_mode << " invalid!!");
    }
#endif
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    std::atomic_init(&state->write_index, 0U);
    std::atomic_init(&state->read_index, 0U);
#else
    state->write_index = 0U;
    state->read_index = 0U;
#endif
    state->read_pid = getpid();
    state->read_tid = syscall(SYS_gettid);
    state->create_ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                 std::chrono::high_resolution_clock::now().time_since_epoch())
                                                 .count());
    return queue_ret_t::RETCODE_SHMMQ_SUCCESS;
}

bool QueueSegment::CompareShmConfig() const {
    if ((segment_->p_config->queue_size != config_.queue_size) ||
        (segment_->p_config->aligned_queue_size != config_.aligned_queue_size) ||
        (segment_->p_config->queue_mode != config_.queue_mode) || (segment_->p_config->priority != config_.priority) ||
        (segment_->p_config->deadline != config_.deadline) ||
        (segment_->p_config->block_buf_size != config_.block_buf_size) ||
        (segment_->p_config->queue_authentication != config_.queue_authentication)
#ifdef ENABLE_DES
        || (segment_->p_config->wake_up_mode != config_.wake_up_mode)
#endif
    ) {
        return false;
    } else {
        return true;
    }
}

void QueueSegment::SetConfigFromSegment() {
    config_.queue_size = segment_->p_config->queue_size;
    config_.aligned_queue_size = segment_->p_config->aligned_queue_size;
    config_.queue_mode = segment_->p_config->queue_mode;
    config_.priority = segment_->p_config->priority;
    config_.deadline = segment_->p_config->deadline;
    config_.block_buf_size = segment_->p_config->block_buf_size;
    config_.reserved_size = segment_->p_config->reserved_size;
    config_.queue_authentication = segment_->p_config->queue_authentication;
#ifdef ENABLE_DES
    config_.wake_up_mode = segment_->p_config->wake_up_mode;
#endif
}

void QueueSegment::InitShmProfile(ProfileStruct* profile) {
    profile->enable = false;
    profile->msg_sent = 0U;
    profile->msg_received = 0U;
    profile->sum_latency_us = 0U;
    profile->max_latency_us = 0U;
    profile->min_latency_us = 0U;
    profile->enable_dfx = 0U;
}

void QueueSegment::UpdateIndexStatisticInfo(const mbuf::BufferDescriptor& desc) const {
    // statistic diff max and diff max timestamp
    auto diff = atomic_load(&segment_->p_state->write_index) - atomic_load(&segment_->p_state->read_index);
    if (diff > segment_->p_state->max_num_in_queue) {
        segment_->p_state->max_num_in_queue = diff;
        segment_->p_state->max_num_in_queue_ts = desc.timestamp();
    }
}

uint32_t QueueSegment::GetQueueId() const {
    return queue_id_;
}

void QueueSegment::SetQueueId(uint32_t queueId) {
    queue_id_ = queueId;
}

void* QueueSegment::GetReservedPtr() const {
    return segment_->p_reserved;
}

#ifdef ENABLE_DES
uint8_t QueueSegment::GetWakeUpMode() const {
    return config_.wake_up_mode;
}
#endif
}  // namespace shmmq
