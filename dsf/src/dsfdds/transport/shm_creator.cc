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

#include "dsfdds/transport/shm_creator.h"
// linux api
#include <unistd.h>

// std include
#include <cstdint>
#include <string>
#include <utility>
#include <limits>
#include "common/property/PropertyParse.h"

// dsfdds entity
#include "DsfLog.hpp"
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "dsfdds/discovery/endpoint_proxy.h"

namespace li {
namespace dsfdds {

std::shared_ptr<CrashListener> ShmCreator::CreateCrashListener(std::shared_ptr<DiscoveryEndpoint> discovery) {
    return CrashListener::GetInstance(std::move(discovery));
}

bool ShmCreator::CheckCreateBufferPoolParam(uint16_t domain_id, const std::string& topic_name,
                                            const DataWriterQos& qos) {
    if (topic_name.empty()) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_TOPIC_NAME_EMPTY,
                      "topic_name is nullptr when CheckCreateBufferPoolParam");
        return false;
    }
    if (domain_id > MAX_DOMAIN_ID) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                      "domain id = " << domain_id << ",  domain id > " << MAX_DOMAIN_ID << "!!!");
        return false;
    }
    ReliabilityQosPolicyKind reliability_qos_policy_kind = qos.reliability().kind;
    if ((reliability_qos_policy_kind != ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS) &&
        (reliability_qos_policy_kind != ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS)) {
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
            "create mbuf pool with unknown kind : " << static_cast<int32_t>(reliability_qos_policy_kind) << "!!!");
        return false;
    }
    return true;
}

bool ShmCreator::CheckPoolSize(const DataWriterQos& qos, uint32_t& buf_max_num, uint32_t& buf_pool_size,
                               const ReliabilityQosPolicyKind& reliability_qos_policy_kind) {
    buf_max_num = PropertyParse::GetMaxAllocation(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::MaxAllocation));

    buf_pool_size =
        PropertyParse::GetPoolSize(PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::PoolSize));
    if ((buf_max_num == 0U) || (buf_pool_size == 0U)) {
        DSF_LOG_INFO(ShmCreator, "buf_max_num:" << buf_max_num << ",or buf_pool_size:" << buf_pool_size << " is 0");
        buf_max_num = (buf_max_num == 0 ? MbufPropertyField::DefaultMaxAllocation : buf_max_num);
        buf_pool_size = (buf_pool_size == 0 ? MbufPropertyField::DefaultPoolSize : buf_pool_size);
    }
    DSF_LOG_DEBUG(ShmCreator, "mbuf pool: buf_pool_size: " << buf_pool_size << ",buf_max_num: " << buf_max_num
                                                           << ",reliability_qos_policy_kind: "
                                                           << +static_cast<int32_t>(reliability_qos_policy_kind));
    return true;
}

mbuf::AllocCfg ShmCreator::GetAllocCfg(const DataWriterQos& qos) {
    size_t poolSizeUpLimit = PropertyParse::GetPoolSizeUpLimit(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::PoolSizeUpLimit));
    size_t allocSizeLimit = PropertyParse::GetAllocSizeLimit(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::AllocSizeLimit));
    const size_t bufCountUpLimit = PropertyParse::GetBufCountUpLimit(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::BufCountUpLimit));
    const double poolSizeUsageScaleDownThld = PropertyParse::GetPoolSizeUsageScaleDownThld(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::PoolSizeUsageScaleDownThld));
    const uint64_t lowUsageDurationForScaleDown = PropertyParse::GetLowUsageDurationForScaleDown(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::LowUsageDurationForScaleDown));
    const double allocSizeSteadyTol = PropertyParse::GetAllocSizeSteadyTol(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::AllocSizeSteadyTol));
    const size_t allocSteadyDurationForScaleDown = PropertyParse::GetAllocSteadyDurationForScaleDown(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::AllocSteadyDurationForScaleDown));
    const double scaleDownTargetUsage = PropertyParse::GetScaleDownTargetUsage(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::ScaleDownTargetUsage));
    const uint64_t queryStatusInterval = PropertyParse::GetQueryStatusInterval(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::QueryStatusInterval));
    if (poolSizeUpLimit < MbufPropertyField::DefaultPoolSizeUpLimit) {
        DSF_LOG_WARNING(ShmCreator, "config " << MbufPropertyField::PoolSizeUpLimit << " is less than default: "
                                              << MbufPropertyField::DefaultPoolSizeUpLimit << " , set to default");
        poolSizeUpLimit = MbufPropertyField::DefaultPoolSizeUpLimit;
    }
    if (allocSizeLimit > poolSizeUpLimit) {
        DSF_LOG_WARNING(ShmCreator, "config " << MbufPropertyField::AllocSizeLimit << " is bigger than "
                                              << MbufPropertyField::PoolSizeUpLimit << " , set to "
                                              << MbufPropertyField::PoolSizeUpLimit << poolSizeUpLimit);
        allocSizeLimit = poolSizeUpLimit;
    }
    return mbuf::AllocCfg(poolSizeUpLimit, allocSizeLimit, bufCountUpLimit, poolSizeUsageScaleDownThld,
                          lowUsageDurationForScaleDown, allocSizeSteadyTol, allocSteadyDurationForScaleDown,
                          scaleDownTargetUsage, queryStatusInterval);
}

std::unique_ptr<mbuf::BufferPoolOwnerManager> ShmCreator::CreateBufferPool(const std::string& topic_name,
                                                                           uint16_t domain_id, uint16_t participant_id,
                                                                           uint32_t entity_id, const DataWriterQos& qos,
                                                                           bool authentication) {
    uint32_t buf_max_num = 0U;
    uint32_t buf_pool_size = 0U;
    const size_t alignAddress = PropertyParse::GetAlignAddress(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::AlignAddress));
    const size_t maxBufferSize = std::numeric_limits<size_t>::max();  // NOLINT
    const mbuf::AllocateMode allocateMode = static_cast<mbuf::AllocateMode>(PropertyParse::GetAllocateMode(
        PropertyPolicyHelper::find_property(qos.properties(), MbufPropertyField::AllocateMode)));
    const bool ret = CheckCreateBufferPoolParam(domain_id, topic_name, qos);
    if (!ret) {
        return nullptr;
    }

    std::string mbuf_name;
    static_cast<void>(mbuf_name
                          .append("membuf_")                       // 7
                          .append(std::to_string(domain_id))       // uint16 max:5
                          .append("_")                             // 1
                          .append(std::to_string(participant_id))  // uint32 max:10
                          .append("_")                             // 1
                          .append(std::to_string(entity_id))       // uint32 max:10
                          .append("_")                             // 1
                          .append(std::to_string(getpid()))        // int max:10
                          .append("_")                             // 1
                          .append(std::to_string(
                              std::chrono::high_resolution_clock::now().time_since_epoch().count()))  // uint64_t max:20
                          .append("_")                                                                // 1
                          .append(topic_name));  // prefix:67  topic max : 256-67=189 > 128

    const ReliabilityQosPolicyKind reliability_qos_policy_kind = qos.reliability().kind;
    const bool check_pool_size = CheckPoolSize(qos, buf_max_num, buf_pool_size, reliability_qos_policy_kind);
    if (!check_pool_size) {
        return nullptr;
    }
    mbuf::RecoverMode recoverMode;
    if (reliability_qos_policy_kind == RELIABLE_RELIABILITY_QOS) {
        recoverMode = mbuf::RecoverMode::Normal;
    } else {
        recoverMode = mbuf::RecoverMode::Force;
    }

    mbuf::AllocCfg allocCfg = GetAllocCfg(qos);
    auto poolManager =
        mbuf::BufferPoolOwnerManager::Create(mbuf_name, allocCfg, buf_pool_size, buf_max_num, maxBufferSize, getpid(),
                                             alignAddress, recoverMode, allocateMode, true, authentication);
    if (poolManager == nullptr) {
        DSF_LOG_WARNING(ShmCreator, "writer ids:" << domain_id << "," << participant_id << "," << entity_id
                                                  << ",create mbuf pool with name:" << mbuf_name << " failed!");
        return nullptr;
    }
    DSF_LOG_INFO(ShmCreator, "writer ids:" << domain_id << "," << participant_id << "," << entity_id
                                           << ",create mbuf pool success,name:" << mbuf_name
                                           << ",pool size:" << buf_pool_size << ",buf count:" << buf_max_num
                                           << ",align address:" << alignAddress << ",recover mode:" << recoverMode
                                           << ",allocateMode:" << allocateMode);
    return poolManager;
}

ShmWriter* ShmCreator::CreateShmWriter(GUID_s guid, const std::string& topic_name, const DataWriterQos& writer_qos,
                                       bool is_static_discovery) {
    // RETURN_VAL_IF_NULL(topic, nullptr);
    uint8_t domain_id = guid.get_domain_id_from_guid();
    uint16_t participant_id = guid.get_participant_id_from_guid();
    auto entity_id = guid.get_entity_id_from_guid();
    auto* shmWriter = new ShmWriter(topic_name, guid, is_static_discovery, writer_qos.reliability().kind);  // NOLINT
    auto pool = CreateBufferPool(topic_name, domain_id, participant_id, entity_id, writer_qos);

    if (pool) {
        shmWriter->pool_owner_manager_ = std::shared_ptr<mbuf::BufferPoolOwnerManager>(std::move(pool));
    } else {
        // fix memory leak when pool nullptr
        delete shmWriter;
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CREATE_MEMBUF_POOL_ERR,
            "writer ids:" << +domain_id << "," << participant_id << "," << entity_id << ",membuf pool create failed");
        return nullptr;
    }

    return shmWriter;
}

ShmWriter* ShmCreator::CreateShmWriter(uint32_t domainID, vbsutil::xmlparser::GUID_t guid,
                                       const std::string& topic_name, std::string& topic_type,
                                       const DataWriterQos& writer_qos, bool is_static_discovery) {
    // RETURN_VAL_IF_NULL(topic, nullptr);
    uint16_t participant_id = guid.get_participant_id();
    auto entity_id = guid.entityId.to_uint32();
    auto* shmWriter = new ShmWriter(domainID, topic_name, topic_type, guid, is_static_discovery,
                                    writer_qos.reliability().kind);  // NOLINT
    auto pool = CreateBufferPool(topic_name, domainID, participant_id, entity_id, writer_qos, true);
    if (pool) {
        shmWriter->pool_owner_manager_ = std::shared_ptr<mbuf::BufferPoolOwnerManager>(std::move(pool));
    } else {
        // fix memory leak when pool nullptr
        delete shmWriter;
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CREATE_MEMBUF_POOL_ERR,
            "writer ids:" << domainID << "," << participant_id << "," << entity_id << ",membuf pool create failed");
        return nullptr;
    }

    return shmWriter;
}

bool ShmCreator::RemoveShmWriter(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint, ShmWriter* writer,
                                 const DataWriterQos& qos) {
    bool ret = false;
    if ((discovery_endpoint == nullptr) || (writer == nullptr)) {
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ILLEGAL_OPERATION,
            "RemoveShmWriter destruct check nullptr,discovery_endpoint:" << discovery_endpoint << ",writer:" << writer);
        return ret;
    }
    DSF_LOG_DEBUG(ShmCreator, "writer ids:" << writer->get_domain_id() << "," << writer->get_participant_id() << ","
                                            << writer->get_entity_id() << ",destructor");
    if (discovery_endpoint->RemoveLocalWriterInfo(writer, qos)) {
        ret = true;
        static_cast<void>(discovery_endpoint->RemoveRemoteReaderInfo(writer));
    } else {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_REMOVE_LOCAL_ENTITY_ERR,
                      "writer ids:" << writer->get_domain_id() << "," << writer->get_participant_id() << ","
                                    << writer->get_entity_id()
                                    << ",discovery_endpoint->RemoveLocalWriterInfo failed!!!");
    }

    delete writer;
    writer = nullptr;

    return ret;
}

bool ShmCreator::CheckCreateShmReaderParam(uint16_t domain_id, uint16_t parId, const std::string& topic_name,
                                           ReaderListener* const readerListener, int16_t entity_id,
                                           const DataReaderQos& qos) {
    if (!readerListener) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                      "reader ids:" << domain_id << "," << parId << "," << entity_id << ",readerListener is empty");
        return false;
    }
    if (topic_name.empty()) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_TOPIC_NAME_EMPTY,
                      "reader ids:" << domain_id << "," << parId << "," << entity_id << ",topic_name is empty");
        return false;
    }
    ReliabilityQosPolicyKind reliability_qos_policy_kind = qos.reliability().kind;
    if ((reliability_qos_policy_kind != RELIABLE_RELIABILITY_QOS) &&
        (reliability_qos_policy_kind != BEST_EFFORT_RELIABILITY_QOS)) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
                      "reader ids:" << domain_id << "," << parId << "," << entity_id
                                    << ",create entity with unknown reliable kind : "
                                    << +static_cast<int32_t>(reliability_qos_policy_kind) << "!!!");
        return false;
    }
    return true;
}

std::shared_ptr<shmmq::MessageQueue> ShmCreator::CreateQueue(uint8_t domain_id, uint16_t participant_id,
                                                             uint32_t entity_id, const std::string& topic_name,
                                                             ReaderListener* const readerListener,
                                                             const DataReaderQos& reader_qos, bool is_static_discovery,
                                                             bool authentication) {
    std::string queue_name;
    bool ret = CheckCreateShmReaderParam(domain_id, participant_id, topic_name, readerListener, entity_id, reader_qos);
    if (!ret) {
        return nullptr;
    }
    static_cast<void>(
        queue_name.append(std::to_string(domain_id)).append("_").append(std::to_string(entity_id)).append("_"));
    if (!is_static_discovery) {
        static_cast<void>(queue_name.append(std::to_string(::getpid())).append("_"));
    }
    static_cast<void>(queue_name.append(topic_name));
    uint32_t queue_size = PropertyParse::GetQueueSize(
        PropertyPolicyHelper::find_property(reader_qos.properties(), QueuePropertyField::QueueSize));
    auto filter_expression =
        PropertyPolicyHelper::find_property(reader_qos.properties(), ReaderPropertyFiled::FilterExpression);
    auto filter_params =
        PropertyPolicyHelper::find_property(reader_qos.properties(), ReaderPropertyFiled::FilterParameters);

#ifdef ENABLE_DES
    auto queue_wake_up = PropertyParse::GetQueueWakeUp(
        PropertyPolicyHelper::find_property(reader_qos.properties(), QueuePropertyField::WakeUpMode));

    shmmq::QueueAttr attr {static_cast<uint8_t>(shmmq::queue_role::READER),
                           0U,
                           queue_size,
                           0U,
                           0U,
                           static_cast<uint8_t>(shm::MemoryMode::KUNIMEM),  // 0:um-heap, 1: posix shm
                           shmmq::block_type::MBUF_TYPE,
                           0U,
                           static_cast<uint8_t>(queue_wake_up)};
#else
    shmmq::QueueAttr attr {
        static_cast<uint8_t>(shmmq::queue_role::READER), 0U, queue_size, 0U, 0U,
        static_cast<uint8_t>(shm::MemoryMode::KUNIMEM)  // 0:um-heap, 1: posix shm
    };
#endif
    ReliabilityQosPolicyKind kind = reader_qos.reliability().kind;
    if (kind == BEST_EFFORT_RELIABILITY_QOS) {
        attr.SetMode(shmmq::queue_mode::MODE_OVERWRITE);
        DSF_LOG_DEBUG(ShmCreator, "reader ids:" << +domain_id << "," << participant_id << "," << entity_id
                                                << ",create queue with kind : " << +static_cast<int32_t>(kind)
                                                << ",MODE_OVERWRITE");
    } else {
        attr.SetMode(shmmq::queue_mode::MODE_WRITE);
        DSF_LOG_DEBUG(ShmCreator, "reader ids:" << +domain_id << "," << participant_id << "," << entity_id
                                                << ",create queue with kind : " << +static_cast<int32_t>(kind)
                                                << ",MODE_WRITE");
    }
    if (authentication) {
        attr.SetToAuthenticate(1U);
        DSF_LOG_DEBUG(ShmCreator, "reader ids:" << +domain_id << "," << participant_id << "," << entity_id
                                                << " open queue authentication.");
    } else {
        attr.SetToAuthenticate(0U);
        DSF_LOG_DEBUG(ShmCreator, "reader ids:" << +domain_id << "," << participant_id << "," << entity_id
                                                << " NOT open queue authentication.");
    }

    // 提前返回不符合预期的情况
    if ((filter_expression == nullptr && filter_params != nullptr) ||
        (filter_expression != nullptr && filter_params == nullptr)) {
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
            "filter_expression:" << filter_expression << ",or filter_params:" << filter_params << " is nullptr");
        return nullptr;
    }

    if (filter_expression && (filter_expression->size() >= FilterExpressionMaxLen)) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
                      "filter_expression size:" << filter_expression->size()
                                                << " >= FilterExpressionMaxLen:" << FilterExpressionMaxLen);
        return nullptr;
    }

    if (filter_params && (filter_params->size() >= (FilterReservedSize - FilterExpressionMaxLen))) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
                      "filter_params size:" << filter_params->size() << " >= FilterReservedSize:" << FilterReservedSize
                                            << " - FilterExpressionMaxLen:" << FilterExpressionMaxLen);
        return nullptr;
    }

    // 过滤需求
    auto is_filtered = filter_expression && filter_params;
    if (is_filtered) {
        // the unit of reserved size is KB
        attr.SetReservedSize(FilterReservedSize / 1024U);
    }

    DSF_LOG_DEBUG(
        ShmCreator,
        "reader ids:" << +domain_id << "," << participant_id << "," << entity_id << ",create queue with attr : role = "
                      << ((attr.role == shmmq::queue_role::READER) ? "reader" : "writer") << " queue_mode = "
                      << ((attr.queue_mode == shmmq::queue_mode::MODE_OVERWRITE) ? "MODE_OVERWRITE" : "MODE_WRITE")
                      << " queue_size = " << attr.queue_size << ",reserved size:" << attr.reserved_size);
    auto msg_queue = shmmq::ShmmqManager::GetMQ(queue_name, attr);
    if (msg_queue == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_OPEN_QUEUE_ERR,
                      "reader ids:" << +domain_id << "," << participant_id << "," << entity_id
                                    << ",ShmmqManager::GetMQ with name " << queue_name << " return nullptr")
        return nullptr;
    }
    // 写入过滤数据
    auto ptr = msg_queue->get_reserved_ptr();
    if (is_filtered && ptr) {
        constexpr uint32_t SizeWith = sizeof(uint32_t);
        uint32_t exp_size = filter_expression->size();
        std::memcpy(ptr, &exp_size, SizeWith);
        std::memcpy((char*)ptr + SizeWith, filter_expression->c_str(), exp_size);

        uint32_t param_size = filter_params->size();
        std::memcpy((char*)ptr + SizeWith + exp_size, &param_size, SizeWith);
        std::memcpy((char*)ptr + SizeWith + exp_size + SizeWith, filter_params->c_str(), param_size);
    }
    return msg_queue;
}

ShmReader* ShmCreator::CreateShmReader(GUID_s guid, const std::string& topic_name, ReaderListener* const readerListener,
                                       const DataReaderQos& reader_qos, bool is_static_discovery) {
    // get queueName
    uint8_t domain_id = guid.get_domain_id_from_guid();
    uint16_t participant_id = guid.get_participant_id_from_guid();
    uint32_t entity_id = guid.get_entity_id_from_guid();
    auto msg_queue =
        CreateQueue(domain_id, participant_id, entity_id, topic_name, readerListener, reader_qos, is_static_discovery);
    if (msg_queue == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_OPEN_QUEUE_ERR,
                      "reader ids:" << +domain_id << "," << participant_id << "," << entity_id << ",CreateQueue "
                                    << " return nullptr")
        return nullptr;
    }
    auto shm_reader = new ShmReader(topic_name, std::move(msg_queue), guid, readerListener, is_static_discovery, false,
                                    reader_qos.reliability().kind);
    return shm_reader;
}

ShmReader* ShmCreator::CreateShmReader(uint32_t domainID, vbsutil::xmlparser::GUID_t guid,
                                       const std::string& topic_name, std::string& topicType,
                                       ReaderListener* const readerListener, const DataReaderQos& reader_qos,
                                       bool is_static_discovery) {
    // get queueName
    uint16_t participant_id = guid.get_participant_id();
    uint32_t entity_id = guid.entityId.to_uint32();
    auto msg_queue = CreateQueue(domainID, participant_id, entity_id, topic_name, readerListener, reader_qos,
                                 is_static_discovery, true);
    if (msg_queue == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_OPEN_QUEUE_ERR,
                      "reader ids:" << domainID << "," << participant_id << "," << entity_id << ",CreateQueue "
                                    << " return nullptr")
        return nullptr;
    }
    auto shm_reader = new ShmReader(domainID, topic_name, topicType, std::move(msg_queue), guid, readerListener,
                                    is_static_discovery, false, reader_qos.reliability().kind);
    return shm_reader;
}

ShmReader* ShmCreator::CreateSharedQueueShmReader(GUID_s guid, int16_t entity_id, ReaderListener* const readerListener,
                                                  const std::string& topic_name, const DataReaderQos& reader_qos,
                                                  bool is_static_discovery, std::shared_ptr<shmmq::MessageQueue> mq) {
    uint8_t domain_id = guid.get_domain_id_from_guid();
    uint16_t participant_id = guid.get_participant_id_from_guid();
    bool ret = CheckCreateShmReaderParam(domain_id, participant_id, topic_name, readerListener, entity_id, reader_qos);
    if (!ret) {
        return nullptr;
    }
    DSF_LOG_INFO(ShmCreator,
                 "shared queue reader create ids:" << +domain_id << "," << participant_id << "," << entity_id);
    auto shm_reader = new ShmReader(topic_name, std::move(mq), guid, readerListener, is_static_discovery, true,
                                    reader_qos.reliability().kind);
    return shm_reader;
}

ShmReader* ShmCreator::CreateSharedQueueShmReader(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, int16_t entity_id,
                                                  ReaderListener* const readerListener, const std::string& topic_name,
                                                  std::string& topic_type, const DataReaderQos& reader_qos,
                                                  bool is_static_discovery, std::shared_ptr<shmmq::MessageQueue> mq) {
    uint16_t participant_id = guid.get_participant_id();
    bool ret = CheckCreateShmReaderParam(domainID, participant_id, topic_name, readerListener, entity_id, reader_qos);
    if (!ret) {
        return nullptr;
    }
    DSF_LOG_INFO(ShmCreator,
                 "shared queue reader create ids:" << domainID << "," << participant_id << "," << entity_id);
    auto shm_reader = new ShmReader(domainID, topic_name, topic_type, std::move(mq), guid, readerListener,
                                    is_static_discovery, true, reader_qos.reliability().kind);
    return shm_reader;
}

bool ShmCreator::RemoveShmReader(std::shared_ptr<DiscoveryEndpoint> discovery_endpoint, ShmReader* reader,
                                 const DataReaderQos& qos) {
    bool ret = false;
    if ((discovery_endpoint == nullptr) || (reader == nullptr)) {
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ILLEGAL_OPERATION,
            "RemoveShmReader destruct check nullptr,discovery_endpoint:" << discovery_endpoint << ",reader:" << reader);
        return ret;
    }

    DSF_LOG_DEBUG(ShmCreator, "reader ids:" << reader->get_domain_id() << "," << reader->get_participant_id() << ","
                                            << reader->get_entity_id() << ",destructor");
    if (discovery_endpoint->RemoveLocalReaderInfo(reader, qos)) {
        ret = true;
        static_cast<void>(discovery_endpoint->RemoveRemoteWriterInfo(reader));
    } else {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_REMOVE_LOCAL_ENTITY_ERR,
                      "reader ids:" << reader->get_domain_id() << "," << reader->get_participant_id() << ","
                                    << reader->get_entity_id()
                                    << ",discovery_endpoint->RemoveLocalReaderInfo failed!!!");
    }

    delete reader;
    reader = nullptr;

    return ret;
}
}  // namespace dsfdds
}  // namespace li
