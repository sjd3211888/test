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

#include "dsfdds/transport/channel_resource.h"
#include <ctime>
#include <cstring>
#include <cerrno>
#include <utility>
#include "DsfLog.hpp"

namespace li {
namespace dsfdds {

ChannelResource::ChannelResource(std::function<void(ChannelResource*)> func, std::string name)
    : call_back_func_(std::move(func)), queue_name_(std::move(name)), alive_(true), is_clear_(false) {
    DSF_LOG_DEBUG(ChannelResource, "Created ChannelResource with queue name: " << queue_name_);
    auto ret = sem_init(&sem_take_, 0, 0U);
    if (ret != 0) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                      "sem_take_ init failed ret:" << ret << ",error no str:" << strerror(errno));
        static_cast<void>(sem_destroy(&sem_take_));
    }
}

ChannelResource::ChannelResource(ChannelResource&& channelResource)
    : receive_thread_(std::move(channelResource.receive_thread_)), queue_name_(channelResource.queue_name_) {
    bool b = channelResource.alive_;
    alive_.store(b);
    is_clear_.store(channelResource.is_clear_);
    DSF_LOG_DEBUG(ChannelResource, "Created ChannelResource with queue name:" << queue_name_);
}

ChannelResource::~ChannelResource() {
    if (is_clear_.load() == false) {
        ChannelResource::clear();
    }
}

void ChannelResource::clear() {
    if (is_clear_.load() == true) {
        return;
    }
    static_cast<void>(sem_destroy(&sem_take_));
    alive_.store(false);
    is_clear_.store(true);
    if (receive_thread_.joinable()) {
        DSF_LOG_DEBUG(ChannelResource,
                      "ChannelResource receive thread joinable,"
                      "receive_thread_.get_id() = "
                          << receive_thread_.get_id()
                          << ",std::this_thread::get_id() = " << std::this_thread::get_id());
        if (receive_thread_.get_id() != std::this_thread::get_id()) {
            // wait for it to finish
            receive_thread_.join();
            DSF_LOG_DEBUG(ChannelResource, "ChannelResource join");
        } else {
            // killing my own thread
            receive_thread_.detach();
            DSF_LOG_DEBUG(ChannelResource, "ChannelResource detach");
        }
    }
    DSF_LOG_DEBUG(ChannelResource, "ChannelResource receive thread not joinable,thread is already over!!!");
}

void ChannelResource::CallBackFunc(const std::string& topic_name) {
    auto call_back_fn = [this, topic_name]() {
#if defined(__unix__)
        // maximum available space (15 - 4 bytes for "DSF:")
        constexpr size_t max_topic_len = 11;
        std::string truncated_name = topic_name;

        // take suffix of the topic name(last 14 char) to preserve the most important identifying information
        if (topic_name.length() > max_topic_len) {
            truncated_name = topic_name.substr(topic_name.length() - max_topic_len);
        }

        // construct full thread name
        std::string thread_name = "DSF:" + truncated_name;

        // safely set thread name (avoid unintended truncation)
        char safe_name[16] = {0};
        (void)strncpy(safe_name, thread_name.c_str(), sizeof(safe_name) - 1);
        (void)pthread_setname_np(pthread_self(), safe_name);
#endif  // defined(__unix__)
        while (alive()) {
            call_back_func_(this);
        }
    };
    thread(std::thread(call_back_fn));
}

namespace {
const uint32_t MS_PER_SECOND = 1000U;
const uint32_t NS_PER_MS = 1000000U;
const int32_t TV_NSEC_SECOND_THRESHOLD = 1000000000;
}  // namespace

bool ChannelResource::SemTakeWait(uint32_t timeout_ms) const {
    struct timespec ts {};
    static_cast<void>(clock_gettime(CLOCK_REALTIME, &ts));
    ts.tv_sec += timeout_ms / MS_PER_SECOND;
    ts.tv_nsec += static_cast<uint64_t>(timeout_ms % MS_PER_SECOND) * NS_PER_MS;
    if (ts.tv_nsec >= TV_NSEC_SECOND_THRESHOLD) {
        ts.tv_sec += 1;
        ts.tv_nsec -= TV_NSEC_SECOND_THRESHOLD;
    }
    // wait reader take message
    int ret = 0;
    if (timeout_ms > 0U) {
        ret = sem_timedwait(&sem_take_, &ts);
        if (ret != 0) {
            if (errno == ETIMEDOUT) {
                DSF_LOG_DEBUG(ChannelResource, vbsutil::elog::decToHex(ReturnCode_t::RETCODE_PUBSUB_TIMEOUT)
                                                   << " "
                                                   << "sem_timedwait timeout:" << timeout_ms << ",ret:" << ret
                                                   << ",error no str:" << strerror(errno));
                return false;
            }
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                          "sem_wait failed, ret:" << ret << ",errno: " << errno << " sem_take_ addr: " << &sem_take_
                                                  << ". TimeStample tv_sec = " << ts.tv_sec
                                                  << " TimeStample tv_nsec = " << ts.tv_nsec);
            return false;
        }
    } else {
        // wait forever
        ret = sem_wait(&sem_take_);
        if (ret != 0) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                          "sem_wait failed, ret:" << ret << ",errno: " << errno);
            return false;
        }
    }
    return true;
}

bool ChannelResource::SemTakePost() {
    std::lock_guard<std::mutex> lk(mtx_sem_post_);
    int sem_value = 0;
    auto ret = sem_getvalue(&sem_take_, &sem_value);
    if (ret != 0) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                      "sem_take_ sem_getvalue failed ret:" << ret << ",error no str:" << strerror(errno));
        return false;
    } else {
        if (sem_value == 0) {
            auto ret_post = sem_post(&sem_take_);
            if (ret_post != 0) {
                DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                              "sem_take_ sem_post failed ret:" << ret_post << ",error no str:" << strerror(errno));
                return false;
            }
        } else if (sem_value > 1) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEM_ERR,
                          "sem_take_ getvalue:" << sem_value << " > 1 unexpected");
            return false;
        }
    }
    return true;
}

}  // namespace dsfdds
}  // namespace li
