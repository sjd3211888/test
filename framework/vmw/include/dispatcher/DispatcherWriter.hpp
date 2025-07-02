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

#ifndef INCLUDE_DISPATCHER_WRITER_H
#define INCLUDE_DISPATCHER_WRITER_H

#include "elog/ReturnCode.hpp"
#include "edds/dds/builtin/lookup/common/LookupTypes.hpp"
#include "edds/rtps/attributes/WriterAttributes.h"
#include "ertps/attributes/TopicAttributes.h"
#include "xmlparser/qos/XMLWriterQos.hpp"
#include "deps/common/Guid.h"
#include "deps/common/EntityId_t.hpp"
#include "history/IChangePool.h"
#include "history/IPayloadPool.h"
#include "history/WriterHistory.h"
#include "history/WriterListener.h"
#include "history/CacheChange.h"
#include "deps/base/SequenceNumber.h"
#include "deps/common/TimedMutex.hpp"
#include "deps/common/Time_t.h"
#include "deps/common/shared_mutex.hpp"
#include "ertps/qos/LivelinessLostStatus.h"
#include "edds/rtps/interfaces/IReaderDataFilter.hpp"
#include "DispatcherParticipant.hpp"
#include "history/SubstrateWriter.h"

using namespace evbs::ertps::rtps;
using namespace vbsutil::xmlparser;

namespace vbs {
namespace dispatcher {

class DispatcherWriter {
 public:
    DispatcherWriter(DispatcherParticipant* disPart, GUID_t guid, WriterAttributes& watt, TopicAttributes& topic_att,
                     WriterQos& wqos, std::shared_ptr<IPayloadPool> payload_pool,
                     std::shared_ptr<IChangePool> change_pool, WriterHistory* history, WriterListener* listener);
    ~DispatcherWriter();

    void write(CacheChange_t* data, const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time,
               bool in_history);
    WriterListener* get_uplistener() { return m_uplistener; }

    bool set_listener(WriterListener* const listener) {
        std::lock_guard<vbs::shared_mutex> guard(m_listener_mtx);
        m_uplistener = listener;
        return true;
    }

    const GUID_t& getGuid() { return m_guid; }

    const std::string getTopicName() const { return m_topic_attr.getTopicName().to_string(); }
    const std::string getTopicType() const { return m_topic_attr.getTopicDataType().to_string(); }
    size_t getMatchedReadersSize() const { return m_matched_reader_size; }

    bool registerWriter();
    bool updateWriter(const TopicAttributes& topicAtt, const WriterQos& wqos);

    SubstrateWriter* getSubstrateEndpoint(uint32_t type) {
        if (type >= SUBSTRATE_TYPE_MAX) {
            return nullptr;
        }
        return substrate_writers[type];
    }
    RetCode_t assert_liveliness();

    void acknack_request();

    bool try_remove_change(const std::chrono::steady_clock::time_point& max_blocking_time_point,
                           std::unique_lock<vbs::common::RecursiveTimedMutex>& lock);

    bool wait_for_acknowledgement(const SequenceNumber_t& seq,
                                  const std::chrono::steady_clock::time_point& max_blocking_time_point,
                                  std::unique_lock<vbs::common::RecursiveTimedMutex>& lock);

    bool change_is_acked_or_fully_delivered(const CacheChange_t* change);

    bool wait_for_all_acked(const vbsutil::Duration_t& max_wait);

    bool change_removed_by_history(CacheChange_t* change);

    std::vector<GUID_t> get_remote_guids() { return m_remote_guids; }
    evbs::edds::dds::builtin::StatisticProxyInfos get_proxy_infos();
    vbs::shared_mutex& getListenerMutex() { return m_listener_mtx; }

    WriterAttributes& getAttributes() { return m_attr; }

    bool has_non_plain_reader() { return m_no_plain_reader > 0 ? true : false; }

    void set_register_transport(bool flag) { m_register_transport = flag; }

    bool get_register_transport() { return m_register_transport; }

    TopicAttributes get_topic_attr() { return m_topic_attr; }

    WriterQos get_qos() { return m_qos; }

    std::shared_ptr<IChangePool> get_change_pool() { return m_change_pool; }

    std::shared_ptr<IPayloadPool> get_payload_pool() { return m_payload_pool; }

    WriterHistory* get_history() { return m_history; }

    void setSubstrateEndpoint(uint32_t type, SubstrateWriter* sub_writer) {
        if (type >= SUBSTRATE_TYPE_MAX) {
            return;
        }
        substrate_writers[type] = sub_writer;
    }

    void reader_data_filter(evbs::ertps::rtps::IReaderDataFilter* filter);

 protected:
    //! Listener to capture the events of the Writer
    class DispatcherWriterListener : public WriterListener {
        friend class DispatcherWriter;

     public:
        DispatcherWriterListener(DispatcherWriter* w) : dispatch_writer_(w) {}

        ~DispatcherWriterListener() override {}

        void on_offered_incompatible_qos(evbs::edds::dds::PolicyMask qos) override;

        void onWriterChangeReceivedByAll(vbs::dispatcher::SubstrateWriter* const writer,
                                         vbs::common::CacheChange_t* change) override;

        void on_liveliness_lost(const evbs::ertps::LivelinessLostStatus& status) override;

        void on_reader_discovery(READER_DISCOVERY_STATUS reason, const GUID_t& reader_guid,
                                 vbs::RemoteEndpointInfo& reader_info) override;

        void on_endpoint_crash(const GUID_t& reader_guid) override;

        void on_non_plain_reader_discovery() override;

        DispatcherWriter* dispatch_writer_;
    } m_inner_listener_;

 private:
    DispatcherParticipant* m_participant;
    WriterHistory* m_history;
    WriterListener* m_uplistener;
    std::shared_ptr<IPayloadPool> m_payload_pool;
    std::shared_ptr<IChangePool> m_change_pool;
    GUID_t m_guid;
    WriterAttributes m_attr;
    TopicAttributes m_topic_attr;
    WriterQos m_qos;

    bool m_register_transport = false;
    size_t m_matched_reader_size = 0;

    std::array<SubstrateWriter*, SUBSTRATE_TYPE_MAX> substrate_writers = {nullptr};

    std::vector<GUID_t> m_remote_guids;
    vbs::shared_mutex m_listener_mtx;

    /*记录当前writer匹配的非plain的reader个数，决定发送数据是否需要做序列化 */
    std::atomic<uint16_t> m_no_plain_reader {0};
    void process_discovered_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info);
    void process_changed_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info);
    inline void process_removed_reader(const GUID_t& reader_guid, vbs::RemoteEndpointInfo& reader_info);
    inline void notify_matched_status(vbs::RemoteEndpointInfo& reader_info);
};

}  // namespace dispatcher
}  // namespace vbs

#endif
