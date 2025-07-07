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

#ifndef PING_VBS_PUBLISHER_HPP_
#define PING_VBS_PUBLISHER_HPP_

#include <atomic>
#include <map>
#include <thread>
#include <vbs/Global.hpp>
#include <vbs/DataWriter.hpp>
#include <vbs/DataWriterListener.hpp>
#include <vbs/types/VBSDynamicData.h>
#include <vbs/types/VBSDynamicType.h>
#include "nlohmann/json.hpp"
#include "ping/VbsParticipant.hpp"

namespace vbstoolsdk {

class VbsPublisher : public VbsParticipant {
 public:
    VbsPublisher(const std::string& xml_file_name, const std::string& participant_profile,
                 const std::string& writer_profile, const std::string& topic_name, int32_t domain = -1,
                 const std::string& type_xml = "", const std::string& type_name = "",
                 const std::string& json_path = "");
    virtual ~VbsPublisher();

    void run(uint32_t number, uint32_t sleep);

 protected:
    bool create_writer_();
    bool create_writer_base_(const std::string topic_name, vbs::VBSDynamicType& dynamic_type);
    void run_thread_(uint32_t samples, uint32_t sleep);
    void publish_();
    bool create_json_file_(const std::string& type_name);
    bool set_data_by_json_(vbs::VBSDynamicType& type, vbs::VBSDynamicData* data, nlohmann::json& json_data);
    bool parse_array_and_set_(const std::string& key, vbs::TypeKind& member_kind, vbs::VBSDynamicData* data,
                              nlohmann::json& json_data);
    bool parse_and_set_(std::string key, vbs::TypeKind& member_kind, uint32_t member_id, vbs::VBSDynamicData* data,
                        nlohmann::json& json_data, bool is_array);
    template <typename T>
    T get_json_value_(std::string key, uint32_t member_id, nlohmann::json& json_data, bool is_array);

    std::string writer_profile_;
    std::string topic_name_;
    std::string type_xml_;
    std::string type_name_;
    std::string json_path_;
    vbs::Topic* topic_;
    vbs::DataWriter* writer_;
    vbs::VBSDynamicData* dynamic_data_ptr_;
    vbs::VBSDynamicType dynamic_type_;
    std::thread thread_;

    class PubListener : public vbs::DataWriterListener {
     public:
        PubListener() : matched_(0) {}

        int32_t get_matched_num() { return matched_.load(); }

        void on_publication_matched(vbs::DataWriter* writer, const vbs::PublicationMatchedStatus& info) override;

     private:
        std::atomic_int32_t matched_;
    };
    PubListener* pub_listener_;
};

}  // namespace vbstoolsdk

#endif
