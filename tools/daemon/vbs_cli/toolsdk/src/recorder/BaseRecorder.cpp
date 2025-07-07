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
#include <iomanip>
#include <unistd.h>

#include <elog/Log.hpp>
#include "recorder/BaseRecorder.hpp"

namespace vbstoolsdk {

bool BaseRecorder::start() {
    std::lock_guard<std::mutex> guard(lock_);
    db_ = std::make_shared<DbOperator>(full_path_name_, true);
    if (nullptr != db_) {
        record_status_ = RecordStatus::START;
        return true;
    }

    logError_("VBSTOOLSDK", "BaseRecorder db_ init failed, full_path: " << full_path_name_);
    record_status_ = RecordStatus::STOP;
    return false;
}

bool BaseRecorder::reinit() {
    return init_();
}

void BaseRecorder::stop() {
    record_status_ = RecordStatus::STOP;
}

void BaseRecorder::set_record_all_topic() {
    mode_ = RecordMode::RECORDMODE_ALL;
}

void BaseRecorder::set_record_topic(const std::vector<std::string>& topic_list_vec) {
    mode_ = RecordMode::RECORDMODE_VECTOR;
    topic_list_vec_ = topic_list_vec;
}

void BaseRecorder::set_file_path(const std::string& path) {
    path_ = path;
    full_path_name_ = path_ + "/" + file_name_;
}

void BaseRecorder::set_file_name(const std::string& file_name) {
    file_name_ = file_name;
    full_path_name_ = path_ + "/" + file_name_;
}

void BaseRecorder::set_file_limit(uint64_t limit_size, uint64_t limit_package_size) {
    limit_type_ = LimitType::LIMITTYPE_SIZE;
    limit_size_ = limit_size;
    if (limit_package_size == 0) {
        limit_package_size_ = limit_size;
    } else {
        limit_package_size_ = limit_package_size;
    }
}

void BaseRecorder::set_time_limit(uint64_t limit_time, uint64_t limit_package_time) {
    limit_type_ = LimitType::LIMITTYPE_TIME;
    limit_time_ = limit_time;
    if (limit_package_time == 0) {
        limit_package_time_ = limit_time;
    } else {
        limit_package_time_ = limit_package_time;
    }
}

void BaseRecorder::set_all_limit(uint64_t limit_time, uint64_t limit_size, uint64_t limit_package_time,
                                 uint64_t limit_package_size) {

    limit_type_ = LimitType::LIMITTYPE_ALL;
    limit_time_ = limit_time;
    limit_size_ = limit_size;
    if (limit_package_time == 0) {
        limit_package_time_ = limit_time;
    } else {
        limit_package_time_ = limit_package_time;
    }
    if (limit_package_size == 0) {
        limit_package_size_ = limit_size;
    } else {
        limit_package_size_ = limit_package_size;
    }
}

void BaseRecorder::process_msg_() {
    while (thread_runnging_.load()) {

        if ((db_ == nullptr) ||
            (meta_data_queue_.empty() && raw_data_queue_.empty() && update_timestamp_queue_.empty())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::lock_guard<std::mutex> guard(lock_);
        DbOperator::DbMetaDataInfo meta_info;
        while (meta_data_queue_.try_pop(meta_info)) {
            if (false == db_->add_metadata(meta_info)) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        DbOperator::UpdateTimestampInfo update_info;
        while (update_timestamp_queue_.try_pop(update_info)) {
            if (false == db_->update_metadata_timestamp(update_info)) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::vector<DbOperator::DbRawDataInfo> raw_info_vec;
        while (raw_data_queue_.try_pop_batch(raw_info_vec, BATCH_SIZE)) {
            if (false == db_->add_rawdata(raw_info_vec)) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (record_status_ == RecordStatus::STOP) {
            DbOperator::DbRawDataInfo raw_info;
            while (raw_data_queue_.try_pop(raw_info)) {
                if (false == db_->add_rawdata(raw_info)) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            for (auto& end_timestamp_info : end_timestamp_infos_) {
                if (false == db_->update_metadata_timestamp(end_timestamp_info.second)) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string BaseRecorder::get_current_path_() {
    char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        logError_("VBSTOOLSDK", "BaseRecorder getcwd failed");
        return std::string("");
    }
    return std::string(cwd);
}

bool BaseRecorder::generate_file_name_() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    path_ = get_current_path_();
    if (path_ == "") {
        logError_("VBSTOOLSDK", "VbsRecorder path_ is empty");
        return false;
    }

    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d_%H_%M_%S");
    ss << ".dat";
    file_name_ = ss.str();

    full_path_name_ = path_ + "/" + file_name_;
    return true;
}

}  // namespace vbstoolsdk
