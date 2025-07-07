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

#include <chrono>
#include "replayer/VbsReplayer.hpp"

namespace vbstoolsdk {

void VbsReplayer::start() {
    replay_status_ = ReplayStatus::START;
    process_thread_ = std::thread(&VbsReplayer::process_msg_, this);
}

void VbsReplayer::pause() {
    replay_status_ = ReplayStatus::PAUSE;
}

void VbsReplayer::stop() {
    replay_status_ = ReplayStatus::STOP;
    if (process_thread_.joinable()) {
        process_thread_.join();
    }
}

void VbsReplayer::set_loop(bool loop) {
    replay_loop_ = loop;
}

void VbsReplayer::set_loop_times(int32_t loop_times) {
    loop_times_ = loop_times;
}

float VbsReplayer::get_progress() {
    float progress = (file_total_size_ > 0) ? static_cast<float>(file_cur_size_ / file_total_size_) : 0;
    return progress;
}

VbsReplayer::ReplayStatus VbsReplayer::get_stop_status() {
    return replay_status_;
}

}  // namespace vbstoolsdk
