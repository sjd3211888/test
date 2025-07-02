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

#include "dsfdds/subscriber/DSFReaderImpl.h"
#include <cstdint>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "dsfdds/transport/shm_creator.h"
#include <sys/types.h>
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "xmlparser/xmlutils/DSFLocator.hpp"
#include "DsfLog.hpp"
#include "common/property/PropertyParse.h"
#include "common/all_common.h"
#include "deps/core/status/SampleRejectedStatus.hpp"
#include "common/SharingPayloadPoolFactory.h"
#include "common/types/TypesBase.h"
#include "deps/common/Guid.h"

namespace li {
namespace dsfdds {

DSFReaderImpl::DSFReaderImpl(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                             std::string& topic_type, const DataReaderQos& qos,
                             std::shared_ptr<vbs::common::IChangePool>& changePool,
                             vbs::common::ReaderHistory* historyPtr, vbsutil::xmlparser::Locator_t* local_loc,
                             vbs::common::ReaderListener* listener)
    : dsfListener(this, listener),
      domainId(domainID),
      rGuid(guid),
      topicName(topic_name),
      topicType(topic_type),
      dataReaderQos(qos),
      localLocatorPtr(local_loc),
      changePool(changePool),
      historyPtr(historyPtr) {
    // 创建payloadPool
    {
        payloadPool = SharingPayloadFactory::create_empty(guid);
        if (payloadPool) {
            DSF_LOG_DEBUG(DSFReaderImpl, "payloadPool is not nullptr! guid: " << guid << ", topic name: " << topicName
                                                                              << ", topic type: " << topic_type
                                                                              << ", domainID: " << domainID);
        } else {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "payloadPool is nullptr! guid: " << guid << ", topic name: " << topicName << ", topic type: "
                                                           << topic_type << ", domainID: " << domainID);
        }
    }
}

ReturnCode_t DSFReaderImpl::enable() {
    assert(shmReaderPtr == nullptr);
    if (guid().entityId.to_uint32() == ENTITY_TYPE_UNKNOW) {
        return ReturnCode_t::RETCODE_PUBSUB_GUID_GENERATE_ERR;
    }

    ShmReader* reader = nullptr;

    reader = ShmCreator::CreateShmReader(domainId, rGuid, topicName, topicType, &dsfListener, dataReaderQos, false);
  if (reader == nullptr) {
      DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CREATE_OBJECT_ERR,
                    "Problem creating associated ShmReader. guid:" << rGuid << " topic name " << topicName
                                                                   << " topic type " << topicType);
      return ReturnCode_t::RETCODE_PUBSUB_ERROR;
  }
  shmReaderPtr = reader;
  auto queueId = shmReaderPtr->get_msg_queue()->get_segment()->GetQueueId();
  using namespace vbsutil::xmlparser;
  *localLocatorPtr = DSFLocator::create_locator(0U, queueId);
#ifdef DSFDDS_STATISTICS
    StatisticsDataCallBackRegister();
#endif

#ifdef ENABLE_DES
    QueueWakeUp wake_up = PropertyParse::GetQueueWakeUp(
        PropertyPolicyHelper::find_property(qos_.properties(), QueuePropertyField::WakeUpMode));
    if (wake_up == DTS_WAKE_UP) {
        auto lightTaskFunc = std::make_shared<std::function<void(void)>>(
            [this, readerListener]() { DataReceiveCallBack(readerListener); });
        shmmq::TaskCfg cfg {
            reader_->get_msg_queue()->get_queue_id(),
            PropertyParse::GetResGroupID(
                PropertyPolicyHelper::find_property(qos_.properties(), DesPropertyField::ResGroupID)),
            PropertyParse::GetDeadlineTime(
                PropertyPolicyHelper::find_property(qos_.properties(), DesPropertyField::DeadlineTime)),
            PropertyParse::GetRunTime(
                PropertyPolicyHelper::find_property(qos_.properties(), DesPropertyField::RunTime)),
            PropertyParse::GetSchedulePeriod(
                PropertyPolicyHelper::find_property(qos_.properties(), DesPropertyField::SchedulePeriod)),
        };
        auto ret = reader_->get_msg_queue()->RegisterTask(lightTaskFunc, cfg);
        DSF_LOG_INFO(DSFReaderImpl, "reader ids:" << subscriber_->get_domain_id() << ","
                                                  << subscriber_->get_participant_id() << ","
                                                  << qos_.endpoint().entity_id << ",queue name:" << reader_->queue_name_
                                                  << ",queue id:" << reader_->msg_queue_->get_queue_id()
                                                  << ",wake up mode: des" << ret << " RegisterTask done,ret:" << ret);
        if (!ret) {
            return ReturnCode_t::RETCODE_PUBSUB_ERROR;
        }
    } else if (wake_up == PTHREAD_CONDITION_WAKE_UP) {
#else
    auto call_back_fn = [this](const ChannelResource* cr) {
        DataReceiveCallBack(cr);
    };
    auto channel_resource =
        std::unique_ptr<ChannelResource>(new ChannelResource(call_back_fn, shmReaderPtr->get_queue_name()));
    shmReaderPtr->set_channel_resource(std::move(channel_resource));
    shmReaderPtr->channel_resource_->CallBackFunc(topicName);
#endif

#ifdef ENABLE_DES
    }
#endif

    // crash call back
    auto crash_call_back = [this](const EndpointProxy& ep) {
        CrashCallBack(ep);
    };
    shmReaderPtr->set_on_endpoint_crash(std::move(crash_call_back));

    static_cast<void>(EdpRegisterInfo());
    crash_listener_ = ShmCreator::CreateCrashListener(VbsDiscoveryEndpoint::GetInstance());
    return ReturnCode_t::RETCODE_PUBSUB_OK;
}

void DSFReaderImpl::HandleDataReceive(const ChannelResource* cr) {
  (void)cr;
  // must setting 超时时间 否则在结束程序的时候可能会阻塞在这里
  shmmq::queue_ret_t ret = shmReaderPtr->msg_queue_->WaitMessage(1000U);
  if (shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS == ret) {
    vbs::common::SequenceNumber_t sequence = {};
    vbsutil::xmlparser::GUID_t w_guid = {};
    mbuf::BufferDescriptor descriptor;
    std::shared_ptr<mbuf::Buffer> mbuf = nullptr;
    auto queue_ret = shmReaderPtr->TakeSampleInternal(descriptor, mbuf, [](mbuf::PoolID pool_id) {
      CrashListener::GetInstance(VbsDiscoveryEndpoint::GetInstance())->poolCrashStrategy.UpdateUnmatchedMbufPools(pool_id);
    });
    if ((queue_ret != shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) || !mbuf) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_TAKE_SAMPLE_FAILED,
                      "TakeSampleInternal failed or mbuf queue is nullptr! ret: " << queue_ret());
        return;
    }
    uint64_t timestamp = descriptor.timestamp();
    sequence = vbs::common::SequenceNumber_t(descriptor.sequence_number_high_, descriptor.sequence_number_low_);
    w_guid = vbsutil::xmlparser::GUID_t::guid_create_by_info(rGuid, descriptor.pid_, descriptor.writer_participant_id_,
                                                             descriptor.writer_id_);
    {
      //向change_pool申请一个change_change
      vbs::common::CacheChange_t* change_to_add = nullptr;
      std::unique_lock<vbs::common::RecursiveTimedMutex> history_lock(historyPtr->getMutex());
      {
        if (!changePool->reserve_cache(change_to_add)) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "apply change from change_pool failed! sequence: " << sequence.to64long() << ",w_guid: "
                                                                             << w_guid << ",rGuid: " << rGuid);
            return;
        }
      }
      history_lock.unlock();
      // 判断是否进入大buf处理逻辑
      if ((descriptor.auto_shm_bigbuffer_ & MultiModeChannel::BIGBUFFER) != MultiModeChannel::NONE) {
        // 大buf链路
        parser_meta_buf(descriptor, mbuf, change_to_add, timestamp, sequence);
      } else {
        // 普通链路 直接将mbuf内容及payload_pool赋值到change_to_add
        move_mbuf_into_change(mbuf, change_to_add, descriptor.data_real_length_, timestamp, sequence);
      }
      // 内部调用add_change 上传change到history
      if (!change_received(change_to_add, w_guid)) {
          DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                        "add change to history failed! sequence: " << sequence.to64long() << ",w_guid: " << w_guid
                                                                   << ",rGuid: " << rGuid);
          change_to_add->serializedPayload.mbuf = nullptr;
          change_to_add->serializedPayload.data = nullptr;
          change_to_add->payload_owner(nullptr);
          history_lock.lock();
          changePool->release_cache(change_to_add);
          history_lock.unlock();
          return;
      }
      bool value = true;

      DSF_LOG_DEBUG(DSFReaderImpl,
                    "change added to history. Sample buf_desc: " << descriptor << " .Now Triger on_data_available. "
                                                                 << "sequence: " << sequence.to64long()
                                                                 << ",w_guid: " << w_guid << ",rGuid: " << rGuid);
      dsfListener.evbsListener->on_data_available(w_guid, sequence, sequence, value);
    }
  } else {
    DSF_LOG_DEBUG(DSFReaderImpl, "reader guid:" << rGuid << ",WaitMessage not success ret: " << ret());
  }
  return;
}

bool DSFReaderImpl::change_received(vbs::common::CacheChange_t* change, vbsutil::xmlparser::GUID_t w_guid) {
    change->writerGUID = w_guid;
    size_t unknown_missing_changes_up_to = 1U;
    evbs::edds::dds::SampleRejectedStatusKind rejection_reason;
    change->bCanRead = true;
    auto is_add = historyPtr->received_change(change, unknown_missing_changes_up_to, rejection_reason, true);
    if (is_add) {
        historyPtr->total_unread_increase();
        DSF_LOG_DEBUG(DSFReaderImpl, "add change to history success! sequence number: "
                                         << change->sequenceNumber << "writer guid: " << change->writerGUID
                                         << "reader guid: " << change->readerGUID);
        return true;
    }
    DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                  "add change to history fail! sequence number: " << change->sequenceNumber
                                                                  << "writer guid: " << change->writerGUID
                                                                  << "reader guid: " << change->readerGUID);
    return false;
}

void DSFReaderImpl::DataReceiveCallBack(const ChannelResource* cr) {
  if (!shmReaderPtr->get_enable()) {
    DSF_LOG_INFO(DSFReaderImpl, "reader guid:" << rGuid << ",ShmReader is not enable yet!!!");
    // wait on condition wait for enable true
    std::unique_lock<std::mutex> lk(shmReaderPtr->mtx_enable_);
    shmReaderPtr->cv_enable_.wait(lk, [this, cr] { return shmReaderPtr->enabled_ || (!cr->alive()); });
    if (!cr->alive()) {
      // force finish
      DSF_LOG_DEBUG(DSFReaderImpl, "reader guid:" << rGuid << ",ChannelResource alive : " << std::boolalpha
                                                  << cr->alive() << " force finish now!!!");
      return;
    }
    DSF_LOG_DEBUG(DSFReaderImpl, "reader guid:" << rGuid << " is matched now, start receive msg!!!");
  }
  HandleDataReceive(cr);
}
#ifdef ENABLE_DES
void DSFReaderImpl::DataReceiveCallBack(ReaderListener* readerListener) {
    if (!reader_->get_enable()) {
        DSF_LOG_WARNING(DSFReaderImpl, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                     << get_entity_id() << ",ShmReader is not enable yet!!!");
    }

    if (readerListener != nullptr) {
        readerListener->on_data_available(reader_);
    } else {
        DSF_LOG_WARNING(DSFReaderImpl, "readerListener is nullptr,reader ids:"
                                           << get_domain_id() << "," << get_participant_id() << "," << get_entity_id());
    }
}
#endif
void DSFReaderImpl::CrashCallBack(const EndpointProxy& ep) {
    DSF_LOG_DEBUG(DSFReaderImpl, "Triger crash_call_back, the guid of remote endpoint is: " << ep.get_local_guid());
    dsfListener.evbsListener->on_endpoint_crash(ep.get_local_guid());
}

void DSFReaderImpl::disable() {
    // 停线程
    if (shmReaderPtr != nullptr) {
        DSF_LOG_INFO(DSFReaderImpl, "reader guid:" << rGuid << "topic" << topicName << " stop");
        shmReaderPtr->Stop();
    }
}

DSFReaderImpl::~DSFReaderImpl() {
    if (shmReaderPtr != nullptr) {
        DSF_LOG_INFO(DSFReaderImpl, "reader guid:" << rGuid << "topic" << topicName << " remove shm reader");
        (void)ShmCreator::RemoveShmReader(VbsDiscoveryEndpoint::GetInstance(), shmReaderPtr, dataReaderQos);
        shmReaderPtr = nullptr;
    }
}

const DataReaderQos& DSFReaderImpl::get_qos() const {
    return dataReaderQos;
}

uint16_t DSFReaderImpl::get_domain_id() const {
    return domainId;
}

vbsutil::xmlparser::GUID_t DSFReaderImpl::guid() const {
    return rGuid;
}

mbuf::BufferPoolUserManager* DSFReaderImpl::GetPool() const {
    if (shmReaderPtr == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_PRECONDITION_NOT_MET,
                      "reader guid:" << rGuid << ",ShmReader is nullptr, not enable");
        return nullptr;
    }
    return shmReaderPtr->GetPool().get();
}

bool DSFReaderImpl::EdpRegisterInfo() {
    return (VbsDiscoveryEndpoint::GetInstance()->RegisterLocalReaderInfo(shmReaderPtr, dataReaderQos));
}

void DSFReaderImpl::move_mbuf_into_change(std::shared_ptr<mbuf::Buffer>& mbuf, vbs::common::CacheChange_t* change,
                                          uint32_t length, uint64_t timestamp,
                                          vbs::common::SequenceNumber_t current_sequence) {
    change->serializedPayload.data = static_cast<vbsutil::xmlparser::octet*>(mbuf->data());
    change->serializedPayload.mbuf = mbuf;
    change->serializedPayload.length = length;
    change->sequenceNumber = current_sequence;
    change->sourceTimestamp.from_ns(timestamp);
    change->payload_owner(payloadPool.get());
}

void DSFReaderImpl::move_meta_auto_mbuf_into_change(const std::shared_ptr<mbuf::Buffer>& mbuf,
                                                    vbs::common::CacheChange_t* change, uint32_t ser_length,
                                                    uint32_t non_ser_length, uint64_t timestamp,
                                                    vbs::common::SequenceNumber_t current_sequence,
                                                    uint64_t head_count) {
    change->serializedPayload.data =
        static_cast<vbsutil::xmlparser::octet*>((uint8_t*)mbuf->data() + head_count * sizeof(uint32_t));
    change->serializedPayload.mbuf = mbuf;
    change->serializedPayload.length = ser_length + non_ser_length;
    change->sequenceNumber = current_sequence;
    change->sourceTimestamp.from_ns(timestamp);
    change->payload_owner(payloadPool.get());
}

void DSFReaderImpl::move_meta_shm_mbuf_into_change(std::shared_ptr<mbuf::Buffer>* mbufs,
                                                   vbs::common::CacheChange_t* change, uint32_t ser_length,
                                                   uint32_t non_ser_length, uint64_t timestamp,
                                                   vbs::common::SequenceNumber_t current_sequence) {
  change->serializedPayload.data = static_cast<vbsutil::xmlparser::octet*>(mbufs[0]->data());
  change->serializedPayload.mbuf = mbufs[0];
  change->serializedPayload.length = ser_length;
  change->nonSerializedPayload.data = static_cast<vbsutil::xmlparser::octet*>(mbufs[1]->data());
  change->nonSerializedPayload.mbuf = mbufs[1];
  change->nonSerializedPayload.length = non_ser_length;
  change->sequenceNumber = current_sequence;
  change->sourceTimestamp.from_ns(timestamp);
  change->payload_owner(payloadPool.get());
}

void DSFReaderImpl::DSFReaderListener::on_data_available(ShmReader* reader) {
    (void)reader;
}

void DSFReaderImpl::DSFReaderListener::on_sample_lost(ShmReader* reader, int32_t sample_lost_since_last_update,
                                                      int32_t error_code) {
    static_cast<void>(reader);
    DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                  "reader ids:" << dsf_reader_->get_domain_id() << "," << dsf_reader_->guid().get_participant_id()
                                << "," << dsf_reader_->guid().entityId.to_uint32()
                                << ", lost sample: " << sample_lost_since_last_update << "error_code: " << error_code);
    evbsListener->on_sample_lost(sample_lost_since_last_update, SampleLostStatusKind::NOT_LOST);
}

void DSFReaderImpl::parser_meta_buf(const mbuf::BufferDescriptor& descriptor, const std::shared_ptr<mbuf::Buffer>& mbuf,
                                    vbs::common::CacheChange_t* change, uint64_t timestamp,
                                    vbs::common::SequenceNumber_t sequence) {
    // 大buffer处理逻辑
    auto head_count = *static_cast<uint32_t*>(mbuf->data());
    auto ser_data_real_length = *(static_cast<uint32_t*>(mbuf->data()) + 1U);
    auto non_ser_data_real_length = *(static_cast<uint32_t*>(mbuf->data()) + 2U);
    if ((descriptor.auto_shm_bigbuffer_ & MultiModeChannel::SHM) != MultiModeChannel::NONE) {
        mbuf::BuildMbufContext context;
        context.manager = GetPool();
        context.userId = descriptor.GetUserID();
        auto p_buffer_info =
            static_cast<mbuf::BufferInfo*>((void*)((uint8_t*)mbuf->data() + head_count * sizeof(uint32_t)));
        std::shared_ptr<mbuf::Buffer> p_sub_buffer[2];
        for (uint32_t i = 0; i < (head_count - 1); ++i) {
            p_sub_buffer[i] = mbuf::BufferUtil::GetBufferFromBufferInfo(*p_buffer_info, context);
            if (p_sub_buffer[i] == nullptr) {
                DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_GET_REBUILD_BUFFER_FAILED,
                              "GetBufferFromBufferInfo failed , the value of i is" << i);
                return;
            }
            p_buffer_info++;
        }
        move_meta_shm_mbuf_into_change(p_sub_buffer, change, ser_data_real_length, non_ser_data_real_length, timestamp,
                                       sequence);
    } else {
        // auto链路 只有1个mbuf
        move_meta_auto_mbuf_into_change(mbuf, change, ser_data_real_length, non_ser_data_real_length, timestamp,
                                        sequence, head_count);
    }
    DSF_LOG_DEBUG(DSFReaderImpl, "ser_data_real_length:" << ser_data_real_length
                                                         << ",non_ser_data_real_length: " << non_ser_data_real_length);
    return;
}

}  // namespace dsfdds
}  // namespace li
