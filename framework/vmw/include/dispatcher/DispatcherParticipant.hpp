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

#ifndef INCLUDE_DISPATCHER_PARTICIPANT_H
#define INCLUDE_DISPATCHER_PARTICIPANT_H

#include "deps/common/Time_t.h"
#include "edds/rtps/attributes/RTPSParticipantAttributes.h"
#include "edds/rtps/attributes/WriterAttributes.h"
#include "edds/rtps/attributes/ReaderAttributes.h"
#include "history/IPayloadPool.h"
#include "history/IChangePool.h"
#include "history/ReaderHistory.h"
#include "history/WriterHistory.h"
#include "history/ReaderListener.h"
#include "history/WriterListener.h"
#include "history/ParticipantListener.h"
#include "transport/TransportFactory.hpp"
#include "history/SubstrateParticipant.h"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"

using namespace evbs::ertps::rtps;
using namespace vbsutil::xmlparser;

namespace evbs {
namespace edds {
namespace dds {
namespace builtin {
class LookupManager;
}
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {
namespace dispatcher {

class DispatcherWriter;
class DispatcherReader;

class DispatcherParticipant {
 public:
    DispatcherParticipant(uint32_t domainId, evbs::ertps::rtps::RTPSParticipantAttributes& attr,
                          vbs::common::ParticipantListener* listener);
    ~DispatcherParticipant();

    DispatcherWriter* create_writer(GUID_t guid, WriterAttributes& watt, TopicAttributes& topic_att, WriterQos& wqos,
                                    std::shared_ptr<IPayloadPool>& payload_pool,
                                    std::shared_ptr<IChangePool>& change_pool, WriterHistory* history,
                                    WriterListener* listener);

    DispatcherReader* create_reader(GUID_t guid, ReaderAttributes& ratt, TopicAttributes& topic_att, ReaderQos& wqos,
                                    std::shared_ptr<IChangePool>& change_pool, ReaderHistory* history,
                                    ReaderListener* listener);

    bool delete_writer(DispatcherWriter* writer);

    bool delete_reader(DispatcherReader* reader);

    const std::vector<DispatcherWriter*>& getAllWriters() const;
    const std::vector<DispatcherReader*>& getAllReaders() const;

    bool active() { return b_active; }
    //! Get the GUID_t of the DispatcherParticipant.
    const GUID_t& getGuid() const;
    void enable();
    void set_listener(vbs::common::ParticipantListener* listener);

    //only for rtps
    evbs::edds::dds::builtin::LookupManager* lookup_manager() const;

    bool startRTPSParticipantInitialAnnouncement(uint32_t count, vbsutil::Duration_t period);
    EvbsVersion_t get_remote_evbsversion();

    /**
     * Get a copy of the actual state of the DispatcherParticipantParameters
     * @return DispatcherParticipantAttributes copy of the params.
     */
    const evbs::ertps::rtps::RTPSParticipantAttributes& getParticipantAttributes() const { return m_attribute; }

    void update_attributes(const evbs::ertps::rtps::RTPSParticipantAttributes& patt);

    SubstrateParticipant* getSubstrateParticipant(uint32_t type) {
        if (type < SUBSTRATE_TYPE_MAX) {
            return substrate_participants[type];
        }
        return nullptr;
    }
    RetCode_t assert_liveliness();

    vbs::common::ResourceEvent& get_resource_event() const;

    void create_dsf_participant();

    std::vector<std::string> getParticipantNames();
    uint32_t domainId() { return m_domain_id; }

    bool dsfDriverUp() { return b_dsf_driver_up; }

    template <typename RW, typename RW_INNER_LISTERNER>
    bool process_discovered_use_dsf(RW* dispatcher_endpoint, RW_INNER_LISTERNER* inner_listener,
                                    const GUID_t& remote_guid) {
        Locator locator;
        if (!(dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF))) {
            SubstrateParticipant* subPart = getSubstrateParticipant(SUBSTRATE_TYPE_DSF);
            if (!subPart) {
                create_dsf_participant();
                subPart = getSubstrateParticipant(SUBSTRATE_TYPE_DSF);
            }
            if (subPart) {
                SubstrateParticipant* rtps_subPart = getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
                std::shared_ptr<IChangePool> change_pool = dispatcher_endpoint->get_change_pool();
                if constexpr (std::is_same_v<RW, DispatcherReader>) {
                    SubstrateReader* dsf_reader = subPart->create_reader(
                        dispatcher_endpoint->getGuid(), dispatcher_endpoint->getAttributes(),
                        dispatcher_endpoint->getTopicName(), dispatcher_endpoint->getTopicType(), change_pool,
                        dispatcher_endpoint->get_history(), inner_listener, &locator);
                    if (!dsf_reader) {
                        logInfo(DISPATCHER, "create dsf reader failed .");
                        return false;
                    }
                    dispatcher_endpoint->setSubstrateEndpoint(SUBSTRATE_TYPE_DSF, dsf_reader);
                    if (dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF)) {
                        dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF)
                            ->set_trust_remote_endpoint(remote_guid);
                        SubstrateReader* rtps_reader = dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS);
                        if (rtps_reader && rtps_subPart) {
                            rtps_reader->add_announce_locator(locator);
                            rtps_subPart->updateReader(rtps_reader, dispatcher_endpoint->get_topic_attr(),
                                                       dispatcher_endpoint->get_qos(), nullptr, false);
                            logInfo(DISPATCHER, "create dsf reader success .");
                            return true;
                        }
                    } else {
                        logInfo(DISPATCHER, "dsf reader is nullptr .");
                        return false;
                    }
                } else if constexpr (std::is_same_v<RW, DispatcherWriter>) {
                    std::shared_ptr<IPayloadPool> payload_pool = dispatcher_endpoint->get_payload_pool();
                    SubstrateWriter* dsf_writer = subPart->create_writer(
                        dispatcher_endpoint->getGuid(), dispatcher_endpoint->getAttributes(),
                        dispatcher_endpoint->getTopicName(), dispatcher_endpoint->getTopicType(), payload_pool,
                        change_pool, dispatcher_endpoint->get_history(), inner_listener, &locator);
                    if (!dsf_writer) {
                        logInfo(DISPATCHER, "create dsf writer failed .");
                        return false;
                    }
                    dispatcher_endpoint->setSubstrateEndpoint(SUBSTRATE_TYPE_DSF, dsf_writer);
                    if (dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF)) {
                        SubstrateWriter* rtps_writer = dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS);
                        if (rtps_subPart && rtps_writer) {
                            rtps_writer->add_announce_locator(locator);
                            rtps_subPart->updateWriter(rtps_writer, dispatcher_endpoint->get_topic_attr(),
                                                       dispatcher_endpoint->get_qos(), nullptr, false);
                            logInfo(DISPATCHER, "create dsf writer success .");
                            return true;
                        }
                    } else {
                        logInfo(DISPATCHER, "dsf writer is nullptr .");
                        return false;
                    }
                }
            } else {
                logInfo(DISPATCHER, "create dsf participant failed.");
                return false;
            }
        } else {
            if constexpr (std::is_same_v<RW, DispatcherReader>) {
                dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_DSF)->set_trust_remote_endpoint(remote_guid);
            }
        }
        return true;
    }

    template <typename RW>
    void process_discovered_use_uds_or_udp(RW* dispatcher_endpoint, vbs::transport::TransmitorType type) {
        if (!(dispatcher_endpoint->get_register_transport())) {
            Locator locator;
            SubstrateParticipant* subPart = getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
#if !defined(_WIN32)
            if (type == vbs::transport::TransmitorType::TRANSMITOR_TYPE_UDS) {
                std::string path = IPLocator::uintToHexString(getGuid().get_process_id()) +
                                   IPLocator::uintToHexString(getGuid().get_participant_id() % 0xFF, 2) + "D";
                (void)IPLocator::setPathToLocator(path, locator);
                locator.kind = LOCATOR_KIND_UDS;
            } else
#endif
            {
                locator.kind = LOCATOR_PORT_INVALID;
            }
            (void)subPart->RegisterTransport(static_cast<uint32_t>(type));
            dispatcher_endpoint->set_register_transport(true);
            (dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->add_announce_locator(locator);
            if constexpr (std::is_same_v<RW, DispatcherReader>) {
                subPart->updateReader(dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS),
                                      dispatcher_endpoint->get_topic_attr(), dispatcher_endpoint->get_qos(), nullptr,
                                      false);
            } else if constexpr (std::is_same_v<RW, DispatcherWriter>) {
                subPart->updateWriter(dispatcher_endpoint->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS),
                                      dispatcher_endpoint->get_topic_attr(), dispatcher_endpoint->get_qos(), nullptr,
                                      false);
            }
        }
    }

 private:
    uint32_t m_domain_id;
    evbs::ertps::rtps::RTPSParticipantAttributes m_attribute;
    vbs::common::ParticipantListener* m_listener;
    bool b_active = false;
    bool b_dsf_driver_up = false;

    std::array<SubstrateParticipant*, SUBSTRATE_TYPE_MAX> substrate_participants = {nullptr};

    std::vector<DispatcherWriter*> m_writer_list;
    std::vector<DispatcherReader*> m_reader_list;
};

}  // namespace dispatcher
}  // namespace vbs

#endif
