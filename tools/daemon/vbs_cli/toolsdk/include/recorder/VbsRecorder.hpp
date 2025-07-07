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

#ifndef VBS_RECORDER_HPP_
#define VBS_RECORDER_HPP_

#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>

#include <vbs/DomainParticipant.hpp>
#include <vbs/DataReader.hpp>
#include <vbs/DataReaderListener.hpp>
#include <vbs/Global.hpp>
#include <vbs/DataReaderQos.hpp>
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/VBSDynamicTypeBuilderFactory.h"
#include "vbs/types/VBSDynamicDataFactory.h"
#include "vbs/types/VBSDynamicType.h"
#include "vbs/types/VBSDynamicTypeBuilder.h"

#include "recorder/BaseRecorder.hpp"

namespace vbstoolsdk {

class VbsRecorder : public BaseRecorder {
    friend class ParticipantListener;

 public:
    struct TopicInfo {
        int topic_id;
        std::string topic_name;
        std::string type_name;
        int64_t start_timestamp;
    };

    VbsRecorder(const std::string& xml_file_name, const std::string& participant_profile,
                const std::string& reader_profile);

    VbsRecorder(const std::vector<uint32_t>& domains, const std::string& xml_file_name,
                const std::string& participant_profile, const std::string& reader_profile);

    VbsRecorder(const std::string& xml_file_name, const std::string& participant_profile,
                const std::string& reader_profile, const std::vector<std::string>& topic_list_vec);

    VbsRecorder(const std::vector<uint32_t>& domains, const std::string& xml_file_name,
                const std::string& participant_profile, const std::string& reader_profile,
                const std::vector<std::string>& topic_list_vec);

    virtual ~VbsRecorder();

 private:
    bool init_() override;
    void reset_() override;
    bool register_topic_();
    bool create_participant_(const std::string& xml_file_name, const std::string& participant_profile);
    bool create_participant_(std::vector<uint32_t> domains, const std::string& xml_file_name,
                             const std::string& participant_profile);

    std::string xml_file_name_;
    std::string participant_profile_;
    std::string reader_profile_;

    std::vector<vbs::DomainParticipant*> participants_;
    std::vector<vbs::Topic*> topics_;
    std::vector<vbs::DataReader*> readers_;
    std::map<vbs::DataReader*, TopicInfo> topic_info_;
    std::map<vbs::DataReader*, vbs::VBSDynamicData*> datas_;

    class ParticipantListener : public vbs::DomainParticipantListener {
     public:
        ParticipantListener() {}
        ParticipantListener(VbsRecorder* sub) : sub_(sub) {}
        ~ParticipantListener() {}

        void on_type_info(vbs::DomainParticipant* participant, const std::string& type_name,
                          const vbs::DynamicTypeInformation& type_info) override;

        VbsRecorder* sub_;
    };
    ParticipantListener* participant_listener_;

    class SubListener : public vbs::DataReaderListener {
     public:
        SubListener() {}
        SubListener(VbsRecorder* sub) : sub_(sub) {}
        ~SubListener() override {}
        void on_data_available(vbs::DataReader* reader) override;

        VbsRecorder* sub_;
    };
    SubListener* read_listener_;
};

}  // namespace vbstoolsdk

#endif
