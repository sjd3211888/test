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

#ifndef PING_VBS_PARTICIPANT_HPP_
#define PING_VBS_PARTICIPANT_HPP_

#include <string>
#include <atomic>
#include <unordered_map>
#include <vbs/DomainParticipant.hpp>

namespace vbstoolsdk {

class VbsParticipant {
 public:
    VbsParticipant(int32_t domain, const std::string& xml_file_name, const std::string& participant_profile);
    virtual ~VbsParticipant();

    void stop();
    bool get_stop_status();
    uint64_t return_samples();

 protected:
    bool create_participant_(int32_t domain, const std::string& xml_file_name, const std::string& participant_profile);

    vbs::DomainParticipant* participant_;
    std::atomic_bool stop_;
    std::unordered_map<std::string, vbs::VBSDynamicType> topic_dyntype_;
    std::atomic_uint64_t samples_ {0};

    class ParticipantListener : public vbs::DomainParticipantListener {
     public:
        ParticipantListener() {}
        ParticipantListener(VbsParticipant* parent) : parent_(parent) {}
        ~ParticipantListener() {}

        void on_type_info(vbs::DomainParticipant* participant, const std::string& type_name,
                          const vbs::DynamicTypeInformation& type_info) override;

        VbsParticipant* parent_;
    };
    ParticipantListener* participant_listener_;
};

}  // namespace vbstoolsdk

#endif
