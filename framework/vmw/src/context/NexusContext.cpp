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

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>  // 主动包含新接口（必须在 windows.h 之前）
#include <ws2tcpip.h>  // 如果你有 IPv6 或 getaddrinfo 需求
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "context/NexusContext.hpp"
#include "context/NexusUnits.hpp"
#include "context/QosManager.hpp"
#include "domain/DomainParticipantImpl.hpp"

namespace vbs {
NexusContext::NexusContext() {
    qos_manager_ = new QosManager;
    nexus_units_ = new NexusUnits;
}

NexusContext::~NexusContext() {
    delete nexus_units_;
    nexus_units_ = nullptr;
    delete qos_manager_;
    qos_manager_ = nullptr;
}

void NexusContext::init_context() {
    std::call_once(context_init_flag_, []() {
        context_shared_ptr_ = std::shared_ptr<NexusContext>(new NexusContext);
        context_ = context_shared_ptr_.get();
    });
}

std::shared_ptr<NexusContext> NexusContext::get_context_shared() {
    init_context();
    return context_shared_ptr_;
}

NexusContext& NexusContext::get_context() {
    init_context();
    return *context_;
}

NexusUnits& NexusContext::get_unit() {
    return *nexus_units_;
}

QosManager* NexusContext::get_qos_manager() {
    return qos_manager_;
}

#if !defined(_WIN32)
/**
     * number[23:16] val:
     *   number[23:16] & 0x01 == 1,  trace control;
     *   number[23:16] & 0x02 == 1,  log control;
     * number[15:8] val:
     *   number[15:8] == 0,  disable trace;
     *   number[15:8] == 1,  enable trace;
     *   number[15:8] == 2,  dump history trace;
     * number[7:0] val:
     *   number[7:0] == 0, is vbsutil::elog::Log::Kind
     */
void NexusContext::obv_enable(uint32_t number) {
    uint8_t type = ((number >> 16) & 0xFF);
    uint8_t log_level = (number & 0xFF);

    if (type & 0x02) {
        vbsutil::elog::Log::SetVerbosity(log_level);
    }
}
#endif

void NexusContext::dump_topic_traffic() {
    if (nexus_units_) {
        nexus_units_->dump_topic_traffic(dump_topic_traffic_file_.c_str());
    }
}

std::once_flag NexusContext::context_init_flag_;
std::shared_ptr<NexusContext> NexusContext::context_shared_ptr_;
NexusContext* NexusContext::context_ = nullptr;

}  // namespace vbs
