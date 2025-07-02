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

#include "context/EnvManager.hpp"
#include "context/NexusContext.hpp"

namespace vbs {
EnvManager& EnvManager::get_instance() {
    return NexusContext::get_context().env_manager_;
}

EnvManager::EnvManager() {
    const char* env;
    env = std::getenv("VBSTRACER_ACTIVE");
    if (env) {
        if (!std::strcmp(env, "1") || !std::strcmp(env, "2")) {
            trace_active_.store(true);
        } else if (!std::strcmp(env, "0")) {
            trace_active_.store(false);
        }
    } else {
        trace_active_.store(false);
    }

    env = std::getenv("VBS_DUMP_TOPIC_TRAFFIC");
    dump_topic_traffic_enabled_ = env && !std::strcmp(env, "1");

    env = std::getenv("VBS_DUMP_TOPIC_TRAFFIC_PATH");
    dump_topic_traffic_path_ = env ? std::string(env) : "";

    env = std::getenv("VBSTRACER_ONLINE");
    trace_online_active_ = env && !std::strcmp(env, "1");

    env = std::getenv("VBSPROCESS_INFO_INTVAL");
    process_info_interval_ = env ? std::atoi(env) : 0;
}
}  // namespace vbs
