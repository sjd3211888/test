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

#ifndef DSFDDS_DISCOVERY_MESSAGE_H
#define DSFDDS_DISCOVERY_MESSAGE_H
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <ostream>
#include <string>
#include <unordered_map>
#include "xmlparser/qos/XMLQosPolicies.hpp"
#include "common/QosProperties.hpp"
#include "common/types/GUID.h"
#include "common/all_common.h"

namespace li {
namespace dsfdds {
#define UNKNOWN_RESERVE_VALUE 0xFF
// header_version define. rule: major << 8 | minor
#define VERSION_1_1 0x0101
#define HEADER_VERSION VERSION_1_1
class EntityInfoV1;
using EntityInfo = EntityInfoV1;

enum EntityStatus : int8_t { ENTITY_ONLINE, ENTITY_OFFLINE, ENTITY_CRASH, ENTITY_MATCH };
enum DiscoveryType : int8_t { STATIC_DISCOVERY, DYNAMIC_DISCOVERY };
enum PayloadType : uint8_t { DISCOVERY_REQ, DISCOVERY_RESP, DFX_REQ, DFX_RESP };
enum DiscoveryDfxCmd : uint8_t {
    DFX_LIST_ENTITIES,
    DFX_GET_ENTITY_INFO,
    DFX_SET_LOGLEVEL_TO_ERROR,
    DFX_SET_LOGLEVEL_TO_WARNING,
    DFX_SET_LOGLEVEL_TO_INFO,
    DFX_CMD_END
};
// entity class type
enum EntityClassType : uint8_t { EntityType1 };
inline std::ostream& operator<<(std::ostream& os, const EntityStatus& status) {
    switch (status) {
        case ENTITY_ONLINE:
            os << "online";
            break;
        case ENTITY_OFFLINE:
            os << "offline";
            break;
        case ENTITY_CRASH:
            os << "crash";
            break;
        case ENTITY_MATCH:
            os << "match";
            break;
        default:
            os << std::to_string(static_cast<int32_t>(status));
            break;
    }
    return os;
}
// Characterize the correspondence between EntityClassType and HEADER_VERSION for ease of performing Bane's test.
const std::unordered_map<EntityClassType, uint16_t> entity_class_type_to_header_version_map = {
    {EntityType1, VERSION_1_1}};

inline std::ostream& operator<<(std::ostream& os, EntityClassType type) {
    switch (type) {
        case EntityType1:
            os << "EntityClassType is EntityType1";
            break;
        default:
            os << "EntityClassType is Unknown";
            break;
    }
    return os;
}
struct DiscoveryHeader {
    DiscoveryHeader() = default;
    DiscoveryHeader(uint16_t header_version_, PayloadType payload_type_, uint32_t payload_len_)
        : header_version(header_version_),
          msg_header_len(sizeof(DiscoveryHeader)),
          payload_type(payload_type_),
          payload_len(payload_len_),
          seq_num(0),
          reserved {0} {}
    uint16_t header_version;  // 服务发现协议自身版本，服务发现无变化时版本号无变化，major（8bit）+ minor（8bit)
    uint8_t msg_header_len;    // 头部长度 in bytes
    PayloadType payload_type;  // 分为 DISCOVERY_REQ/DISCOVERY_RESP/DFX_REQ/DFX_RESP四类
    uint32_t payload_len;      // 负载长度 in bytes
    uint32_t seq_num;          // 服务发现协议消息自身的消息序号
    uint32_t reserved[5];      // 预留字段，用于消息协议的扩展
};

class EntityInfoBase {  // sizeof(EntityInfoBase) = 360
 public:
    ~EntityInfoBase() = default;
    EntityInfoBase() = default;
    EntityInfoBase(GUID_s guid_, EntityClassType entity_class_type_, const char* ptopic_name, const char* pdata_type,
                   char const* pbuild_version, EntityStatus status_, uint32_t recv_queue_id_,
                   ReliabilityQosPolicyKind kind_, uint32_t queue_id_, DiscoveryType discovery_type_, uint32_t poolId)
        : guid(guid_),
          entity_class_type(entity_class_type_),
          status(status_),
          kind(kind_),
          discovery_type(discovery_type_),
          recv_queue_id(recv_queue_id_),
          pool_id(poolId),
          queue_id(queue_id_) {
        static_cast<void>(memset(topic_name, 0, sizeof(topic_name)));
        static_cast<void>(memset(data_type, 0, sizeof(data_type)));
        static_cast<void>(memset(build_version, 0, sizeof(build_version)));
        static_cast<void>(strncpy(topic_name, ptopic_name, sizeof(topic_name) - 1));
        static_cast<void>(strncpy(data_type, pdata_type, sizeof(data_type) - 1));
        static_cast<void>(strncpy(build_version, pbuild_version, sizeof(build_version) - 1));
        timestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

 public:
    // 变量类型和顺序固定
    GUID_s guid;
    EntityClassType entity_class_type;
    EntityStatus status;
    ReliabilityQosPolicyKind kind;
    DiscoveryType discovery_type;
    char build_version[64];
    char topic_name[128];
    char data_type[128];
    uint32_t recv_queue_id;
    uint32_t pool_id;
    uint32_t queue_id;
    uint64_t timestamp;

    bool operator==(const EntityInfoBase& rhs) const { return (guid == rhs.guid); }

    bool operator!=(const EntityInfoBase& rhs) const { return !operator==(rhs); }

    bool operator<(const EntityInfoBase& rhs) const {
        if (guid < rhs.guid) {
            return true;
        }
        return false;
    }

    std::string Show() const {
        std::stringstream info;
        info << guid << ",topic_name:" << topic_name << ",data_type:" << data_type << ",status:" << status
             << ",recv_queue id:" << recv_queue_id << ",timestamp:" << timestamp
             << ",kind:" << ((kind == BEST_EFFORT_RELIABILITY_QOS) ? "best_effort" : "reliable")
             << ",queue_id:" << queue_id
             << ",discovery_type:" << ((discovery_type == STATIC_DISCOVERY) ? "static" : "dynamic")
             << ",pool id:" << pool_id << ",build_version:" << build_version << ", " << entity_class_type;
        return info.str();
    }
};

class EntityInfoV1 : public EntityInfoBase {  // sizeof(EntityInfoV1) = 392
 public:
    EntityInfoV1() = default;
    // for dsf discovery service
    EntityInfoV1(GUID_s guid_, const char* ptopic_name, const char* pdata_type, char const* pbuild_version,
                 EntityStatus status_, uint32_t recv_queue_id_, ReliabilityQosPolicyKind kind_, uint32_t queue_id_,
                 DiscoveryType discovery_type_, uint32_t poolId)
        : EntityInfoBase(guid_, EntityType1, ptopic_name, pdata_type, pbuild_version, status_, recv_queue_id_, kind_,
                         queue_id_, discovery_type_, poolId) {
        std::fill(std::begin(reserved), std::end(reserved), UNKNOWN_RESERVE_VALUE);
    }

    // for global discovery service
    EntityInfoV1(GUID_s guid_, const char* ptopic_name, const char* pdata_type, char const* pbuild_version,
                 EntityStatus status_, ReliabilityQosPolicyKind kind_, uint32_t queue_id_,
                 DiscoveryType discovery_type_, uint32_t poolId)
        : EntityInfoBase(guid_, EntityType1, ptopic_name, pdata_type, pbuild_version, status_, 0, kind_, queue_id_,
                         discovery_type_, poolId) {
        std::fill(std::begin(reserved), std::end(reserved), UNKNOWN_RESERVE_VALUE);
    }
    uint8_t reserved[32];  // 预留, 在构造函数中填充UNKNOWN_RESERVE_VALUE.
};

struct DiscoveryReqMsg {  // sizeof(DiscoveryReqMsg) = 424
    DiscoveryHeader header;
    EntityInfo entity_info;
    DiscoveryReqMsg() = default;
    DiscoveryReqMsg(const DiscoveryHeader& header_, const EntityInfo& entity_info_)
        : header(header_), entity_info(entity_info_) {}

    bool operator==(const DiscoveryReqMsg& rhs) const { return entity_info == rhs.entity_info; }

    bool operator!=(const DiscoveryReqMsg& rhs) const { return !operator==(rhs); }

    bool operator<(const DiscoveryReqMsg& rhs) const { return entity_info < rhs.entity_info; }

    std::string Show() const {
        std::stringstream info;
        std::string header_version_;
        if (header.header_version == VERSION_1_1) {
            header_version_ = "VERSION_1_1";
        }
        info << "Version: " << header_version_ << ", payload_type: "
             << ((header.payload_type == DISCOVERY_REQ || header.payload_type == DFX_REQ) ? "REQ" : "RESP")
             << entity_info.guid;
        return info.str();
    }
};

template <typename EntityInfoType>
struct EntityToEntityInfo {
    EntityInfoType local_info;
    EntityInfoType remote_info;
};
template <typename EntityInfoType>
struct DiscoveryRespMsg {
    DiscoveryHeader header;
    EntityToEntityInfo<EntityInfoType> e2e_info;
};

// Define DFX related structures from here on.
struct CliReqInfo {
    CliReqInfo() = default;
    CliReqInfo(DiscoveryDfxCmd dfx_cmd_, uint8_t domain_id_, const char* topic_name_)
        : dfx_cmd(dfx_cmd_), domain_id(domain_id_) {
        static_cast<void>(memset(topic_name, 0, 128U));
        static_cast<void>(memset(recv_queue, 0, 256U));
        static_cast<void>(strncpy(topic_name, topic_name_, 127U));
    }

    bool operator==(const CliReqInfo& rhs) const { return (domain_id == rhs.domain_id); }
    bool operator!=(const CliReqInfo& rhs) const { return !operator==(rhs); }

    DiscoveryDfxCmd dfx_cmd;
    uint8_t domain_id;
    char topic_name[128];
    char recv_queue[256];
};

struct CliRespInfo {
    CliRespInfo() = default;

    explicit CliRespInfo(const EntityInfoBase& base_entity_info) {
        dfx_cmd = DFX_CMD_END;
        domain_id = base_entity_info.guid.get_domain_id_from_guid();
        entity_type = base_entity_info.guid.get_entity_type_from_guid();
        participant_id = base_entity_info.guid.get_participant_id_from_guid();
        entity_id = base_entity_info.guid.get_entity_id_from_guid();
        pid = base_entity_info.guid.get_pid_from_guid();
        snprintf(topic_name, sizeof(topic_name), "%s", base_entity_info.topic_name);
        snprintf(data_type, sizeof(data_type), "%s", base_entity_info.data_type);
    }

    bool operator==(const CliRespInfo& rhs) const {
        return (domain_id == rhs.domain_id) && (participant_id == rhs.participant_id) && (entity_id == rhs.entity_id) &&
               (pid == rhs.pid);
    }

    bool operator!=(const CliRespInfo& rhs) const { return !operator==(rhs); }

    DiscoveryDfxCmd dfx_cmd;
    uint8_t domain_id;
    EntityType entity_type;
    uint16_t participant_id;
    uint32_t entity_id;
    uint32_t pid;
    char topic_name[128];
    char data_type[128];
};

struct DfxReqMsg {
    DiscoveryHeader header;
    CliReqInfo cli_info;
};

struct DfxRespMsg {
    DiscoveryHeader header;
    CliRespInfo cli_info;
};

}  // namespace dsfdds
}  // namespace li
#endif  // DSFDDS_DISCOVERY_MESSAGE_H
