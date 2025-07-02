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

// std include
#include <algorithm>
#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>
#include <sys/syscall.h>
#include <unistd.h>

// dsfdds include
#include "shm_creator.h"
#include "DsfLog.hpp"
#include "common/utils/SequenceNumber.h"
#include "dsfdds/discovery/endpoint_proxy.h"
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "dsfdds/transport/shm_reader.h"
#include "membuf/BufferPoolUserManager.h"
#include "common/all_common.h"
#include "common/status/BaseStatus.hpp"
#include "shared_channel_resource.h"

#define READER_METRICS_COUNT_THRESHOLD 10

namespace li {
namespace dsfdds {

constexpr uint32_t ReaderMatchedWriterMaxCount = 128U;

ShmReader::ShmReader(const std::string& topic_name, std::shared_ptr<shmmq::MessageQueue> mq, GUID_s guid,
                     ReaderListener* reader_listener, bool is_static_discovery, bool use_shared_queue,
                     ReliabilityQosPolicyKind reliability_kind)
    : TransportEntity(guid, topic_name, is_static_discovery, reliability_kind),
      domainID_(0U),
      msg_queue_(mq),
      channel_resource_(nullptr),
      queue_name_(mq->get_name()),
      enabled_(false),
      force_kill_(false),
      listener_(reader_listener),
      buffer_pool_user_manager_(std::make_unique<mbuf::BufferPoolUserManager>()),
      use_shared_queue_(use_shared_queue),
      shared_queue_enable_(false) {
    msg_queue_->get_segment()->GetSegmentInfo()->p_state->entityId = get_entity_id();
    if (reliability_kind == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS) {
        buffer_pool_user_manager_->SetRecoverMode(mbuf::RecoverMode::Force);
    }
    DSF_LOG_INFO(ShmReader, "ctor, this: " << this << ",reader ids:" << get_domain_id() << "," << get_participant_id()
                                           << "," << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                           << ",queue name:" << msg_queue_->get_name()
                                           << ",pool recover mode:" << buffer_pool_user_manager_->GetRecoverMode());
}

ShmReader::ShmReader(uint32_t domainID, const std::string& topic_name, std::string& topic_type,
                     std::shared_ptr<shmmq::MessageQueue> mq, vbsutil::xmlparser::GUID_t guid,
                     ReaderListener* reader_listener, bool is_static_discovery, bool use_shared_queue,
                     ReliabilityQosPolicyKind reliability_kind)
    : TransportEntity(domainID, guid, topic_name, topic_type, is_static_discovery, reliability_kind),
      domainID_(domainID),
      msg_queue_(mq),
      channel_resource_(nullptr),
      queue_name_(mq->get_name()),
      enabled_(false),
      force_kill_(false),
      listener_(reader_listener),
      buffer_pool_user_manager_(std::make_unique<mbuf::BufferPoolUserManager>()),
      use_shared_queue_(use_shared_queue),
      shared_queue_enable_(false),
      guid_t(guid) {
    msg_queue_->get_segment()->GetSegmentInfo()->p_state->entityId = get_entity_id();
    if (reliability_kind == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS) {
        buffer_pool_user_manager_->SetRecoverMode(mbuf::RecoverMode::Force);
    }
    DSF_LOG_INFO(ShmReader, "ctor, this: " << this << ",reader ids:" << get_domain_id() << "," << get_participant_id()
                                           << "," << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                           << ",queue name:" << msg_queue_->get_name()
                                           << ",pool recover mode:" << buffer_pool_user_manager_->GetRecoverMode());
}

bool ShmReader::AddMatchedWriter(const EndpointProxy& ep, const WriterProxy& wp) {
    std::lock_guard<std::mutex> lock(mtx_matched_writer_gs_);
    const auto& it = matched_writers_.find(ep);
    if (it == matched_writers_.end()) {
        static_cast<void>(matched_writers_.emplace(ep, wp));
    } else {
        DSF_LOG_WARNING(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                 << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                                 << ",queue name:" << msg_queue_->get_name()
                                                 << ",AddMatchedWriter ep:" << ep << " already in matched_writers_");
    }
    return true;
}
// todo matched writer异常的时候调用
bool ShmReader::RemoveMatchedWriter(const EndpointProxy& ep) {
    std::lock_guard<std::mutex> lock(mtx_matched_writer_gs_);
    auto eraseIt = matched_writers_.find(ep);
    if (eraseIt == matched_writers_.end()) {
        DSF_LOG_WARNING(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                 << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                                 << ",queue name:" << msg_queue_->get_name()
                                                 << ",RemoveMatchedWriter ep:" << ep << " not in matched_writers_");
        return false;
    } else {
        static_cast<void>(matched_writers_.erase(eraseIt));
    }
    return true;
}

void ShmReader::set_channel_resource(std::shared_ptr<ChannelResource> cr) {
    channel_resource_ = std::move(cr);
}

std::shared_ptr<mbuf::Buffer> ShmReader::TakeSample(SampleInfo& sample_info) {
    mbuf::BufferDescriptor descriptor;
    std::shared_ptr<mbuf::Buffer> shared_mbuf = nullptr;
    shmmq::queue_ret_t queue_ret = TakeSampleInternal(descriptor, shared_mbuf);
    if ((queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) && shared_mbuf) {
        sample_info.source_timestamp.from_ns(descriptor.timestamp());
        auto now = std::chrono::high_resolution_clock::now();
        sample_info.reception_timestamp.from_ns(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()));
        sample_info.publication_sequence_number =
            SequenceNumber_t {descriptor.sequence_number_high_, descriptor.sequence_number_low_};
        sample_info.reception_sequence_number = reception_sequence_number_;
        sample_info.valid_data = true;
        return shared_mbuf;
    } else {
        return nullptr;
    }
}

std::shared_ptr<mbuf::Buffer> ShmReader::TakeSample() {
    mbuf::BufferDescriptor descriptor;
    std::shared_ptr<mbuf::Buffer> shared_mbuf = nullptr;
    shmmq::queue_ret_t queue_ret = TakeSampleInternal(descriptor, shared_mbuf);
    if ((queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) && shared_mbuf) {
        return shared_mbuf;
    } else {
        return nullptr;
    }
}

std::shared_ptr<mbuf::Buffer> ShmReader::TakeMetaSample(mbuf::BuildMbufContext& context) {
    mbuf::BufferDescriptor descriptor;
    std::shared_ptr<mbuf::Buffer> shared_mbuf = nullptr;
    shmmq::queue_ret_t queue_ret = TakeSampleInternal(descriptor, shared_mbuf);
    if ((queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) && shared_mbuf) {
        context.manager = buffer_pool_user_manager_.get();
        context.userId = descriptor.GetUserID();
        return shared_mbuf;
    } else {
        return nullptr;
    }
}

bool ShmReader::IsAllUnmatched() const {
    for (const auto& it : matched_writers_) {
        if (!it.second.IsUnmatched()) {
            return false;
        }
    }
    return true;
}

ShmReader::~ShmReader() {
    DSF_LOG_DEBUG(ShmReader, "dtor, this: " << this << ",reader ids:" << get_domain_id() << "," << get_participant_id()
                                            << "," << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                            << ",queue name:" << msg_queue_->get_name());
    Stop();

    if (use_shared_queue_) {
        std::dynamic_pointer_cast<SharedChannelResource>(channel_resource_)->RemoveReader(this);
    }

    DSF_LOG_DEBUG(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                           << ",queue id:" << msg_queue_->get_queue_id()
                                           << ",queue name:" << msg_queue_->get_name()
                                           << ",call_back_thread_ not joinable,thread is already over");
    {
        std::lock_guard<std::mutex> lock(mtx_matched_writer_gs_);
        matched_writers_.clear();
    }
}

void ShmReader::Stop() {
    if (channel_resource_ && !use_shared_queue_) {
        // trigger channel resource destruct
        {
            std::lock_guard<std::mutex> lk(mtx_enable_);
            channel_resource_->disable();
            cv_enable_.notify_all();
        }

        channel_resource_->clear();
    }
}

void ShmReader::add_endpoint_proxy(const EndpointProxy& endpoint_proxy) {
    local_endpoint_proxies_.push_back(endpoint_proxy);
}

ReturnCode_t ShmReader::take(std::vector<std::shared_ptr<mbuf::Buffer>>& data_values,
                             std::vector<SampleInfo>& sample_infos) {
#ifdef ENABLE_DES
    while (true) {
#else
    uint32_t queue_size = get_msg_queue()->get_size();
    uint32_t write_count = 0U;
    while (write_count <= queue_size) {
#endif
        mbuf::BufferDescriptor descriptor;
        std::shared_ptr<mbuf::Buffer> mbuf_ptr;
        shmmq::queue_ret_t ret = TakeSampleInternal(descriptor, mbuf_ptr);
        if ((ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) && mbuf_ptr) {
            auto now = std::chrono::high_resolution_clock::now();
            data_values.push_back(std::move(mbuf_ptr));
            static_cast<void>(sample_infos.emplace_back(
                descriptor.timestamp(),
                static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()),
                SequenceNumber_t(descriptor.sequence_number_high_, descriptor.sequence_number_low_),
                reception_sequence_number_, true));
#ifndef ENABLE_DES
            ++write_count;
        } else {
#else
        } else if (ret == shmmq::queue_ret_t::RETCODE_SHMMQ_QUEUE_EMPTY) {
#endif
            if (data_values.empty()) {
                return ReturnCode_t::RETCODE_PUBSUB_NO_DATA;
            } else {
                return ReturnCode_t::RETCODE_PUBSUB_OK;
            }
        }
    }
    return ReturnCode_t::RETCODE_PUBSUB_OK;
}

std::string ShmReader::PrintLocalEndpointProxies() const {
    std::stringstream proxies;
    for (const auto& it : local_endpoint_proxies_) {
        proxies << it << ", ";
    }
    return proxies.str();
}

shmmq::queue_ret_t ShmReader::TakeSampleInternal(mbuf::BufferDescriptor& buf_desc,
                                                 std::shared_ptr<mbuf::Buffer>& buffer,
                                                 std::function<void(mbuf::PoolID)> recycle_cb) {
    shmmq::queue_ret_t queue_ret;
    if (use_shared_queue_) {
        // peek and not wait
        queue_ret = msg_queue_->ReadFront(&buf_desc, -1);
        if ((buf_desc.reader_id_ == get_entity_id()) && (queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS)) {
            queue_ret = msg_queue_->PopFront();
        } else {
            return queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS
                       ? ReturnCode_t::RETCODE_PUBSUB_SHARED_QUEUE_READER_ID_NOT_MY_OWN
                       : queue_ret;
        }
    } else {
        queue_ret = msg_queue_->Dequeue(buf_desc);
    }

    if (queue_ret == shmmq::queue_ret_t::RETCODE_SHMMQ_SUCCESS) {
#ifndef ENABLE_DES
        if (!channel_resource_->SemTakePost()) {
            DSF_LOG_WARNING(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                     << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                                     << ",queue name:" << msg_queue_->get_name()
                                                     << ",SemTakePost failed");
        }
#endif
        {
            std::lock_guard<std::mutex> lk(this->mtx_writer_pool_infos_);
            auto rebuild_buffer = buffer_pool_user_manager_->RebuildBuffer(buf_desc);
            if (rebuild_buffer) {
                UpdateWriterPoolIdNTS(buf_desc, std::move(recycle_cb));
            }
            buffer.swap(rebuild_buffer);
        }

#ifdef DSFDDS_STATISTICS
        if (li::dsfdds::enable_statistics) {
            reader_statistics(buf_desc);
        }
#endif
        // 虽然desc读取到了但是buffer可能为空 其实也是丢失了数据,所以这里增加判断buffer有效
        if (buffer) {
#ifdef EDDS_METRICS
            uint32_t receive_data_size = buffer->size();
            CountReaderMetrics(buf_desc, receive_data_size);
#endif
            ++reception_sequence_number_;
            msg_queue_->get_segment()->GetSegmentInfo()->p_state->reception_sequence_number =
                reception_sequence_number_;
            static_cast<void>(ProcessSampleLost(buf_desc));
            DSF_LOG_DEBUG(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                   << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                                   << ",queue name:" << msg_queue_->get_name() << ",pid:" << getpid()
                                                   << ",tid:" << syscall(SYS_gettid) << ",ts:"
                                                   << std::chrono::duration_cast<std::chrono::milliseconds>(
                                                          std::chrono::system_clock::now().time_since_epoch())
                                                          .count()
                                                   << ",Dequeue desc:" << buf_desc << " success");
        } else {
            last_error_code_ = ReturnCode_t::RETCODE_MEMBUF_REBUILD_BUFFER_ERR;
            DSF_LOG_ERROR_KEY_T(
                DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_REBUILD_BUFFER_ERR, std::to_string(get_entity_id()),
                "reader ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                              << ",queue id:" << msg_queue_->get_queue_id() << ",queue name:" << msg_queue_->get_name()
                              << ",RebuildBuffer desc:" << buf_desc << " failed");
        }
    } else {
        last_error_code_.store(queue_ret());
        DSF_LOG_DEBUG(ShmReader,
                      ",reader ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                                     << ",queue id:" << msg_queue_->get_queue_id() << ",queue name:"
                                     << msg_queue_->get_name() << " Dequeue return queue_ret: " << queue_ret()
                                     << ", receive mbuf is nullptr:" << std::boolalpha << (buffer == nullptr));
    }
    return queue_ret;
}

bool ShmReader::ProcessSampleLost(const mbuf::BufferDescriptor& buf_desc) {
    auto listener = GetListener();
    if (listener != nullptr) {
        uint32_t writer_participant_id = buf_desc.writer_participant_id();
        uint32_t writer_id = buf_desc.writer_id();
        uint64_t writer_key = (static_cast<uint64_t>(writer_participant_id) << 32) | static_cast<uint64_t>(writer_id);
        auto iter_writer = previous_seq_.find(writer_key);
        if (iter_writer != previous_seq_.end()) {
            auto writer_seq = iter_writer->second;
            SequenceNumber_t cur_seq {buf_desc.sequence_number_high_, buf_desc.sequence_number_low_};
            if (writer_seq < cur_seq) {
                uint64_t tmp = (cur_seq - writer_seq).to64long() - 1;
                int32_t lost_samples = (tmp > static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
                                           ? std::numeric_limits<int32_t>::max()
                                           : static_cast<int32_t>(tmp);
                if (0 < lost_samples) {  // There are lost samples.
                    if ((reliability_kind_ == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS) &&
                        (last_error_code_ == 0
#ifdef ENABLE_DES
                         // 原因参见：https://li.feishu.cn/docx/CCCNdUjdmoHJ5Vxdid3cYac5ndg#Xsp5dcNVuoTpI1xI8VYc262YnLv
                         || (last_error_code_ == ReturnCode_t::RETCODE_SHMMQ_QUEUE_EMPTY)
#endif
                             )) {
                        // in best effort qos lost sample because overwrite
                        listener->on_sample_lost(this, lost_samples, SampleLostStatusErrorCode::BEST_EFFORT_OVERWRITE);
                    } else {
                        listener->on_sample_lost(this, lost_samples, last_error_code_.load());
                    }
#ifdef EDDS_METRICS
                    GUID_s writer_guid = GetWriterGuid(buf_desc);
                    GUID_s guid_ = get_guid();
                    vbsutil::xmlparser::GUID_t* w_guid = reinterpret_cast<vbsutil::xmlparser::GUID_t*>(&writer_guid);
                    vbsutil::xmlparser::GUID_t* r_guid = reinterpret_cast<vbsutil::xmlparser::GUID_t*>(&guid_);
                    ReliabilityKind_t kind =
                        (get_reliability_kind() == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS)
                            ? ReliabilityKind_t::BEST_EFFORT
                            : ReliabilityKind_t::RELIABLE;
                    vbsutil::statistics::StatisticsReader::on_packet_lost(kind, *w_guid, *r_guid, writer_seq.to64long(),
                                                                          cur_seq.to64long());
#endif
                    iter_writer->second = cur_seq;
                    return true;
                }
                iter_writer->second = cur_seq;
            } else {
                DSF_LOG_DEBUG(ShmReader, "reader ids:" << get_domain_id() << "," << get_participant_id() << ","
                                                       << get_entity_id() << ",queue id:" << msg_queue_->get_queue_id()
                                                       << ",queue name:" << msg_queue_->get_name()
                                                       << ", matched writer_id:" << writer_id
                                                       << ",previous_seq_:" << writer_seq.to64long()
                                                       << " >= cur_seq:" << cur_seq.to64long());
                return false;
            }
        } else {
            previous_seq_[writer_key] = {buf_desc.sequence_number_high_, buf_desc.sequence_number_low_};
        }
    }
    return false;
}

#ifdef EDDS_METRICS
void ShmReader::CountReaderMetrics(const mbuf::BufferDescriptor& descriptor, uint32_t data_size) {
    ++metric_receive_count_;
    uint64_t send_time = descriptor.timestamp();
    uint64_t receive_time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                      std::chrono::high_resolution_clock::now().time_since_epoch())
                                                      .count());
    metric_latency_sum_ += static_cast<float>(receive_time - send_time);
    metric_data_size_sum_ += data_size;
    metric_receive_count_diff_ = metric_receive_count_ - last_metric_receive_count_;
    if (metric_receive_count_diff_ >= READER_METRICS_COUNT_THRESHOLD) {
        GUID_s guid_r = get_guid();
        vbsutil::xmlparser::GUID_t* r_guid = reinterpret_cast<vbsutil::xmlparser::GUID_t*>(&guid_r);
        GUID_s writer_guid = GetWriterGuid(descriptor);
        vbsutil::xmlparser::GUID_t* w_guid = reinterpret_cast<vbsutil::xmlparser::GUID_t*>(&writer_guid);
        // Writer to Reader throughput
        ReliabilityKind_t kind = (get_reliability_kind() == ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS)
                                     ? ReliabilityKind_t::BEST_EFFORT
                                     : ReliabilityKind_t::RELIABLE;
        vbsutil::statistics::StatisticsReader::on_receive_throughput(kind, *w_guid, *r_guid, metric_data_size_sum_);
        metric_data_size_sum_ = 0U;
        // Writer to Reader latency
        vbsutil::statistics::StatisticsReader::on_history_latency(
            kind, *w_guid, *r_guid, metric_latency_sum_ / static_cast<float>(metric_receive_count_diff_));
        last_metric_receive_count_ = metric_receive_count_;
        metric_latency_sum_ = 0.0;
    }
}
// use writer participant_id pid entity_id replace reader_guid content , produce writer_guid
GUID_s ShmReader::GetWriterGuid(const mbuf::BufferDescriptor& buf_desc) {
    uint32_t writer_participant_id = buf_desc.writer_participant_id();
    uint32_t writer_pid = buf_desc.pid();
    uint32_t writer_id = buf_desc.writer_id();
    uint8_t did = static_cast<uint8_t>(domain_id_);
    GUID_s writer_guid = {};
    static_cast<void>(writer_guid.guidPrefix.GenerateGuidPrefix(writer_participant_id, did, &writer_guid.guidPrefix));
    static_cast<void>(memcpy(writer_guid.guidPrefix.value + 6, &writer_pid, 4U));
    static_cast<void>(memcpy(writer_guid.entityId.value, &writer_id, 4U));
    return writer_guid;
}
#endif

const std::string& ShmReader::getTopicName() const {
    return topic_name_;
}

#ifdef DSFDDS_STATISTICS
void ShmReader::reader_statistics(const mbuf::BufferDescriptor& descriptor) {
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
    if (receive_count_ == 0U) {
        prev_time_ = now;
    } else {
        diff = std::chrono::duration_cast<std::chrono::microseconds>(now - prev_time_);
    }
    prev_time_ = now;

    ++receive_count_;

    if ((receive_count_ % static_cast<uint32_t>(sample_freq)) == 0U) {
        // Reader receive count
        auto type = statistics::RECEIVE;
        statistics::DsfEntityData entity_data(type, get_guid(), receive_count_);
        on_entity_data_(entity_data);
        if (receive_count_ > 1) {
            // Reader throughput
            float throughput = (static_cast<float>(sizeof(mbuf::BufferDescriptor))) / static_cast<float>(diff.count());
            auto type = statistics::THROUGHPUT;
            statistics::DsfEntityData throughput_data(type, get_guid(), throughput);
            on_entity_data_(throughput_data);
        }
        // Writer reader latency
        uint64_t send_time = descriptor.timestamp();
        uint64_t receive_time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                          std::chrono::high_resolution_clock::now().time_since_epoch())
                                                          .count());
        float latency_ = static_cast<float>(receive_time - send_time);
        GUID_s reader_guid = get_guid();
        GUID_s writer_guid {};
        {
            std::lock_guard<std::mutex> lock(mtx_matched_writer_gs_);
            EndpointProxy writer_ep(0U, get_domain_id(), static_cast<uint32_t>(descriptor.writer_participant_id()),
                                    descriptor.writer_id(), get_domain_id(),
                                    static_cast<uint32_t>(get_participant_id()), static_cast<uint32_t>(get_entity_id()),
                                    descriptor.pid());
            auto wit = matched_writers_.find(writer_ep);
            if (wit != matched_writers_.end()) {
                writer_guid = wit->second.get_guid();
            } else {
                DSF_LOG_ERROR(DSF_PUBSUB_DISCOVERY, ReturnCode_t::RETCODE_DISCOVERY_NOT_FIND_EP_MATCHED_WRITER,
                              "matched_writers_ can not find endpoint_proxy:" << writer_ep);
                return;
            }
        }
        statistics::DsfWriterReaderLatencyData latency_data(reader_guid, writer_guid, latency_);
        on_latency_data_(latency_data);
    }
}
#endif

bool ShmReader::IsCrashPidInMatchedWriter(const uint32_t uuid, const uint64_t pid) {
    std::lock_guard<std::mutex> lk(this->mtx_writer_pool_infos_);
    for (const auto& writer_pool_info : writer_pool_infos_) {
        // 当收到crash的信号时，需要判断其与 pool_id 是收到过的 writer 的 pool id
        // 且pid相等，才认为是收到了匹配writer的crash信号
        if (writer_pool_info.his_pool_ids_.find(uuid) != writer_pool_info.his_pool_ids_.end() &&
            writer_pool_info.writer_pid_ == pid) {
            return true;
        }
    }
    return false;
}

void ShmReader::UpdateWriterPoolIdNTS(const mbuf::BufferDescriptor& buf_desc,
                                      std::function<void(mbuf::PoolID)> recycle_cb) {
    bool pool_matched = false;
    for (auto& writer_pool_info : writer_pool_infos_) {
        if (writer_pool_info.writer_entity_id_ == buf_desc.writer_id() &&
            writer_pool_info.writer_participant_id_ == buf_desc.writer_participant_id() &&
            writer_pool_info.writer_pid_ == buf_desc.pid()) {
            pool_matched = true;
            if (writer_pool_info.cur_pool_id_ != buf_desc.GetPoolID()) {
                DSF_LOG_INFO(ShmReader, "UpdateWriterPoolId. writer entity id:"
                                            << buf_desc.writer_id()
                                            << ",writer participant id:" << buf_desc.writer_participant_id()
                                            << ",pid:" << buf_desc.pid() << ",pool id:" << buf_desc.GetPoolID()
                                            << ",cur pool id:" << writer_pool_info.cur_pool_id_);
                // 对应 writer 的旧 pool 入回收池
                if (recycle_cb) {
                    recycle_cb(writer_pool_info.cur_pool_id_);
                }
                writer_pool_info.his_pool_ids_.erase(writer_pool_info.cur_pool_id_);
                writer_pool_info.cur_pool_id_ = buf_desc.GetPoolID();
                writer_pool_info.his_pool_ids_.emplace(buf_desc.GetPoolID());
                // 如果对应 writer 已经 crash 了, 新的 pool 直接入回收池(crash 信号先于新 pool 的消息处理的 corner case)
                if (writer_pool_info.is_writer_crashed_ && recycle_cb) {
                    recycle_cb(buf_desc.GetPoolID());
                }
            }
            break;
        }
    }
    if (!pool_matched) {
        DSF_LOG_INFO(ShmReader, "UpdateWriterPoolId new writer. writer entity id "
                                    << buf_desc.writer_participant_id() << " writer participant id "
                                    << buf_desc.writer_participant_id() << " pid " << buf_desc.pid() << " pool id "
                                    << buf_desc.GetPoolID());
        writer_pool_infos_.emplace_back(buf_desc.writer_id(), buf_desc.writer_participant_id(), buf_desc.pid(),
                                        buf_desc.GetPoolID());
    }
}

void ShmReader::HandleOfflineWriterPoolIds(const WriterProxy& writer_proxy,
                                           std::function<void(mbuf::PoolID)> recycle_cb) {
    DSF_LOG_DEBUG(ShmReader, "HandleOfflineWriterPoolIds writer proxy entity id "
                                 << writer_proxy.get_entity_id() << " writer participant id "
                                 << writer_proxy.get_participant_id() << " writer pid " << writer_proxy.get_pid());
    std::lock_guard<std::mutex> lk(this->mtx_writer_pool_infos_);
    for (auto& writer_pool_info : writer_pool_infos_) {
        if (writer_pool_info.writer_entity_id_ == writer_proxy.get_entity_id() &&
            writer_pool_info.writer_participant_id_ == writer_proxy.get_participant_id() &&
            writer_pool_info.writer_pid_ == writer_proxy.get_pid()) {
            for (auto pool_id : writer_pool_info.his_pool_ids_) {
                recycle_cb(pool_id);
            }
            writer_pool_info.is_writer_crashed_ = true;
            break;
        }
    }
}

void ShmReader::HandleMatchedWriterStates(uint32_t w_entity_id, uint64_t w_pid,
                                          std::shared_ptr<shmmq::MessageQueue> mq) {
    auto mq_state = mq->get_segment()->GetSegmentInfo()->p_state;
    for (size_t i = 0; i < ReaderMatchedWriterMaxCount; i++) {
        bool match_condition = (w_entity_id == mq_state->write_info[i].entity_id) &&
                               (w_pid == mq_state->write_info[i].write_pid) &&
                               mq_state->write_info[i].is_used_flag.load();
        if (match_condition) {
            mq_state->write_count--;
            mq_state->write_info[i].send_count.store(0);
            mq_state->write_info[i].is_used_flag.store(false);
            DSF_LOG_DEBUG(VbsDiscoveryEndpoint,
                          "write_info [" << i << "] is set to be free! ,reader ids:" << get_domain_id() << ","
                                         << get_participant_id() << "," << get_entity_id() << ",queue id:"
                                         << msg_queue_->get_queue_id() << ",queue name:" << msg_queue_->get_name()
                                         << ", handle w_entity_id: " << w_entity_id << ", w_pid: " << w_pid
                                         << ", write_info entity_id" << mq_state->write_info[i].entity_id
                                         << ", write_info write_pid" << mq_state->write_info[i].write_pid
                                         << ", write_count:" << mq_state->write_count);
            break;
        } else {
            DSF_LOG_DEBUG(
                VbsDiscoveryEndpoint,
                "is_used_flag value is: "
                    << mq_state->write_info[i].is_used_flag.load()
                    << ",Proxy’s info is not equal to write_info's info: " << ", handle w_entity_id: " << w_entity_id
                    << ", w_pid: " << w_pid << ", write_info entity_id" << mq_state->write_info[i].entity_id
                    << ", write_info write_pid" << mq_state->write_info[i].write_pid
                    << ",reader ids:" << get_domain_id() << "," << get_participant_id() << "," << get_entity_id()
                    << ",queue id:" << msg_queue_->get_queue_id() << ",queue name:" << msg_queue_->get_name()
                    << ",write_count:" << mq_state->write_count);
        }
    }
}

}  // namespace dsfdds
}  // namespace li
