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

#include "ping/VbsParticipant.hpp"

#include <vbs/DomainParticipantFactory.hpp>
#include <elog/Log.hpp>

namespace vbstoolsdk {

VbsParticipant::VbsParticipant(int32_t domain, const std::string& xml_file_name, const std::string& participant_profile)
    : participant_(nullptr), stop_(false), samples_(0), participant_listener_(nullptr) {
    if (false == create_participant_(domain, xml_file_name, participant_profile)) {
        return;
    }
}

VbsParticipant::~VbsParticipant() {
    if (participant_ != nullptr) {
        delete participant_;
    }
    if (participant_listener_ != nullptr) {
        delete participant_listener_;
    }
}

void VbsParticipant::stop() {
    stop_ = true;
}

bool VbsParticipant::get_stop_status() {
    return stop_;
}

uint64_t VbsParticipant::return_samples() {
    return samples_;
}

bool VbsParticipant::create_participant_(int32_t domain, const std::string& xml_file_name,
                                         const std::string& participant_profile) {
    participant_listener_ = new ParticipantListener(this);
    if (domain == -1) {
        try {
            participant_ = vbs::DomainParticipantFactory::create_participant(xml_file_name, participant_profile,
                                                                             participant_listener_);
        } catch (const std::exception& e) {
            logError_("VBSTOOLSDK", "exception occurs : " << e.what());
        }
    } else {
        try {
            participant_ = vbs::DomainParticipantFactory::create_participant(domain, xml_file_name, participant_profile,
                                                                             participant_listener_);
        } catch (const std::exception& e) {
            logError_("VBSTOOLSDK", "exception occurs : " << e.what());
        }
    }
    if (participant_ == nullptr) {
        logError_("VBSTOOLSDK", "VbsParticipant participant_ is nullptr");
        return false;
    }
    participant_->register_remote_topic();
    return true;
}

void VbsParticipant::ParticipantListener::on_type_info(vbs::DomainParticipant* participant,
                                                       const std::string& type_name,
                                                       const vbs::DynamicTypeInformation& type_info) {
    logInfo_("VBSTOOLSDK", "TypeDynamicDiscovery on_type_discovery type_name: " << type_name << ", topic name: "
                                                                                << type_info.get_topic_name());

    vbs::VBSDynamicType dyn_type = participant->get_dynamic_type(type_name);
    std::string topic_name = type_info.get_topic_name();
    parent_->topic_dyntype_.emplace(topic_name, dyn_type);
}

}  // namespace vbstoolsdk
