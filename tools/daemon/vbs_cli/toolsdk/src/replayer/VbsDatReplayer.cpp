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
#include "replayer/VbsDatReplayer.hpp"
#include "utils/XmlConverter.hpp"

namespace vbstoolsdk {

VbsDatReplayer::VbsDatReplayer(const std::string& path, const std::string& xml_file_name,
                               const std::string& participant_profile, const std::string& writer_profile, int domain_id,
                               bool rti_flag)
    : BaseDatReplayer(path, xml_file_name, participant_profile, writer_profile, domain_id, rti_flag),
      writer_listener_(nullptr) {
    if (false == init_(xml_file_name, participant_profile, writer_profile)) {
        logError_("VBSTOOLSDK", "VbsDatReplayer init failed");
    }
}

VbsDatReplayer::~VbsDatReplayer() {
    reset_();
}

void VbsDatReplayer::reset_() {
    stop();
    for (auto& writer : writers_) {
        vbs::DomainParticipant* participant;
        if (domain_id_ == -1) {
            participant = participants_.at(topicid_domain_.at(writer.first));
        } else {
            participant = participants_.at(domain_id_);
        }
        if (participant == nullptr) {
            continue;
        }
        participant->delete_data_writer(writer.second);
        participant->delete_topic(topics_.at(writer.first));
    }
    writers_.clear();
    topics_.clear();
    dyn_types_.clear();
    topicid_domain_.clear();

    if (writer_listener_ != nullptr) {
        delete writer_listener_;
    }

    for (auto& participant : participants_) {
        if (participant.second != nullptr) {
            delete participant.second;
        }
    }
    participants_.clear();
}

bool VbsDatReplayer::reinit() {
    return init_(xml_file_name_, participant_profile_, writer_profile_);
}

bool VbsDatReplayer::reinit(const std::string& path) {
    path_ = path;
    return init_(xml_file_name_, participant_profile_, writer_profile_);
}

bool VbsDatReplayer::init_(const std::string& xml_file_name, const std::string& participant_profile,
                           const std::string& writer_profile) {
    reset_();
    writer_listener_ = new PubListener();
    if (false == init_db_()) {
        return false;
    }

    for (auto& info : meta_info_) {
        int domain_temp;
        if (domain_id_ == -1) {
            domain_temp = info.domain_id;
            topicid_domain_[info.topic_id] = info.domain_id;
        } else {
            domain_temp = domain_id_;
        }
        auto it = participants_.find(domain_temp);
        vbs::DomainParticipant* participant;
        if (it == participants_.end()) {
            try {
                participant = create_participant_(domain_temp, xml_file_name, participant_profile);
            } catch (...) {
                logError_("VBSTOOLSDK", "VbsDatReplayer participant create occur exception !" << domain_temp);
                continue;
            }
        } else {
            participant = it->second;
        }

        if (participant == nullptr) {
            logError_("VBSTOOLSDK", "VbsDatReplayer participant is nullptr, domain id: " << domain_temp);
            continue;
        }
        if (false == create_writer_(participant, info, writer_profile)) {
            continue;
        }
    }

    return true;
}

vbs::DomainParticipant* VbsDatReplayer::create_participant_(const uint32_t domain_id, const std::string& xml_file_name,
                                                            const std::string& participant_profile) {
    vbs::DomainParticipant* participant =
        vbs::DomainParticipantFactory::create_participant(domain_id, xml_file_name, participant_profile);

    if (participant == nullptr) {
        logError_("VBSTOOLSDK", "VbsDatReplayer participant is nullptr, domain id: " << domain_id);
        return nullptr;
    }
    participants_.emplace(domain_id, participant);
    return participant;
}

bool VbsDatReplayer::create_writer_(vbs::DomainParticipant* participant, DbOperator::DbMetaDataInfo& info,
                                    const std::string& writer_profile) {

    if (rti_flag_) {
        std::string vbs_xml;
        if (!rtixml_to_vbsxml(info.topic_idl_xml, vbs_xml)) {
            logError_("VBSTOOLSDK", "rti xml convert to vbs xml error");
            return false;
        }
        vbs::VBSDynamicData::load_xml(vbs_xml.c_str(), vbs_xml.length());
    } else {
        vbs::VBSDynamicData::load_xml(info.topic_idl_xml.c_str(), info.topic_idl_xml.length());
    }
    vbs::VBSDynamicType dyn_type = vbs::VBSDynamicData::get_dynamic_type(info.topic_type_name);

    vbs::Topic* topic = participant->create_dynamic_type_topic(info.topic_name, info.topic_type_name, dyn_type);
    if (topic == nullptr) {
        logError_("VBSTOOLSDK", "VbsDatReplayer topic is nullptr, topic name: " << info.topic_name << " type name: "
                                                                                << info.topic_type_name);
        return false;
    }

    vbs::DataWriter* writer = participant->create_data_writer(topic, writer_profile, writer_listener_);
    if (writer == nullptr) {
        logError_("VBSTOOLSDK", "VbsDatReplayer writer is nullptr, topic name: " << info.topic_name);
        return false;
    }
    dyn_types_.emplace(info.topic_id, dyn_type);
    topics_.emplace(info.topic_id, topic);
    writers_.emplace(info.topic_id, writer);
    return true;
}

bool VbsDatReplayer::writer_(DbOperator::DbRawDataInfo& info) {
    if (writers_.find(info.topic_id) != writers_.end()) {
        return writers_.at(info.topic_id)->write_buffer(info.raw_data.data(), info.raw_data.size());
    } else {
        logError_("VBSTOOLSDK", "topic id: " << info.topic_id << " has no writer");
        return false;
    }
}

void VbsDatReplayer::PubListener::on_publication_matched(vbs::DataWriter*, const vbs::PublicationMatchedStatus& info) {
    if (info.current_count_change() == 1) {
        logInfo_("VBSTOOLSDK", "VbsDatReplayer Publisher matched");
    } else if (info.current_count_change() == -1) {
        logInfo_("VBSTOOLSDK", "VbsDatReplayer Publisher unmatched");
    } else {
        logInfo_("VBSTOOLSDK", "VbsDatReplayer Invaild value");
    }
}

}  // namespace vbstoolsdk
