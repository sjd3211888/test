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

#ifndef SHMMQ_MANAGER_H
#define SHMMQ_MANAGER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "shmmq/message_queue.h"

namespace shmmq {

class ShmmqManager {
 private:
    friend class MessageQueue;
    static std::mutex map_mutex_;
    // reader use this
    static std::unordered_map<std::string, std::weak_ptr<MessageQueue>> name_shmmq_map_;
    // writer use this
    static std::unordered_map<uint32_t, std::weak_ptr<MessageQueue>> id_shmmq_map_;
    /**
   * @brief Delete std::weak_ptr<MessageQueue> from name_shmmq_map_
   * @details  MessageQueue destruction will call DeleteMQ.
   * @param name [in] Roles to create name: message_name+"_"+role
   */
    static void DeleteMQ(const std::string& name);

    /**
   * @brief Delete std::weak_ptr<MessageQueue> from id_shmmq_map_
   * @details  MessageQueue destruction will call DeleteMQ.
   * @param queue_id [in] queue id to create mq
   */
    static void DeleteMQ(uint32_t queue_id);

 public:
    /**
   * @brief get a MessageQueue
   * @details  if MessageQueue with same queue size already created，reuse it,
   * otherwise new a MessageQueue.
   * @param name [in] Name of MessageQueue want to get.
   * @param attr [in] Attribute of MessageQueue want to get
   * @return std::shared_ptr<MessageQueue>.
   */
    static std::shared_ptr<MessageQueue> GetMQ(const std::string& name, const QueueAttr& attr);

    /**
   * @brief get a MessageQueue
   * @details  if MessageQueue with same queue size already created，reuse it,
   * otherwise new a MessageQueue.
   * @param queue_id [in] queue id of MessageQueue want to get.
   * @param attr [in] Attribute of MessageQueue want to get
   * @return std::shared_ptr<MessageQueue>.
   */
    static std::shared_ptr<MessageQueue> GetMQ(uint32_t queue_id, const QueueAttr& attr);

    /**
   * @brief Get signal from all MQ
   * @details  Get MessageQueue exception signal 
   * @param name [in] ms time out
   * @return std::vector<signal_info>.
   */
    static std::vector<signal_info> WaitSignal(const uint32_t& timeout_ms);

    /**
   * @brief close shm heap
   * @details  close shm heap fd and epoll wait fd,test interface
   * @return 
   */
    static void CloseShmHeap();

 public:
    static std::vector<signal_info> vec_event;
    static std::vector<std::shared_ptr<struct parse_type>> vec_type;
};

}  // namespace shmmq
#endif
