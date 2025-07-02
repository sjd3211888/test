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

#include "shmmq/message_queue.h"
#include <sys/epoll.h>
#include <cstdint>
#include <cstring>
#include "shm_heap_wrapper.h"
#include "shmmq/shmmq_manager.h"
#ifdef ENABLE_DES
#include "des_api.h"
#endif
namespace shmmq {

MessageQueue::MessageQueue(const std::string& name, const QueueAttr& attr) {
    name_ = name;
    queueId_ = 0U;
    queue_attr_ = attr;
    segment_ = nullptr;
}

MessageQueue::MessageQueue(uint32_t queue_id, const QueueAttr& attr)
    : name_(""), queueId_(queue_id), queue_attr_(attr), segment_(nullptr) {}

MessageQueue::~MessageQueue() {
    DSF_LOG_DEBUG(shmmq, " ~MessageQueue!");
    if (inited_) {
#ifdef ENABLE_DES
        auto ret = UnRegisterTask(cfg_);
        DSF_LOG_INFO(shmmq, "UnRegisterTask ret:" << ret);
#endif
        if (queueId_ == 0 && !name_.empty()) {
            std::string mq_key = name_ + "_" + std::to_string(static_cast<int32_t>(queue_attr_.role));
            ShmmqManager::DeleteMQ(mq_key);
        } else {
            ShmmqManager::DeleteMQ(queueId_);
        }

        if (segment_ != nullptr) {
            delete segment_;
        }
        inited_ = false;
    }
}

queue_ret_t MessageQueue::Init() {
    if (!CheckAttr()) {
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    ConfigStruct config;
    static_cast<void>(std::memset(reinterpret_cast<void*>(&config), 0, sizeof(config)));
    config.queue_role = queue_attr_.role;
    config.queue_size = queue_attr_.queue_size;
    config.queue_mode = queue_attr_.queue_mode;
    config.queue_authentication = queue_attr_.queue_authentication;
    config.priority = queue_attr_.priority;
    config.deadline = queue_attr_.deadline;
    config.memory_mode = queue_attr_.memory_mode;
    config.block_buf_size = queue_attr_.queue_block_size;
    config.reserved_size = queue_attr_.reserved_size;
#ifdef ENABLE_DES
    config.wake_up_mode = queue_attr_.queue_wake_up_mode;
#endif

    queue_ret_t ret = queue_ret_t::RETCODE_SHMMQ_SUCCESS;
    try {
        if (queue_attr_.queue_block_type == MBUF_TYPE) {
            segment_ = new QueueSegment(name_, queueId_);
        } else {
            segment_ = new QueueSegment(name_, queue_attr_.queue_block_size, queueId_);
        }
        ret = segment_->InitSegment(config);
        if (ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
            if (segment_ != nullptr) {
                delete segment_;
                segment_ = nullptr;
            }
            return ret;
        }
    } catch (const std::exception& e) {
        DSF_LOG_ERROR(DSF_SHMMQ, ret(),
                      "Failed to create Queue Segment, the error is: " << e.what() << ",name:" << name_);

        if (segment_ != nullptr) {
            delete segment_;
            segment_ = nullptr;
        }
        return queue_ret_t::RETCODE_SHMMQ_RUNTIME_EXCEPTION;
    }

    inited_ = true;
    DSF_LOG_DEBUG(shmmq, "Init done");
    return ret;
}

void MessageQueue::AddPeerId(const uint32_t peer_id) {
    return segment_->AddPeerId(peer_id);
}

queue_ret_t MessageQueue::Enqueue(const mbuf::BufferDescriptor& desc) {
    if (queue_attr_.queue_block_type == OTHER_TYPE) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue block type not match, name:" << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }
    if (!inited_) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue is not inited, name:" << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }

    if (segment_->GetConfig().queue_role != WRITER) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "Only Writer can enqueue, but current role is:" << segment_->GetConfig().queue_role);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    queue_ret_t ret = queue_ret_t::RETCODE_SHMMQ_SUCCESS;

    uint32_t queue_mode = segment_->GetConfig().queue_mode;
    if (queue_mode == MODE_OVERWRITE) {
        ret = segment_->OverWriteNoBlock(desc);
    } else {
        ret = segment_->WriteNoBlock(desc);
    }

    return ret;
}

queue_ret_t MessageQueue::Dequeue(mbuf::BufferDescriptor& desc) {
    if (queue_attr_.queue_block_type == OTHER_TYPE) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue block type not match, name: " << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }
    if (!inited_) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue is not inited, name: " << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }

    if (segment_->GetConfig().queue_role != READER) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "Only Reader can Dequeue, but current role is:" << segment_->GetConfig().queue_role);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    return segment_->ReadNoBlock(desc);
}

queue_ret_t MessageQueue::Dequeue(mbuf::BufferDescriptor& desc, uint32_t timeout_ms) {
    if (queue_attr_.queue_block_type == OTHER_TYPE) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue block type not match, name: " << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }
    if (!inited_) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue is not inited, name: " << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }

    if (segment_->GetConfig().queue_role != READER) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT,
                      "Only Reader can Dequeue, but current role is:" << segment_->GetConfig().queue_role);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT;
    }

    return segment_->ReadBlock(desc, timeout_ms);
}

queue_ret_t MessageQueue::WaitMessage(uint32_t timeout_ms) {
    if (!inited_) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION,
                      "Message queue is not inited, name: " << name_);
        return queue_ret_t::RETCODE_SHMMQ_INVALID_OPERATION;
    }

    return segment_->WaitMessage(timeout_ms);
}

bool MessageQueue::CheckAttr() {
    if (queue_attr_.queue_mode == MODE_MONITOR) {
        return true;
    }

    if (queue_attr_.role >= ROLE_INVALID) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                      "Check attribute failed ,unknown role: " << queue_attr_.role);
        return false;
    }
    if (queue_attr_.queue_mode >= MODE_INVALID) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                      "Check attribute failed ,unknown mode: " << queue_attr_.queue_mode);
        return false;
    }

    // 仅仅在reader下判断
    if (queue_attr_.role == READER && queue_attr_.queue_size == 0U) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                      "Check attribute failed ,illegal queue size: " << queue_attr_.queue_size);

        return false;
    }
#ifdef ENABLE_DES
    // 仅仅在reader下判断
    if (queue_attr_.role == READER && (queue_attr_.queue_wake_up_mode >= INVALID_WAKE_UP)) {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                      "Check attribute failed ,illegal queue_wake_up_mode: " << queue_attr_.queue_wake_up_mode);

        return false;
    }
#endif
    return true;
}
uint32_t MessageQueue::get_size() const {
    if (segment_) {
        return segment_->Getsize();
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT, "segment_ is nullptr");
        return 0;
    }
}

uint32_t MessageQueue::get_queue_id() const {
    if (segment_) {
        return segment_->GetQueueId();
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_INVALID_ARGUMENT, "segment_ is nullptr");
        return 0;
    }
}

uint8_t MessageQueue::get_reserved_size() const {
    return segment_->GetConfig().reserved_size;
}

void* MessageQueue::get_reserved_ptr() const {
    return segment_->GetReservedPtr();
}

#ifdef ENABLE_DES
bool MessageQueue::RegisterTask(const std::shared_ptr<std::function<void()>>& func, const TaskCfg& cfg) {
    Des::TaskCfg task_cfg {get_queue_id(), cfg.resGroupId,     cfg.deadlineTime,
                           cfg.runTime,    cfg.schedulePeriod, Des::DesTaskType::TaskTypeCpu};
    cfg_ = cfg;
    cfg_.taskId = get_queue_id();
    DSF_LOG_INFO(MessageQueue, "queue id:" << get_queue_id() << ",queue name:" << get_name()
                                           << ", register task cfg task id:" << cfg.taskId
                                           << ",resGroupId:" << cfg.resGroupId << ",deadlineTime:" << cfg.deadlineTime
                                           << ",runTime:" << cfg.runTime << ",schedulePeriod:" << cfg.schedulePeriod);
    return Des::RegisterTask(func, task_cfg) == get_queue_id();
}

bool MessageQueue::UnRegisterTask(const TaskCfg& cfg) {
    if ((get_attr().role != queue_role::READER) || (get_attr().queue_block_type != block_type::MBUF_TYPE)) {
        return true;
    }
    if (cfg.taskId != 0) {
        Des::TaskCfg task_cfg {cfg_.taskId,  cfg_.resGroupId,     cfg_.deadlineTime,
                               cfg_.runTime, cfg_.schedulePeriod, Des::DesTaskType::TaskTypeCpu};
        return Des::UnRegisterTask(task_cfg);
    } else {
        DSF_LOG_WARNING(MessageQueue, "queue id:" << get_queue_id() << ",queue name:" << get_name()
                                                  << ", Unregister task cfg task id:" << cfg_.taskId << ",resGroupId:"
                                                  << cfg_.resGroupId << ",deadlineTime:" << cfg_.deadlineTime
                                                  << ",runTime:" << cfg_.runTime << ",schedulePeriod:"
                                                  << cfg_.schedulePeriod << ",not register yet!!");
        return false;
    }
}
#endif

}  // namespace shmmq
