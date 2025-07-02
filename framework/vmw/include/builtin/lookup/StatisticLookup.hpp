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

#ifndef INCLUDE_EDDS_DDS_BUILTIN_STATISTIC_LOOKUP_HPP
#define INCLUDE_EDDS_DDS_BUILTIN_STATISTIC_LOOKUP_HPP

#include <mutex>

#include "ertps/types/TypesBase.h"
#include "edds/rtps/common/Guid.h"
#include "xmlparser/qos/XMLWriterQos.hpp"
#include "xmlparser/qos/XMLReaderQos.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/DomainParticipantListenerInner.hpp"

namespace vbs {
namespace builtin {

using StatisticDomainInfos_map = std::map<DomainId_t, std::vector<evbs::ertps::rtps::ParticipantDiscoveryInfo>>;
using DomainId_t = vbs::common::types::DomainId_t;

class StatisticLookup {
 private:
    void update_participant_info(DomainId_t domain, const evbs::ertps::rtps::ParticipantDiscoveryInfo& info) {
        auto& infos_ = statistic_domain_info_map_[domain];
        auto guid = info.info.m_guid;
        const auto& it = std::find_if(
            infos_.begin(), infos_.end(),
            [guid](evbs::ertps::rtps::ParticipantDiscoveryInfo& element) { return element.info.m_guid == guid; });
        if (it == infos_.end()) {
            infos_.push_back(info);
        } else {
            it->status = info.status;
        }
    }
    bool domainExist(DomainId_t domain);
    bool initDomain(DomainId_t domain);

 public:
    class StatisticLookupListener : public vbs::DomainParticipantListenerInner {
     public:
        StatisticLookupListener(StatisticLookup* parent) : statistic_loop_up_(parent) {}

        void on_participant_discovery(vbs::DomainParticipantImpl* participant,
                                      evbs::ertps::rtps::ParticipantDiscoveryInfo&& info) override {
            statistic_loop_up_->update_participant_info(participant->get_domain_id(), info);
        }

        StatisticLookup* statistic_loop_up_;
    } listener_;

    StatisticLookup();

    ~StatisticLookup();

    static StatisticLookup* get_instance();

    std::recursive_mutex& get_mutex();

    /**
     * 查询指定domain下所有的participant信息
     */
    std::vector<evbs::ertps::rtps::ParticipantDiscoveryInfo> lookupParticipantsByDomain(DomainId_t domain,
                                                                                        uint32_t timeout = 100);

    /**
     * 查询指定participant下所有的writer、reader的摘要信息
     */
    evbs::edds::dds::builtin::StatisticWrtierReaderInfos lookupParticipantDetailInfo(
        DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout = 1000);

    /**
     * 查询和指定writer/reader matched的guid信息
     */
    evbs::edds::dds::builtin::StatisticMatchGuids lookupMatchGuidsInfo(DomainId_t domain,
                                                                       const evbs::ertps::rtps::GUID_t& guid,
                                                                       uint32_t timeout = 1000);

    /**
     * 查询指定writer的发包信息
     */
    vbsutil::elog::ReturnCode_t lookupWriterSendInfo(DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid,
                                                     evbs::edds::dds::builtin::StatisticSendInfo* info,
                                                     uint32_t timeout = 1000);

    /**
     * 查询指定reader的收包信息
     */
    vbsutil::elog::ReturnCode_t lookupReaderRecvInfo(DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid,
                                                     evbs::edds::dds::builtin::StatisticRecvInfo* info,
                                                     uint32_t timeout = 1000);

    /**
     * 查询指定writer/reader的qos信息
     */
    vbsutil::elog::ReturnCode_t lookupWriterReaderQos(DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid,
                                                      evbs::edds::dds::builtin::StatisticWriterReaderQos* qos,
                                                      uint32_t timeout = 1000);

    /**
     * 查询指定进程下的历史消息摘要，其中domain id来指定查询的通信通道
     */
    vbsutil::elog::Log::GuidMessageBriefsMap lookupMessageBrief(DomainId_t domain, const std::string& host_name,
                                                                uint32_t pid, const std::string& topic_name,
                                                                uint32_t timeout = 1000);

    /**
     * 查询与指定writer/reader匹配的reader/writer的proxy信息
     */
    evbs::edds::dds::builtin::StatisticProxyInfos lookupWriterReaderProxyInfos(DomainId_t domain,
                                                                               const evbs::ertps::rtps::GUID_t& guid,
                                                                               uint32_t timeout = 1000);

    /**
     * 远程配置命令，当前支持以下功能：
     * 1、远程配置日志级别，params内容格式： "logLevel:0"
     * 2、远程配置日志周期，params内容格式： "logPeriod:0"
     * 3、持续拓展中
     */
    vbsutil::elog::ReturnCode_t remoteConfig(DomainId_t domain, const std::string& host_name, uint32_t pid,
                                             const std::string& params, uint32_t timeout = 1000);

    /**
     * 查询通信通道的相关资源释放
     */
    void release();
    void set_qos(const vbs::DomainParticipantQosInner& pqos);
    const vbs::DomainParticipantQosInner& get_qos() { return pqos_; };

 private:
    std::recursive_mutex mutex_;
    std::vector<DomainId_t> registered_domains_;
    StatisticDomainInfos_map statistic_domain_info_map_;
    std::map<DomainId_t, vbs::DomainParticipantImpl*> domain_participants_map_;
    vbs::DomainParticipantQosInner pqos_;
};

} /* namespace builtin */
} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_BUILTIN_STATISTIC_LOOKUP_HPP
