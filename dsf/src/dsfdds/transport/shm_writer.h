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

#ifndef DSFDDS_TRANSPORT_SHMWRITER_H_
#define DSFDDS_TRANSPORT_SHMWRITER_H_

// std
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

// shm transport
#include "dsfdds/transport/channel_resource.h"
#include "membuf/buffer.h"
#include "BufferPoolOwnerManager.h"
#include "membuf/ShareBuffer.h"
#include "membuf/BufferDescriptor.h"
#include "dsfdds/transport/reader_proxy.h"

// dds components
#include "common/utils/SequenceNumber.h"

// lib include
#include "dsfdds/transport/transport_entity.h"

#ifdef DSFDDS_STATISTICS
#include "statistics/data/data.hpp"
#endif
#include <elog/statistics/Statistics.hpp>
#include "common/all_common.h"

namespace li {
namespace dsfdds {

class ShmWriter : public TransportEntity, public vbsutil::statistics::StatisticsWriter {
 public:
    ShmWriter(const std::string& topic_name, GUID_s guid, bool is_static_discovery,
              ReliabilityQosPolicyKind reliability_kind = ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS);

    ShmWriter(uint32_t domainID, const std::string& topic_name, std::string& topic_type,
              vbsutil::xmlparser::GUID_t guid, bool is_static_discovery,
              ReliabilityQosPolicyKind reliability_kind = ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS);

    virtual ~ShmWriter();

 public:
    /**
   *
   * @param ep matched 对端的端点信息
   * @param rp matched 对端在本地的信息映射
   * @return
   */
    bool AddMatchedReader(const EndpointProxy& ep, ReaderProxy& rp);

    bool RemoveMatchedReader(const EndpointProxy& ep);

    /**
   * @brief send mbuf to reader. used in vbs1.x and vbs2.0
   * BufferDescriptor.timestamp is now()
   * 
   * @param mbuf mbuf to be sent
   * @param is_retry 
   * @param guids for reader filter
   * @return true send to all reader success
   * @return false send to any reader fail, or no reader mateched
   */
    bool Send(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry = false,
              const std::vector<GUID_s>& guids = std::vector<GUID_s>());

    /**
   * @brief send mbuf to all matched readers who will pass the filter. only used in vbs2.0
   * 
   * @param mbuf mbuf to be sent
   * @param is_retry 
   * @param length actual length of data
   * @param timestamp timestamp of cache change added into writer history
   * @param filter if not filter or filter return true, mbuf will be sent
   * @return true send to all reader success
   * @return false send to any reader fail, or no reader mateched
   */
    bool SendToAllMatchedNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, uint32_t length,
                             MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                             const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                             std::function<bool(const ReaderProxy&)> filter);

    /**
   * @brief used in vbs1.x and vbs2.0
   * 
   * @param mbuf 
   * @param is_retry 
   * @param guids 
   * @return true 
   * @return false 
   */
    bool SendMeta(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry = false,
                  const std::vector<GUID_s>& guids = std::vector<GUID_s>());
    bool SendMetaToAllMatchedNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, bool is_retry, uint32_t serLength,
                                 MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp,
                                 const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                                 std::function<bool(const ReaderProxy&)> filter);

    bool IsAllUnmatched() const;
    inline void set_enabled(bool enabled) {
        {
            std::lock_guard<std::mutex> lk(mtx_enable_);
            enabled_ = enabled;
        }
        cv_enable_.notify_all();
    }
    inline bool get_enable() const { return enabled_; }

    void add_endpoint_proxy(const EndpointProxy& endpoint_proxy);

    std::string PrintLocalEndpointProxies() const;

    /**
   * @brief 是否有新上线的 Reader
   * 
   * @return true 
   * @return false 
   */
    bool HasNewOnlineReader() const;

    /**
   * @brief 将所有 Reader 标记为非新上线的
   * 
   */
    void MarkAllReaderOld();

#ifdef DSFDDS_STATISTICS
    void set_on_entity_data(const std::function<void(const statistics::DsfEntityData)>&& on_entity_data) {
        on_entity_data_ = on_entity_data;
    }
#endif

 public:
    // key is remote reader entity id, value is crashed
    std::unordered_map<uint32_t, bool> matched_is_crashed_;

    inline std::recursive_timed_mutex& getMutex() { return mp_mutex; }

    inline vbsutil::xmlparser::GUID_t guid() { return guid_t; };

    inline void IncrementSequenceNumber() { ++sequence_number_; };

 protected:
    friend class DiscoveryEndpoint;
    friend class GlobalDiscoveryEndpoint;
    friend class MqDiscoveryEndpoint;
    friend class VbsDiscoveryEndpoint;
    friend class DataWriterImpl;
    friend class DSFWriterImpl;
    friend class CrashListener;
    friend class ShmCreator;
    friend class MembufPoolCrashStrategy;

    mutable std::mutex mtx_matched_reader_gs_;
    // EndpointProxy 远程id信息， ReaderProxy 远程队列相关信息，通过discovery 中的
    // remote_register_readers_ 可以访问ReaderProxy
    std::unordered_map<EndpointProxy, ReaderProxy, EndpointProxyHasher> matched_readers_;
    using MatchedReaderIterator = decltype(matched_readers_.begin());
    // for template use
    inline std::mutex& GetMtxMatchedGs() { return mtx_matched_reader_gs_; }
    inline std::unordered_map<EndpointProxy, ReaderProxy, EndpointProxyHasher>& GetMatchedProxy() {
        return matched_readers_;
    }
    std::shared_ptr<mbuf::BufferPoolOwnerManager>& GetPool();

 private:
    std::vector<ChannelResource> writer_queues_;
    // use for enable setting
    std::mutex mtx_enable_;
    std::condition_variable cv_enable_;
    bool enabled_;
    mutable std::mutex mtx_local_endpoint_proxies_;
    // use for multicast interact,
    // 在一个writer多个reader这样的场景需要多个proxy对应起来
    std::vector<EndpointProxy> local_endpoint_proxies_;
    std::atomic<bool> force_kill_;

 private:
#ifdef EDDS_METRICS
    uint64_t last_metric_send_count_ = 0U;
    uint64_t metric_send_count_ = 0U;
    uint32_t metric_send_count_diff_ = 0U;
    uint32_t metric_data_size_sum = 0U;
    void CountWriterMetrics(bool writer_success, uint32_t data_size);
#endif
    const std::string& getTopicName() const override;

#ifdef DSFDDS_STATISTICS
    std::function<void(statistics::DsfEntityData)> on_entity_data_;
    uint32_t send_count_ = 0;
    std::chrono::high_resolution_clock::time_point prev_time_;
    void writer_statistics(bool writer_success);
#endif
    // Endpoint Mutex
    mutable std::recursive_timed_mutex mp_mutex;
    SequenceNumber_t sequence_number_ {0, 1U};
    bool SendToSingleNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, const MatchedReaderIterator& it, bool is_retry,
                         uint32_t length, MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp);
    bool SendMetaToSingleNTS(const std::shared_ptr<mbuf::Buffer>& mbuf, const MatchedReaderIterator& it, bool is_retry,
                             uint32_t serLength, MultiModeChannel auto_shm_bigbuffer, uint64_t timestamp);
    ReturnCode_t SendRetry(bool is_retry, const MatchedReaderIterator& it, const mbuf::BufferDescriptor& msg) const;
    void HandleSendRet(ReturnCode_t ret, const MatchedReaderIterator& it,
                       const std::shared_ptr<mbuf::ShareBufferOwner>&);

    // membuf
    std::shared_ptr<mbuf::BufferPoolOwnerManager> pool_owner_manager_;

    mbuf::RecoverMode mode;

    uint32_t w_pid_;
    inline bool reader_not_filter_out(const std::vector<GUID_s>& guids, const GUID_s& reader_guid);
    inline bool vbs_reader_not_filter_out(const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                                          const vbsutil::xmlparser::GUID_t& reader_guid);
    vbsutil::xmlparser::GUID_t guid_t;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_SHMWRITER_H_
