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

#ifndef DSFDDS_SRC_DSFDDS_TRANSPORT_TRANSPORT_ENTITY_H_
#define DSFDDS_SRC_DSFDDS_TRANSPORT_TRANSPORT_ENTITY_H_
#include <mutex>
#include <functional>
#include <unordered_set>
#include "dsfdds/transport/entity_proxy.h"
#include "dsfdds/discovery/endpoint_proxy.h"
#include "common/types/GUID.h"
#include "common/all_common.h"
#include "deps/common/Guid.h"

namespace li {
namespace dsfdds {
using MbufPoolUUID = uint32_t;
class TransportEntity {
    friend class DiscoveryEndpoint;
    friend class GlobalDiscoveryEndpoint;
    friend class MqDiscoveryEndpoint;
    friend class VbsDiscoveryEndpoint;
    friend class DataWriterImpl;
    friend class DataReaderImpl;
    friend class ShmCreator;
    friend class CrashListener;
    friend class MembufPoolCrashStrategy;
    friend class ShmReader;
    friend class ShmWriter;

 public:
    TransportEntity(GUID_s guid, const std::string& topic_name, bool is_static_discovery,
                    ReliabilityQosPolicyKind reliability_kind)
        : domain_id_(guid.get_domain_id_from_guid()),
          participant_id_(guid.get_participant_id_from_guid()),
          entity_id_(guid.get_entity_id_from_guid()),
          guid_(guid),
          topic_name_(topic_name),
          topic_type_(""),
          is_static_discovery_(is_static_discovery),
          reliability_kind_(reliability_kind) {}
    TransportEntity(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                    const std::string& topic_type, bool is_static_discovery, ReliabilityQosPolicyKind reliability_kind)
        : domain_id_(domainID),
          participant_id_(guid.get_participant_id()),
          entity_id_(guid.entityId.to_uint32()),
          guid_(),
          topic_name_(topic_name),
          topic_type_(topic_type),
          is_static_discovery_(is_static_discovery),
          reliability_kind_(reliability_kind) {}
    ~TransportEntity() = default;

    inline uint16_t get_domain_id() const { return domain_id_; }
    inline uint16_t get_participant_id() const { return participant_id_; }
    inline uint32_t get_entity_id() const { return entity_id_; }
    inline GUID_s get_guid() const { return guid_; };
    inline const std::string& get_topic_name() const { return topic_name_; }
    inline const std::string& get_topic_type() const { return topic_type_; }
    inline ReliabilityQosPolicyKind get_reliability_kind() const { return reliability_kind_; };
    void set_on_endpoint_matched(std::function<void(EntityProxy&)>&& on_endpoint_matched) {
        on_endpoint_matched_ = std::move(on_endpoint_matched);
    }
    void set_on_endpoint_crash(std::function<void(const EndpointProxy&)>&& on_endpoint_crash) {
        on_endpoint_crash_ = std::move(on_endpoint_crash);
    }

 private:
    uint16_t domain_id_;
    uint16_t participant_id_;
    uint32_t entity_id_;
    GUID_s guid_;
    // 便于异常处理加入topic指针
    std::string topic_name_;
    std::string topic_type_;
    bool is_static_discovery_ {false};
    std::function<void(EntityProxy&)> on_endpoint_matched_;
    std::function<void(const EndpointProxy&)> on_endpoint_crash_;
    ReliabilityQosPolicyKind reliability_kind_;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_SRC_DSFDDS_TRANSPORT_TRANSPORT_ENTITY_H_
