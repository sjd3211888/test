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

#ifndef INCLUDE_CONTEXT_NEXUS_CONTEXT_H_
#define INCLUDE_CONTEXT_NEXUS_CONTEXT_H_

#include "context/NexusUnits.hpp"
#include "core/Macros.h"
#include "context/EnvManager.hpp"

#define VBS_DUMP_TOPIC_TRAFFIC_ENV "VBS_DUMP_TOPIC_TRAFFIC"
#define VBS_DUMP_TOPIC_TRAFFIC_PATH_ENV "VBS_DUMP_TOPIC_TRAFFIC_PATH"

namespace vbs {
class MsgFlowInfo;
class DomainParticipantImpl;
class QosManager;
class NexusContext {
    friend class TopicImpl;
    friend class DomainParticipant;
    friend class EnvManager;
    friend class QosManager;

 public:
    ~NexusContext();

    static void init_context();
    static std::shared_ptr<NexusContext> get_context_shared();
    static NexusContext& get_context();
    NexusUnits& get_unit();
    QosManager* get_qos_manager();
    void dump_topic_traffic();
#if !defined(_WIN32)
    void obv_enable(uint32_t number);
#endif

 private:
    NexusContext();

    static std::once_flag context_init_flag_;
    static std::shared_ptr<NexusContext> context_shared_ptr_;
    static NexusContext* context_;

    EnvManager env_manager_;
    QosManager* qos_manager_;
    NexusUnits* nexus_units_;

    std::string dump_topic_traffic_file_;

    DISALLOW_COPY_AND_ASSIGN(NexusContext)
};
}  // namespace vbs

#endif  // INCLUDE_CONTEXT_NEXUS_CONTEXT_H_
