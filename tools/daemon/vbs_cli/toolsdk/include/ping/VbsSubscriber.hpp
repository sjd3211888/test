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

#ifndef PING_VBS_SUBSCRIBER_HPP_
#define PING_VBS_SUBSCRIBER_HPP_

#include <string>
#include <fstream>
#include <vbs/Global.hpp>
#include <vbs/DataReader.hpp>
#include <vbs/DataReaderListener.hpp>
#include <vbs/types/VBSDynamicData.h>
#include <vbs/types/VBSDynamicType.h>
#include "ping/VbsParticipant.hpp"

namespace vbstoolsdk {

class VbsSubscriber : public VbsParticipant {
    friend class SubListener;

 public:
    VbsSubscriber(const std::string& xml_file_name, const std::string& participant_profile,
                  const std::string& reader_profile, const std::string& topic_name, int32_t domain = -1,
                  bool data_show_on = true, const std::string& type_xml = "", const std::string& type_name = "",
                  const std::string& json_path = "");
    ~VbsSubscriber();
    void run(uint32_t samples);

 protected:
    bool create_reader_();
    bool create_reader_base_(const std::string topic_name, vbs::VBSDynamicType& dynamic_type);

    std::string reader_profile_;
    std::string topic_name_;
    std::string type_xml_;
    std::string type_name_;
    std::string json_path_;
    std::ofstream out_file_;
    std::atomic_uint64_t max_sample_;
    vbs::Topic* topic_;
    vbs::DataReader* reader_;
    vbs::VBSDynamicData* dynamic_data_ptr_;
    vbs::VBSDynamicType dynamic_type_;
    bool data_show_on_;

    class SubListener : public vbs::DataReaderListener {
     public:
        SubListener() {}
        SubListener(VbsSubscriber* sub) : sub_(sub), matched_(0) {}
        ~SubListener() override {}
        void on_subscription_matched(vbs::DataReader*, const vbs::SubscriptionMatchedStatus& info) override;
        void on_data_available(vbs::DataReader* reader) override;

     protected:
        void compute_statistics_(const std::vector<int64_t>& rtt);

        VbsSubscriber* sub_;
        std::vector<int64_t> rtt_;
        std::atomic_int32_t matched_;
    };
    SubListener* sub_listener_;
};

}  // namespace vbstoolsdk
#endif
