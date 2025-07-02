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

#ifndef DSFDDS_TRANSPORT_WRITERPROXY_H_
#define DSFDDS_TRANSPORT_WRITERPROXY_H_
// std include
#include <memory>
#include <string>
#include <utility>
#include <unordered_set>
#include <ostream>

#include "dsfdds/transport/entity_proxy.h"
// lib include
#include "shmmq/message_queue.h"
#include "history/RemoteEndpointInfo.h"

namespace li {
namespace dsfdds {

class WriterProxy : public EntityProxy {
 public:
    WriterProxy() : EntityProxy(), pool_id_(0), pool_ids_ {} {}
    WriterProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint64_t pid, const std::string& topic_name,
                uint16_t domain_id, uint16_t participant_id, uint32_t entity_id, uint32_t pool_id)
        : EntityProxy(msg_queue, pid, topic_name, 0U, domain_id, participant_id, entity_id),
          pool_id_(pool_id),
          pool_ids_ {} {
        pool_ids_.emplace(pool_id);
    }

    WriterProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, GUID_s guid, const std::string& topic_name,
                uint32_t pool_id)
        : EntityProxy(msg_queue, guid.get_pid_from_guid(), topic_name, 0U, guid.get_domain_id_from_guid(),
                      guid.get_participant_id_from_guid(), guid.get_entity_id_from_guid(), guid),
          pool_id_(pool_id),
          pool_ids_ {} {
        pool_ids_.emplace(pool_id);
    }

    WriterProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint16_t domain_id,
                const vbs::RemoteEndpointInfo& wdata, uint32_t pool_id)
        : EntityProxy(msg_queue, wdata.guid().get_process_id(), wdata.topicName(), 0U, domain_id,
                      wdata.guid().get_participant_id(), wdata.guid().entityId.to_uint32()),
          pool_id_(pool_id),
          pool_ids_ {} {
        pool_ids_.emplace(pool_id);
    }

    WriterProxy(const WriterProxy& wp)
        : EntityProxy(wp.msg_queue_, wp.pid_, wp.topic_name_, wp.matched_status_, wp.domain_id_, wp.participant_id_,
                      wp.entity_id_, wp.guid_),
          pool_id_(wp.pool_id_),
          pool_ids_ {} {
        pool_ids_.emplace(wp.pool_id_);
    }

    friend std::ostream& operator<<(std::ostream& os, const WriterProxy& proxy) {
        os << static_cast<const EntityProxy&>(proxy) << " pool_id_: " << proxy.pool_id_;
        return os;
    }
    uint32_t pool_id_;

    // 用于记录 writer 发生扩容时使用过的所有 pool id, 当 writer crash 时, crash 回调收到 pool crash 信号判断该 pool 是否属于 matched writer
    std::unordered_set<uint32_t> pool_ids_;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_WRITERPROXY_H_
