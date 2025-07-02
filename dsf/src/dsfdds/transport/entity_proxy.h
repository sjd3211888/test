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

#ifndef DSFDDS_SRC_DSFDDS_TRANSPORT_ENTITY_PROXY_H_
#define DSFDDS_SRC_DSFDDS_TRANSPORT_ENTITY_PROXY_H_

// std include
#include <cstdint>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

// lib include
#include "DsfLog.hpp"
#include "shmmq/message_queue.h"
#include "common/types/GUID.h"

namespace li {
namespace dsfdds {

enum PeerMatchedBitMask : uint8_t {
    kLocalMatchedBitMask = (0x01U),
    kRemoteMatchedBitMask = (0x01U << 1),
    kUnmatchedBitMask = (0x01U << 2),
    kCrashedBitMask = (0x01U << 3),
    kMatchedCallBackBitMask = (0x01U << 4),
    kUnMatchedCallBackBitMask = (0x01U << 5),
    kAllMatched = 0x03U,
};
class EntityProxy {
 public:
    EntityProxy()
        : msg_queue_(nullptr),
          pid_(0U),
          topic_name_(""),
          matched_status_(0U),
          domain_id_(0U),
          participant_id_(0U),
          entity_id_(0U) {}
    EntityProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint64_t pid, const std::string& topic_name,
                uint8_t matched_status, uint16_t domain_id, uint16_t participant_id, uint32_t entity_id)
        : msg_queue_(msg_queue),
          pid_(pid),
          topic_name_(topic_name),
          matched_status_(matched_status),
          domain_id_(domain_id),
          participant_id_(participant_id),
          entity_id_(entity_id) {}
    EntityProxy(const std::shared_ptr<shmmq::MessageQueue>& msg_queue, uint64_t pid, const std::string& topic_name,
                uint8_t matched_status, uint16_t domain_id, uint16_t participant_id, uint32_t entity_id, GUID_s guid)
        : msg_queue_(msg_queue),
          pid_(pid),
          topic_name_(topic_name),
          matched_status_(matched_status),
          domain_id_(domain_id),
          participant_id_(participant_id),
          entity_id_(entity_id),
          guid_(guid) {}

    EntityProxy(const EntityProxy& ep) = default;

    EntityProxy& operator=(const EntityProxy& ep) = default;

    EntityProxy(EntityProxy&& ep) = default;

    EntityProxy& operator=(EntityProxy&& ep) = default;

    virtual ~EntityProxy() { DSF_LOG_DEBUG(EntityProxy, "EntityProxy:" << *this << " destruct!!"); }

    // 判断相等matched_status_ 不需要参与，因为这个值会动态变化
    // msg_queue_writer在初始化时候为nullptr 也不参与判断
    bool operator==(const EntityProxy& rhs) const {
        return (pid_ == rhs.pid_) && (topic_name_ == rhs.topic_name_) && (domain_id_ == rhs.domain_id_) &&
               (participant_id_ == rhs.participant_id_) && (entity_id_ == rhs.entity_id_);
    }
    bool operator!=(const EntityProxy& rhs) const { return !(rhs == *this); }

    std::shared_ptr<shmmq::MessageQueue> get_msg_queue() const { return msg_queue_; }

    inline void SetLocalMatched() {
        matched_status_ |= static_cast<unsigned>(PeerMatchedBitMask::kLocalMatchedBitMask);
    }
    inline void ClearLocalMatched() {
        matched_status_ &= (~static_cast<unsigned>(PeerMatchedBitMask::kLocalMatchedBitMask));
    }

    inline void SetRemoteMatched() {
        matched_status_ |= static_cast<unsigned>(PeerMatchedBitMask::kRemoteMatchedBitMask);
    }
    inline void ClearRemoteMatched() {
        matched_status_ &= (~static_cast<unsigned>(PeerMatchedBitMask::kRemoteMatchedBitMask));
    }

    inline void SetUnmatched() { matched_status_ |= static_cast<unsigned>(PeerMatchedBitMask::kUnmatchedBitMask); }
    inline void ClearUnmatched() { matched_status_ &= (~static_cast<unsigned>(PeerMatchedBitMask::kUnmatchedBitMask)); }
    inline bool IsUnmatched() const {
        return (matched_status_ & static_cast<unsigned>(PeerMatchedBitMask::kUnmatchedBitMask)) ==
               static_cast<unsigned>(PeerMatchedBitMask::kUnmatchedBitMask);
    }

    inline void SetCrashed() { matched_status_ |= static_cast<unsigned>(PeerMatchedBitMask::kCrashedBitMask); }
    inline void ClearCrashed() {
        matched_status_ &= static_cast<uint8_t>((~static_cast<uint8_t>(PeerMatchedBitMask::kCrashedBitMask)));
    }
    inline bool IsCrashed() const {
        return (matched_status_ & static_cast<unsigned>(PeerMatchedBitMask::kCrashedBitMask)) ==
               static_cast<unsigned>(PeerMatchedBitMask::kCrashedBitMask);
    }

    inline bool IsLocalMatched() const {
        return (matched_status_ & static_cast<unsigned>(PeerMatchedBitMask::kLocalMatchedBitMask)) ==
               static_cast<unsigned>(PeerMatchedBitMask::kLocalMatchedBitMask);
    }

    inline bool IsRemoteMatched() const {
        return (matched_status_ & static_cast<unsigned>(PeerMatchedBitMask::kRemoteMatchedBitMask)) ==
               static_cast<unsigned>(PeerMatchedBitMask::kRemoteMatchedBitMask);
    }

    inline bool IsPeerMatched() const {
        return (matched_status_ & static_cast<unsigned>(PeerMatchedBitMask::kAllMatched)) ==
               static_cast<unsigned>(PeerMatchedBitMask::kAllMatched);
    }

    inline std::string get_queue_name() const { return msg_queue_->get_name(); }
    inline uint32_t get_queue_id() const { return msg_queue_->get_queue_id(); }
    inline uint32_t get_queue_size() const { return msg_queue_->get_size(); }

    inline uint8_t get_matched_status() const { return matched_status_; }

    inline uint64_t get_pid() const { return pid_; }
    void set_pid(uint64_t pid) { pid_ = pid; }
    inline uint8_t get_domain_id() const { return domain_id_; }
    void set_domain_id(uint8_t domainId) { domain_id_ = domainId; }
    inline uint16_t get_participant_id() const { return participant_id_; }
    void set_participant_id(uint16_t participantId) { participant_id_ = participantId; }
    inline uint32_t get_entity_id() const { return entity_id_; }
    void set_entity_id(uint32_t entityId) { entity_id_ = entityId; }
    inline GUID_s get_guid() const { return guid_; }
    void set_guid(GUID_s guid) { guid_ = guid; }

    inline void SetMatchedCallBack() { matched_status_ |= PeerMatchedBitMask::kMatchedCallBackBitMask; }
    inline void ClearMatchedCallBack() {
        matched_status_ &= static_cast<uint8_t>((~static_cast<uint8_t>(PeerMatchedBitMask::kMatchedCallBackBitMask)));
    }
    inline bool IsMatchedCallBack() const {
        return (matched_status_ & PeerMatchedBitMask::kMatchedCallBackBitMask) ==
               static_cast<unsigned>(PeerMatchedBitMask::kMatchedCallBackBitMask);
    }

    inline void SetUnmatchedCallBack() { matched_status_ |= PeerMatchedBitMask::kUnMatchedCallBackBitMask; }
    inline void ClearUnmatchedCallBack() {
        matched_status_ &= static_cast<uint8_t>((~static_cast<uint8_t>(PeerMatchedBitMask::kUnMatchedCallBackBitMask)));
    }
    inline bool IsUnmatchedCallBack() const {
        return (matched_status_ & PeerMatchedBitMask::kUnMatchedCallBackBitMask) ==
               static_cast<unsigned>(PeerMatchedBitMask::kUnMatchedCallBackBitMask);
    }

    inline void ClearMatchedReverseStatus() {
        ClearUnmatched();
        ClearCrashed();
        ClearUnmatchedCallBack();
    }

    inline void ClearUnmatchedReverseStatus() {
        ClearLocalMatched();
        ClearRemoteMatched();
        ClearMatchedCallBack();
    }

    inline const std::string& get_topic_name() const { return topic_name_; }

    friend std::ostream& operator<<(std::ostream& os, const EntityProxy& proxy) {
        os << "Ids:" << proxy.pid_ << "_" << +proxy.domain_id_ << "_" << proxy.participant_id_ << "_"
           << proxy.entity_id_ << ",queue:" << ((proxy.msg_queue_ == nullptr) ? "nullptr" : proxy.get_queue_name())
           << ",Qid:" << ((proxy.msg_queue_ == nullptr) ? 0U : proxy.get_queue_id()) << ",topic:" << proxy.topic_name_
           << ",status:" << +proxy.matched_status_;
        return os;
    }
    /*
mbuf_0_10_HelloWorldTopic_8adee->0_10_HelloWorldTopic
*/
    static std::string extractStringBetweenUnderscores(const std::string& str) {
        size_t first = str.find_first_of("_");  // 找到第一个下划线位置
        size_t last = str.find_last_of("_");    // 找到最后一个下划线位置
        if ((first == std::string::npos) || (last == std::string::npos) || (first == last)) {
            // 如果字符串中没有下划线或只有一个下划线，则返回空字符串
            return "";
        }
        return str.substr(first + 1U,
                          last - first - 1U);  // 返回第一个和最后一个下划线之间的子串
    }

    static bool starts_with(const std::string& str, const std::string& prefix) {
        return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0);
    }

    void ResetMsgQueue() { msg_queue_.reset(); }

 protected:
    std::shared_ptr<shmmq::MessageQueue> msg_queue_;
    uint64_t pid_;
    std::string topic_name_;
    /**
   * 第0位为1表示本地的writer已经成功匹配远程的reader
   *     该值在receive到远程多播消息并且匹配成功时候更新
   * 第1位为1表示远程的reader已经成功匹配本地的writer
   *     该值在receive到远程的匹配应答消息更新
   * 低2位为3表示 已经双向匹配
   * 第2位为1标识 已经触发peer matched回调无需再触发
   * 第3位为1标识 收到了对端的正常析构报文触发unmatched，初始值为0
   * 第4位为1表示 收到了对端crash的信息，初始值为0
     总结：
     1. 0和1一起组成了matched状态标识，3是unmatched状态标识，4是crashed状态标识
     2. 但是对应的状态需要有回调，所以还需要有回调事件的标识，但是由于crash是同步回调的，就不需要额外的事件标识
     这里只针对异步的matched和unmatched事件进行标识.

     同时为了支持在没有matched状态切换期间只回调一次还需要设置已经触发的标识，事件回调完在matched期间不再重复回调
      第2位为matched是否已经回调的标识，在异常或者unmatched的时候（这2个时刻是matched状态的临界点）要清除。
      第5位为unmatched是否回调的标识，在新匹配开始的时候（（这个时刻是unmatched状态的临界点））清除。
   */
    uint8_t matched_status_;
    uint8_t domain_id_;
    uint32_t participant_id_;
    uint32_t entity_id_;
    GUID_s guid_ {};
};
}  // namespace dsfdds
}  // namespace li

#endif  // DTSDDS_SRC_DSFDDS_TRANSPORT_ENTITY_PROXY_H_
