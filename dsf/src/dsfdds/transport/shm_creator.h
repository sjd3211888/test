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

#ifndef DSFDDS_TRANSPORT_SHM_CREATOR_H_
#define DSFDDS_TRANSPORT_SHM_CREATOR_H_

// std include
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// dds include
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"

// dsf dds include
#include "dsfdds/discovery/discovery_endpoint.h"
#include "dsfdds/transport/crash_listener.h"
#include "dsfdds/transport/reader_listener.h"
#include "dsfdds/transport/shm_reader.h"
#include "dsfdds/transport/shm_writer.h"
#include "dsfdds/transport/writer_listener.h"
// membuf shmqueue include
#include "BufferPoolOwner.h"
#include "common/all_common.h"
// shmmq
#include "shmmq/shm_manager.h"

namespace li {
namespace dsfdds {

constexpr uint32_t FilterExpressionMaxLen = 256U;
// 注意：预留的FilterReservedSize必须是1024的整数倍
constexpr uint32_t FilterReservedSize = 1024U;

class ShmCreator {
 public:
    static std::shared_ptr<CrashListener> CreateCrashListener(std::shared_ptr<DiscoveryEndpoint> discovery);
    static ShmWriter* CreateShmWriter(GUID_s guid, const std::string& topic_name, const DataWriterQos& writer_qos,
                                      bool is_static_discovery);
    static ShmWriter* CreateShmWriter(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                                      std::string& topic_type, const DataWriterQos& writer_qos,
                                      bool is_static_discovery);

    static std::unique_ptr<mbuf::BufferPoolOwnerManager> CreateBufferPool(
        const std::string& topic_name, uint16_t domain_id, uint16_t participant_id, uint32_t entity_id,
        const DataWriterQos& qos = DATAWRITER_QOS_DEFAULT, bool authentication = false);

    struct PoolParam {
        PoolParam(uint32_t buf_pool_size, const std::string& name, uint32_t buf_max_num, mbuf::RecoverMode level)
            : buf_pool_size(buf_pool_size), name(name), buf_max_num(buf_max_num), mode(level), key() {}

        PoolParam() = default;

        uint32_t buf_pool_size;
        std::string name;
        uint32_t buf_max_num;
        mbuf::RecoverMode mode;
        std::string key;
    };

    static bool RemoveShmWriter(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint, ShmWriter* writer,
                                const DataWriterQos& qos);

    static ShmReader* CreateShmReader(GUID_s guid, const std::string& topic_name, ReaderListener* const readerListener,
                                      const DataReaderQos& reader_qos, bool is_static_discovery);
    static ShmReader* CreateShmReader(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                                      std::string& topicType, ReaderListener* const readerListener,
                                      const DataReaderQos& reader_qos, bool is_static_discovery);
    static ShmReader* CreateSharedQueueShmReader(GUID_s guid, int16_t entity_id, ReaderListener* const readerListener,
                                                 const std::string& topic_name, const DataReaderQos& reader_qos,
                                                 bool is_static_discovery, std::shared_ptr<shmmq::MessageQueue> mq);
    static ShmReader* CreateSharedQueueShmReader(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, int16_t entity_id,
                                                 ReaderListener* const readerListener, const std::string& topic_name,
                                                 std::string& topic_type, const DataReaderQos& reader_qos,
                                                 bool is_static_discovery, std::shared_ptr<shmmq::MessageQueue> mq);

    static bool RemoveShmReader(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint, ShmReader* reader,
                                const DataReaderQos& qos);

 private:
    static bool CheckCreateBufferPoolParam(uint16_t domain_id, const std::string& topic_name, const DataWriterQos& qos);
    static bool CheckCreateShmReaderParam(uint16_t domain_id, uint16_t parId, const std::string& topic_name,
                                          ReaderListener* const readerListener, int16_t entity_id,
                                          const DataReaderQos& qos);
    static mbuf::AllocCfg GetAllocCfg(const DataWriterQos& qos);
    static bool CheckPoolSize(const DataWriterQos& qos, uint32_t& buf_max_num, uint32_t& buf_pool_size,
                              const ReliabilityQosPolicyKind& reliability_qos_policy_kind);
    static std::shared_ptr<shmmq::MessageQueue> CreateQueue(uint8_t domain_id, uint16_t participant_id,
                                                            uint32_t entity_id, const std::string& topic_name,
                                                            ReaderListener* const readerListener,
                                                            const DataReaderQos& reader_qos, bool is_static_discovery,
                                                            bool authentication = false);
};

}  // namespace dsfdds
}  // namespace li
#endif  // DSFDDS_TRANSPORT_SHM_CREATOR_H_
