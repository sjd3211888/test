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

#ifndef DSFDDS_TRANSPORT_CHANNELRESOURCE_H_
#define DSFDDS_TRANSPORT_CHANNELRESOURCE_H_

// std include
#include <semaphore.h>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#include <functional>

// dts entity include
#include "DsfLog.hpp"
// lib include
#include "shmmq/message_queue.h"

namespace li {
namespace dsfdds {

class ChannelResource {
 public:
    ChannelResource(ChannelResource&& channelResource);

    ChannelResource(std::function<void(ChannelResource* cr)> func, std::string name);

    virtual ~ChannelResource();

    virtual void clear();

    inline void thread(std::thread&& pThread) {
        if (receive_thread_.joinable()) {
            DSF_LOG_DEBUG(CHANNELRESOURCE, "receive thread joinable");
            receive_thread_.join();
        }
        DSF_LOG_DEBUG(CHANNELRESOURCE, "receive thread not joinable");
        receive_thread_.swap(pThread);
    }

    inline bool alive() const { return alive_.load(); }

    inline void disable() { alive_.store(false); }
    bool SemTakeWait(uint32_t ms) const;
    bool SemTakePost();

    void CallBackFunc(const std::string& topic_name);

 protected:
    std::thread receive_thread_;
    std::function<void(ChannelResource* cr)> call_back_func_;
    std::string queue_name_;

    std::atomic<bool> alive_;
    std::atomic<bool> is_clear_;
    std::mutex mtx_sem_post_;
    mutable sem_t sem_take_;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_CHANNELRESOURCE_H_
