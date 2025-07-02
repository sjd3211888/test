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

#include "dsfdds/publisher/DSFWriterImpl.h"
#include "dsfdds/transport/shm_creator.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "xmlparser/xmlutils/DSFLocator.hpp"
#include "DsfLog.hpp"
#include "history/WriterListener.h"
#include "common/SharingPayloadPool.hpp"

namespace li {
namespace dsfdds {
namespace {
constexpr uint32_t HeadCount = 3U;
}

DSFWriterImpl::DSFWriterImpl(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                             std::string& topic_type, std::shared_ptr<vbs::common::IPayloadPool>& payload_pool,
                             const DataWriterQos& qos, vbs::common::WriterHistory* history,
                             vbsutil::xmlparser::Locator_t* local_loc, vbs::common::WriterListener* listener)
    : is_shm_only_(false),
      topicName(topic_name),
      topicType(topic_type),
      payload_pool_(payload_pool),
      dsfWriterQos(qos),
      listener(listener),
      history_(history),
      localLocatorPtr(local_loc),
      wGuid(guid),
      domainId(domainID),
      crash_listener_() {
    if (payload_pool->get_payload_type() == PAYLOAD_TYPE_DSF) {
        is_shm_only_ = true;
    }
}

ReturnCode_t DSFWriterImpl::enable() {
    if (is_shm_only_) {
        shmWriterPtr = new ShmWriter(domainId, topicName, topicType, wGuid, false, dsfWriterQos.reliability().kind);
        std::shared_ptr<SharingPayloadPool> sharing_pool = std::dynamic_pointer_cast<SharingPayloadPool>(payload_pool_);
        if (!sharing_pool) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "Problem casting payloadpool ids: " << get_domain_id() << "," << wGuid);
            return ReturnCode_t::RETCODE_PUBSUB_ERROR;
        }
        shmWriterPtr->pool_owner_manager_ = sharing_pool->get_pool_owner();

    } else {
        ShmWriter* writer = ShmCreator::CreateShmWriter(domainId, wGuid, topicName, topicType, dsfWriterQos, false);
        if (writer == nullptr) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "Problem creating associated ShmWriter ids:" << get_domain_id() << "," << wGuid);
            return ReturnCode_t::RETCODE_PUBSUB_ERROR;
        }
        shmWriterPtr = writer;
    }
    using namespace vbsutil::xmlparser;
    auto pool_id = shmWriterPtr->pool_owner_manager_->GetID();
    *localLocatorPtr = DSFLocator::create_locator(pool_id, 0U);
    DSF_LOG_INFO(DSFWriterImpl,
                 "writer guid: " << wGuid << " writer topicName: " << topicName << " is_shm_only_: " << is_shm_only_);
    // crash call back
    auto crash_call_back = [this](const EndpointProxy& ep) {
        DSF_LOG_DEBUG(DSFReaderImpl, "Triger crash_call_back, the guid of remote endpoint is: " << ep.get_local_guid());
        CrashCallBack(ep);
    };
    shmWriterPtr->set_on_endpoint_crash(std::move(crash_call_back));
    // register local info
    static_cast<void>(EdpRegisterInfo());
    crash_listener_ = ShmCreator::CreateCrashListener(VbsDiscoveryEndpoint::GetInstance());
    return ReturnCode_t::RETCODE_PUBSUB_OK;
}

void DSFWriterImpl::CrashCallBack(const EndpointProxy& ep) {
    listener->on_endpoint_crash(ep.get_local_guid());
}

void DSFWriterImpl::disable() {}

DSFWriterImpl::~DSFWriterImpl() {
    DSF_LOG_DEBUG(DSFWriterImpl, "dtor, this: " << this << ", guid: " << guid());
    DSFWriterImpl::disable();

    if (shmWriterPtr != nullptr) {
        static_cast<void>(ShmCreator::RemoveShmWriter(VbsDiscoveryEndpoint::GetInstance(), shmWriterPtr, dsfWriterQos));
        shmWriterPtr = nullptr;
        if (is_shm_only_) {
            std::shared_ptr<SharingPayloadPool> sharing_pool =
                std::dynamic_pointer_cast<SharingPayloadPool>(payload_pool_);
            if (!sharing_pool) {
                DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                              "Problem casting payloadpool ids: " << get_domain_id() << "," << wGuid);
            } else {
                sharing_pool->reset_pool_owner();
            }
        }
    }
}
// dts dds implement
std::shared_ptr<mbuf::Buffer> DSFWriterImpl::loan_sample(uint32_t size) {
    // Writer should be enabled
    if (nullptr == shmWriterPtr) {
        DSF_LOG_WARNING(DSFWriterImpl,
                        "writer ids:" << get_domain_id() << "," << wGuid << ",ShmWriter is nullptr,not enable");
        return nullptr;
    }

    auto& buf_pool = shmWriterPtr->pool_owner_manager_;
    if (buf_pool == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_NOT_ENABLED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ",The pool_owner is nullptr");
        return nullptr;
    }

    int64_t poolId = -1;
    auto mbuf = buf_pool->AllocBuffer(size, poolId);
    if (poolId > 0) {
        // place old pool id into free pool list
        CrashListener::GetInstance(VbsDiscoveryEndpoint::GetInstance())
            ->poolCrashStrategy.UpdateWriterRecycleMbufPools(poolId);
    }
    return mbuf;
}

bool DSFWriterImpl::unsent_change_added_to_history(
    vbs::common::CacheChange_t* change,
    const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) {
    static_cast<void>(max_blocking_time);
    if (shmWriterPtr == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_NOT_ENABLED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", ShmWriter is nullptr, not enable");
        return false;
    }

  uint32_t serDataRealLength = change->serializedPayload.length;
  uint32_t nonSerDataRealLength = 0U;
  bool is_meta_buf = false;
  if ((change->nonSerializedPayload.mbuf != nullptr) || (change->nonSerializedPayload.data != nullptr)) {
      is_meta_buf = true;
      nonSerDataRealLength = change->nonSerializedPayload.length;
  }
  if ((change->nonSerializedPayload.mbuf != nullptr) && (change->nonSerializedPayload.data == nullptr)) {
      DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                    "writer ids:" << get_domain_id() << "," << wGuid
                                  << ", illegal state: nonSerializedPayload.mbuf must be null when data "
                                     "is null. current state: mbuf=non-null, data=null");
      return false;
  }
  // 1. shm 链路
  if (is_shm_only_) {
    // 1.1 big_buffer 链路
    MultiModeChannel auto_shm_bigbuffer = MultiModeChannel::SHM;
    if (is_meta_buf) {
        auto_shm_bigbuffer = auto_shm_bigbuffer | MultiModeChannel::BIGBUFFER;
        return write_shm_meta_buffer(serDataRealLength, nonSerDataRealLength, auto_shm_bigbuffer, change);
    } else {  // 1.2 普通 链路
      mbuf::BufferPtr mbuf {};
      mbuf = change->serializedPayload.mbuf;
      if (mbuf == nullptr) {
          DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CHANGE_MBUF_IS_EMPTY,
                        "writer ids:" << get_domain_id() << "," << wGuid << ", shm_only, change mbuf is empty");
          return false;
      }
      uint64_t timestamp = static_cast<uint64_t>(change->sourceTimestamp.to_ns());
      return write(mbuf, serDataRealLength, auto_shm_bigbuffer, timestamp, change->filtered_out_readers,
                   change->sequenceNumber);
    }
  } else {  // 2 auto 链路
      MultiModeChannel auto_shm_bigbuffer = MultiModeChannel::AUTO;
      if (is_meta_buf) {  // 2.1 big_buffer 链路
          auto_shm_bigbuffer = auto_shm_bigbuffer | MultiModeChannel::BIGBUFFER;
          return write_auto_meta_buffer(serDataRealLength, nonSerDataRealLength, auto_shm_bigbuffer, change);
      } else {  // 2.2 普通 链路
          mbuf::BufferPtr mbuf {};
          mbuf = loan_sample(serDataRealLength);
          if (mbuf == nullptr) {
              DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_LOAN_SAMPLE_FAILED,
                            "writer ids:" << get_domain_id() << "," << wGuid << ", alloc buffer failed");
              return false;
          }
          if (change->serializedPayload.origin_data) {
              memcpy(mbuf->data(), change->serializedPayload.data,
                     change->serializedPayload.representation_header_size);
              memcpy(static_cast<unsigned char*>(mbuf->data()) + change->serializedPayload.representation_header_size,
                     change->serializedPayload.origin_data,
                     serDataRealLength - change->serializedPayload.representation_header_size);
          } else {
              memcpy(mbuf->data(), change->serializedPayload.data, serDataRealLength);
          }
          // auto 链路将 mbuf 挂到 CacheChange_t 上
          change->serializedPayload.mbuf = mbuf;

          uint64_t timestamp = static_cast<uint64_t>(change->sourceTimestamp.to_ns());
          return write(mbuf, serDataRealLength, auto_shm_bigbuffer, timestamp, change->filtered_out_readers,
                       change->sequenceNumber);
      }
  }
}

vbsutil::xmlparser::GUID_t DSFWriterImpl::guid() const {
    return wGuid;
}

bool DSFWriterImpl::EdpRegisterInfo() {
    return (VbsDiscoveryEndpoint::GetInstance()->RegisterLocalWriterInfo(shmWriterPtr, dsfWriterQos));
}

/**
 * Get the domain id of this DSFWriterImpl.
 * @return domain id
 */
uint32_t DSFWriterImpl::get_domain_id() const {
    return domainId;
}

std::vector<mbuf::PoolID> DSFWriterImpl::get_pool_ids() const {
    if (shmWriterPtr == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_NOT_ENABLED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", ShmWriter is nullptr, not enable");
        return {};
    }
    auto pool_manager = shmWriterPtr->GetPool();
    if (!pool_manager) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_NOT_ENABLED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", pool owner manager is nullptr");
        return {};
    }
    return pool_manager->GetPoolIds();
}

bool DSFWriterImpl::is_reissue_required(int lp_durability, int rp_durability) {
    return ((lp_durability >= rp_durability) &&
            (rp_durability >=
             (static_cast<int>(vbsutil::xmlparser::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS))));
}

void DSFWriterImpl::LogSendResult(bool success, const std::string& changeType,
                                  vbs::common::SequenceNumber_t seqNum) const {
    if (success) {
        DSF_LOG_DEBUG(DSFWriterImpl, "writer ids:" << get_domain_id() << "," << wGuid << ", send " << changeType
                                                   << " success"
                                                   << ", change sequenceNumber" << seqNum);
    } else {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_SEND_MBUF_FAILED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", send " << changeType << " failed"
                                    << ", change sequenceNumber" << seqNum);
    }
}

bool DSFWriterImpl::write(const std::shared_ptr<mbuf::Buffer>& mbuf, uint32_t length,
                          MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                          const std::vector<vbsutil::xmlparser::GUID_t>& guids, vbs::common::SequenceNumber_t seqNum) {
    // 发送有如下3个步骤：
    // 1.给旧 reader 发送最新 change
    // 2.给新 reader 发送历史 change
    // 3.如果不需要给新 reader 发送历史 change , 就需要给新 reader 补发一包最新change
    // 1.给旧 reader 发送最新 change
    std::lock_guard<std::mutex> lock(shmWriterPtr->GetMtxMatchedGs());

    bool success = shmWriterPtr->SendToAllMatchedNTS(mbuf, false, length, auto_shm_bigbuffer, timestamp, guids,
                                                     [](const ReaderProxy& rp) { return !rp.is_new_online(); });
    LogSendResult(success, "change", seqNum);

    // 2.给新 reader 发送历史 change
    // 补发前需要 根据qos判断 是否需要补发
    // 若需补发且补发失败 当前版本仅输出错误日志提醒
    if (shmWriterPtr->HasNewOnlineReader()) {
        std::unique_lock<vbs::common::RecursiveTimedMutex> history_lock(history_->getMutex());
        for (auto cit = history_->changesBegin(); cit != history_->changesEnd(); ++cit) {
            vbs::common::CacheChange_t* his_ch = *cit;
            if (!his_ch->serializedPayload.mbuf) {
                DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CHANGE_MBUF_IS_EMPTY,
                              "writer ids:" << get_domain_id() << "," << wGuid
                                            << ", history change mbuf is nullptr, change sequenceNumber"
                                            << his_ch->sequenceNumber);
                continue;
            }
            bool success1 = shmWriterPtr->SendToAllMatchedNTS(
                his_ch->serializedPayload.mbuf, false, his_ch->serializedPayload.length, auto_shm_bigbuffer,
                static_cast<uint64_t>(his_ch->sourceTimestamp.to_ns()), guids, [=](const ReaderProxy& rp) {
                    return (rp.is_new_online() && is_reissue_required(static_cast<int>(dsfWriterQos.durability().kind),
                                                                      static_cast<int>(rp.durabilityKind())));
                });
            LogSendResult(success1, "history change", his_ch->sequenceNumber);
        }
        history_lock.unlock();
        // 3.如果不需要给新 reader 发送历史 change , 就需要给新 reader 补发一包最新change
        //   reader为best_effort 为新上线 qos一定不会符合补发逻辑 需单发1包最新change 否则新上线的reader会丢首包
        //   reader为reliable 为新上线 且qos不符合补发逻辑 需单发1包最新change 否则新上线的reader会丢首包
        bool success2 = shmWriterPtr->SendToAllMatchedNTS(
            mbuf, false, length, auto_shm_bigbuffer, timestamp, guids, [=](const ReaderProxy& rp) {
                return (rp.is_new_online() && !is_reissue_required(static_cast<int>(dsfWriterQos.durability().kind),
                                                                   static_cast<int>(rp.durabilityKind())));
            });
        success = success && success2;

        shmWriterPtr->MarkAllReaderOld();
    }

    if (success) {
        shmWriterPtr->IncrementSequenceNumber();
    }

    return success;
}

bool DSFWriterImpl::write_meta(const std::shared_ptr<mbuf::Buffer>& mbuf, uint32_t serLength,
                               MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                               const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                               vbs::common::SequenceNumber_t seqNum) {
  // 发送有如下3个步骤：
  // 1.给旧 reader 发送最新 change
  // 2.给新 reader 发送历史 change
  // 3.如果不需要给新 reader 发送历史 change , 就需要给新 reader 补发一包最新change
  // 1.给旧 reader 发送最新 change
  std::lock_guard<std::mutex> lock(shmWriterPtr->GetMtxMatchedGs());

  bool success = shmWriterPtr->SendMetaToAllMatchedNTS(mbuf, false, serLength, auto_shm_bigbuffer, timestamp, guids,
                                                       [](const ReaderProxy& rp) { return !rp.is_new_online(); });
  LogSendResult(success, "change", seqNum);

  // 2.给新 reader 发送历史 change
  // 补发前需要 根据qos判断 是否需要补发
  // 若需补发且补发失败 当前版本仅输出错误日志提醒
  if (shmWriterPtr->HasNewOnlineReader()) {
    for (auto cit = history_->changesBegin(); cit != history_->changesEnd(); ++cit) {
      vbs::common::CacheChange_t* his_ch = *cit;
      if (!his_ch->serializedPayload.mbuf) {
          DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CHANGE_MBUF_IS_EMPTY,
                        "writer ids:" << get_domain_id() << "," << wGuid
                                      << ", history change mbuf is nullptr, change sequenceNumber"
                                      << his_ch->sequenceNumber);
          continue;
      }
      bool success1 = shmWriterPtr->SendMetaToAllMatchedNTS(
          his_ch->serializedPayload.mbuf, false, his_ch->serializedPayload.length, auto_shm_bigbuffer,
          static_cast<uint64_t>(his_ch->sourceTimestamp.to_ns()), guids, [=](const ReaderProxy& rp) {
            return (rp.is_new_online() && is_reissue_required(static_cast<int>(dsfWriterQos.durability().kind),
                                                              static_cast<int>(rp.durabilityKind())));
          });
      LogSendResult(success1, "history change", his_ch->sequenceNumber);
    }

    // 3.如果不需要给新 reader 发送历史 change , 就需要给新 reader 补发一包最新change
    //   reader为best_effort 为新上线 qos一定不会符合补发逻辑 需单发1包最新change 否则新上线的reader会丢首包
    //   reader为reliable 为新上线 且qos不符合补发逻辑 需单发1包最新change 否则新上线的reader会丢首包
    bool success2 = shmWriterPtr->SendMetaToAllMatchedNTS(
        mbuf, false, serLength, auto_shm_bigbuffer, timestamp, guids, [=](const ReaderProxy& rp) {
          return (rp.is_new_online() && !is_reissue_required(static_cast<int>(dsfWriterQos.durability().kind),
                                                             static_cast<int>(rp.durabilityKind())));
        });
    success = success && success2;

    shmWriterPtr->MarkAllReaderOld();
  }

  if (success) {
    shmWriterPtr->IncrementSequenceNumber();
  }

  return success;
}

std::shared_ptr<mbuf::Buffer> DSFWriterImpl::prepare_shm_meta_buffer(uint32_t serDataRealLength,
                                                                     uint32_t nonSerDataRealLength,
                                                                     vbs::common::CacheChange_t* change) {
    // 走大buffer head_count*sizeof(uint32_t) + BufferInfo1 + BufferInfo2
    auto meta_buf = loan_sample(HeadCount * sizeof(uint32_t) + sizeof(mbuf::BufferInfo) * (HeadCount - 1));
    if (meta_buf == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_LOAN_SAMPLE_FAILED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", alloc shm_meta_buf failed");
        return nullptr;
    }
    uint32_t* meta_ptr = reinterpret_cast<uint32_t*>(meta_buf->data());
    meta_ptr[0] = HeadCount;
    meta_ptr[1] = serDataRealLength;
    meta_ptr[2] = nonSerDataRealLength;
    auto p_buffer_info =
        static_cast<mbuf::BufferInfo*>((void*)((uint8_t*)meta_buf->data() + HeadCount * sizeof(uint32_t)));

    // 先存储序列化部分
    mbuf::BufferPtr ser_mbuf {};
    ser_mbuf = change->serializedPayload.mbuf;
    if (ser_mbuf == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CHANGE_SER_MBUF_IS_EMPTY,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", shm_only, change ser_mbuf is empty");
        return nullptr;
    }
  auto ser_buffer_info = mbuf::BufferUtil::GetBufferInfoFromBuffer(ser_mbuf);
  *p_buffer_info = ser_buffer_info;
  p_buffer_info++;
  // 后存储未序列化部分
  mbuf::BufferPtr non_ser_mbuf {};
  non_ser_mbuf = change->nonSerializedPayload.mbuf;
  if (non_ser_mbuf == nullptr) {
      DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CHANGE_NON_SER_MBUF_IS_EMPTY,
                    "writer ids:" << get_domain_id() << "," << wGuid << ", shm_only, change non_ser_mbuf is empty");
      return nullptr;
  }
  auto nonser_buffer_info = mbuf::BufferUtil::GetBufferInfoFromBuffer(non_ser_mbuf);
  *p_buffer_info = nonser_buffer_info;

  return meta_buf;
}

std::shared_ptr<mbuf::Buffer> DSFWriterImpl::prepare_auto_meta_buffer(uint32_t serDataRealLength,
                                                                      uint32_t nonSerDataRealLength,
                                                                      vbs::common::CacheChange_t* change) {
    // 走大buffer head_count*sizeof(uint32_t) + serDataRealLength + nonSerDataRealLength
    auto meta_buf = loan_sample(HeadCount * sizeof(uint32_t) + serDataRealLength + nonSerDataRealLength);
    if (meta_buf == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_LOAN_SAMPLE_FAILED,
                      "writer ids:" << get_domain_id() << "," << wGuid << ", alloc auto_meta_buf failed");
        return nullptr;
    }
    uint32_t* meta_ptr = reinterpret_cast<uint32_t*>(meta_buf->data());
    meta_ptr[0] = HeadCount;
    meta_ptr[1] = serDataRealLength;
    meta_ptr[2] = nonSerDataRealLength;
    memcpy((uint8_t*)meta_buf->data() + HeadCount * sizeof(uint32_t), change->serializedPayload.data,
           serDataRealLength);  // 拷贝序列化部分
    memcpy((uint8_t*)meta_buf->data() + HeadCount * sizeof(uint32_t) + serDataRealLength,
           change->nonSerializedPayload.data,
           change->nonSerializedPayload.length);  // 拷贝非序列化部分
    change->serializedPayload.mbuf = meta_buf;
    return meta_buf;
}

bool DSFWriterImpl::write_shm_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                                          MultiModeChannel auto_shm_bigbuffer, vbs::common::CacheChange_t* change) {
    auto meta_buf = prepare_shm_meta_buffer(serDataRealLength, nonSerDataRealLength, change);
    if (meta_buf == nullptr) {
        return false;
    }
    uint64_t timestamp = static_cast<uint64_t>(change->sourceTimestamp.to_ns());
    return write_meta(meta_buf, serDataRealLength, auto_shm_bigbuffer, timestamp, change->filtered_out_readers,
                      change->sequenceNumber);
}

bool DSFWriterImpl::write_auto_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                                           MultiModeChannel auto_shm_bigbuffer, vbs::common::CacheChange_t* change) {
    auto meta_buf = prepare_auto_meta_buffer(serDataRealLength, nonSerDataRealLength, change);
    if (meta_buf == nullptr) {
        return false;
    }
    uint64_t timestamp = static_cast<uint64_t>(change->sourceTimestamp.to_ns());
    return write_meta(meta_buf, serDataRealLength, auto_shm_bigbuffer, timestamp, change->filtered_out_readers,
                      change->sequenceNumber);
}

}  // namespace dsfdds
}  // namespace li
