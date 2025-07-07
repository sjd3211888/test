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

#include <iostream>
#include "replayer/BaseDatReplayer.hpp"
#include "elog/Log.hpp"

namespace vbstoolsdk {

bool BaseDatReplayer::init_db_() {
    db_ = std::make_shared<DbOperator>(path_, 0);
    if (nullptr == db_) {
        logError_("VBSTOOLSDK", "BaseReplayer db_ init failed, full_path: " << path_);
        return false;
    }

    if (false == db_->get_metadata(meta_info_)) {
        logError_("VBSTOOLSDK", "BaseReplayer db_ get_metadata failed");
        return false;
    }
    return true;
}

void BaseDatReplayer::control_speed_(uint64_t last, uint64_t cur) {
    int64_t interal = static_cast<int64_t>(cur - last);
    if (last == 0 || interal <= 0) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(interal));
}

void BaseDatReplayer::process_msg_() {
    std::vector<DbOperator::DbRawDataInfo> info_vec;
    if (db_ == nullptr) {
        return;
    }
    int count = db_->get_rawdata_count();
    file_total_size_ = count;
    if (count == 0) {
        return;
    }
    int send_times = 0;
    uint64_t last_time_stamp = 0;

    while (replay_status_ != ReplayStatus::STOP) {
        //maybe loop
        while (send_times < count) {
            if (replay_status_ == ReplayStatus::PAUSE) {
                //等待条件变量触发
            }
            size_t select_num = db_->get_rawdata(info_vec, send_times, send_times + SELECT_NUM_ONCE - 1);
            if (info_vec.size() < select_num) {
                logError_("VBSTOOLSDK",
                          "get_rawdata vector size: " << info_vec.size() << " less than select_num: " << select_num);
                return;
            }

            for (size_t i = 0; i < select_num; i++) {
                if (replay_status_ == ReplayStatus::PAUSE) {
                    //等待条件变量触发
                } else if (replay_status_ == ReplayStatus::STOP) {
                    return;
                }

                if (info_vec.at(i).raw_timestamp < 94665600000000) {
                    continue;
                }
                control_speed_(last_time_stamp, info_vec.at(i).raw_timestamp);
                writer_(info_vec.at(i));
                last_time_stamp = info_vec.at(i).raw_timestamp;
                file_cur_size_++;
            }
            send_times += select_num;
            info_vec.clear();
        }

        if (!replay_loop_) {
            if (loop_times_ > 0) {
                loop_times_--;
            }

            if (loop_times_ <= 0) {
                replay_status_ = ReplayStatus::STOP;
                return;
            }
        }

        send_times = 0;
        file_cur_size_ = 0;
        last_time_stamp = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

}  // namespace vbstoolsdk
