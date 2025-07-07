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

#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include "ping/VbsSubscriber.hpp"

namespace vbstoolsdk {

VbsSubscriber::VbsSubscriber(const std::string& xml_file_name, const std::string& participant_profile,
                             const std::string& reader_profile, const std::string& topic_name, int32_t domain,
                             bool data_show_on, const std::string& type_xml, const std::string& type_name,
                             const std::string& json_path)
    : VbsParticipant(domain, xml_file_name, participant_profile),
      reader_profile_(reader_profile),
      topic_name_(topic_name),
      type_xml_(type_xml),
      type_name_(type_name),
      json_path_(json_path),
      max_sample_(0),
      topic_(nullptr),
      reader_(nullptr),
      dynamic_data_ptr_(nullptr),
      data_show_on_(data_show_on),
      sub_listener_(nullptr) {
    if (json_path != "") {
        out_file_.open(json_path);
    }
}

VbsSubscriber::~VbsSubscriber() {
    stop_ = true;
    if (participant_ == nullptr) {
        return;
    }
    if (reader_ != nullptr) {
        participant_->delete_data_reader(reader_);
    }
    if (topic_ != nullptr) {
        if (dynamic_data_ptr_ != nullptr) {
            topic_->delete_data(dynamic_data_ptr_);
        }
        participant_->delete_topic(topic_);
    }
    if (sub_listener_ != nullptr) {
        delete sub_listener_;
    }
    out_file_.close();
}

void VbsSubscriber::run(uint32_t samples) {
    max_sample_ = samples;
    if (false == create_reader_()) {
        return;
    }
}

void VbsSubscriber::SubListener::on_subscription_matched(vbs::DataReader*, const vbs::SubscriptionMatchedStatus& info) {
    if (info.current_count_change() == 1) {
        matched_++;
        logInfo_("VBSTOOLSDK", "VbsSubscriber matched, match num: " << matched_);
    } else if (info.current_count_change() == -1) {
        matched_--;
        logInfo_("VBSTOOLSDK", "VbsSubscriber unmatched, match num: " << matched_);
    }
}

void VbsSubscriber::SubListener::on_data_available(vbs::DataReader* reader) {
    if (sub_->stop_) {
        return;
    }

    vbs::SampleInfo sample_info;
    if (reader->take_next_sample(sub_->dynamic_data_ptr_, sample_info)() == vbs::ReturnCode_t::RETCODE_OK) {
        rtt_.emplace_back(sample_info.reception_timestamp(0) - sample_info.source_timestamp(0));

        if (sub_->json_path_.empty() && sub_->data_show_on_) {
            std::cout << vbs::Topic::to_string(sub_->dynamic_data_ptr_) << std::endl;
        } else {
            if (sub_->out_file_.is_open()) {
                sub_->out_file_ << vbs::Topic::to_string(sub_->dynamic_data_ptr_);
            }
        }

        sub_->samples_++;
        if (sub_->max_sample_ > 0 && sub_->max_sample_ <= sub_->samples_) {
            compute_statistics_(rtt_);
            sub_->stop();
        }
    }
}

void VbsSubscriber::SubListener::compute_statistics_(const std::vector<int64_t>& rtt) {
    if (rtt.empty()) {
        std::cout << "Min RTT: 0 ns" << std::endl;
        std::cout << "Max RTT: 0 ns" << std::endl;
        std::cout << "Average RTT: 0 ns" << std::endl;
        std::cout << "Standard RTT: 0 ns" << std::endl;
        return;
    }
    int64_t min_rtt = *std::min_element(rtt.begin(), rtt.end());
    int64_t max_rtt = *std::max_element(rtt.begin(), rtt.end());

    int64_t sum = std::accumulate(rtt.begin(), rtt.end(), 0.0);
    double average_rtt = sum / rtt.size();

    double variance_sum = 0;
    std::for_each(rtt.begin(), rtt.end(),
                  [&](int64_t val) { variance_sum += (val - average_rtt) * (val - average_rtt); });

    double standard_rtt = std::sqrt(variance_sum / rtt.size());
    std::cout << "Min RTT: " << min_rtt << "ns" << std::endl;
    std::cout << "Max RTT: " << max_rtt << "ns" << std::endl;
    std::cout << "Average RTT: " << average_rtt << "ns" << std::endl;
    std::cout << "Standard RTT: " << standard_rtt << "ns" << std::endl;
}

bool VbsSubscriber::create_reader_() {
    if (type_xml_.empty()) {
        auto it = topic_dyntype_.find(topic_name_);
        if (it == topic_dyntype_.end()) {
            logError_("VBSTOOLSDK", "type_xml_ empty and not discovered");
            return false;
        }

        dynamic_type_ = it->second;
        return create_reader_base_(topic_name_, dynamic_type_);
    }

    vbs::VBSDynamicData::load_xml(type_xml_);
    dynamic_type_ = vbs::VBSDynamicData::get_dynamic_type(type_name_);
    return create_reader_base_(topic_name_, dynamic_type_);
}

bool VbsSubscriber::create_reader_base_(const std::string topic_name, vbs::VBSDynamicType& dynamic_type) {
    if (participant_ == nullptr) {
        return false;
    }
    topic_ = participant_->create_topic(topic_name, dynamic_type.get_name(), dynamic_type.type());
    if (topic_ == nullptr) {
        logError_("VBSTOOLSDK", "topic_ create failed");
        return false;
    }
    sub_listener_ = new SubListener(this);
    reader_ = participant_->create_data_reader(topic_, reader_profile_, sub_listener_);
    if (reader_ == nullptr) {
        logError_("VBSTOOLSDK", "reader_ create failed");
        return false;
    }
    dynamic_data_ptr_ = topic_->create_data();
    return true;
}

}  // namespace vbstoolsdk
