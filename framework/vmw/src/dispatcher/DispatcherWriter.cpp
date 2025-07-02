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
#include "dispatcher/DispatcherWriter.hpp"
#include "history/SubstrateWriter.h"
#include "edds/rtps/writer/RTPSWriter.h"
#include "elog/Log.hpp"
#include "transport/TransportFactory.hpp"

using namespace vbsutil::xmlparser;
using namespace evbs::ertps::rtps;
using namespace vbs::common;
namespace vbs {
namespace dispatcher {

DispatcherWriter::DispatcherWriter(DispatcherParticipant* disPart, GUID_t guid, WriterAttributes& watt,
                                   TopicAttributes& topicAtt, WriterQos& wqos,
                                   std::shared_ptr<IPayloadPool> payload_pool, std::shared_ptr<IChangePool> change_pool,
                                   WriterHistory* history, WriterListener* listener)
    : m_inner_listener_(this),
      m_participant(disPart),
      m_history(history),
      m_uplistener(listener),
      m_payload_pool(std::move(payload_pool)),
      m_change_pool(std::move(change_pool)),
      m_guid(guid),
      m_attr(watt) {

    m_topic_attr.qos.history(topicAtt.qos.history());
    m_topic_attr.qos.resource_limits(topicAtt.qos.resource_limits());
    m_topic_attr.topicName = topicAtt.topicName;
    m_topic_attr.topicDataType = topicAtt.topicDataType;
    m_topic_attr.topicKind = topicAtt.topicKind;
    m_topic_attr.m_crc16Idl = topicAtt.m_crc16Idl;
    m_qos.setQos(wqos, true);

    //缺省创建rtps writer
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        SubstrateWriter* writer =
            subPart->create_writer(m_guid, m_attr, topicAtt.topicName.to_string(), topicAtt.topicDataType.to_string(),
                                   m_payload_pool, m_change_pool, history, &m_inner_listener_);
        if (writer) {
            substrate_writers[SUBSTRATE_TYPE_RTPS] = writer;
            m_guid = static_cast<RTPSWriter*>(writer)->getGuid();
            logDebug(DISPATCHER, "create rtps writer " << m_guid << " success." << std::endl);
        } else {
            elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "create rtps writer " << m_guid << " failed.");
        }
    }
}

DispatcherWriter::~DispatcherWriter() {
    m_uplistener = nullptr;

    //先删除rtps，避免后续发现流程触发回调
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart && substrate_writers[SUBSTRATE_TYPE_RTPS]) {
        SubstrateWriter* swriter = substrate_writers[SUBSTRATE_TYPE_RTPS];
        substrate_writers[SUBSTRATE_TYPE_RTPS] = nullptr;
        subPart->delete_writer(swriter);
    }
    subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_DSF);
    if (subPart && substrate_writers[SUBSTRATE_TYPE_DSF]) {
        SubstrateWriter* swriter = substrate_writers[SUBSTRATE_TYPE_DSF];
        substrate_writers[SUBSTRATE_TYPE_DSF] = nullptr;
        subPart->delete_writer(swriter);
    }
    m_history = nullptr;
    m_change_pool = nullptr;
    m_payload_pool = nullptr;
    m_participant = nullptr;
}

void DispatcherWriter::DispatcherWriterListener::on_offered_incompatible_qos(evbs::edds::dds::PolicyMask qos) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (dispatch_writer_->get_uplistener()) {
        dispatch_writer_->get_uplistener()->on_offered_incompatible_qos(qos);
    }
}

void DispatcherWriter::DispatcherWriterListener::onWriterChangeReceivedByAll(
    vbs::dispatcher::SubstrateWriter* const writer, CacheChange_t* ch) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (!dispatch_writer_->get_uplistener()) {
        return;
    }
    if (dispatch_writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF) == writer) {
        ch->set_received(SUBSTRATE_TYPE_DSF);
    } else if (dispatch_writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS) == writer) {
        ch->set_received(SUBSTRATE_TYPE_RTPS);
    }

    if (!((dispatch_writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF) && !ch->check_received(SUBSTRATE_TYPE_DSF)) ||
          (dispatch_writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS) && dispatch_writer_->m_register_transport &&
           !ch->check_received(SUBSTRATE_TYPE_RTPS)))) {
        dispatch_writer_->get_uplistener()->onWriterChangeReceivedByAll(writer, ch);
    }
}

void DispatcherWriter::DispatcherWriterListener::on_liveliness_lost(const evbs::ertps::LivelinessLostStatus& status) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (dispatch_writer_->get_uplistener()) {
        dispatch_writer_->get_uplistener()->on_liveliness_lost(status);
    }
}

inline void DispatcherWriter::notify_matched_status(vbs::RemoteEndpointInfo& reader_info) {
    if (get_uplistener()) {
        MatchingInfo info(MATCHED_MATCHING, reader_info.guid());
        {
            const auto& filter_property = reader_info.content_filter_property();
            info.remoteContentFilterProperty.content_filtered_topic_name = filter_property.content_filtered_topic_name;
            info.remoteContentFilterProperty.expression_parameters.reserve(
                filter_property.expression_parameters.size());
            for (const auto& s : filter_property.expression_parameters) {
                info.remoteContentFilterProperty.expression_parameters.push_back(s.to_string());
            }
            info.remoteContentFilterProperty.filter_class_name = filter_property.filter_class_name;
            info.remoteContentFilterProperty.filter_expression = filter_property.filter_expression;
            info.remoteContentFilterProperty.related_topic_name = filter_property.related_topic_name;
        }
        if (!reader_info.type_plain()) {
            m_no_plain_reader++;
        }
        get_uplistener()->onWriterMatched(info);
        m_matched_reader_size++;
        m_remote_guids.push_back(reader_info.guid());
        logInfo(DISPATCHER, "Writer " << getGuid() << "matched " << reader_info.guid()
                                      << " notify uplistener Topic:" << getTopicName());
    }
}

void DispatcherWriter::process_discovered_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info) {
#if !defined(_WIN32)
    if (m_participant->getGuid().is_on_same_host_as(reader_guid) &&
        (m_participant->getParticipantAttributes().transport_type != TRANSPORT_TYPE_UDP)) {
        if (m_participant->dsfDriverUp()) {
            bool dsf_flag = m_participant->process_discovered_use_dsf(this, &m_inner_listener_, reader_guid);
            if (!dsf_flag) {
                return;
            }
            //如果对端已经携带了locator信息，直接match，不再等待CHANGED_QOS通知
            if (reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_DSF)) {
                auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), reader_guid);  //本端寻找
                if (it == m_remote_guids.end()) {
                    reader_info.domainId(m_participant->domainId());
                    if (substrate_writers[SUBSTRATE_TYPE_DSF]->add_matched_reader(reader_info)) {
                        notify_matched_status(reader_info);
                    } else {
                        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                                  "add matched reader" << reader_guid << "for DSF failed.");
                    }
                }
            }
        } else {
            if (substrate_writers[SUBSTRATE_TYPE_RTPS]) {
                m_participant->process_discovered_use_uds_or_udp(this,
                                                                 vbs::transport::TransmitorType::TRANSMITOR_TYPE_UDS);
                logDebug(DISPATCHER, "Add matched reader for RTPS, register transport UDS ." << reader_guid);
                if (reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDS)) {
                    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), reader_guid);
                    if (it == m_remote_guids.end()) {
                        notify_matched_status(reader_info);
                    }
                }
            } else {
                elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                          "dsf driver is down and rtps writer is nullptr! GUID: " << m_guid);
            }
        }
    } else
#endif
    {
        if (substrate_writers[SUBSTRATE_TYPE_RTPS]) {
            m_participant->process_discovered_use_uds_or_udp(this, vbs::transport::TransmitorType::TRANSMITOR_TYPE_UDP);
            logDebug(DISPATCHER, "Add matched reader for RTPS, register transport UDP ." << reader_guid);
            if (reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP)) {
                auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), reader_guid);
                if (it == m_remote_guids.end()) {
                    notify_matched_status(reader_info);
                }
            }
        } else {
            elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                      "dsf driver is down and rtps writer is nullptr! GUID: " << m_guid);
        }
    }
}

void DispatcherWriter::process_changed_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info) {
    bool ret = false;

    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), reader_guid);
    if (it != m_remote_guids.end()) {
        logDebug(DISPATCHER, "Reader already matched before.");
        return;
    }

#if !defined(_WIN32)
    if (m_participant->getGuid().is_on_same_host_as(reader_guid) && m_participant->dsfDriverUp() &&
        (m_participant->getParticipantAttributes().transport_type != TRANSPORT_TYPE_UDP)) {
        //已经存在底层writer，进行matched_add
        if (substrate_writers[SUBSTRATE_TYPE_DSF]) {
            if (!reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_DSF)) {
                logWarning(DISPATCHER, "The locator list in reader_info is empty, guid: " << reader_guid);
                return;
            }
            reader_info.domainId(m_participant->domainId());
            ret = substrate_writers[SUBSTRATE_TYPE_DSF]->add_matched_reader(reader_info);
            if (!ret) {
                elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "add matched reader for DSF failed.");
            }
        } else {
            logInfo(DISPATCHER, "no dsf writer, add matched reader error.");
        }
    } else if (reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP) ||
               reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDS)) {
#else
    if (reader_info.locator_valid(vbs::RemoteEndpointInfo::LocatorType::LOCATOR_TYPE_UDP)) {
#endif
        ret = true;
    }

    if (ret) {
        notify_matched_status(reader_info);
    }
}

inline void DispatcherWriter::process_removed_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info) {
    auto it = std::find(m_remote_guids.begin(), m_remote_guids.end(), reader_guid);
    if (it == m_remote_guids.end()) {
        logInfo(DISPATCHER, "Reader " << reader_guid << "has not been matched before");
        return;
    }

    for (auto writer : substrate_writers) {
        if (writer) {
            vbs::RemoteEndpointInfo readerInfo;
            readerInfo.domainId(m_participant->domainId());
            readerInfo.guid(reader_guid);
            writer->delete_matched_reader(readerInfo);
        }
    }

    if (get_uplistener()) {
        MatchingInfo info(REMOVED_MATCHING, reader_guid);
        get_uplistener()->onWriterMatched(info);
    }
    m_matched_reader_size--;
    m_remote_guids.erase(std::remove(m_remote_guids.begin(), m_remote_guids.end(), reader_guid), m_remote_guids.end());
    if (!reader_info.type_plain()) {
        if (m_no_plain_reader < 1) {
            elogError(DISPATCHER, RetCode_t::RETCODE_ERROR,
                      "Reader " << reader_guid << "is not plain, dec no_plain_reader counters error.");
        }
        m_no_plain_reader--;
    }
}

void DispatcherWriter::DispatcherWriterListener::on_reader_discovery(evbs::edds::dds::READER_DISCOVERY_STATUS reason,
                                                                     const evbs::ertps::rtps::GUID_t& reader_guid,
                                                                     vbs::RemoteEndpointInfo& reader_info) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (!dispatch_writer_->m_uplistener) {
        return;
    }
    dispatch_writer_->m_uplistener->on_reader_discovery(reason, reader_guid, reader_info);

    switch (reason) {
        case READER_DISCOVERY_STATUS::DISCOVERED_READER:
            logInfo(DISPATCHER, "DISCOVERED_READER " << reader_guid);
            dispatch_writer_->process_discovered_reader(reader_guid, reader_info);
            break;
        case READER_DISCOVERY_STATUS::CHANGED_QOS_READER:
            logInfo(DISPATCHER, "CHANGED_QOS_READER " << reader_guid);
            dispatch_writer_->process_changed_reader(reader_guid, reader_info);
            break;
        case READER_DISCOVERY_STATUS::REMOVED_READER:
            logInfo(DISPATCHER, "REMOVED_READER " << reader_guid);
            dispatch_writer_->process_removed_reader(reader_guid, reader_info);
            break;
        default:
            break;
    }
}

void DispatcherWriter::DispatcherWriterListener::on_endpoint_crash(const evbs::ertps::rtps::GUID_t& reader_guid) {
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (dispatch_writer_->get_uplistener()) {
        dispatch_writer_->get_uplistener()->on_endpoint_crash(reader_guid);
    }
}

void DispatcherWriter::DispatcherWriterListener::on_non_plain_reader_discovery() {
    /*之前已经有no plain的reader，不用做处理。 */
    if (dispatch_writer_->has_non_plain_reader()) {
        return;
    }
    vbs::shared_lock<vbs::shared_mutex> lock(dispatch_writer_->getListenerMutex());
    if (dispatch_writer_->get_uplistener()) {
        dispatch_writer_->get_uplistener()->on_non_plain_reader_discovery();
    }
}

void DispatcherWriter::write(CacheChange_t* data,
                             const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time,
                             bool in_history) {
    if (substrate_writers[SUBSTRATE_TYPE_DSF]) {
        substrate_writers[SUBSTRATE_TYPE_DSF]->write(data, max_blocking_time, in_history);
    }
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport && data->payload_owner() &&
        (data->payload_owner()->get_payload_type() == PAYLOAD_TYPE_NORMAL)) {
        substrate_writers[SUBSTRATE_TYPE_RTPS]->write(data, max_blocking_time, in_history);
    }
}

bool DispatcherWriter::registerWriter() {
    if (!substrate_writers[SUBSTRATE_TYPE_RTPS]) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "Register writer failed. No rtps substrate writer.");
        return false;
    }

    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        logDebug(DISPATCHER, "registerWriter " << getTopicName());
        return subPart->registerWriter(substrate_writers[SUBSTRATE_TYPE_RTPS],
                                       dynamic_cast<BaseTopicAttributes&>(m_topic_attr), m_qos);
    }
    return false;
}

bool DispatcherWriter::updateWriter(const TopicAttributes& topicAtt, const WriterQos& wqos) {
    if (!substrate_writers[SUBSTRATE_TYPE_RTPS]) {
        elogError(DISPATCHER, RetCode_t::RETCODE_ERROR, "Update writer failed. No rtps substrate writer.");
        return false;
    }
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        return subPart->updateWriter(substrate_writers[SUBSTRATE_TYPE_RTPS], topicAtt, wqos);
    }
    return false;
}

RetCode_t DispatcherWriter::assert_liveliness() {
    SubstrateParticipant* subPart = m_participant->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    if (subPart) {
        return subPart->assert_liveliness(substrate_writers[SUBSTRATE_TYPE_RTPS]);
    }
    return RetCode_t::RETCODE_ERROR;
}

void DispatcherWriter::acknack_request() {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        substrate_writers[SUBSTRATE_TYPE_RTPS]->acknack_request();
    }
}

/* 该函数实现用于当writer为transient_local时，keep all模式下，队列满需要删除acked change释放空间，
* 域内没有ack机制，只要发送了就代表acked，域间需要底层RTPS协议确认删除哪个change
* 1. 只有域间reader：需要域间底层检测，可以删除的话由RTPS删除。
* 2. 域内和域间reader同时存在：需要域间底层检测，可以删除的话由RTPS删除。
* 3. 只有域内reader: 如果最小的change已经被DSF发送，删除最小序列号的change，如果没有被DSF发送，大序列号的肯定也没有发送，不删除返回false
*/
bool DispatcherWriter::try_remove_change(const std::chrono::steady_clock::time_point& max_blocking_time_point,
                                         std::unique_lock<vbs::common::RecursiveTimedMutex>& lock) {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return substrate_writers[SUBSTRATE_TYPE_RTPS]->try_remove_change(max_blocking_time_point, lock);
    } else if (substrate_writers[SUBSTRATE_TYPE_DSF]) {
        CacheChange_t* first = nullptr;
        m_history->get_min_change(&first);
        if (first && first->check_received(SUBSTRATE_TYPE_DSF)) {
            return m_history->remove_min_change();
        }
    }
    return false;
}

bool DispatcherWriter::wait_for_acknowledgement(const SequenceNumber_t& seq,
                                                const std::chrono::steady_clock::time_point& max_blocking_time_point,
                                                std::unique_lock<vbs::common::RecursiveTimedMutex>& lock) {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return substrate_writers[SUBSTRATE_TYPE_RTPS]->wait_for_acknowledgement(seq, max_blocking_time_point, lock);
    }
    return true;
}

bool DispatcherWriter::change_is_acked_or_fully_delivered(const CacheChange_t* ch) {
    return !((substrate_writers[SUBSTRATE_TYPE_DSF] &&
              !const_cast<CacheChange_t*>(ch)->check_received(SUBSTRATE_TYPE_DSF)) ||
             (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport &&
              !substrate_writers[SUBSTRATE_TYPE_RTPS]->change_is_acked_or_fully_delivered(ch)));
}

bool DispatcherWriter::wait_for_all_acked(const vbsutil::Duration_t& max_wait) {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return substrate_writers[SUBSTRATE_TYPE_RTPS]->wait_for_all_acked(max_wait);
    } else if (substrate_writers[SUBSTRATE_TYPE_DSF]) {
        return true;
    }
    return false;
}

bool DispatcherWriter::change_removed_by_history(CacheChange_t* change) {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] && m_register_transport) {
        return substrate_writers[SUBSTRATE_TYPE_RTPS]->change_removed_by_history(change);
    }
    return false;
}

evbs::edds::dds::builtin::StatisticProxyInfos DispatcherWriter::get_proxy_infos() {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS]) {
        return static_cast<RTPSWriter*>(substrate_writers[SUBSTRATE_TYPE_RTPS])->get_proxy_infos();
    }
    return evbs::edds::dds::builtin::StatisticProxyInfos();
}

void DispatcherWriter::reader_data_filter(evbs::ertps::rtps::IReaderDataFilter* filter) {
    if (substrate_writers[SUBSTRATE_TYPE_RTPS] == nullptr) {
        return;
    }
    auto writer = static_cast<RTPSWriter*>(substrate_writers[SUBSTRATE_TYPE_RTPS]);
    writer->reader_data_filter(filter);
}

}  // namespace dispatcher
}  // namespace vbs
