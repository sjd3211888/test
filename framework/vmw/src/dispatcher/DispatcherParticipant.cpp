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

#include <unistd.h>

#include "dispatcher/DispatcherParticipant.hpp"
#include "history/SubstrateParticipant.h"
#include "dispatcher/DispatcherWriter.hpp"
#include "dispatcher/DispatcherReader.hpp"
#include "xmlparser/XMLParserCommonPart.h"
#include "edds/rtps/participant/RTPSParticipant.h"
#include "edds/rtps/RTPSDomain.h"
#include "elog/ReturnCode.hpp"
#if !defined(_WIN32)
#include "dsfdds/dds/domain/DSFParticipant.h"
#endif

using namespace vbsutil::xmlparser;
using namespace evbs::ertps::rtps;

namespace vbs {
namespace dispatcher {

DispatcherParticipant::DispatcherParticipant(uint32_t domainId, evbs::ertps::rtps::RTPSParticipantAttributes& attr,
                                             vbs::common::ParticipantListener* listener)
    : m_domain_id(domainId), m_attribute(attr), m_listener(listener) {

    //初始化创建rtps participant，用于中心服务发现
    substrate_participants[SUBSTRATE_TYPE_RTPS] = RTPSDomain::createParticipant(m_domain_id, false, attr, m_listener);
    if (substrate_participants[SUBSTRATE_TYPE_RTPS] == nullptr) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create rtps participant failed.");
    } else {
        b_active = true;
    }

    if ((access(vbsutil::constString::DSF_SHM_DRIVER_PATH, F_OK) != -1)) {  // dsf driver is up
        b_dsf_driver_up = true;
    } else {
        logInfo(DISPATCHER, "DSF driver is not ready, use UDP for intra communication.");
    }
}

DispatcherParticipant::~DispatcherParticipant() {
    for (auto writer : m_writer_list) {
        delete writer;
    }
    for (auto reader : m_reader_list) {
        delete reader;
    }
    m_writer_list.clear();
    m_reader_list.clear();

    if (substrate_participants[SUBSTRATE_TYPE_RTPS]) {
        RTPSDomain::removeRTPSParticipant(
            dynamic_cast<evbs::ertps::rtps::RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS]));
    }
#if !defined(_WIN32)
    if (substrate_participants[SUBSTRATE_TYPE_DSF]) {
        li::dsfdds::DSFParticipant::delete_DSFParticipant(substrate_participants[SUBSTRATE_TYPE_DSF]);
    }
#endif
}

void DispatcherParticipant::enable() {
    substrate_participants[SUBSTRATE_TYPE_RTPS]->enable();
}

const GUID_t& DispatcherParticipant::getGuid() const {
    return substrate_participants[SUBSTRATE_TYPE_RTPS]->getGuid();
}

void DispatcherParticipant::set_listener(vbs::common::ParticipantListener* listener) {
    m_listener = listener;
    substrate_participants[SUBSTRATE_TYPE_RTPS]->set_listener(listener);
}

evbs::edds::dds::builtin::LookupManager* DispatcherParticipant::lookup_manager() const {
    return static_cast<evbs::ertps::rtps::RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS])
        ->lookup_manager();
}

bool DispatcherParticipant::startRTPSParticipantInitialAnnouncement(uint32_t count, vbsutil::Duration_t period) {
    return static_cast<evbs::ertps::rtps::RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS])
        ->startRTPSParticipantInitialAnnouncement(count, period);
}

EvbsVersion_t DispatcherParticipant::get_remote_evbsversion() {
    return static_cast<evbs::ertps::rtps::RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS])
        ->get_remote_evbsversion();
}

void DispatcherParticipant::update_attributes(const evbs::ertps::rtps::RTPSParticipantAttributes& patt) {
    m_attribute = patt;

    static_cast<evbs::ertps::rtps::RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS])
        ->update_attributes(patt);
}

const std::vector<DispatcherWriter*>& DispatcherParticipant::getAllWriters() const {
    return m_writer_list;
}

const std::vector<DispatcherReader*>& DispatcherParticipant::getAllReaders() const {
    return m_reader_list;
}

void DispatcherParticipant::create_dsf_participant(void) {
#if !defined(_WIN32)
    substrate_participants[SUBSTRATE_TYPE_DSF] = li::dsfdds::DSFParticipant::create_DSFParticipant(m_domain_id);
    if (!substrate_participants[SUBSTRATE_TYPE_DSF]) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create dsf participant failed.");
    }
#else
    elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create dsf participant failed.");
#endif
}

DispatcherWriter* DispatcherParticipant::create_writer(GUID_t guid, WriterAttributes& watt, TopicAttributes& topic_att,
                                                       WriterQos& wqos, std::shared_ptr<IPayloadPool>& payload_pool,
                                                       std::shared_ptr<IChangePool>& change_pool,
                                                       WriterHistory* history, WriterListener* listener) {
    if (!b_active) {
        return nullptr;
    }

    DispatcherWriter* writer =
        new DispatcherWriter(this, guid, watt, topic_att, wqos, payload_pool, change_pool, history, listener);
    if (!writer) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create dispatcher writer failed.");
        return nullptr;
    }
    /*目前根据RTPS创建成功与否管理资源*/
    if (!writer->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS)) {
        delete writer;
        return nullptr;
    }

    m_writer_list.emplace_back(writer);

    return writer;
}

DispatcherReader* DispatcherParticipant::create_reader(GUID_t guid, ReaderAttributes& ratt, TopicAttributes& topic_att,
                                                       ReaderQos& rqos, std::shared_ptr<IChangePool>& change_pool,
                                                       ReaderHistory* history, ReaderListener* listener) {
    if (!b_active) {
        return nullptr;
    }

    DispatcherReader* reader = new DispatcherReader(this, guid, ratt, topic_att, rqos, change_pool, history, listener);
    if (!reader) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create dispatcher reader failed.");
        return nullptr;
    }
    /*目前根据RTPS创建成功与否管理资源*/
    if (!reader->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS)) {
        delete reader;
        return nullptr;
    }

    m_reader_list.emplace_back(reader);

    return reader;
}

bool DispatcherParticipant::delete_writer(DispatcherWriter* writer) {
    m_writer_list.erase(std::remove(m_writer_list.begin(), m_writer_list.end(), writer), m_writer_list.end());
    delete writer;
    return true;
}

bool DispatcherParticipant::delete_reader(DispatcherReader* reader) {
    m_reader_list.erase(std::remove(m_reader_list.begin(), m_reader_list.end(), reader), m_reader_list.end());
    delete reader;
    return true;
}

vbsutil::elog::ReturnCode_t DispatcherParticipant::assert_liveliness() {
    SubstrateParticipant* subPart = substrate_participants[SUBSTRATE_TYPE_RTPS];
    if (subPart) {
        return subPart->assert_liveliness();
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
}

ResourceEvent& DispatcherParticipant::get_resource_event() const {
    RTPSParticipant* rtpsPart = dynamic_cast<RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS]);
    if (rtpsPart) {
        return rtpsPart->get_resource_event();
    }
    static ResourceEvent tmp;
    return tmp;
}

std::vector<std::string> DispatcherParticipant::getParticipantNames() {
    RTPSParticipant* rtpsPart = dynamic_cast<RTPSParticipant*>(substrate_participants[SUBSTRATE_TYPE_RTPS]);
    if (rtpsPart) {
        return rtpsPart->getParticipantNames();
    }
    return std::vector<std::string> {};
}

}  // namespace dispatcher
}  // namespace vbs
