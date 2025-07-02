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

#ifndef VBS_ENV_MANAGER_HPP_
#define VBS_ENV_MANAGER_HPP_

#include <cstring>
#include <string>
#include <cstdlib>
#include <atomic>

#include "core/Macros.h"

namespace vbs {
class EnvManager {
 public:
    static EnvManager& get_instance();
    EnvManager();

    // Check if trace is active
    bool check_trace_active() { return trace_active_.load(); }

    void set_trace_active(bool flag) { trace_active_.store(flag); }

    // Check if dump topic traffic is enabled
    bool check_dump_topic_traffic_enabled() { return dump_topic_traffic_enabled_; }

    // Get dump topic traffic path
    const std::string& get_dump_topic_traffic_path() { return dump_topic_traffic_path_; }

    // Check if trace online is active
    bool check_trace_online_active() { return trace_online_active_; }

    // Get process info interval
    int get_process_info_interval() { return process_info_interval_; }

 private:
    std::atomic<bool> trace_active_ {false};
    bool dump_topic_traffic_enabled_;
    std::string dump_topic_traffic_path_;
    std::string shm_type_discovery_path_;
    bool trace_online_active_;
    int process_info_interval_;

    DISALLOW_COPY_AND_ASSIGN(EnvManager)
};
}  // namespace vbs

#endif  // VBS_ENV_MANAGER_HPP_
