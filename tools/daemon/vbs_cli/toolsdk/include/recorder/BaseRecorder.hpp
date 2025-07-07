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

#ifndef BASE_RECORDER_HPP_
#define BASE_RECORDER_HPP_

#include <cstdint>
#include <atomic>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include "utils/ThreadSafeQueue.hpp"
#include "utils/DbOperator.hpp"

namespace vbstoolsdk {

class BaseRecorder {
 public:
    static constexpr int16_t BATCH_SIZE = 16;

    enum RecordMode : uint8_t { RECORDMODE_ALL = 0, RECORDMODE_VECTOR, RECORDMODE_MAP };

    enum LimitType : uint8_t { LIMITTYPE_NONE = 0, LIMITTYPE_SIZE, LIMITTYPE_TIME, LIMITTYPE_ALL };

    enum RecordStatus : uint8_t { START, PAUSE, STOP };

    BaseRecorder(RecordMode mode)
        : thread_runnging_(true),
          process_thread_(std::thread(&BaseRecorder::process_msg_, this)),
          record_status_(RecordStatus::STOP),
          mode_(mode),
          limit_type_(LimitType::LIMITTYPE_NONE) {}

    BaseRecorder(RecordMode mode, const std::vector<uint32_t>& domains)
        : thread_runnging_(true),
          process_thread_(std::thread(&BaseRecorder::process_msg_, this)),
          record_status_(RecordStatus::STOP),
          mode_(mode),
          limit_type_(LimitType::LIMITTYPE_NONE),
          domains_(domains) {}

    BaseRecorder(RecordMode mode, const std::vector<std::string>& topic_list_vec)
        : thread_runnging_(true),
          process_thread_(std::thread(&BaseRecorder::process_msg_, this)),
          record_status_(RecordStatus::STOP),
          mode_(mode),
          limit_type_(LimitType::LIMITTYPE_NONE),
          topic_list_vec_(topic_list_vec) {}

    BaseRecorder(RecordMode mode, const std::vector<uint32_t>& domains, const std::vector<std::string>& topic_list_vec)
        : thread_runnging_(true),
          process_thread_(std::thread(&BaseRecorder::process_msg_, this)),
          record_status_(RecordStatus::STOP),
          mode_(mode),
          limit_type_(LimitType::LIMITTYPE_NONE),
          topic_list_vec_(topic_list_vec),
          domains_(domains) {}

    virtual ~BaseRecorder() {}

    bool start();
    bool reinit();
    void stop();
    void set_record_all_topic();
    void set_record_topic(const std::vector<std::string>& topic_list_vec);
    void set_file_path(const std::string& path);
    void set_file_name(const std::string& file_name);
    void set_file_limit(uint64_t limit_size, uint64_t limit_package_size);
    void set_time_limit(uint64_t limit_time, uint64_t limit_package_time);
    void set_all_limit(uint64_t limit_time, uint64_t limit_size, uint64_t limit_package_time = 0,
                       uint64_t limit_package_size = 0);

 protected:
    virtual bool init_() = 0;
    virtual void reset_() = 0;
    void process_msg_();
    std::string get_current_path_();
    bool generate_file_name_();

    std::atomic_int16_t topic_id_seed_;
    std::atomic_bool thread_runnging_;
    std::thread process_thread_;
    RecordStatus record_status_;
    RecordMode mode_;
    LimitType limit_type_;
    uint64_t limit_size_;
    uint64_t limit_package_size_;
    uint64_t limit_time_;
    uint64_t limit_package_time_;
    std::string path_;
    std::string file_name_;
    std::string full_path_name_;
    std::vector<std::string> topic_list_vec_;

    std::vector<uint32_t> domains_;
    std::mutex lock_;
    std::shared_ptr<DbOperator> db_;
    ThreadSafeQueue<DbOperator::DbRawDataInfo> raw_data_queue_;
    ThreadSafeQueue<DbOperator::DbMetaDataInfo> meta_data_queue_;
    ThreadSafeQueue<DbOperator::UpdateTimestampInfo> update_timestamp_queue_;
    std::map<int, DbOperator::UpdateTimestampInfo> end_timestamp_infos_;
};

}  // namespace vbstoolsdk

#endif
