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

#include "replayer/RtpsDatReplayer.hpp"
#include <elog/Log.hpp>
#include <edds/rtps/common/ChangeKind_t.hpp>

namespace vbstoolsdk {

RtpsDatReplayer::RtpsDatReplayer(const std::string& path) : BaseDatReplayer(path, "", "", "", -1, false) {
    init_();
}

RtpsDatReplayer::~RtpsDatReplayer() {
    reset_();
}

void RtpsDatReplayer::reset_() {
    stop();
    for (auto& writer : writers_) {
        if (writer.second != nullptr) {
            ERTPS_RTPS::RTPSDomain::removeRTPSWriter(writer.second);
        }
        if (historys_.at(writer.first) != nullptr) {
            delete historys_.at(writer.first);
        }
    }
    for (auto participant : participants_) {
        if (participant.second != nullptr) {
            ERTPS_RTPS::RTPSDomain::removeRTPSParticipant(participant.second);
        }
    }

    writers_.clear();
    historys_.clear();
    participants_.clear();
    topicid_domain_.clear();
}

bool RtpsDatReplayer::reinit() {
    return init_();
}

bool RtpsDatReplayer::reinit(const std::string& path) {
    path_ = path;
    return init_();
}

bool RtpsDatReplayer::init_() {
    reset_();
    if (false == init_db_()) {
        return false;
    }

    for (auto& info : meta_info_) {
        topicid_domain_[info.topic_id] = info.domain_id;
        auto it = participants_.find(info.domain_id);
        ERTPS_RTPS::RTPSParticipant* participant;
        if (it == participants_.end()) {
            participant = create_participant_(info.domain_id);
        } else {
            participant = it->second;
        }
        if (participant == nullptr) {
            logError_("VBSTOOLSDK", "RtpsDatReplayer participant is nullptr, domain id: " << info.domain_id);
            continue;
        }

        if (false == create_writer_(participant, info)) {
            continue;
        }
    }

    return true;
}

ERTPS_RTPS::RTPSParticipant* RtpsDatReplayer::create_participant_(uint32_t domain) {
    ERTPS_RTPS::RTPSParticipant* participant;
    ERTPS_RTPS::RTPSParticipantAttributes attributes;
    participant = ERTPS_RTPS::RTPSDomain::createParticipant(domain, attributes);
    if (participant == nullptr) {
        logError_("VBSTOOLSDK", "VbsReplayer participant is nullptr, domain id: " << domain);
        return nullptr;
    }
    participants_.emplace(domain, participant);
    return participant;
}

bool RtpsDatReplayer::create_writer_(ERTPS_RTPS::RTPSParticipant* participant, DbOperator::DbMetaDataInfo& info) {
    ERTPS_RTPS::WriterAttributes writer_attributes;
    ERTPS_RTPS::HistoryAttributes history_attributes;
    ERTPS_RTPS::WriterHistory* history = new ERTPS_RTPS::WriterHistory(history_attributes);
    ERTPS_RTPS::RTPSWriter* writer;
    try {
        writer = ERTPS_RTPS::RTPSDomain::createRTPSWriter(participant, writer_attributes, history);
    } catch (const std::exception& e) {
        logError_("VBSTOOLSDK", "exception occurs : " << e.what());
        return false;
    }
    if (writer == nullptr) {
        logError_("VBSTOOLSDK", "RtpsDatReplayer writer is nullptr, topic name: " << info.topic_name);
        return false;
    }

    ERTPS::TopicAttributes topic_attributes;
    ERTPS::WriterQos writer_qos;
    topic_attributes.topicKind = ERTPS_RTPS::TopicKind_t::NO_KEY;
    topic_attributes.topicDataType = info.topic_type_name;
    topic_attributes.topicName = info.topic_name;
    if (false == participant->registerWriter(writer, topic_attributes, writer_qos)) {
        logError_("VBSTOOLSDK", "RtpsDatReplayer registerWriter failed, topic name: " << info.topic_name);
        return false;
    }

    historys_.emplace(info.topic_id, history);
    writers_.emplace(info.topic_id, writer);
    return true;
}

bool RtpsDatReplayer::writer_(DbOperator::DbRawDataInfo& info) {
    ERTPS_RTPS::CacheChange_t* new_change;
    new_change =
        writers_.at(info.topic_id)
            ->new_change([info]() -> uint32_t { return info.raw_data.size(); }, vbs::common::ChangeKind_t::ALIVE);
    if (nullptr == new_change) {
        logError_("VBSTOOLSDK", "RtpsDatReplayer new_change is nullptr, topic id: " << info.topic_id);
        return false;
    }

    memcpy(new_change->serializedPayload.data, info.raw_data.data(), info.raw_data.size());
    new_change->serializedPayload.length = static_cast<uint32_t>(info.raw_data.size());
    historys_.at(info.topic_id)->add_change(new_change);

    return true;
}

}  // namespace vbstoolsdk
