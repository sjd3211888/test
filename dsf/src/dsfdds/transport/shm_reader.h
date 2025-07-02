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

#ifndef DSFDDS_TRANSPORT_SHMREADER_H_
#define DSFDDS_TRANSPORT_SHMREADER_H_

// std
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <atomic>

// shm transport
#include "dsfdds/discovery/endpoint_proxy.h"
#include "dsfdds/transport/channel_resource.h"
#include "dsfdds/transport/reader_listener.h"
#include "dsfdds/transport/transport_entity.h"
#include "dsfdds/transport/writer_proxy.h"

// dds components
#include "dsfdds/dds/subscriber/SampleInfo.hpp"
// mbuf
#include "membuf/buffer.h"
#include "membuf/buffer_info.h"
#include "membuf/BufferPoolUserManager.h"
#ifdef DSFDDS_STATISTICS
#include "statistics/data/data.hpp"
#endif
#include <elog/statistics/Statistics.hpp>
#include "common/all_common.h"
namespace li {
namespace dsfdds {

class ShmReader : public TransportEntity, public vbsutil::statistics::StatisticsReader {
 public:
    ShmReader(const std::string& topic_name, std::shared_ptr<shmmq::MessageQueue> mq, GUID_s guid,
              ReaderListener* reader_listener, bool is_static_discovery, bool use_shared_queue = false,
              ReliabilityQosPolicyKind reliability_kind = ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS);
    ShmReader(uint32_t domainID, const std::string& topic_name, std::string& topic_type,
              std::shared_ptr<shmmq::MessageQueue> mq, vbsutil::xmlparser::GUID_t guid, ReaderListener* reader_listener,
              bool is_static_discovery, bool use_shared_queue = false,
              ReliabilityQosPolicyKind reliability_kind = ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS);
    virtual ~ShmReader();
    void Stop();
    bool AddMatchedWriter(const EndpointProxy& ep, const WriterProxy& wp);
    bool RemoveMatchedWriter(const EndpointProxy& ep);
    std::shared_ptr<mbuf::Buffer> TakeSample(SampleInfo& sample_info);
    std::shared_ptr<mbuf::Buffer> TakeSample();
    std::shared_ptr<mbuf::Buffer> TakeMetaSample(mbuf::BuildMbufContext& context);
    ReturnCode_t take(std::vector<std::shared_ptr<mbuf::Buffer>>& data_values, std::vector<SampleInfo>& sample_infos);
    void set_channel_resource(std::shared_ptr<ChannelResource> cr);
    inline void set_enabled(bool enabled) {
        {
            std::lock_guard<std::mutex> lk(mtx_enable_);
            enabled_ = enabled;
        }
        cv_enable_.notify_all();
    }
    inline bool get_enable() const { return enabled_; }
    void set_shared_queue_enabled(bool enable) { return shared_queue_enable_.store(enable); }
    bool get_shared_queue_enable() const { return shared_queue_enable_.load(); }
    inline std::string get_queue_name() { return queue_name_; }
    inline std::shared_ptr<shmmq::MessageQueue> get_msg_queue() const { return msg_queue_; }

    bool IsAllUnmatched() const;

    void add_endpoint_proxy(const EndpointProxy& endpoint_proxy);
    std::string PrintLocalEndpointProxies() const;

#ifdef DSFDDS_STATISTICS
    void set_on_latency_data(
        const std::function<void(const statistics::DsfWriterReaderLatencyData)>&& on_latency_data) {
        on_latency_data_ = on_latency_data;
    }
    void set_on_entity_data(const std::function<void(const statistics::DsfEntityData)>&& on_entity_data) {
        on_entity_data_ = on_entity_data;
    }
#endif

 public:
    // key is writer entity id value is crashed  todo remove不再需要删除
    std::unordered_map<uint32_t, bool> matched_is_crashed_;
    // key is writer entity id value is current writer mbuf pool id is insert
    inline std::recursive_timed_mutex& getMutex() { return mp_mutex; }
    ReaderListener* GetListener() const { return listener_; }
    bool IsCrashPidInMatchedWriter(const uint32_t uuid, const uint64_t pid);
    inline vbsutil::xmlparser::GUID_t guid() { return guid_t; }

    /**
   * @brief mbuf pool 扩容 feature, reader 收到新的 pool id 后进行更新,
   *  并将旧的 pool id 放入回收池
   * 
   * @param buf_desc 
   */
    void UpdateWriterPoolIdNTS(const mbuf::BufferDescriptor& buf_desc, std::function<void(mbuf::PoolID)> recycle_cb);

    /**
   * @brief 处理 crashed writer 使用过的所有 pool_id
   * 
   * @param writer_proxy 
   */
    void HandleOfflineWriterPoolIds(const WriterProxy& writer_proxy, std::function<void(mbuf::PoolID)> recycle_cb);

    /**
   * @brief 处理下线writer 在reader中存储的状态
   * 
   * @param w_entity_id
   * @param w_pid
   * @param mq 
   */
    void HandleMatchedWriterStates(uint32_t w_entity_id, uint64_t w_pid, std::shared_ptr<shmmq::MessageQueue> mq);

 private:
    friend class DiscoveryEndpoint;
    friend class GlobalDiscoveryEndpoint;
    friend class MqDiscoveryEndpoint;
    friend class VbsDiscoveryEndpoint;
    friend class DataReaderImpl;
    friend class DSFReaderImpl;
    friend class ShmCreator;
    friend class CrashListener;
    friend class MbufCrashHandler;
    friend class MembufPoolCrashStrategy;
    friend class SharedChannelResource;
    // 匹配的对端通信资源
    mutable std::mutex mtx_matched_writer_gs_;
    std::unordered_map<EndpointProxy, WriterProxy, EndpointProxyHasher> matched_writers_;
    // for template use
    inline std::mutex& GetMtxMatchedGs() { return mtx_matched_writer_gs_; }
    inline std::unordered_map<EndpointProxy, WriterProxy, EndpointProxyHasher>& GetMatchedProxy() {
        return matched_writers_;
    }
    inline std::unique_ptr<mbuf::BufferPoolUserManager>& GetPool() { return buffer_pool_user_manager_; }

    // pool 回收管理逻辑相关
    mutable std::mutex mtx_writer_pool_infos_;
    struct WriterPoolInfo {
        uint64_t writer_entity_id_;
        uint64_t writer_participant_id_;
        uint64_t writer_pid_;
        mbuf::PoolID cur_pool_id_;
        std::set<mbuf::PoolID> his_pool_ids_;
        bool is_writer_crashed_;

        WriterPoolInfo(uint64_t writer_entity_id, uint64_t writer_participant_id, uint64_t writer_pid,
                       mbuf::PoolID cur_pool_id)
            : writer_entity_id_(writer_entity_id),
              writer_participant_id_(writer_participant_id),
              writer_pid_(writer_pid),
              cur_pool_id_(cur_pool_id),
              his_pool_ids_ {cur_pool_id},
              is_writer_crashed_ {false} {}
    };
    std::vector<WriterPoolInfo> writer_pool_infos_;

    uint32_t domainID_;
    // 本端的通信资源
    std::shared_ptr<shmmq::MessageQueue> msg_queue_;
    std::shared_ptr<ChannelResource> channel_resource_;
    std::string queue_name_;
    // use for enable setting
    bool enabled_;
    std::condition_variable cv_enable_;
    std::mutex mtx_enable_;
    // use for multicast interact
    mutable std::mutex mtx_local_endpoint_proxies_;
    std::vector<EndpointProxy> local_endpoint_proxies_;

    std::atomic<bool> force_kill_;

#ifdef EDDS_METRICS
    uint64_t last_metric_receive_count_ = 0U;
    uint64_t metric_receive_count_ = 0U;
    uint32_t metric_receive_count_diff_ = 0U;
    float metric_latency_sum_ = 0.0;
    uint32_t metric_data_size_sum_ = 0U;
    void CountReaderMetrics(const mbuf::BufferDescriptor& descriptor, uint32_t data_size);
    GUID_s GetWriterGuid(const mbuf::BufferDescriptor& buf_desc);
#endif
    const std::string& getTopicName() const override;

#ifdef DSFDDS_STATISTICS
    std::function<void(statistics::DsfEntityData)> on_entity_data_;
    std::function<void(statistics::DsfWriterReaderLatencyData)> on_latency_data_;
    uint32_t receive_count_ = 0U;
    std::chrono::high_resolution_clock::time_point prev_time_;
    void reader_statistics(const mbuf::BufferDescriptor& descriptor);
#endif

 private:
    shmmq::queue_ret_t TakeSampleInternal(mbuf::BufferDescriptor& buf_desc, std::shared_ptr<mbuf::Buffer>& buffer,
                                          std::function<void(mbuf::PoolID)> recycle_cb = {});
    // Endpoint Mutex
    mutable std::recursive_timed_mutex mp_mutex;
    ReaderListener* listener_;
    // key是 writer_participant_id << 32 | writer_id,后续使用guid作为key
    std::unordered_map<uint64_t, SequenceNumber_t> previous_seq_ {};
    bool ProcessSampleLost(const mbuf::BufferDescriptor& buf_desc);
    std::atomic_int last_error_code_ {0};
    SequenceNumber_t reception_sequence_number_;
    // membuf
    std::unique_ptr<mbuf::BufferPoolUserManager> buffer_pool_user_manager_;
    bool use_shared_queue_;
    // 因为共享队列模式下，
    // 1. 不能wait cv在原有的enable变量上，因为一旦一个reader wait会阻塞其他的reader
    // 2. 不能复用原有的enable变量，因为原有的enable是个普通变量，一是可能被内联优化，二是可能有并发竞争问题
    // 所以对于共享队列新增一个表示enable的变量
    std::atomic_bool shared_queue_enable_;
    vbsutil::xmlparser::GUID_t guid_t;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_SHMREADER_H_
