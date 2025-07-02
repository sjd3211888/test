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

#include <limits>
#include <unistd.h>
// dsfdds include
#include "DsfLog.hpp"
#include "dsfdds/transport/shm_writer.h"
// lib include
#include "membuf/ShareBuffer.h"
#include "membuf/BufferDescriptor.h"
#include "shmmq/message_queue.h"
#include "shm_creator.h"
#include "common/all_common.h"
#include <sys/syscall.h>

#define WRITER_METRICS_COUNT_THRESHOLD 10

namespace li {
namespace dsfdds {

ShmWriter::ShmWriter(const std::string& topic_name, GUID_s guid, bool is_static_discovery,
                     ReliabilityQosPolicyKind reliability_kind)
    : TransportEntity(guid, topic_name, is_static_discovery, reliability_kind),
      enabled_(false),
      force_kill_(false),
      mode(mbuf::RecoverMode::Normal),
      w_pid_(guid.get_pid_from_guid()) {
    DSF_LOG_DEBUG(ShmWriter, "ctor, this: " << this << ",writer ids:" << get_domain_id() << "," << get_participant_id()
                                            << "," << get_entity_id());
}

ShmWriter::ShmWriter(uint32_t domainID, const std::string& topic_name, std::string& topic_type,
                     vbsutil::xmlparser::GUID_t guid, bool is_static_discovery,
                     ReliabilityQosPolicyKind reliability_kind)
    : TransportEntity(domainID, guid, topic_name, topic_type, is_static_discovery, reliability_kind),
      enabled_(false),
      force_kill_(false),
      mode(mbuf::RecoverMode::Normal),
      w_pid_(guid.get_process_id()),
      guid_t(guid) {
    DSF_LOG_INFO(ShmWriter, "ctor, this: " << this << ",writer ids:" << get_domain_id() << "," << get_participant_id()
                                           << "," << get_entity_id());
}

bool ShmWriter::AddMatchedReader(const EndpointProxy& ep, ReaderProxy& rp) {
    std::lock_guard<std::mutex> lock(mtx_matched_reader_gs_);
    auto findIt = matched_readers_.find(ep);
    if (findIt == matched_readers_.end()) {
        static_cast<void>(matched_readers_.emplace(ep, rp));
    } else {
        DSF_LOG_DEBUG(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                               << get_entity_id() << ",AddMatchedReader ep:" << ep
                                               << ",already in matched_readers_");
        // return false;
    }
    return true;
}
// same as shmreader
bool ShmWriter::RemoveMatchedReader(const EndpointProxy& ep) {
    std::lock_guard<std::mutex> lock(mtx_matched_reader_gs_);
    auto eraseIt = matched_readers_.find(ep);
    if (eraseIt == matched_readers_.end()) {
        DSF_LOG_WARNING(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                 << get_entity_id() << ",RemoveMatchedReader ep:" << ep
                                                 << " not in matched_readers_");
        return false;
    } else {
        static_cast<void>(matched_readers_.erase(eraseIt));
    }
    return true;
}

bool ShmWriter::Send(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, const std::vector<GUID_s>& guids) {
    std::lock_guard<std::mutex> lock(mtx_matched_reader_gs_);
    int ret = std::numeric_limits<int>::min();
    // 匹配成功的是否发送成功
    bool success = true;
    auto it = matched_readers_.begin();
    while (it != matched_readers_.end()) {
        if (it->second.IsPeerMatched() && reader_not_filter_out(guids, it->second.get_guid())) {
            ret = std::numeric_limits<int>::min();
            const std::shared_ptr<mbuf::ShareBufferOwner>& shareBufferOwner =
                std::dynamic_pointer_cast<mbuf::ShareBufferOwner>(mbuf);
            auto desc = shareBufferOwner->BuildDescFor(it->second.get_user_id());

            bool userIdInValid = pool_owner_manager_->IsMultiReader() ? desc.GetUserID().IsMultiUserInvalid()
                                                                      : desc.GetUserID().IsUserInvalid();
            DSF_LOG_DEBUG(ShmWriter, "multi reader entity id:" << it->second.get_entity_id()
                                                               << ",user id:" << it->second.get_user_id());
            if (userIdInValid) {
                DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_USERID_INVALID,
                              "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                            << ",queue id:" << it->second.get_queue_id()
                                            << ",queue name:" << it->second.get_queue_name()
                                            << ",BuildDescFor Input parameters user id:" << it->second.get_user_id()
                                            << ",return desc user id:" << desc.GetUserID() << " is invalid!!!");
                // 一旦失败不能通过队列发送，记录失败
                success = false;
                ++it;
                continue;
            }
            auto now = std::chrono::high_resolution_clock::now();
            mbuf::BufferDescriptor msg {
                desc.GetPoolID(),
                desc.GetUserID(),
                desc.GetNodeOffset(),
                desc.GetRecycleNums(),
                static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()),
                0,
                0U,
                static_cast<uint64_t>(w_pid_),
                entity_id_,
                participant_id_,
                it->second.get_entity_id(),
                it->second.get_sequence_number().high,
                it->second.get_sequence_number().low};
            ret = SendRetry(is_retry, it, msg)();
            HandleSendRet(ret, it, shareBufferOwner);
            if (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
                it->second.add_sequence_number();
                // 队列中增加每个匹配的writer的pid tid entityID
                auto mq_state = it->second.get_msg_queue()->get_segment()->GetSegmentInfo()->p_state;
                if (mq_state->write_info[it->second.get_index()].send_count.fetch_add(1) == 0) {
                    mq_state->write_info[it->second.get_index()].write_pid = w_pid_;
                    mq_state->write_info[it->second.get_index()].write_tid = syscall(SYS_gettid);
                    mq_state->write_info[it->second.get_index()].entity_id = entity_id_;
                }
                DSF_LOG_DEBUG(
                    ShmWriter,
                    "write pid: " << mq_state->write_info[it->second.get_index()].write_pid
                                  << ",tid: " << mq_state->write_info[it->second.get_index()].write_tid
                                  << ",entity id: " << entity_id_ << ",write_index: " << it->second.get_index()
                                  << ",send_count:" << mq_state->write_info[it->second.get_index()].send_count);
            }
            // 已经匹配是否发送成功的判断：所有匹配的发送成功才成功，只要有一个匹配的发送失败则失败
            success = (success && (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS));
        } else {
            DSF_LOG_DEBUG(
                ShmWriter,
                "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                              << ",queue id:" << (it->second.get_msg_queue() ? it->second.get_queue_id() : 0)
                              << ",queue name:" << (it->second.get_msg_queue() ? it->second.get_queue_name() : "")
                              << ", not send reader guids:" << it->second.get_guid()
                              << ",matched:" << it->second.IsPeerMatched()
                              << ",filter out:" << reader_not_filter_out(guids, it->second.get_guid()));
        }
        ++it;
    }
    // 如果所有都未匹配，则ret==std::numeric_limits<int>::min() 返回失败
    bool all_success = success && (ret != std::numeric_limits<int>::min());
#ifdef DSFDDS_STATISTICS
    if (enable_statistics) {
        writer_statistics(all_success);
    }
#endif

#ifdef EDDS_METRICS
    if (mbuf != nullptr) {
        uint32_t send_data_size = mbuf->size();
        CountWriterMetrics(all_success, send_data_size);
    }
#endif

    if (all_success) {
        ++sequence_number_;
    }
    return all_success;
}

bool ShmWriter::SendToAllMatchedNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, uint32_t length,
                                    MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                                    const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                                    std::function<bool(const ReaderProxy&)> filter) {

    bool all_success = true;
    bool any_matched = false;
    auto it = matched_readers_.begin();
    while (it != matched_readers_.end()) {
        if (it->second.IsPeerMatched()) {
            any_matched = true;
            if (vbs_reader_not_filter_out(guids, it->second.get_vbs_guid())) {
                // 匹配成功的是否发送成功
                if (!filter || filter(it->second)) {
                    bool success = SendToSingleNTS(mbuf, it, is_retry, length, auto_shm_bigbuffer, timestamp);
                    all_success = all_success && success;
                }
            }
        } else {
            DSF_LOG_INFO(
                ShmWriter,
                "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                              << ",queue id:" << (it->second.get_msg_queue() ? it->second.get_queue_id() : 0)
                              << ",queue name:" << (it->second.get_msg_queue() ? it->second.get_queue_name() : "")
                              << ", not send reader guids:" << it->second.get_guid()
                              << ",matched:" << it->second.IsPeerMatched());
        }
        ++it;
    }
    // 如果所有都未匹配，则ret==std::numeric_limits<int>::min() 返回失败
    all_success = all_success && any_matched;
#ifdef DSFDDS_STATISTICS
    if (enable_statistics) {
        writer_statistics(all_success);
    }
#endif

#ifdef EDDS_METRICS
    if (mbuf != nullptr) {
        uint32_t send_data_size = mbuf->size();
        CountWriterMetrics(all_success, send_data_size);
    }
#endif

    return all_success;
}

bool ShmWriter::SendMetaToAllMatchedNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, uint32_t serLength,
                                        MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                                        const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                                        std::function<bool(const ReaderProxy&)> filter) {

  bool all_success = true;
  bool any_matched = false;
  auto it = matched_readers_.begin();
  while (it != matched_readers_.end()) {
    if (it->second.IsPeerMatched()) {
      any_matched = true;
      if (vbs_reader_not_filter_out(guids, it->second.get_vbs_guid())) {
        // 匹配成功的是否发送成功
        if (!filter || filter(it->second)) {
          bool success = SendMetaToSingleNTS(mbuf, it, is_retry, serLength, auto_shm_bigbuffer, timestamp);
          all_success = all_success && success;
        }
      }
    } else {
      DSF_LOG_DEBUG(ShmWriter,
                    "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                  << ",queue id:" << (it->second.get_msg_queue() ? it->second.get_queue_id() : 0)
                                  << ",queue name:" << (it->second.get_msg_queue() ? it->second.get_queue_name() : "")
                                  << ", not send reader guids:" << it->second.get_guid()
                                  << ",matched:" << it->second.IsPeerMatched());
    }
    ++it;
  }
  // 如果所有都未匹配，则ret==std::numeric_limits<int>::min() 返回失败
  all_success = all_success && any_matched;
  
#ifdef DSFDDS_STATISTICS
  if (enable_statistics) {
    writer_statistics(all_success);
  }
#endif

#ifdef EDDS_METRICS
  if (mbuf != nullptr) {
    uint32_t send_data_size = mbuf->size();
    CountWriterMetrics(all_success, send_data_size);
  }
#endif

  return all_success;
}

bool ShmWriter::SendMetaToSingleNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, const MatchedReaderIterator& it,
                                    bool is_retry, uint32_t serLength, MultiModeChannel auto_shm_bigbuffer,
                                    uint64_t timestamp) {
  std::shared_ptr<mbuf::ShareBufferOwner> shareBufferOwner;
  ReturnCode_t processRet = ReturnCode_t::RETCODE_MEMBUF_OK;
  if ((auto_shm_bigbuffer & MultiModeChannel::SHM) != MultiModeChannel::NONE)  // auto链路不用处理Meta状态
  {
    processRet = pool_owner_manager_->ProcessStateInMeta(mbuf, it->second.get_user_id(), mbuf::Ope_On_Pending,
                                                         auto_shm_bigbuffer);
  }
  int ret = std::numeric_limits<int>::min();
  if (processRet != ReturnCode_t::RETCODE_MEMBUF_OK) {
      DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_PENDING_FAILED,
                    "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                  << ",queue id:" << it->second.get_queue_id()
                                  << ",queue name:" << it->second.get_queue_name()
                                  << ",process buffer state failed, user id:" << it->second.get_user_id()
                                  << ",state:Ope_On_Pending,ret code:" << processRet());
      ret = ReturnCode_t::RETCODE_SHMMQ_INVALID_MBUF;
  } else {
    const std::shared_ptr<mbuf::ShareBufferOwner>& shareBufferOwner =
        std::dynamic_pointer_cast<mbuf::ShareBufferOwner>(mbuf);
    auto desc = shareBufferOwner->BuildDescFor(it->second.get_user_id());
    auto user_id = desc.GetUserID();
    bool id_invalid = pool_owner_manager_->IsMultiReader() ? user_id.IsMultiUserInvalid() : user_id.IsUserInvalid();
    if (id_invalid) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_USERID_INVALID,
                      "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                    << ",queue id:" << it->second.get_queue_id()
                                    << ",queue name:" << it->second.get_queue_name()
                                    << ",BuildDescFor Input parameters user id:" << it->second.get_user_id()
                                    << ",return desc user id:" << desc.GetUserID() << " is invalid!!!");
        return false;
    }
    mbuf::BufferDescriptor msg {desc.GetPoolID(),
                                desc.GetUserID(),
                                desc.GetNodeOffset(),
                                desc.GetRecycleNums(),
                                timestamp,
                                0,
                                0U,
                                static_cast<uint64_t>(w_pid_),
                                entity_id_,
                                participant_id_,
                                it->second.get_entity_id(),
                                it->second.get_sequence_number().high,
                                it->second.get_sequence_number().low,
                                serLength,
                                auto_shm_bigbuffer};
    ret = SendRetry(is_retry, it, msg)();
  }
  HandleSendRet(ret, it, shareBufferOwner);
  if (ret != ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
    DSF_LOG_DEBUG(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                           << ",queue id:" << it->second.get_queue_id()
                                           << ",queue name:" << it->second.get_queue_name()
                                           << ",send to reader guid: " << it->second.get_guid() << ",return: " << ret);
    if (!pool_owner_manager_->ProcessStateInMeta(mbuf, it->second.get_user_id(), mbuf::Ope_On_Free,
                                                 auto_shm_bigbuffer)) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_FREE_FAILED,
                      "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                    << ",queue id:" << it->second.get_queue_id()
                                    << ",queue name:" << it->second.get_queue_name()
                                    << ",process buffer count failed after send, state:Ope_On_Free");
    }
  }
  if (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
    it->second.add_sequence_number();
    auto mq_state = it->second.get_msg_queue()->get_segment()->GetSegmentInfo()->p_state;
    if (mq_state->write_info[it->second.get_index()].send_count.fetch_add(1) == 0) {
      mq_state->write_info[it->second.get_index()].write_pid = w_pid_;
      mq_state->write_info[it->second.get_index()].write_tid = syscall(SYS_gettid);
      mq_state->write_info[it->second.get_index()].entity_id = entity_id_;
    }
    DSF_LOG_DEBUG(ShmWriter,
                  "write pid: " << mq_state->write_info[it->second.get_index()].write_pid
                                << ",tid: " << mq_state->write_info[it->second.get_index()].write_tid
                                << ",entity id: " << entity_id_ << ",write_index: " << it->second.get_index()
                                << ",send_count:" << mq_state->write_info[it->second.get_index()].send_count);
    return true;
  }
  return false;
}

bool ShmWriter::SendToSingleNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, const MatchedReaderIterator& it,
                                bool is_retry, uint32_t length, MultiModeChannel auto_shm_bigbuffer,
                                uint64_t timestamp) {
    const std::shared_ptr<mbuf::ShareBufferOwner>& shareBufferOwner =
        std::dynamic_pointer_cast<mbuf::ShareBufferOwner>(mbuf);
    auto desc = shareBufferOwner->BuildDescFor(it->second.get_user_id());
    auto user_id = desc.GetUserID();
    bool id_invalid = pool_owner_manager_->IsMultiReader() ? user_id.IsMultiUserInvalid() : user_id.IsUserInvalid();
    if (id_invalid) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_USERID_INVALID,
                      "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                    << ",queue id:" << it->second.get_queue_id()
                                    << ",queue name:" << it->second.get_queue_name()
                                    << ",BuildDescFor Input parameters user id:" << it->second.get_user_id()
                                    << ",return desc user id:" << desc.GetUserID() << " is invalid!!!");
        return false;
    }
    int ret = std::numeric_limits<int>::min();
    mbuf::BufferDescriptor msg {desc.GetPoolID(),
                                desc.GetUserID(),
                                desc.GetNodeOffset(),
                                desc.GetRecycleNums(),
                                timestamp,
                                0,
                                0U,
                                static_cast<uint64_t>(w_pid_),
                                entity_id_,
                                participant_id_,
                                it->second.get_entity_id(),
                                it->second.get_sequence_number().high,
                                it->second.get_sequence_number().low,
                                length,
                                auto_shm_bigbuffer};
    ret = SendRetry(is_retry, it, msg)();
    HandleSendRet(ret, it, shareBufferOwner);
    if (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
        it->second.add_sequence_number();
        auto mq_state = it->second.get_msg_queue()->get_segment()->GetSegmentInfo()->p_state;
        if (mq_state->write_info[it->second.get_index()].send_count.fetch_add(1) == 0) {
            mq_state->write_info[it->second.get_index()].write_pid = w_pid_;
            mq_state->write_info[it->second.get_index()].write_tid = syscall(SYS_gettid);
            mq_state->write_info[it->second.get_index()].entity_id = entity_id_;
        }
        DSF_LOG_DEBUG(ShmWriter,
                      "write pid: " << mq_state->write_info[it->second.get_index()].write_pid
                                    << ",tid: " << mq_state->write_info[it->second.get_index()].write_tid
                                    << ",entity id: " << entity_id_ << ",write_index: " << it->second.get_index()
                                    << ",send_count:" << mq_state->write_info[it->second.get_index()].send_count);
        return true;
    } else {
        DSF_LOG_INFO(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                              << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                              << ",queue name:" << it->second.get_queue_name()
                                              << ",Enqueue desc:" << msg << " failed,ret:" << ret);
    }
    return false;
}

bool ShmWriter::SendMeta(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, const std::vector<GUID_s>& guids) {
    std::lock_guard<std::mutex> lock(mtx_matched_reader_gs_);
    int ret = std::numeric_limits<int>::min();
    // 匹配成功的是否发送成功
    bool success = true;
    auto it = matched_readers_.begin();
    while (it != matched_readers_.end()) {
        if (it->second.IsPeerMatched() && reader_not_filter_out(guids, it->second.get_guid())) {
            ret = std::numeric_limits<int>::min();
            std::shared_ptr<mbuf::ShareBufferOwner> shareBufferOwner;
            auto processRet =
                pool_owner_manager_->ProcessStateInMeta(mbuf, it->second.get_user_id(), mbuf::Ope_On_Pending);
            if (processRet != ReturnCode_t::RETCODE_MEMBUF_OK) {
                DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_PENDING_FAILED,
                              "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                            << ",queue id:" << it->second.get_queue_id()
                                            << ",queue name:" << it->second.get_queue_name()
                                            << ",process buffer state failed, user id:" << it->second.get_user_id()
                                            << ",state:Ope_On_Pending,ret code:" << processRet());
                ret = ReturnCode_t::RETCODE_SHMMQ_INVALID_MBUF;
            } else {
                shareBufferOwner = std::dynamic_pointer_cast<mbuf::ShareBufferOwner>(mbuf);
                auto desc = shareBufferOwner->BuildDescFor(it->second.get_user_id());
                auto now = std::chrono::high_resolution_clock::now();
                mbuf::BufferDescriptor buf_desc {
                    desc.GetPoolID(),
                    desc.GetUserID(),
                    desc.GetNodeOffset(),
                    desc.GetRecycleNums(),
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()),
                    0,
                    0U,
                    static_cast<uint64_t>(w_pid_),
                    entity_id_,
                    participant_id_,
                    it->second.get_entity_id(),
                    it->second.get_sequence_number().high,
                    it->second.get_sequence_number().low};
                ret = SendRetry(is_retry, it, buf_desc)();
            }
            HandleSendRet(ret, it, shareBufferOwner);
            if (ret != ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
                DSF_LOG_DEBUG(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                       << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                                       << ",queue name:" << it->second.get_queue_name()
                                                       << ",send to reader guid: " << it->second.get_guid()
                                                       << ",return: " << ret);
                if (!pool_owner_manager_->ProcessStateInMeta(mbuf, it->second.get_user_id(), mbuf::Ope_On_Free)) {
                    DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_SUBBUF_ON_FREE_FAILED,
                                  "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                                << ",queue name:" << it->second.get_queue_name()
                                                << ",process buffer count failed after send, state:Ope_On_Free");
                }
            }
            if (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
                it->second.add_sequence_number();
                auto mq_state = it->second.get_msg_queue()->get_segment()->GetSegmentInfo()->p_state;
                if (mq_state->write_info[it->second.get_index()].send_count.fetch_add(1) == 0) {
                    mq_state->write_info[it->second.get_index()].write_pid = w_pid_;
                    mq_state->write_info[it->second.get_index()].write_tid = syscall(SYS_gettid);
                    mq_state->write_info[it->second.get_index()].entity_id = entity_id_;
                }
                DSF_LOG_DEBUG(
                    ShmWriter,
                    "write pid: " << mq_state->write_info[it->second.get_index()].write_pid
                                  << ",tid: " << mq_state->write_info[it->second.get_index()].write_tid
                                  << ",entity id: " << entity_id_ << ",write_index: " << it->second.get_index()
                                  << ",send_count:" << mq_state->write_info[it->second.get_index()].send_count);
            }
            // 已经匹配是否发送成功的判断：所有匹配的发送成功才成功，只要有一个匹配的发送失败则失败
            success = (success && (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS));
        } else {
            DSF_LOG_DEBUG(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                   << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                                   << ",queue name:" << it->second.get_queue_name()
                                                   << ", not send reader guids:" << it->second.get_guid()
                                                   << ",matched:" << it->second.IsPeerMatched() << ",filter out:"
                                                   << reader_not_filter_out(guids, it->second.get_guid()));
        }
        ++it;
    }
    // 如果所有都未匹配，则ret==std::numeric_limits<int>::min() 返回失败
    bool all_success = success && (ret != std::numeric_limits<int>::min());
#ifdef DSFDDS_STATISTICS
    if (enable_statistics) {
        writer_statistics(all_success);
    }
#endif

    if (all_success) {
        ++sequence_number_;
    }
    return all_success;
}

void ShmWriter::HandleSendRet(ReturnCode_t ret, const MatchedReaderIterator& it,
                              const std::shared_ptr<mbuf::ShareBufferOwner>& shareBufferOwner) {
    if (ret != ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
        if (shareBufferOwner.get()) {
            // 发送失败回退mbuf user 状态从pending 到 free
            shareBufferOwner->ResetToFree(it->second.get_user_id());
        }
        if (it->second.get_queue_mode() == RELIABLE_RELIABILITY_QOS) {
            DSF_LOG_WARNING(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                     << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                                     << ",queue name:" << it->second.get_queue_name()
                                                     << ",pid:" << getpid() << ",tid:" << syscall(SYS_gettid) << ",ts:"
                                                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                                                            std::chrono::system_clock::now().time_since_epoch())
                                                            .count()
                                                     << " send fail!!!, ret:" << ret());
        }
        if ((ret == ReturnCode_t::RETCODE_SHMMQ_QUEUE_FULL) || (ret == ReturnCode_t::RETCODE_SHMMQ_TIME_OUT)) {
            it->second.set_last_send_failed(true);
        } else {
            DSF_LOG_ERROR(DSF_MEMBUF, ret(),
                          "writer ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                        << ",queue id:" << it->second.get_queue_id()
                                        << ",queue name:" << it->second.get_queue_name() << ",send fail,can not retry"
                                        << ",endpoint proxy:" << it->first);
            it->second.set_last_send_failed(false);
        }
    } else {
        it->second.set_last_send_failed(false);
    }
}

ReturnCode_t ShmWriter::SendRetry(bool is_retry, const MatchedReaderIterator& it,
                                  const mbuf::BufferDescriptor& buf_desc) const {
    ReturnCode_t ret;  // is_retry说明在重试
    if (is_retry) {
        // 只重试上次发送失败的reader
        if (it->second.is_last_send_failed()) {
            ret = it->second.get_msg_queue()->Enqueue(buf_desc);
        } else {
            // 上次没有失败的reader 默认返回成功
            ret = ReturnCode_t::RETCODE_SHMMQ_SUCCESS;
        }
    } else {
        // 非重试,匹配reader正常发送
        ret = it->second.get_msg_queue()->Enqueue(buf_desc);
    }
    if (ret == ReturnCode_t::RETCODE_SHMMQ_SUCCESS) {
        DSF_LOG_DEBUG(ShmWriter, "writer ids:" << get_domain_id() << "," << get_participant_id() << ","
                                               << get_entity_id() << ",queue id:" << it->second.get_queue_id()
                                               << ",queue name:" << it->second.get_queue_name() << ",pid:" << getpid()
                                               << ",tid:" << syscall(SYS_gettid) << ",ts:"
                                               << std::chrono::duration_cast<std::chrono::milliseconds>(
                                                      std::chrono::system_clock::now().time_since_epoch())
                                                      .count()
                                               << ",Enqueue desc:" << buf_desc << " success");
    }
    return ret;
}

bool ShmWriter::IsAllUnmatched() const {
    for (const auto& it : matched_readers_) {
        if (!it.second.IsUnmatched()) {
            return false;
        }
    }
    return true;
}

ShmWriter::~ShmWriter() {
    DSF_LOG_DEBUG(ShmWriter, "dtor, this: " << this << ",writer ids:" << get_domain_id() << "," << get_participant_id()
                                            << "," << get_entity_id());
    {
        std::lock_guard<std::mutex> lock(mtx_matched_reader_gs_);
        matched_readers_.clear();
    }
}

void ShmWriter::add_endpoint_proxy(const EndpointProxy& endpoint_proxy) {
    local_endpoint_proxies_.push_back(endpoint_proxy);
}

std::string ShmWriter::PrintLocalEndpointProxies() const {
    std::stringstream proxies;
    for (const auto& it : local_endpoint_proxies_) {
        proxies << it << ", ";
    }
    return proxies.str();
}

bool ShmWriter::HasNewOnlineReader() const {
    for (const auto& it : matched_readers_) {
        if (it.second.is_new_online()) {
            return true;
        }
    }
    return false;
}

void ShmWriter::MarkAllReaderOld() {
    for (auto& it : matched_readers_) {
        it.second.set_new_online(false);
    }
}

#ifdef EDDS_METRICS
void ShmWriter::CountWriterMetrics(bool writer_success, uint32_t data_size) {
    if (writer_success) {
        // update state
        ++metric_send_count_;
        metric_data_size_sum += data_size;
        metric_send_count_diff_ = metric_send_count_ - last_metric_send_count_;
        if (metric_send_count_diff_ >= WRITER_METRICS_COUNT_THRESHOLD) {
            // Writer throughput
            GUID_s guid_w = get_guid();
            vbsutil::xmlparser::GUID_t* w_guid = reinterpret_cast<vbsutil::xmlparser::GUID_t*>(&guid_w);
            ReliabilityKind_t kind = (get_reliability_kind() == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS)
                                         ? ReliabilityKind_t::BEST_EFFORT
                                         : ReliabilityKind_t::RELIABLE;
            vbsutil::statistics::StatisticsWriter::on_send_throughput(kind, *w_guid, metric_data_size_sum);
            last_metric_send_count_ = metric_send_count_;
            metric_data_size_sum = 0U;
        }
    }
}
#endif
const std::string& ShmWriter::getTopicName() const {
    return topic_name_;
}

#ifdef DSFDDS_STATISTICS
void ShmWriter::writer_statistics(bool writer_success) {
    if (writer_success) {
        // Default sample frequency is 10
        int32_t sample_freq = 10;
        const char* env = std::getenv("DSFDDS_STATISTICS_SAMPLE_FREQ");
        if (nullptr != env) {
            int32_t freq = std::stoi(env);
            if (freq >= 1) {
                sample_freq = freq;
            }
        }

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds diff;
        if (send_count_ == 0U) {
            prev_time_ = now;
        } else {
            diff = std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time_);
        }
        prev_time_ = now;

        ++send_count_;

        if ((send_count_ % static_cast<uint32_t>(sample_freq)) == 0U) {
            // Writer send count
            auto type = statistics::SEND;
            statistics::DsfEntityData entity_data(type, get_guid(), send_count_);
            on_entity_data_(entity_data);
            if (send_count_ > 1) {
                // Writer throughput
                float throughput = (static_cast<float>(sizeof(mbuf::BufferDescriptor))) / (diff.count());
                auto type = statistics::THROUGHPUT;
                statistics::DsfEntityData throughput_data(type, get_guid(), throughput);
                on_entity_data_(throughput_data);
            }
        }
    }
}
#endif

inline bool ShmWriter::reader_not_filter_out(const std::vector<GUID_s>& guids, const GUID_s& reader_guid) {
    return std::find(guids.begin(), guids.end(), reader_guid) == guids.end();
}

inline bool ShmWriter::vbs_reader_not_filter_out(const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                                                 const vbsutil::xmlparser::GUID_t& reader_guid) {
    DSF_LOG_DEBUG(ShmWriter, "filtered_out_readers size: " << guids.size() << ",reader_guid:" << reader_guid);
    for (size_t i = 0; i < guids.size(); i++) {
        DSF_LOG_DEBUG(ShmWriter, "filtered_out_readers i: " << i << ", guid: " << guids[i]);
    }
    return std::find(guids.begin(), guids.end(), reader_guid) == guids.end();
}

std::shared_ptr<mbuf::BufferPoolOwnerManager>& ShmWriter::GetPool() {
    return pool_owner_manager_;
}

}  // namespace dsfdds
}  // namespace li
