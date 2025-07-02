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

#ifndef INCLUDE_DISPATCHER_READER_H
#define INCLUDE_DISPATCHER_READER_H

#include "edds/rtps/attributes/ReaderAttributes.h"
#include "ertps/attributes/TopicAttributes.h"
#include "xmlparser/qos/XMLReaderQos.hpp"
#include "deps/common/Guid.h"
#include "deps/common/EntityId_t.hpp"
#include "history/IChangePool.h"
#include "history/ReaderHistory.h"
#include "history/ReaderListener.h"
#include "history/RemoteEndpointInfo.h"
#include "deps/common/shared_mutex.hpp"
#include "DispatcherParticipant.hpp"
#include "history/SubstrateReader.h"

using namespace evbs::ertps::rtps;
using namespace vbsutil::xmlparser;

namespace evbs {
namespace edds {
namespace dds {
namespace builtin {
struct ProxyInfo;
using StatisticProxyInfos = std::vector<ProxyInfo>;
}  // namespace builtin
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {
namespace dispatcher {

class DispatcherReader {

 public:
    DispatcherReader(DispatcherParticipant* disPart, GUID_t guid, ReaderAttributes& ratt, TopicAttributes& topic_att,
                     ReaderQos& rqos, std::shared_ptr<IChangePool> change_pool, ReaderHistory* history,
                     ReaderListener* listener);

    ~DispatcherReader();
    bool setListener(ReaderListener* const listener) {
        std::lock_guard<vbs::shared_mutex> guard(m_listener_mtx);
        m_uplistener = listener;
        return true;
    }
    ReaderListener* get_uplistener() { return m_uplistener; }

    const GUID_t getGuid() { return m_guid; }
    const std::string getTopicName() const { return m_topic_attr.getTopicName().to_string(); }
    const std::string getTopicType() const { return m_topic_attr.getTopicDataType().to_string(); }
    size_t getMatchedWritersSize() const { return m_matched_writer_size; }

    void disable() {
        for (SubstrateReader* substrate_reader : substrate_readers) {
            if (substrate_reader) {
                substrate_reader->disable();
            }
        }
    }

    ReaderAttributes& getAttributes() { return m_attr; }

    bool registerReader(const vbsutil::xmlparser::ContentFilterProperty* content_filter_property = nullptr);

    bool updateReader(const TopicAttributes& topicAtt, const ReaderQos& wqos,
                      const vbsutil::xmlparser::ContentFilterProperty* content_filter = nullptr);

    bool begin_sample_access_nts(CacheChange_t* change, bool& is_future_change);
    evbs::edds::dds::builtin::StatisticProxyInfos get_proxy_infos();

    std::vector<GUID_t> get_remote_guids() { return m_remote_guids; }

    bool wait_for_unread_cache(const evbs::ertps::Duration_t& timeout);

    SubstrateReader* getSubstrateEndpoint(uint32_t type) {
        if (type >= SUBSTRATE_TYPE_MAX) {
            return nullptr;
        }
        return substrate_readers[type];
    }
    vbs::shared_mutex& getListenerMutex() { return m_listener_mtx; }

    void set_register_transport(bool flag) { m_register_transport = flag; }

    bool get_register_transport() { return m_register_transport; }

    TopicAttributes get_topic_attr() { return m_topic_attr; }

    ReaderQos get_qos() { return m_qos; }

    std::shared_ptr<IChangePool> get_change_pool() { return m_change_pool; }

    ReaderHistory* get_history() { return m_history; }

    void setSubstrateEndpoint(uint32_t type, SubstrateReader* sub_reader) {
        if (type >= SUBSTRATE_TYPE_MAX) {
            return;
        }
        substrate_readers[type] = sub_reader;
    }

 protected:
    //! Listener to capture the events of the Writer
    class DispatcherReaderListener : public ReaderListener {
        friend class DispatcherReader;

     public:
        DispatcherReaderListener(DispatcherReader* r) : dispatch_reader_(r) {}

        ~DispatcherReaderListener() override {}

        void on_liveliness_changed(const evbs::edds::dds::LivelinessChangedStatus& status) override;
        void on_requested_incompatible_qos(evbs::edds::dds::PolicyMask qos) override;

        void on_sample_lost(const int32_t sample_lost_since_last_update,
                            const SampleLostStatusKind statusKind) override;
        void on_writer_discovery(evbs::edds::dds::WRITER_DISCOVERY_STATUS reason, const GUID_t& writer_guid,
                                 vbs::RemoteEndpointInfo* writer_info) override;

        void on_sample_rejected(evbs::edds::dds::SampleRejectedStatusKind reason,
                                const CacheChange_t* const change) override;

        void on_data_available(const GUID_t& writer_guid, const SequenceNumber_t& first_sequence,
                               const SequenceNumber_t& last_sequence, bool& should_notify_individual_changes) override;

        void on_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) override;

        void on_endpoint_crash(const GUID_t& reader_guid) override;

        DispatcherReader* dispatch_reader_;
    } m_inner_listener_;

 private:
    DispatcherParticipant* m_participant;
    ReaderHistory* m_history;
    ReaderListener* m_uplistener;
    std::shared_ptr<IChangePool> m_change_pool;
    GUID_t m_guid;
    ReaderAttributes m_attr;
    TopicAttributes m_topic_attr;
    ReaderQos m_qos;

    bool m_register_transport = false;
    uint16_t m_matched_writer_size = 0;
    vbs::shared_mutex m_listener_mtx;

    std::array<SubstrateReader*, SUBSTRATE_TYPE_MAX> substrate_readers = {nullptr};
    std::vector<GUID_t> m_remote_guids;

    void process_discovered_writer(const GUID_t& writer_guid, vbs::RemoteEndpointInfo* writer_info);
    void process_changed_writer(const GUID_t& writer_guid, vbs::RemoteEndpointInfo* writer_info);
    inline void process_removed_writer(const GUID_t& writer_guid, vbs::RemoteEndpointInfo* writer_info);
    inline void notify_matched_status(const GUID_t& writer_guid);
};

}  // namespace dispatcher
}  // namespace vbs

#endif
