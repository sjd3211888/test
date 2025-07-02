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

#ifndef DSFDDS_TRANSPORT_READERPROXY_H_
#define DSFDDS_TRANSPORT_READERPROXY_H_

// std include
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include "dsfdds/discovery/endpoint_proxy.h"
#include "dsfdds/transport/entity_proxy.h"
// lib include
#include "shmmq/message_queue.h"
#include "shmmq/shm_manager.h"
#include "shmmq/shmmq_manager.h"
#include "membuf/BaseType.h"
#include "common/all_common.h"
#include "common/property/PropertyParse.h"
#include "common/all_common.h"
#include "history/RemoteEndpointInfo.h"
#include "common/utils/SequenceNumber.h"
namespace li {
namespace dsfdds {

class ReaderProxy : public EntityProxy {
 public:
    ReaderProxy()
        : EntityProxy(),
          queue_mode_(RELIABLE_RELIABILITY_QOS),
          last_send_failed_(false),
          is_static_discovery_(false),
          user_id_(mbuf::MAX_REF_COUNT) {}
    ReaderProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint64_t pid, const std::string& topic_name,
                uint16_t domain_id, uint16_t participant_id, uint32_t entity_id, ReliabilityQosPolicyKind queue_mode,
                bool is_static_discovery)
        : EntityProxy(msg_queue, pid, topic_name, 0U, domain_id, participant_id, entity_id),
          queue_mode_(queue_mode),
          is_static_discovery_(is_static_discovery),
          user_id_(mbuf::MAX_REF_COUNT) {}

    ReaderProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, GUID_s guid, const std::string& topic_name,
                ReliabilityQosPolicyKind queue_mode, bool is_static_discovery)
        : EntityProxy(msg_queue, guid.get_pid_from_guid(), topic_name, 0U, guid.get_domain_id_from_guid(),
                      guid.get_participant_id_from_guid(), guid.get_entity_id_from_guid(), guid),
          queue_mode_(queue_mode),
          is_static_discovery_(is_static_discovery),
          user_id_(mbuf::MAX_REF_COUNT) {}

    // for vbs2.0
    ReaderProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint16_t domain_id,
                const vbs::RemoteEndpointInfo& rdata)
        : EntityProxy(msg_queue, rdata.guid().get_process_id(), rdata.topicName(), 0U, domain_id,
                      rdata.guid().get_participant_id(), rdata.guid().entityId.to_uint32()),
          queue_mode_((rdata.reliabilityKind() == ReliabilityKind_t::RELIABLE) ? RELIABLE_RELIABILITY_QOS
                                                                               : BEST_EFFORT_RELIABILITY_QOS),
          is_static_discovery_(false),
          user_id_(mbuf::MAX_REF_COUNT),
          durabilityKind_(rdata.durabilityKind()),
          evbs_guid_(rdata.guid()) {}

    ReaderProxy(const ReaderProxy& rp)
        : EntityProxy(rp.msg_queue_, rp.pid_, rp.topic_name_, rp.matched_status_, rp.domain_id_, rp.participant_id_,
                      rp.entity_id_, rp.guid_),
          queue_mode_(rp.queue_mode_),
          is_static_discovery_(rp.is_static_discovery_),
          user_id_(rp.user_id_),
          durabilityKind_(rp.durabilityKind()),
          evbs_guid_(rp.evbs_guid_),
          index_(rp.index_) {}
    //  在peer成功后设置这个值
    bool CheckSetQueueParam(uint32_t queue_id) const;
    inline bool set_msg_queue(uint32_t queue_id, ReliabilityQosPolicyKind remote_queue_mode) {
        bool ret = CheckSetQueueParam(queue_id);
        if (!ret) {
            return false;
        }

        shmmq::QueueAttr attr = {
            static_cast<uint8_t>(shmmq::queue_role::WRITER), 0U, 0U, 0U, 0U,
            static_cast<uint8_t>(shm::MemoryMode::KUNIMEM)  // 0:um-heap, 1: posix shm
        };

        if (queue_mode_ != remote_queue_mode) {
            DSF_LOG_DEBUG(
                READERPROXY,
                "The writer ReliabilityQosPolicyKind is different from the reader, "
                    << "[writer : reader]"
                    << " = [" << ((queue_mode_ == BEST_EFFORT_RELIABILITY_QOS) ? "best_effort" : "reliable") << " : "
                    << ((remote_queue_mode == BEST_EFFORT_RELIABILITY_QOS) ? "best_effort" : "reliable") << "]"
                    << ". and the reader takes precedence");
            queue_mode_ = remote_queue_mode;
        }
        ReliabilityQosPolicyKind kind = queue_mode_;
        if (kind == BEST_EFFORT_RELIABILITY_QOS) {
            attr.SetMode(shmmq::queue_mode::MODE_OVERWRITE);
            DSF_LOG_DEBUG(READERPROXY,
                          "open writer queue with kind : " << +static_cast<unsigned>(kind) << ",MODE_OVERWRITE");
        } else if (kind == RELIABLE_RELIABILITY_QOS) {
            attr.SetMode(shmmq::queue_mode::MODE_WRITE);
            DSF_LOG_DEBUG(READERPROXY,
                          "open writer queue with kind : " << +static_cast<unsigned>(kind) << ",MODE_WRITE");
        } else {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
                          "open writer queue with unknown kind : " << +static_cast<unsigned>(kind) << "!!!");
            return false;
        }
        DSF_LOG_INFO(READERPROXY,
                     "open writer queue with attr : role = "
                         << ((attr.role == (shmmq::queue_role::READER)) ? "reader" : "writer") << " queue_mode = "
                         << (((attr.queue_mode == shmmq::queue_mode::MODE_OVERWRITE)) ? "MODE_OVERWRITE" : "MODE_WRITE")
                         << " queue_size = " << attr.queue_size);
        auto msg_queue = shmmq::ShmmqManager::GetMQ(queue_id, attr);
        if (msg_queue == nullptr) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_GET_MQ_ERR,
                          "open writer queue: ShmmqManager::GetMQ with id " << queue_id << " return nullptr")
            return false;
        }
        msg_queue_ = std::move(msg_queue);
        return true;
    }

    bool is_last_send_failed() const;
    void set_last_send_failed(bool last_send_failed) const { last_send_failed_ = last_send_failed; }

    ReliabilityQosPolicyKind get_queue_mode() const { return queue_mode_; }
    uint64_t get_user_id() const { return user_id_; }
    void set_user_id(const uint64_t id) { user_id_ = id; }

    void add_sequence_number() { ++sequence_number; }
    SequenceNumber_t& get_sequence_number() { return sequence_number; }
    vbsutil::xmlparser::GUID_t get_vbs_guid() const { return evbs_guid_; }

    bool is_new_online() const { return new_online_; }
    void set_new_online(bool new_online) { new_online_ = new_online; }

    uint16_t get_index() const { return index_; }
    void set_index(const uint16_t index) { index_ = index; }

    const vbsutil::xmlparser::DurabilityKind_t& durabilityKind() const { return durabilityKind_; }

 private:
    ReliabilityQosPolicyKind queue_mode_;
    mutable bool last_send_failed_ {false};
    bool is_static_discovery_;
    uint64_t user_id_;
    std::string expression;
    std::string parameters;
    SequenceNumber_t sequence_number {0, 1U};
    bool new_online_ {true};
    vbsutil::xmlparser::DurabilityKind_t durabilityKind_ {DurabilityKind_t::VOLATILE};
    vbsutil::xmlparser::GUID_t evbs_guid_ {};
    uint16_t index_ = 0;  // the location where the write info(pid tid entityID) is stored in shmmq
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_READERPROXY_H_
