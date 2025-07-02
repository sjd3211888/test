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

#include "dispatcher/DispatcherParticipant.hpp"
#include "history/SubstrateParticipant.h"
#include "dispatcher/DispatcherReader.hpp"
#include "history/SubstrateReader.h"
#include "edds/rtps/reader/RTPSReader.h"
#include "elog/Log.hpp"
#include "transport/TransportFactory.hpp"

using namespace vbsutil::xmlparser;
using namespace evbs::ertps::rtps;

namespace vbs {
namespace dispatcher {

DispatcherReader::DispatcherReader(DispatcherParticipant* disPart, GUID_t guid, ReaderAttributes& ratt,
                                   TopicAttributes& topicAtt, ReaderQos& rqos, std::shared_ptr<IChangePool> change_pool,
                                   ReaderHistory* history, ReaderListener* listener)
    : m_inner_listener_(this),
      m_participant(disPart),
      m_history(history),
      m_uplistener(listener),
      m_change_pool(std::move(change_pool)),
      m_guid(guid),
      m_attr(ratt) {

    m_topic_attr.qos.history(topicAtt.qos.history());
    m_topic_attr.qos.resource_limits(topicAtt.qos.resource_limits());
    m_topic_attr.topicName = topicAtt.topicName;
    m_topic_attr.topicDataType = topicAtt.topicDataType;
    m_topic_attr.topicKind = topicAtt.topicKind;
    m_topic_attr.m_crc16Idl = topicAtt.m_crc16Idl;
    m_qos.setQos(rqos, true);

    //缺省创建rtps Reader
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        SubstrateReader* reader =
            subPart->create_reader(m_guid, m_attr, topicAtt.topicName.to_string(), topicAtt.topicDataType.to_string(),
                                   m_change_pool, m_history, &m_inner_listener_);
        if (reader) {
            substrate_readers[SUBSTRATE_TYPE_RTPS] = reader;
            m_guid = static_cast<RTPSReader*>(reader)->getGuid();
            logDebug(DISPATCHER, "Create rtps reader " << m_guid << " success.");
        } else {
            elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "Create rtps reader " << m_guid << " failed.");
        }
    }
}

DispatcherReader::~DispatcherReader() {
    m_uplistener = nullptr;
    for (uint32_t i = 0; i < SUBSTRATE_TYPE_MAX; i++) {
        if (substrate_readers[i]) {
            SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(i);
            if (subPart) {
                subPart->delete_reader(substrate_readers[i]);
            }
        }
    }
    substrate_readers = {nullptr};
    m_participant = nullptr;
    m_history = nullptr;
    m_change_pool = nullptr;
}

void DispatcherReader::DispatcherReaderListener::on_liveliness_changed(
    const evbs::edds::dds::LivelinessChangedStatus& status) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_liveliness_changed(status);
    }
}

void DispatcherReader::DispatcherReaderListener::on_requested_incompatible_qos(evbs::edds::dds::PolicyMask qos) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_requested_incompatible_qos(qos);
    }
}

void DispatcherReader::DispatcherReaderListener::on_sample_lost(const int32_t sample_lost_since_last_update,
                                                                const SampleLostStatusKind statusKind) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_sample_lost(sample_lost_since_last_update, statusKind);
    }
}

void DispatcherReader::DispatcherReaderListener::on_sample_rejected(evbs::edds::dds::SampleRejectedStatusKind reason,
                                                                    const CacheChange_t* const change) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_sample_rejected(reason, change);
    }
}

void DispatcherReader::DispatcherReaderListener::on_data_available(const GUID_t& writer_guid,
                                                                   const SequenceNumber_t& first_sequence,
                                                                   const SequenceNumber_t& last_sequence,
                                                                   bool& should_notify_individual_changes) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_data_available(writer_guid, first_sequence, last_sequence,
                                                              should_notify_individual_changes);
    }
}

void DispatcherReader::DispatcherReaderListener::on_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_e2e_exception(status);
    }
}

inline void DispatcherReader::notify_matched_status(const GUID_t& writer_guid) {
    if (get_uplistener()) {
        MatchingInfo info(MATCHED_MATCHING, writer_guid);
        get_uplistener()->onReaderMatched(info);
        m_matched_writer_size++;
        m_remote_guids.push_back(writer_guid);
        logInfo(DISPATCHER,
                "Reader " << getGuid() << "matched " << writer_guid << " notify uplistener Topic:" << getTopicName());
    }
}

void DispatcherReader::process_discovered_writer(const GUID_t& writer_guid, vbs::RemoteEndpointInfo* writer_info) {
    if (!writer_info) {
        logInfo(DISPATCHER, "Writer info is null.");
        return;
    }
    //check reader is local or not
#if !defined(_WIN32)
    if (m_participant->getGuid().is_on_same_host_as(writer_guid) &&
        (m_participant->getParticipantAttributes().transport_type != TRANSPORT_TYPE_UDP)) {  //如果是域间且未强制指定UDP
        if (m_participant->dsfDriverUp()) {
            bool dsf_flag = m_participant->process_discovered_use_dsf(this, &m_inner_listener_, writer_guid);
            if (!dsf_flag) {
                return;
            }
            //如果对端已经携带了locator信息，直接match，不再等待CHANGED_QOS通知
            if (writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_DSF)) {
                writer_info->domainId(m_participant->domainId());
                auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), writer_guid);
                if (it == m_remote_guids.end()) {
                    if (substrate_readers[SUBSTRATE_TYPE_DSF]->add_matched_writer(*writer_info)) {
                        notify_matched_status(writer_guid);
                    } else {
                        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                                  "Add matched writer" << writer_guid << " for DSF failed.");
                    }
                }
            }
        } else {
            if (substrate_readers[SUBSTRATE_TYPE_RTPS]) {
                m_participant->process_discovered_use_uds_or_udp(this,
                                                                 vbs::transport::TransmitorType::TRANSMITOR_TYPE_UDS);
                logDebug(DISPATCHER, "Add matched writer for RTPS, register transport UDS ." << writer_guid);
                if (writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDS)) {
                    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), writer_guid);
                    if (it == m_remote_guids.end()) {
                        notify_matched_status(writer_guid);
                    }
                }
            } else {
                elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                          "dsf driver is down and rtps reader is nullptr! GUID:" << m_guid);
            }
        }
    } else  //域间 或者强制UDP
#endif
    {
        if (substrate_readers[SUBSTRATE_TYPE_RTPS]) {
            m_participant->process_discovered_use_uds_or_udp(this, vbs::transport::TransmitorType::TRANSMITOR_TYPE_UDP);
            logDebug(DISPATCHER, "Add matched writer for RTPS, register transport UDP ." << writer_guid);
            if (writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP)) {
                auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), writer_guid);
                if (it == m_remote_guids.end()) {
                    notify_matched_status(writer_guid);
                }
            }
        } else {
            elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                      "dsf driver is down and rtps reader is nullptr! GUID: " << m_guid);
        }
    }
}

void DispatcherReader::process_changed_writer(const GUID_t& writer_guid, vbs::RemoteEndpointInfo* writer_info) {
    bool ret = false;

    if (!writer_info) {
        logInfo(DISPATCHER, "Writer info is null.");
        return;
    }
    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), writer_guid);
    if (it != m_remote_guids.end()) {
        logInfo(DISPATCHER, "Writer already matched before.");
        return;  //只有首次才处理，后续change通知不处理。
    }
#if !defined(_WIN32)
    if (m_participant->getGuid().is_on_same_host_as(writer_guid) && m_participant->dsfDriverUp() &&
        (m_participant->getParticipantAttributes().transport_type != TRANSPORT_TYPE_UDP)) {
        //已经存在底层Reader，进行matched_add
        if (substrate_readers[SUBSTRATE_TYPE_DSF]) {
            if (!writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_DSF)) {
                logWarning(DISPATCHER, "The locator list in writer_info is empty, guid: " << writer_guid);
                return;
            }
            writer_info->domainId(m_participant->domainId());
            ret = substrate_readers[SUBSTRATE_TYPE_DSF]->add_matched_writer(*writer_info);
            if (!ret) {
                elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "add matched writer for DSF failed.");
            }
        } else {
            logInfo(DISPATCHER, "no dsf reader, add matched writer error.");
        }
    } else if (writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP) ||
               writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDS)) {
#else
    if (writer_info->locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP)) {
#endif
        ret = true;
    }

    if (ret) {
        notify_matched_status(writer_guid);
    }
}

inline void DispatcherReader::process_removed_writer(const GUID_t& writer_guid,
                                                     vbs::RemoteEndpointInfo* /*writer_info*/) {
    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), writer_guid);
    if (it == m_remote_guids.end()) {
        logInfo(DISPATCHER, "Writer " << writer_guid << "has not been matched before");
        return;
    }

    for (auto reader : substrate_readers) {
        if (reader) {
            vbs::RemoteEndpointInfo writerInfo;
            writerInfo.domainId(m_participant->domainId());
            writerInfo.guid(writer_guid);
            reader->delete_matched_writer(writerInfo);
        }
    }
    if (get_uplistener()) {
        MatchingInfo info(REMOVED_MATCHING, writer_guid);
        get_uplistener()->onReaderMatched(info);
    }
    m_matched_writer_size--;
    m_remote_guids.erase(std::remove(m_remote_guids.begin(), m_remote_guids.end(), writer_guid), m_remote_guids.end());
}

void DispatcherReader::DispatcherReaderListener::on_writer_discovery(evbs::edds::dds::WRITER_DISCOVERY_STATUS reason,
                                                                     const GUID_t& writer_guid,
                                                                     vbs::RemoteEndpointInfo* writer_info) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (!dispatch_reader_->m_uplistener) {
        return;
    }
    switch (reason) {
        case WRITER_DISCOVERY_STATUS::DISCOVERED_WRITER:
            logInfo(DISPATCHER, "DISCOVERED_WRITER " << writer_guid);
            dispatch_reader_->process_discovered_writer(writer_guid, writer_info);
            break;
        case WRITER_DISCOVERY_STATUS::CHANGED_QOS_WRITER:
            logInfo(DISPATCHER, "CHANGED_QOS_WRITER " << writer_guid);
            dispatch_reader_->process_changed_writer(writer_guid, writer_info);
            break;
        case WRITER_DISCOVERY_STATUS::REMOVED_WRITER:
            logInfo(DISPATCHER, "REMOVED_WRITER " << writer_guid);
            dispatch_reader_->process_removed_writer(writer_guid, writer_info);
        default:
            break;
    }
}

void DispatcherReader::DispatcherReaderListener::on_endpoint_crash(const evbs::ertps::rtps::GUID_t& writer_guid) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_reader_->getListenerMutex());
    if (dispatch_reader_->get_uplistener()) {
        dispatch_reader_->get_uplistener()->on_endpoint_crash(writer_guid);
    }
}

bool DispatcherReader::registerReader(const vbsutil::xmlparser::ContentFilterProperty* content_filter_property) {
    if (!substrate_readers[SUBSTRATE_TYPE_RTPS]) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "Register reader failed. No rtps substrate reader.");
        return false;
    }

    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        return subPart->registerReader(substrate_readers[SUBSTRATE_TYPE_RTPS], m_topic_attr, m_qos,
                                       content_filter_property);
    }
    return false;
}

bool DispatcherReader::updateReader(const TopicAttributes& topicAtt, const ReaderQos& rqos,
                                    const vbsutil::xmlparser::ContentFilterProperty* content_filter) {
    if (!substrate_readers[SUBSTRATE_TYPE_RTPS]) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "Update reader failed. No rtps substrate reader.");
        return false;
    }
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        return subPart->updateReader(substrate_readers[SUBSTRATE_TYPE_RTPS], topicAtt, rqos, content_filter);
    }
    return false;
}

bool DispatcherReader::begin_sample_access_nts(CacheChange_t* change, bool& is_future_change) {
    if (substrate_readers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return substrate_readers[SUBSTRATE_TYPE_RTPS]->begin_sample_access_nts(change, is_future_change);
    }
    return false;
}

evbs::edds::dds::builtin::StatisticProxyInfos DispatcherReader::get_proxy_infos() {
    if (substrate_readers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return static_cast<RTPSReader*>(substrate_readers[SUBSTRATE_TYPE_RTPS])->get_proxy_infos();
    }

    return evbs::edds::dds::builtin::StatisticProxyInfos();
}

bool DispatcherReader::wait_for_unread_cache(const vbsutil::Duration_t& timeout) {
    if (substrate_readers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return dynamic_cast<RTPSReader*>(substrate_readers[SUBSTRATE_TYPE_RTPS])->wait_for_unread_cache(timeout);
    }
    return false;
}

}  // namespace dispatcher
}  // namespace vbs
