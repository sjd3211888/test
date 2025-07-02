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

#ifndef SHM_MSG_QUEUE_H
#define SHM_MSG_QUEUE_H
#include <stdint.h>
#include <atomic>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "shmmq/segment.h"
#include "membuf/buffer.h"
#include "DsfLog.hpp"
#ifdef ENABLE_DES
#include <functional>
#endif
#define BUF_NAME_LEN 256
namespace shmmq {

/**
 * @brief queue mode. Writer support overwrite/writing message in no-block
 * mode; Reader block mode can refer to Dequeue()/Dequeue(uint32_t timeout_ms).
 *
 */
enum queue_mode {
    MODE_WRITE,      // Will return when queue is full
    MODE_OVERWRITE,  // Will overwrite when queue is full
    MODE_MONITOR,    // just for debug,will not check attribute, open shm already
                     //  created, monitor statistics of shm.
    MODE_DEFAULT,    // keep same as exited queue mode, only used for writer
    MODE_INVALID     // unknown mode
};

enum block_type {
    MBUF_TYPE,   // block is base on mbuf
    OTHER_TYPE,  // block is data
};
/**
 * @brief queue role
 *
 */
enum queue_role { WRITER, READER, ROLE_INVALID };

#ifdef ENABLE_DES
/**
 * PTHREAD_CONDITION_WAKE_UP actually is sem
 * DES
 */
enum queue_wake_up_mode { PTHREAD_CONDITION_WAKE_UP, DES_WAKE_UP, INVALID_WAKE_UP };

struct TaskCfg {
    uint64_t taskId;
    uint32_t resGroupId;
    uint32_t deadlineTime;
    uint32_t runTime;
    uint32_t schedulePeriod;
};
#endif

/**
 * @brief wait_signal return this struct, signal info from driver
 */
struct signal_info {
    uint64_t pid = 0U;
    std::string name;
    bool is_mbuf_flag = false;
    uint32_t uuid;
};

/**
 * @brief return to pub/sub signal info
 */
struct parse_type {
    uint32_t uuid;
    char name[BUF_NAME_LEN];
};

/**
 * @brief queue attribute
 *
 */
typedef struct queue_attr {
    bool operator!=(const queue_attr& attr) const noexcept {
        return ((queue_size != attr.queue_size) || (queue_mode != attr.queue_mode) || (priority != attr.priority) ||
                (deadline != attr.deadline)
#ifdef ENABLE_DES
                || (queue_wake_up_mode != attr.queue_wake_up_mode)
#endif
        );
    }

    friend std::ostream& operator<<(std::ostream& os, const queue_attr& attr) {
        std::string s_role = "";
        if (attr.role == READER) {
            s_role = "READER";
        } else if (attr.role == WRITER) {
            s_role = "WRITER";
        } else {
            // do nothing
        }
        std::string s_mode = "";
        if (attr.queue_mode == MODE_WRITE) {
            s_mode = "MODE_WRITE";
        } else if (attr.queue_mode == MODE_OVERWRITE) {
            s_mode = "MODE_OVERWRITE";
        } else {
            // do nothing
        }

        os << "shmmq,role:" << s_role << ",mode:" << s_mode << ",queue size:" << attr.queue_size
           << ",priority:" << static_cast<int>(attr.priority)
           << ",authentication:" << static_cast<int>(attr.queue_authentication) << ",deadline:" << attr.deadline
           << ",memory_mode:" << static_cast<int>(attr.memory_mode)
           << ",queue_block_type:" << static_cast<int>(attr.queue_block_type)
           << ",queue_block_size:" << attr.queue_block_size
#ifdef ENABLE_DES
           << ",queue_wake_up_mode:" << static_cast<uint32_t>(attr.queue_wake_up_mode)
#endif
            ;
        return os;
    }

    /**
   * @brief Set the role attribute with queue
   * @details writer enqueue message,reader dequeue message.
   * @param[in] refer to enum queue_role.
   */
    void SetRole(queue_role role_) { this->role = role_; }

    /**
   * @brief Set write mode with queue
   * @details enqueue can be set to write/overwrite, using no-blocking mode
   * @param[in] refer to enum queue_mode.
   */
    void SetMode(queue_mode mode) { this->queue_mode = mode; }

    /**
   * @brief has the queue owner enabled authentication
   * @details 1: Enable authentication，0：Do not enable authentication.
   * @param[in] refer to authentication.
   */
    void SetToAuthenticate(uint8_t authentication) { this->queue_authentication = authentication; }
    /**
   * @brief set reserved size KB
   * @details reserved size for data sharing
   * @param[in] refer to enum queue_mode.
   */
    void SetReservedSize(uint8_t reservedSize) { this->reserved_size = reservedSize; }

    uint64_t deadline;            /*!< exception deadline */
    uint32_t queue_size;          /*!< max msg numbers in the queue */
    uint16_t queue_block_size;    /*!< every block size */
    uint8_t role;                 /*!< refer to enum queue_role */
    uint8_t queue_mode;           /*!< refer to enum queue_mode */
    uint8_t priority;             /*!< queue priority */
    uint8_t memory_mode;          /*!< shared memory type */
    uint8_t queue_block_type;     /*!< refer to enum block_type */
    uint8_t reserved_size;        /*!< reserved size KB for data sharing */
    uint8_t queue_authentication; /*!< queue authentication, 0: not authentication, 1: authentication*/
#ifdef ENABLE_DES
    uint8_t queue_wake_up_mode; /*!< DES or sem(PTHREAD_CONDITION_WAKE_UP) */
#endif
    queue_attr()
        : deadline(0U),
          queue_size(0U),
          queue_block_size(0U),
          role(0U),
          queue_mode(0U),
          priority(0U),
          memory_mode(0U),
          queue_block_type(0U),
          reserved_size(0U),
          queue_authentication(0U)
#ifdef ENABLE_DES
          ,
          queue_wake_up_mode(PTHREAD_CONDITION_WAKE_UP)
#endif
    {
    }

    queue_attr(uint8_t role_, uint8_t queue_mode_, uint32_t queue_size_)
        : deadline(0U),
          queue_size(queue_size_),
          queue_block_size(0U),
          role(role_),
          queue_mode(queue_mode_),
          priority(0U),
          memory_mode(0U),
          queue_block_type(0U),
          reserved_size(0U),
          queue_authentication(0U)
#ifdef ENABLE_DES
          ,
          queue_wake_up_mode(PTHREAD_CONDITION_WAKE_UP)
#endif
    {
    }

    queue_attr(uint8_t role_, uint8_t queue_mode_, uint32_t queue_size_, uint8_t priority_, uint32_t deadline_,
               uint8_t memory_mode_ = 0U, uint8_t queue_block_type_ = 0U, uint16_t queue_block_size_ = 0U,
               uint8_t reserved_size_ = 0U)
        : deadline(deadline_),
          queue_size(queue_size_),
          queue_block_size(queue_block_size_),
          role(role_),
          queue_mode(queue_mode_),
          priority(priority_),
          memory_mode(memory_mode_),
          queue_block_type(queue_block_type_),
          reserved_size(reserved_size_),
          queue_authentication(0U)
#ifdef ENABLE_DES
          ,
          queue_wake_up_mode(PTHREAD_CONDITION_WAKE_UP)
#endif
    {
    }

#ifdef ENABLE_DES
    queue_attr(uint8_t role_, uint8_t queue_mode_, uint32_t queue_size_, uint8_t priority_, uint32_t deadline_,
               uint8_t memory_mode_, uint8_t queue_block_type_, uint16_t queue_block_size_, uint8_t wake_mode,
               uint8_t reserved_size_)
        : deadline(deadline_),
          queue_size(queue_size_),
          queue_block_size(queue_block_size_),
          role(role_),
          queue_mode(queue_mode_),
          priority(priority_),
          memory_mode(memory_mode_),
          queue_block_type(queue_block_type_),
          reserved_size(reserved_size_),
          queue_authentication(0U),
          queue_wake_up_mode(wake_mode) {}
#endif
} QueueAttr;

class MessageQueue {
 private:
    friend class ShmmqManager;
    std::string name_;
    uint32_t queueId_ {};
    QueueAttr queue_attr_;
    QueueSegment* segment_ {nullptr};
    signal_info* sig_info_ {nullptr};
    bool CheckAttr();
    bool inited_ = false; /*!< check if Message Queue initialized */
    queue_ret_t Init();

 public:
    /**
   * @brief Constructor
   * @param[in] name name of message queue
   * @param[in] attr attribution of message queue
   *
   */
    MessageQueue(const std::string& name, const QueueAttr& attr);

    /**
   * @brief Constructor
   * @param[in] queueId id of message queue
   *
   */
    MessageQueue(uint32_t queueId, const QueueAttr& attr);

    /**
   * @brief Destructor.
   * @details In the destructor, delete Message Queue stored in ShmmqManager
   */
    ~MessageQueue();

 public:
    /**
   * @brief add peer_id
   * @details Authorize the writer to open the queue
   * @param[in] peer_id writer pid
   * @return none
   * 
   */
    void AddPeerId(const uint32_t peer_id);

    /**
   * @brief enqueue
   * @details
   * -# non block mode:only support no-block mode
   * write mode:will return -1 when queue full;
   * overwrite mode:will overwrite the oldest msg when queue full.
   * @param[in] desc BufferDescriptor
   * @return queue_ret_t, message queue return type.
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   *
   */
    queue_ret_t Enqueue(const mbuf::BufferDescriptor& desc);

    /**
   * @brief dequeue
   * @details dequeue message
   * @param[in] desc  buffer Descriptor
   * timestamp,pid,priority,deadline,validity and so on. Timestamp indicates the
   * time of enqueue
   * @return queue_ret_t, message queue return type.
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   */
    queue_ret_t Dequeue(mbuf::BufferDescriptor& desc);

    /**
   * @brief dequeue
   * @details
   * -# non block mode:
   * in non block mode dequeue will return when queue empty
   * -# block mode:
   * dequeue will block when queue empty until
   * some notification occurs to run again or timeout to return fail
   * @param[in] desc  buffer Descriptor,describe
   * timestamp,pid,priority,deadline,validity and so on. Timestamp indicates the
   * time of enqueue
   * @param[in] timeout_ms the timeout value will be ignored in non-blocking
   * mode, in blocking mode timeout unit is ms if val > 0 will block val time,
   * if val = 0 will block forever until some notification occurs
   * @return queue_ret_t, message queue return type.
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   */
    queue_ret_t Dequeue(mbuf::BufferDescriptor& desc, uint32_t timeout_ms);

    /**
   * @brief Waiting dequeue Message
   * @details waiting message arriving to be dequeued for timeout_ms.
   * @param[in] timeout_ms  timeout unit is ms if val > 0 will block val time,
   * if val = 0 will return immediately.
   * @return queue_ret_t, message queue return type.
   *              - val = 0 : have message in queue waiting to be dequeued.
   *              - val < 0 : waiting message failed,try to wait again.
   *
   */
    queue_ret_t WaitMessage(uint32_t timeout_ms);

    /**
   * @brief enqueue custom type
   * @details
   * -# non block mode:only support no-block mode
   * write mode:will return -1 when queue full;
   * @param[in] buf_desc custom type
   * @return queue_ret_t, message queue return type.
   * if custom type size not equal the queue_block_size of attr,then
   * return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   *
   */
    template <typename T>
    queue_ret_t Enqueue(const T* ptr) {
        if (queue_attr_.queue_block_type == MBUF_TYPE) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                          "Message queue block type not match!");
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }
        if (sizeof(T) != queue_attr_.queue_block_size) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                          "Message queue block size not match!,sizeof(T):"
                              << sizeof(T) << ",queue_attr_.queue_block_size:" << queue_attr_.queue_block_size);
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }
        if (!inited_) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION, "Message queue is not inited!");
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }
        if (segment_->GetConfig().queue_role != WRITER) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                          "Only Writer can enqueue, but current role is:" << segment_->GetConfig().queue_role);
            return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
        }

        queue_ret_t ret = queue_ret_t::RETCODE_SHMMQ_SUCCESS;
        ret = segment_->WriteNoBlock(ptr);
        return ret;
    }

    /**
   * @brief ReadFront
   * @details ReadFront message,but no pop,for use with PopFront
   * @param[in] ptr  custom type
   * @param[in] timeout_ms timeout int32 is ms if val > 0 will block val time,
   * if val = 0 will block forever until some notification occurs
   * if val < 0 will not block ,If the queue is empty, return immediately
   * @return queue_ret_t, message queue return type.
   * if custom type size not equal the queue_block_size of attr,then
   * return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   */
    template <typename T>
    queue_ret_t ReadFront(T* ptr, int32_t timeout_ms = 0U) {
        if (sizeof(T) != queue_attr_.queue_block_size) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                          "Message queue block size not match!");
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }
        if (!inited_) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION, "Message queue is not inited!");
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }

        if (segment_->GetConfig().queue_role != READER) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                          "Only Reader can Dequeue, but current role is:" << segment_->GetConfig().queue_role);
            return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
        }

        return segment_->ReadNoPop(ptr, timeout_ms);
    }

    /**
   * @brief PopFront
   * @details pop one data from ringbuffer,only support no-block mode
   * for use with ReadFront
   * @return queue_ret_t, message queue return type.
   * if you don't care about the specific reason,
   * you can simply think that val == 0 enqueue succeeds and otherwise fails.
   *              - val = 0 : enqueue success
   *              - val < 0 : enqueue failed
   */
    inline queue_ret_t PopFront() {
        if (!inited_) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION, "Message queue is not inited!");
            return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
        }

        if (segment_->GetConfig().queue_role != READER) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                          "Only Reader can Dequeue, but current role is:" << segment_->GetConfig().queue_role);
            return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
        }
        return segment_->Pop();
    }
    /**
   * @brief get message queue attribute
   * @return message queue attribute
   */
    QueueAttr get_attr() const { return queue_attr_; }

    /**
   * @brief get message queue name
   * @return message queue name
   */
    std::string get_name() { return name_; }

    /**
   * @brief get message queue segment
   * @return message queue segment
   */
    const QueueSegment* get_segment() const { return segment_; }
    /**
   * @brief get message queue real size
   * @return message queue size
   */
    uint32_t get_size() const;

    /**
   * @brief get message queue real size
   * @return message queue size
   */
    uint32_t get_queue_id() const;
#ifdef ENABLE_DES

 private:
    TaskCfg cfg_ {};
    /**
   * @brief register task in des wake up mode
   * @param func call back function
   * @param cfg  task cfg
   * @return true success false failed
   */
 public:
    bool RegisterTask(const std::shared_ptr<std::function<void(void)>>& func, const TaskCfg& cfg);
    bool UnRegisterTask(const TaskCfg& cfg);
#endif
    uint8_t get_reserved_size() const;
    void* get_reserved_ptr() const;
};

} /* namespace shmmq */

#endif  // SHM_MSG_QUEUE_H
