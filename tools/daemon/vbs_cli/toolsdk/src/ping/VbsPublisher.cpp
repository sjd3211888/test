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

#include <csignal>
#include <chrono>
#include <thread>
#include <fstream>
#include <map>
#include "nlohmann/json.hpp"
#include "vbs/types/VBSDynamicTypeMember.h"
#include "ping/VbsPublisher.hpp"

namespace vbstoolsdk {

VbsPublisher::VbsPublisher(const std::string& xml_file_name, const std::string& participant_profile,
                           const std::string& writer_profile, const std::string& topic_name, int32_t domain,
                           const std::string& type_xml, const std::string& type_name, const std::string& json_path)
    : VbsParticipant(domain, xml_file_name, participant_profile),
      writer_profile_(writer_profile),
      topic_name_(topic_name),
      type_xml_(type_xml),
      type_name_(type_name),
      json_path_(json_path),
      topic_(nullptr),
      writer_(nullptr),
      dynamic_data_ptr_(nullptr),
      pub_listener_(nullptr) {}

VbsPublisher::~VbsPublisher() {
    if (thread_.joinable()) {
        thread_.join();
    }
    if (participant_ == nullptr) {
        return;
    }

    if (writer_ != nullptr) {
        participant_->delete_data_writer(writer_);
    }
    if (topic_ != nullptr) {
        if (dynamic_data_ptr_ != nullptr) {
            topic_->delete_data(dynamic_data_ptr_);
        }
        participant_->delete_topic(topic_);
    }
    if (pub_listener_ != nullptr) {
        delete pub_listener_;
    }
}

void VbsPublisher::run(uint32_t samples, uint32_t sleep) {
    stop_ = false;
    if (false == create_writer_()) {
        return;
    }
    thread_ = std::thread(&VbsPublisher::run_thread_, this, samples, sleep);
}

bool VbsPublisher::create_writer_() {
    if (type_xml_.empty()) {
        auto it = topic_dyntype_.find(topic_name_);
        if (it == topic_dyntype_.end()) {
            logError_("VBSTOOLSDK", "type_xml_ empty and not discovered");
            return false;
        }

        dynamic_type_ = it->second;
        return create_writer_base_(topic_name_, dynamic_type_);
    }

    vbs::VBSDynamicData::load_xml(type_xml_);
    dynamic_type_ = vbs::VBSDynamicData::get_dynamic_type(type_name_);
    return create_writer_base_(topic_name_, dynamic_type_);
}

bool VbsPublisher::create_writer_base_(const std::string topic_name, vbs::VBSDynamicType& dynamic_type) {
    if (participant_ == nullptr) {
        return false;
    }
    topic_ = participant_->create_topic(topic_name, dynamic_type.get_name(), dynamic_type.type());
    if (topic_ == nullptr) {
        logError_("VBSTOOLSDK", "topic_ create failed");
        return false;
    }
    pub_listener_ = new PubListener();
    writer_ = participant_->create_data_writer(topic_, writer_profile_, pub_listener_);
    if (writer_ == nullptr) {
        logError_("VBSTOOLSDK", "writer_ create failed");
        return false;
    }
    dynamic_data_ptr_ = topic_->create_data();
    if (json_path_.empty()) {
        if (false == create_json_file_(dynamic_type.get_name())) {
            return false;
        }
    }

    return true;
}

bool VbsPublisher::create_json_file_(const std::string& type_name) {
    std::string file_name = type_name + ".json";
    std::ofstream outfile(file_name);

    if (!outfile.is_open()) {
        logError_("VBSTOOLSDK", "file: " << file_name << " open failed");
        return false;
    }

    outfile << vbs::Topic::to_string(dynamic_data_ptr_);
    outfile.close();
    return true;
}

void VbsPublisher::publish_() {
    if (!json_path_.empty()) {
        nlohmann::json json_data;
        std::ifstream json_file(json_path_);
        if (!json_file.good()) {
            logError_("VBSTOOLSDK", "writer_ create failed");
            return;
        }
        json_file >> json_data;
        json_file.close();

        if (!set_data_by_json_(dynamic_type_, dynamic_data_ptr_, json_data)) {
            return;
        }
    }

    if (true == writer_->write(dynamic_data_ptr_)) {
        logInfo_("VBSTOOLSDK", "publisher success");
    } else {
        logError_("VBSTOOLSDK", "publisher failed");
    }
}

bool VbsPublisher::set_data_by_json_(vbs::VBSDynamicType& type, vbs::VBSDynamicData* data, nlohmann::json& json_data) {
    vbs::TypeKind member_kind = vbs::TypeKind::TK_NONE;
    vbs::VBSDynamicData* member_data;
    std::map<uint32_t, std::shared_ptr<vbs::VBSDynamicTypeMember>> members;

    type.get_all_members(members);
    for (auto member : members) {
        member_kind = member.second->get_kind();
        if (member_kind >= vbs::TypeKind::TK_ANNOTATION && member_kind <= vbs::TypeKind::TK_BITSET) {
            member_data = data->loan_value(member.first);
            vbs::VBSDynamicType sub_type = member.second->get_type();
            set_data_by_json_(sub_type, member_data, json_data[member.second->get_name().c_str()]);
            data->return_loaned_value(member_data);
            continue;
        } else if (member_kind >= vbs::TypeKind::TK_SEQUENCE && member_kind <= vbs::TypeKind::TK_ARRAY) {
            member_data = data->loan_value(member.first);
            vbs::TypeKind sub_kind = member_data->get_type().get_element_type().get_kind();
            if (false == parse_array_and_set_(member.second->get_name().c_str(), sub_kind, member_data, json_data)) {
                data->return_loaned_value(member_data);
                return false;
            }
            data->return_loaned_value(member_data);
            continue;
        }

        if (false ==
            parse_and_set_(member.second->get_name().c_str(), member_kind, member.first, data, json_data, false)) {
            return false;
        }
    }

    return true;
}

bool VbsPublisher::parse_array_and_set_(const std::string& key, vbs::TypeKind& member_kind, vbs::VBSDynamicData* data,
                                        nlohmann::json& json_data) {
    for (uint32_t i = 0; i < json_data.at(key).size(); ++i) {
        if (false == parse_and_set_(key, member_kind, i, data, json_data, true)) {
            return false;
        }
    }
    return true;
}

bool VbsPublisher::parse_and_set_(std::string key, vbs::TypeKind& member_kind, uint32_t member_id,
                                  vbs::VBSDynamicData* data, nlohmann::json& json_data, bool is_array) {
    try {
        switch (member_kind) {
            case vbs::TypeKind::TK_CHAR8:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_char8_value(get_json_value_<char>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_char8_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_CHAR16:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_char16_value(get_json_value_<char16_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_char8_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_STRING8:
            case vbs::TypeKind::TK_STRING16:
            case vbs::TypeKind::TK_ALIAS:
            case vbs::TypeKind::TK_ENUM:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_string_value(get_json_value_<std::string>(key, member_id, json_data, is_array),
                                           member_id)) {
                    logError_("VBSTOOLSDK", "set_char8_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_BOOLEAN:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_bool_value(get_json_value_<bool>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_bool_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_BYTE:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_byte_value(get_json_value_<uint8_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_byte_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_INT16:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_int16_value(get_json_value_<int16_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_int16_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_INT32:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_int32_value(get_json_value_<int32_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_int32_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_INT64:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_int64_value(get_json_value_<int64_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_int64_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_UINT16:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_uint16_value(get_json_value_<uint16_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_uint16_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_UINT32:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_uint32_value(get_json_value_<uint32_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_uint32_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_UINT64:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_uint64_value(get_json_value_<uint64_t>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_uint64_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_FLOAT32:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_float32_value(get_json_value_<float>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_float32_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_FLOAT64:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_float64_value(get_json_value_<double>(key, member_id, json_data, is_array), member_id)) {
                    logError_("VBSTOOLSDK", "set_float64_value failed");
                    return false;
                }
                break;
            case vbs::TypeKind::TK_FLOAT128:
                if (vbs::ReturnCode_t::RETCODE_OK !=
                    data->set_float128_value(get_json_value_<long double>(key, member_id, json_data, is_array),
                                             member_id)) {
                    logError_("VBSTOOLSDK", "set_float128_value failed");
                    return false;
                }
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        logError_("VBSTOOLSDK", e.what() << ", key: " << key);
        throw "parse_json_ failed";
    }
    return true;
}

template <typename T>
T VbsPublisher::get_json_value_(std::string key, uint32_t member_id, nlohmann::json& json_data, bool is_array) {
    T value;
    if (is_array) {
        value = json_data.at(key).at(member_id).get<T>();
    } else {
        value = json_data.at(key).get<T>();
    }
    return value;
}

void VbsPublisher::run_thread_(uint32_t samples, uint32_t sleep) {
    while (!stop_ && (samples == 0 || samples_ < samples)) {
        while (pub_listener_->get_matched_num() == 0) {
            if (stop_) {
                return;
            }
            logError_("VBSTOOLSDK", "wait matched");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        logInfo_("VBSTOOLSDK", "VbsPublisher send " << samples_ << " times");
        publish_();
        samples_++;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }
    stop();
}

void VbsPublisher::PubListener::on_publication_matched(vbs::DataWriter* writer,
                                                       const vbs::PublicationMatchedStatus& info) {
    (void)writer;
    if (info.current_count_change() == 1) {
        matched_++;
        logInfo_("VBSTOOLSDK", "VbsPublisher matched, match num: " << matched_);
    } else if (info.current_count_change() == -1) {
        matched_--;
        logInfo_("VBSTOOLSDK", "VbsPublisher unmatched, match num: " << matched_);
    }
}

}  // namespace vbstoolsdk
