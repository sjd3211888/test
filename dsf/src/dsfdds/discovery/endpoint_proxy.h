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

#ifndef DSFDDS_DISCOVERY_ENDPOINTPROXY_H
#define DSFDDS_DISCOVERY_ENDPOINTPROXY_H

// std include
#include <array>
#include <cstdint>
#include <ostream>
#include <string>

// dds entity include
#include "DsfLog.hpp"
#include "dsfdds/dds/discovery/DiscoveryMessage.hpp"
#include "common/all_common.h"

namespace li {
namespace dsfdds {
enum EndpointMsgType { kPeer = 0x01U, kPeerAck, kDestruct, kDestructAck };

enum EndpointType { kWriter, kReader };

class EndpointProxy {
 public:
    EndpointProxy();
    EndpointProxy(const EndpointProxy& ep);
    EndpointProxy& operator=(const EndpointProxy& ep);
    ~EndpointProxy() = default;
    EndpointProxy(uint8_t endpoint_type, uint16_t local_domain_id, uint32_t local_participant_id,
                  uint32_t local_entity_id, uint16_t remote_domain_id, uint32_t remote_participant_id,
                  uint32_t remote_entity_id, uint64_t pid = 0U);

    // for vbs2.0
    EndpointProxy(uint32_t domain_id, const vbsutil::xmlparser::GUID_t local_guid,
                  const vbsutil::xmlparser::GUID_t remote_guid);

    bool operator==(const EndpointProxy& rhs) const;
    bool operator!=(const EndpointProxy& rhs) const;

    inline uint8_t get_msg_type() const { return msg_type_; }
    inline uint8_t get_endpoint_type() const { return endpoint_type_; }
    inline void SetPeerAckMsg() { msg_type_ = EndpointMsgType::kPeerAck; }
    inline void SetDestructAckMsg() { msg_type_ = EndpointMsgType::kDestructAck; }
    inline void SetDestructMsg() { msg_type_ = EndpointMsgType::kDestruct; }
    inline void SetPeerMsg() { msg_type_ = EndpointMsgType::kPeer; }
    inline void SetReader() { endpoint_type_ = EndpointType::kReader; }
    inline void SetWriter() { endpoint_type_ = EndpointType::kWriter; }
    void SwapCoreInfo();

    inline void set_pid(uint64_t pid) const { pid_ = pid; }

    inline std::string GetMsgTypeStr() const {
        switch (msg_type_) {
            case EndpointMsgType::kPeer:
                return "Peer";
            case EndpointMsgType::kPeerAck:
                return "PeerAck";
            case EndpointMsgType::kDestruct:
                return "Destruct";
            case EndpointMsgType::kDestructAck:
                return "DestructAck";
            default:
                return "";
        }
    }

    inline std::string GetEndpointTypeStr() const {
        switch (endpoint_type_) {
            case EndpointType::kReader:
                return "Reader";
            case EndpointType::kWriter:
                return "Writer";
            default:
                return "";
        }
    }
    static std::string GetEndpointTypeStr(const EndpointType& type) {
        switch (type) {
            case EndpointType::kReader:
                return "reader";
            case EndpointType::kWriter:
                return "writer";
        }
        return "";
    }

    uint16_t get_local_domain_id() const { return local_domain_id_; }
    void set_local_domain_id(uint16_t local_domain_id) { local_domain_id_ = local_domain_id; }
    uint32_t get_local_participant_id() const { return local_participant_id_; }
    void set_local_participant_id(uint32_t local_participant_id) { local_participant_id_ = local_participant_id; }
    uint32_t get_local_entity_id() const { return local_entity_id_; }
    void set_local_entity_id(uint32_t local_entity_id) { local_entity_id_ = local_entity_id; }
    uint16_t get_remote_domain_id() const { return remote_domain_id_; }
    void set_remote_domain_id(uint16_t remote_domain_id) { remote_domain_id_ = remote_domain_id; }
    uint32_t get_remote_participant_id() const { return remote_participant_id_; }
    void set_remote_participant_id(uint32_t remote_participant_id) { remote_participant_id_ = remote_participant_id; }
    uint32_t get_remote_entity_id() const { return remote_entity_id_; }
    void set_remote_entity_id(uint32_t remote_entity_id) { remote_entity_id_ = remote_entity_id; }
    vbsutil::xmlparser::GUID_t get_local_guid() const { return local_guid_; }
    vbsutil::xmlparser::GUID_t get_remote_guid() const { return remote_guid_; }
    inline bool IsPeeredProxy(const EndpointProxy& ep) const {
        return (this->get_local_domain_id() == ep.get_remote_domain_id()) &&
               (this->get_local_participant_id() == ep.get_remote_participant_id()) &&
               (this->get_local_entity_id() == ep.get_remote_entity_id()) &&
               (this->get_remote_domain_id() == ep.get_local_domain_id()) &&
               (this->get_remote_participant_id() == ep.get_local_participant_id()) &&
               (this->get_remote_entity_id() == ep.get_local_entity_id());
    }
    inline bool IsDefaultEntity() const {
        return (msg_type_ == 0U) && (endpoint_type_ == 0U) && (local_domain_id_ == 0U) &&
               (local_participant_id_ == 0U) && (local_entity_id_ == 0U) && (remote_domain_id_ == 0U) &&
               (remote_participant_id_ == 0U) && (remote_entity_id_ == 0U) && (pid_ == 0U);
    }
    uint64_t get_pid() const;
    friend std::ostream& operator<<(std::ostream& os, const EndpointProxy& proxy) {
        os << "msg_type_:" << proxy.GetMsgTypeStr() << ",endpoint_type:" << proxy.GetEndpointTypeStr()
           << ",local_domain_id:" << proxy.local_domain_id_ << ",local_participant_id:" << proxy.local_participant_id_
           << ",local_entity_id:" << proxy.local_entity_id_ << ",remote_domain_id:" << proxy.remote_domain_id_
           << ",remote_participant_id:" << proxy.remote_participant_id_
           << ",remote_entity_id:" << proxy.remote_entity_id_ << ",pid:" << proxy.pid_;
        return os;
    }

 private:
    /**
   * 目前支持类型
   * 0: 初始化默认值，无意义
   * 1: peer 多播匹配消息
   * 2：peer ack  应答匹配消息
   * 3：destruct 析构匹配消息
   * 4：destruct ack 析构匹配应答消息
   */
    uint8_t msg_type_ = 0U;
    /**
   * 端点信息位EndpointType，
   * 0：为写
   * 1：为读
   */
    uint8_t endpoint_type_ = 0U;

    // local 端点信息
    uint16_t local_domain_id_;
    uint32_t local_participant_id_;
    uint32_t local_entity_id_;
    vbsutil::xmlparser::GUID_t local_guid_;  // for vbs2.0
    // remote 端点信息
    uint16_t remote_domain_id_;
    uint32_t remote_participant_id_;
    uint32_t remote_entity_id_;
    vbsutil::xmlparser::GUID_t remote_guid_;  // for vbs2.0

    /**
   * 主要用在peer报文中匹配成功后更新matched_proxy中的pid值
   * 用于在对端异常后删除匹配信息。
   * 在ack 以及 destruct destruct ack报文中暂时未用的该值
   * 所以在这三种报文中pid值可以是任意端的值都无所谓
   */
    mutable uint64_t pid_;

 public:
    ReliabilityQosPolicyKind kind_ = ReliabilityQosPolicyKind {};
    QueueWakeUp queue_wake_up_ = QueueWakeUp {};
    uint32_t queue_msg_num_ = 0U;
    DiscoveryType discovery_type_ = DiscoveryType {};
};

struct EndpointProxyHasher {
    std::size_t operator()(const EndpointProxy& ep) const {
        std::size_t h1l = std::hash<uint16_t> {}(ep.get_local_domain_id());
        std::size_t h2l = std::hash<uint32_t> {}(ep.get_local_participant_id());
        std::size_t h3l = std::hash<uint32_t> {}(ep.get_local_entity_id());

        std::size_t h1r = std::hash<uint16_t> {}(ep.get_remote_domain_id());
        std::size_t h2r = std::hash<uint32_t> {}(ep.get_remote_participant_id());
        std::size_t h3r = std::hash<uint32_t> {}(ep.get_remote_entity_id());

        std::size_t hpid = std::hash<uint64_t> {}(ep.get_pid());

        return h1l ^ (h2l << 1U) ^ (h3l << 2U) ^ (h1r << 3U) ^ (h2r << 4U) ^ (h3r << 5U) ^ (hpid << 6U);
    }
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_DISCOVERY_ENDPOINTPROXY_H
