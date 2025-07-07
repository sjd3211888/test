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

#ifndef VBS_REPLAYER_HPP_
#define VBS_REPLAYER_HPP_

#include <string>
#include <atomic>
#include <thread>

namespace vbstoolsdk {

class VbsReplayer {
 public:
    enum ReplayStatus : uint8_t { START = 0, PAUSE, STOP };

    VbsReplayer(const std::string& path, const std::string& xml_file_name, const std::string& participant_profile,
                const std::string& writer_profile, int domain_id, bool rti_flag)
        : path_(path),
          xml_file_name_(xml_file_name),
          participant_profile_(participant_profile),
          writer_profile_(writer_profile),
          domain_id_(domain_id),
          rti_flag_(rti_flag) {}
    virtual ~VbsReplayer() {}

    virtual bool reinit() = 0;
    virtual bool reinit(const std::string& path) = 0;
    void start();
    void pause();
    void stop();
    void set_loop(bool loop);
    void set_loop_times(int32_t loop_times);
    float get_progress();
    ReplayStatus get_stop_status();

 protected:
    virtual void process_msg_() = 0;
    virtual void control_speed_(uint64_t last, uint64_t cur) = 0;
    virtual void reset_() = 0;

    std::string path_;
    std::string xml_file_name_;
    std::string participant_profile_;
    std::string writer_profile_;
    int domain_id_;
    bool rti_flag_;
    ReplayStatus replay_status_ {ReplayStatus::STOP};
    std::atomic_bool replay_loop_ {false};
    std::atomic_int32_t loop_times_ {0};
    int file_cur_size_ {0};
    int file_total_size_ {0};
    std::thread process_thread_;
};

}  // namespace vbstoolsdk

#endif
