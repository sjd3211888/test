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

#ifndef THREAD_SAFE_QUEUE_HPP_
#define THREAD_SAFE_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>

namespace vbstoolsdk {

template <typename T>
class ThreadSafeQueue {

 public:
    ThreadSafeQueue() {}
    ThreadSafeQueue(ThreadSafeQueue const& other) {
        std::lock_guard<std::mutex> lk(other.mutex_);
        queue_ = other.queue_;
    }
    ~ThreadSafeQueue() {}

    void push(T&& new_value) {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.emplace(std::move(new_value));
        cv_.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this] { return !queue_.empty(); });
        if (stopped)
            return;

        value = std::move(queue_.front());
        queue_.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this] { return !queue_.empty(); });
        if (stopped)
            return nullptr;

        std::shared_ptr<T> res(std::make_shared<T>(std::move(queue_.front())));
        queue_.pop();
        return res;
    }

    bool empty() {
        std::unique_lock<std::mutex> lk(mutex_);
        return queue_.empty();
    }

    bool try_pop(T& value) {
        std::unique_lock<std::mutex> lk(mutex_, std::try_to_lock);
        if (lk.owns_lock() && !queue_.empty()) {
            value = std::move(queue_.front());
            queue_.pop();
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
        cv_.notify_all();  // Notify any waiting threads
    }

    bool try_pop_batch(std::vector<T>& values, size_t batch_size) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.size() < batch_size) {
            return false;
        }

        values.clear();
        size_t count = std::min(batch_size, queue_.size());
        for (size_t i = 0; i < count; ++i) {
            values.emplace_back(std::move(queue_.front()));
            queue_.pop();
        }

        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lk(mutex_);
        stopped = true;
        cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mutex_);
        stopped = false;
    }

 private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cv_;
    bool stopped {false};
};

}  // namespace vbstoolsdk

#endif
