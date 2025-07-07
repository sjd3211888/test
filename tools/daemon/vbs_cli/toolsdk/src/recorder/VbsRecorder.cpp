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

#include <iostream>
#include <chrono>

#include <vbs/DomainParticipantFactory.hpp>
#include <elog/Log.hpp>
#include "recorder/VbsRecorder.hpp"

namespace vbstoolsdk {

VbsRecorder::VbsRecorder(const std::string& xml_file_name, const std::string& participant_profile,
                         const std::string& reader_profile)
    : BaseRecorder(RecordMode::RECORDMODE_ALL),
      xml_file_name_(xml_file_name),
      participant_profile_(participant_profile),
      reader_profile_(reader_profile),
      participant_listener_(nullptr),
      read_listener_(nullptr) {
    if (false == init_()) {
        logError_("VBSTOOLSDK", "VbsRecorder init failed");
    }
}

VbsRecorder::VbsRecorder(const std::vector<uint32_t>& domains, const std::string& xml_file_name,
                         const std::string& participant_profile, const std::string& reader_profile)
    : BaseRecorder(RecordMode::RECORDMODE_ALL, domains),
      xml_file_name_(xml_file_name),
      participant_profile_(participant_profile),
      reader_profile_(reader_profile),
      participant_listener_(nullptr),
      read_listener_(nullptr) {
    if (false == init_()) {
        logError_("VBSTOOLSDK", "VbsRecorder init failed");
    }
}

VbsRecorder::VbsRecorder(const std::string& xml_file_name, const std::string& participant_profile,
                         const std::string& reader_profile, const std::vector<std::string>& topic_list_vec)
    : BaseRecorder(RecordMode::RECORDMODE_VECTOR, topic_list_vec),
      xml_file_name_(xml_file_name),
      participant_profile_(participant_profile),
      reader_profile_(reader_profile),
      participant_listener_(nullptr),
      read_listener_(nullptr) {
    if (false == init_()) {
        logError_("VBSTOOLSDK", "VbsRecorder init failed");
    }
}

VbsRecorder::VbsRecorder(const std::vector<uint32_t>& domains, const std::string& xml_file_name,
                         const std::string& participant_profile, const std::string& reader_profile,
                         const std::vector<std::string>& topic_list_vec)
    : BaseRecorder(RecordMode::RECORDMODE_VECTOR, domains, topic_list_vec),
      xml_file_name_(xml_file_name),
      participant_profile_(participant_profile),
      reader_profile_(reader_profile),
      participant_listener_(nullptr),
      read_listener_(nullptr) {
    if (false == init_()) {
        logError_("VBSTOOLSDK", "VbsRecorder init failed");
    }
}

VbsRecorder::~VbsRecorder() {
    thread_runnging_ = false;
    if (process_thread_.joinable()) {
        process_thread_.join();
    }
    reset_();
}

bool VbsRecorder::register_topic_() {
    for (auto& participant : participants_) {
        if (mode_ == RecordMode::RECORDMODE_ALL) {
            participant->register_remote_topic();
        } else if (mode_ == RecordMode::RECORDMODE_VECTOR) {
            for (auto& topic_name : topic_list_vec_) {
                participant->register_remote_topic(topic_name);
            }
        } else if (mode_ == RecordMode::RECORDMODE_MAP) {
            //map
        } else {
            continue;
        }
    }
    return true;
}

bool VbsRecorder::create_participant_(const std::string& xml_file_name, const std::string& participant_profile) {
    vbs::DomainParticipant* participant = nullptr;
    try {
        participant = vbs::DomainParticipantFactory::create_participant(xml_file_name, participant_profile,
                                                                        participant_listener_);
    } catch (const std::exception& e) {
        logError_("VBSTOOLSDK", "exception occurs : " << e.what());
        return false;
    }

    if (participant == nullptr) {
        logError_("VBSTOOLSDK", "VbsRecorder participant_ is nullptr");
        return false;
    }
    participants_.emplace_back(std::move(participant));
    return true;
}

bool VbsRecorder::create_participant_(std::vector<uint32_t> domains, const std::string& xml_file_name,
                                      const std::string& participant_profile) {
    for (auto& domain : domains) {
        vbs::DomainParticipant* participant = vbs::DomainParticipantFactory::create_participant(
            domain, xml_file_name, participant_profile, participant_listener_);
        if (participant == nullptr) {
            logError_("VBSTOOLSDK", "VbsRecorder participant_ is nullptr, domain: " << domain);
            return false;
        }
        participants_.emplace_back(std::move(participant));
    }
    return true;
}

bool VbsRecorder::init_() {
    topic_id_seed_ = 1;
    reset_();

    participant_listener_ = new ParticipantListener(this);
    read_listener_ = new SubListener(this);

    if (domains_.size() == 0) {
        if (create_participant_(xml_file_name_, participant_profile_) == false) {
            logError_("VBSTOOLSDK", "VbsRecorder create_participant_ failed");
            return false;
        }
    } else {
        if (create_participant_(domains_, xml_file_name_, participant_profile_) == false) {
            logError_("VBSTOOLSDK", "VbsRecorder create_participant_ failed");
            return false;
        }
    }
    register_topic_();

    if (generate_file_name_() == false) {
        logError_("VBSTOOLSDK", "VbsRecorder generate_file_name_ failed");
        return false;
    }

    record_status_ = RecordStatus::STOP;
    return true;
}

void VbsRecorder::reset_() {
    for (auto& participant : participants_) {
        if (participant != nullptr) {
            for (uint32_t i = 0; i < readers_.size(); i++) {
                topics_[i]->delete_data(datas_[readers_[i]]);
                participant->delete_data_reader(readers_[i]);
                participant->delete_topic(topics_[i]);
            }

            delete participant;
        }
    }
    participants_.clear();
    readers_.clear();
    topics_.clear();
    topic_info_.clear();
    datas_.clear();

    if (read_listener_ != nullptr) {
        delete read_listener_;
    }
    if (participant_listener_ != nullptr) {
        delete participant_listener_;
    }

    if (db_ != nullptr) {
        std::lock_guard<std::mutex> guard(lock_);
        db_.reset();
    }
}

void VbsRecorder::ParticipantListener::on_type_info(vbs::DomainParticipant* participant, const std::string& type_name,
                                                    const vbs::DynamicTypeInformation& type_info) {

    logInfo_("VBSTOOLSDK", "TypeDynamicDiscovery on_type_discovery type_name: " << type_name << ", topic name: "
                                                                                << type_info.get_topic_name());

    vbs::VBSDynamicType dyn_type = participant->get_dynamic_type(type_name);
    vbs::TypeSupport type = dyn_type.type();
    std::string topic_name = type_info.get_topic_name();
    vbs::Topic* topic = participant->create_topic(topic_name, type_name, type);
    if (topic == nullptr) {
        logError_("VBSTOOLSDK", "VbsRecorder topic is nullptr");
        return;
    }
    vbs::DataReader* reader = participant->create_data_reader(topic, sub_->reader_profile_, sub_->read_listener_);
    if (reader == nullptr) {
        logError_("VBSTOOLSDK", "VbsRecorder reader is nullptr");
        return;
    }
    vbs::VBSDynamicData* data = topic->create_data();

    sub_->topics_.emplace_back(topic);
    sub_->readers_.emplace_back(reader);
    sub_->datas_[reader] = data;

    DbOperator::DbMetaDataInfo meta_info;
    TopicInfo topic_info;
    meta_info.topic_id = sub_->topic_id_seed_++;
    meta_info.topic_name = type_info.get_topic_name();
    meta_info.data_type = "dds";
    meta_info.topic_type_name = type_info.get_type_name();
    vbs::VBSDynamicType::print_idl(dyn_type, meta_info.topic_idl);
    meta_info.topic_idl_xml = dyn_type.generate_xml();
    meta_info.domain_id = participant->get_domain_id();

    topic_info.topic_id = meta_info.topic_id;
    topic_info.topic_name = type_info.get_topic_name();
    topic_info.start_timestamp = 0;
    topic_info.type_name = type_info.get_type_name();
    sub_->topic_info_[reader] = std::move(topic_info);
    sub_->meta_data_queue_.push(std::move(meta_info));
}

void VbsRecorder::SubListener::on_data_available(vbs::DataReader* reader) {
    if (sub_->record_status_ != RecordStatus::START) {
        return;
    }

    TopicInfo topic_info = sub_->topic_info_[reader];
    vbs::VBSDynamicData* data = sub_->datas_[reader];
    vbs::LoanableCollectionPtr<void*> data_values;
    vbs::SampleInfo sample_infos;
    reader->take_buffer(&data_values, &sample_infos);

    int64_t start_time = sample_infos.reception_timestamp(0);
    int64_t end_time;
    try {
        end_time = sample_infos.reception_timestamp(data_values.length() - 1);
    } catch (...) {
        logError_("VBSTOOLSDK", "reception_timestamp out of range !");
        return;
    }

    if (topic_info.start_timestamp == 0) {
        DbOperator::UpdateTimestampInfo start_timestamp_info;

        topic_info.start_timestamp = start_time;
        start_timestamp_info.key = "start_timestamp";
        start_timestamp_info.value = start_time;
        start_timestamp_info.topic_name = topic_info.topic_name;
        sub_->update_timestamp_queue_.push(std::move(start_timestamp_info));
    }

    DbOperator::UpdateTimestampInfo end_timestamp_info;
    end_timestamp_info.key = "end_timestamp";
    end_timestamp_info.value = end_time;
    end_timestamp_info.topic_name = topic_info.topic_name;
    sub_->end_timestamp_infos_[topic_info.topic_id] = std::move(end_timestamp_info);

    for (int i = 0; i < static_cast<int>(data_values.length()); ++i) {
        DbOperator::DbRawDataInfo rawdata_info;
        rawdata_info.topic_id = topic_info.topic_id;
        rawdata_info.sequence_number = sample_infos.publication_sequence_number(i);
        rawdata_info.raw_timestamp = sample_infos.source_timestamp(i);
        rawdata_info.local_timestamp = sample_infos.reception_timestamp(i);
        rawdata_info.raw_data.resize(sample_infos.payload_len(i));
        memcpy(rawdata_info.raw_data.data(), data_values[i], sample_infos.payload_len(i));

        uint8_t* buffer = reinterpret_cast<uint8_t*>(data_values[i]);
        try {
            data->deserialize(buffer, sample_infos.payload_len(i));
        } catch (std::exception& e) {
            logError_("VBSTOOLSDK", "exception occurs : " << e.what());
        }
        rawdata_info.raw_json = vbs::Topic::to_string(data);
        sub_->raw_data_queue_.push(std::move(rawdata_info));
    }
    //update start_timestamp
    sub_->topic_info_[reader] = std::move(topic_info);
}

}  // namespace vbstoolsdk
