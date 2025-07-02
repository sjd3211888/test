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

#include "shmmq/shmmq_manager.h"
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <cstdint>
#include <iostream>
#include "shm_heap_wrapper.h"
#include "shmmq/shm_manager.h"
#include <unistd.h>
#include "DsfLog.hpp"

#define SHMMQ_PREFIX_NAME "shmmq_"

namespace shmmq {
std::mutex ShmmqManager::map_mutex_;
std::unordered_map<std::string, std::weak_ptr<MessageQueue>> ShmmqManager::name_shmmq_map_;
std::unordered_map<uint32_t, std::weak_ptr<MessageQueue>> ShmmqManager::id_shmmq_map_;

static int32_t epollfd_ = -1;

std::shared_ptr<MessageQueue> ShmmqManager::GetMQ(const std::string& name, const QueueAttr& attr) {
    std::lock_guard<std::mutex> guard(map_mutex_);

    // reader and writer can create its own MessageQueue , which using same share
    // memory.
    std::string mq_name = SHMMQ_PREFIX_NAME + name;
    std::string mq_key = mq_name + "_" + std::to_string(static_cast<int32_t>(attr.role));
    const auto iter = name_shmmq_map_.find(mq_key);
    DSF_LOG_DEBUG(shmmq, "GetMQ:" << mq_key);
    auto create_mq = [&mq_name, attr, &mq_key, name]() -> std::shared_ptr<MessageQueue> {
        try {
            std::shared_ptr<MessageQueue> msg_queue_share = std::make_shared<MessageQueue>(mq_name, attr);
            if (msg_queue_share != nullptr) {
                auto init_ret = msg_queue_share->Init();
                if (init_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
                    DSF_LOG_ERROR(DSF_SHMMQ, init_ret(),
                                  "MessageQueue init failed, queue name: " << mq_name << ",attr:" << attr);
                    return nullptr;
                }
                std::weak_ptr<MessageQueue> msg_queue_weak(msg_queue_share);
                static_cast<void>(name_shmmq_map_.emplace(mq_key, msg_queue_weak));

                DSF_LOG_INFO(shmmq, "new MessageQueue is built, queue name: " << mq_name << ",attr:" << attr);
                return msg_queue_share;
            }

            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_ERROR,
                          "Create MessageQueue failed!, queue name: " << mq_name << ",attr:" << attr);
            return nullptr;
        } catch (const std::exception& e) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_ERROR,
                          "Failed to create new Message Queue: " << name << "," << e.what());
            return nullptr;
        }
    };
    // message queue not found,create it.
    if (iter == name_shmmq_map_.end()) {
        return create_mq();
    } else {
        std::shared_ptr<MessageQueue> message_queue = iter->second.lock();
        if (!message_queue) {
            return create_mq();
        }
        const QueueAttr attribute = message_queue->get_attr();
        if (attribute != attr) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                          "find MessageQueue from map with different attribute, return null, the queue name is: "
                              << mq_name << ", the expect attr is: " << attr << ", the attr in map is: " << attribute);
            return nullptr;
        } else {
            DSF_LOG_INFO(shmmq, "find MessageQueue from map with same attribute");
            return message_queue;
        }
    }
}

/**
 * only used for writer open queue
 * @param queue_id
 * @param attr 仅仅需要填充 role=WRITER,queue_mode=MODE_DEFAULT,memory_mode=shm::KUNIMEM,queue_block_type=MBUF_TYPE
 * @return
 */
std::shared_ptr<MessageQueue> ShmmqManager::GetMQ(uint32_t queue_id, const QueueAttr& attr) {
    std::lock_guard<std::mutex> guard(map_mutex_);
    const auto iter = id_shmmq_map_.find(queue_id);
    auto open_mq = [queue_id, &attr]() -> std::shared_ptr<MessageQueue> {
        try {
            std::shared_ptr<MessageQueue> msg_queue_share = std::make_shared<MessageQueue>(queue_id, attr);
            if (msg_queue_share != nullptr) {
                auto init_ret = msg_queue_share->Init();
                if (init_ret != queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
                    DSF_LOG_ERROR(DSF_SHMMQ, init_ret(),
                                  "MessageQueue init failed, queue id: " << queue_id << ",attr:" << attr);
                    return nullptr;
                }
                std::weak_ptr<MessageQueue> msg_queue_weak(msg_queue_share);
                static_cast<void>(id_shmmq_map_.emplace(queue_id, msg_queue_weak));

                DSF_LOG_INFO(shmmq, "new MessageQueue is built, queue id: " << queue_id << ",attr:" << attr);
                return msg_queue_share;
            }

            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_ERROR,
                          "Create MessageQueue failed!, queue id: " << queue_id << ",attr:" << attr);
            return nullptr;
        } catch (const std::exception& e) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_ERROR,
                          "Failed to create new message queue id: " << queue_id << "," << e.what());
            return nullptr;
        }
    };
    // message queue not found,open it.
    if (iter == id_shmmq_map_.end()) {
        return open_mq();
    } else {
        std::shared_ptr<MessageQueue> message_queue = iter->second.lock();
        if (message_queue) {
            const QueueAttr attribute = message_queue->get_attr();
            if (attribute != attr) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,
                              "find MessageQueue from map with different attribute, return null, the queue queue id: "
                                  << queue_id << ", the expect attr is: " << attr
                                  << ", the attr in map is: " << attribute);
                return nullptr;
            } else {
                DSF_LOG_INFO(shmmq, "find MessageQueue from map with same attribute");
                return message_queue;
            }
        } else {
            return nullptr;
        }
    }
}

void ShmmqManager::DeleteMQ(const std::string& name) {
    DSF_LOG_DEBUG(shmmq, "DeleteMQ,key:" << name);
    std::lock_guard<std::mutex> guard(map_mutex_);
    const auto iter = name_shmmq_map_.find(name);
    if ((iter != name_shmmq_map_.end())) {
        static_cast<void>(name_shmmq_map_.erase(name));
        DSF_LOG_DEBUG(shmmq, "shmmq queue name:" << name << " map erased !");
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_NOT_FIND_QUEUE_NAME_ERR, "not find MQ, name: " << name);
    }
}

void ShmmqManager::DeleteMQ(uint32_t queue_id) {
    DSF_LOG_DEBUG(shmmq, "DeleteMQ,queue id:" << queue_id);
    std::lock_guard<std::mutex> guard(map_mutex_);
    const auto iter = id_shmmq_map_.find(queue_id);
    if ((iter != id_shmmq_map_.end())) {
        static_cast<void>(id_shmmq_map_.erase(queue_id));
        DSF_LOG_DEBUG(shmmq, "shmmq queue id:" << queue_id << " map erased !");
    } else {
        DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_NOT_FIND_QUEUE_NAME_ERR,
                      "not find MQ, queue id:" << queue_id);
    }
}

std::vector<signal_info> ShmmqManager::vec_event;
std::vector<std::shared_ptr<struct parse_type>> ShmmqManager::vec_type;

#ifdef __cplusplus
extern "C" {
#endif

static void buf_parser(struct sys_heap_buf_info* info) {
    std::shared_ptr<struct parse_type> tmp_type = std::make_shared<struct parse_type>();
    tmp_type->uuid = info->uuid;
    static_cast<void>(memcpy(tmp_type->name, info->name, BUF_NAME_LEN));
    static_cast<void>(ShmmqManager::vec_type.emplace_back(tmp_type));
}

namespace {
void event_parser(struct sys_heap_event* ev) {
    const struct sys_heap_event_exit* exit_e;

    switch (ev->type) {
        case HEAP_EVENT_EXIT:
            exit_e = reinterpret_cast<const sys_heap_event_exit*>(ev);
            if (nullptr == exit_e) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_PUBSUB_BAD_PARAMETER, "get sys_heap_event nullptr!");
                return;
            }
            for (size_t i = 0U; i < exit_e->num_uuid; i++) {
                for (auto&& iter : ShmmqManager::vec_type) {
                    signal_info tmp_info;
                    tmp_info.pid = static_cast<uint64_t>(exit_e->pid);
                    tmp_info.name = iter->name;
                    tmp_info.uuid = iter->uuid;
                    if (iter->uuid == exit_e->uuid_list[i]) {
                        if (tmp_info.name.substr(0U, 4U) == "mbuf" || tmp_info.name.substr(0, 6) == "membuf") {
                            tmp_info.is_mbuf_flag = true;
                        }
                        static_cast<void>(ShmmqManager::vec_event.emplace_back(tmp_info));
                    }
                }
            }
            break;
        default:
            break;
    }
}
}  // namespace

#ifdef __cplusplus
}
#endif

// exception notify, to do add a flag 标识有进程退出但驱动相关查询错误
std::vector<signal_info> ShmmqManager::WaitSignal(const uint32_t& timeout_ms) {
    vec_event.clear();
    if (-1 == epollfd_) {
        // init signal epollfd
        int shm_driver_fd = shm::ShmUniMem::shmGetDevFd();
        if (shm_driver_fd >= 0) {
            int epfd = epoll_create1(EPOLL_CLOEXEC);
            if (epfd < 0) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_EPOLL_ERR,
                              "epoll_create1 failed, error num is: " << strerror(errno));
                return vec_event;
            }
            struct epoll_event ev;
            static_cast<void>(memset(&ev, 0, sizeof(ev)));
            ev.events = static_cast<unsigned>(EPOLLIN) | static_cast<unsigned>(EPOLLRDNORM);
            ev.data.fd = shm_driver_fd;
            int epoll_ret = epoll_ctl(epfd, EPOLL_CTL_ADD, shm_driver_fd, &ev);
            if (0 != epoll_ret) {
                DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_EPOLL_ERR,
                              "epoll_ctl failed, error num is " << strerror(errno));
                return vec_event;
            }
            epollfd_ = epfd;
        } else {
            DSF_LOG_ERROR(
                DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_HEAP_FD_ERR,
                "get device fd failed! Perhaps due to open unimem file failed, fstat failed , or file is not a "
                "character device");
            return vec_event;
        }
    }
    int const event_size = 20;
    struct epoll_event events[event_size];
    int nfds = epoll_wait(epollfd_, events, event_size, static_cast<int32_t>(timeout_ms));
    if (0 == nfds) {
        return vec_event;
    } else {
        pid_t process_id = getpid();
        query_t query_type = QUERY_BY_PID;
        vec_type.clear();
        int ret = shm_heap_query(query_type, &process_id, buf_parser);
        if (0 != ret) {
            DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_HEAP_QUERY_ERR,
                          "shm_heap_query error, error is " << strerror(errno));
            return vec_event;
        }
        int wait_fd = shm::ShmUniMem::shmGetDevFd();
        for (int i = 0; i < nfds; i++) {
            if ((events[i].data.fd == wait_fd) &&
                ((events[i].events & (static_cast<unsigned>(EPOLLIN) | static_cast<unsigned>(EPOLLRDNORM))) != 0U)) {
                try {
                    static_cast<void>(shm_heap_get_event(&event_parser));
                } catch (...) {
                    DSF_LOG_ERROR(DSF_SHMMQ, queue_ret_t::RETCODE_SHMMQ_ERROR, "get catch when parse event!");
                }
            } else {
                DSF_LOG_ERROR(DSF_SHMMQ, "The event information obtained from epoll_wait is not as expected."
                                             << "events[i].data.fd = " << events[i].data.fd << ", wait_fd = " << wait_fd
                                             << "events[i].events = " << events[i].events);
            }
        }
    }
    return vec_event;
}

void ShmmqManager::CloseShmHeap() {
    { epollfd_ = -1; }
    shm_heap_close();
}
}  // namespace shmmq
