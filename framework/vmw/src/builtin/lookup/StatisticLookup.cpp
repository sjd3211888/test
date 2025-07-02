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

#include <sys/types.h>
#include <unistd.h>

#include "builtin/lookup/StatisticLookup.hpp"
#include "domain/DomainParticipantFactoryInner.hpp"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "edds/dds/builtin/lookup/LookupManager.hpp"
#include "deps/common/SystemInfo.hpp"

namespace vbs {
namespace builtin {

StatisticLookup::StatisticLookup() : listener_(this) {
    pqos_.name("VbsStatisticCli");
    pqos_.wire_protocol().builtin.lookup_config.use_server = false;
    pqos_.wire_protocol().builtin.lookup_config.use_client = true;
    pqos_.transport().use_builtin_transports = false;
}

StatisticLookup::~StatisticLookup() {
    release();
}

StatisticLookup* StatisticLookup::get_instance() {
    static StatisticLookup instance;
    return &instance;
}

std::recursive_mutex& StatisticLookup::get_mutex() {
    return get_instance()->mutex_;
}

void StatisticLookup::set_qos(const vbs::DomainParticipantQosInner& pqos) {
    pqos_ = pqos;
}

bool StatisticLookup::domainExist(DomainId_t domain) {
    return std::find(registered_domains_.begin(), registered_domains_.end(), domain) != registered_domains_.end();
}

bool StatisticLookup::initDomain(DomainId_t domain) {
    if (domainExist(domain)) {
        return true;
    }

    vbs::DomainParticipantImpl* partcipant =
        vbs::DomainParticipantFactoryInner::get_instance()->create_participant(domain, pqos_, &listener_);
    if (partcipant == nullptr) {
        return false;
    }
    domain_participants_map_[domain] = partcipant;
    registered_domains_.push_back(domain);
    // wait for pdp
    std::this_thread::sleep_for(std::chrono::milliseconds(VBS_TOOL_WAIT_TIMEOUT));
    return true;
}

std::vector<ParticipantDiscoveryInfo> StatisticLookup::lookupParticipantsByDomain(DomainId_t domain, uint32_t timeout) {
    std::unique_lock<std::recursive_mutex> scoped(get_mutex());
    if (!domainExist(domain)) {
        (void)initDomain(domain);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    }

    if (statistic_domain_info_map_.find(domain) == statistic_domain_info_map_.end()) {
        return std::vector<ParticipantDiscoveryInfo>();
    }
    return statistic_domain_info_map_[domain];
}

evbs::edds::dds::builtin::StatisticWrtierReaderInfos StatisticLookup::lookupParticipantDetailInfo(
    DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout) {
    std::unique_lock<std::recursive_mutex> scoped(get_mutex());

    if (initDomain(domain) == false) {
        return evbs::edds::dds::builtin::StatisticWrtierReaderInfos();
    }

    // find from local participant first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid() != guid) {
            continue;
        }

        return partcipant_it->get_writer_reader_infos();
    }

    // find from remote participant
    return domain_participants_map_[domain]->lookup_writer_reader_infos(guid, timeout);
}

evbs::edds::dds::builtin::StatisticMatchGuids StatisticLookup::lookupMatchGuidsInfo(
    DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout) {
    std::unique_lock<std::recursive_mutex> scoped(get_mutex());

    if (initDomain(domain) == false) {
        return evbs::edds::dds::builtin::StatisticMatchGuids();
    }
    // find from local participant first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid().guidPrefix != guid.guidPrefix) {
            continue;
        }

        return partcipant_it->get_remote_guids(guid);
    }

    // find from remote participant
    return domain_participants_map_[domain]->lookup_remote_guids(guid, timeout);
}

vbsutil::elog::ReturnCode_t StatisticLookup::lookupWriterSendInfo(DomainId_t domain,
                                                                  const evbs::ertps::rtps::GUID_t& guid,
                                                                  evbs::edds::dds::builtin::StatisticSendInfo* info,
                                                                  uint32_t timeout) {
    if (info == nullptr) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "param 'info' is null.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (!guid.entityId.is_writer()) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, guid << " is not writer guid.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (guid.is_builtin()) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "Builtin " << guid << " not supported.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    std::unique_lock<std::recursive_mutex> scoped(get_mutex());
    if (initDomain(domain) == false) {
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    // find from local writer first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid().guidPrefix != guid.guidPrefix) {
            continue;
        }

        return partcipant_it->get_statistic_send_info(guid, info);
    }

    // find from remote writer
    return domain_participants_map_[domain]->lookup_statistic_send_info(guid, info, timeout);
}

vbsutil::elog::ReturnCode_t StatisticLookup::lookupReaderRecvInfo(DomainId_t domain,
                                                                  const evbs::ertps::rtps::GUID_t& guid,
                                                                  evbs::edds::dds::builtin::StatisticRecvInfo* info,
                                                                  uint32_t timeout) {
    if (info == nullptr) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "param 'info' is null.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (!guid.entityId.is_reader()) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, guid << " is not reader guid.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (guid.is_builtin()) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "Builtin " << guid << " not supported.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    std::unique_lock<std::recursive_mutex> scoped(get_mutex());
    if (initDomain(domain) == false) {
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    // find from local reader first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid().guidPrefix != guid.guidPrefix) {
            continue;
        }

        return partcipant_it->get_statistic_recv_info(guid, info);
    }

    // find from remote reader
    return domain_participants_map_[domain]->lookup_statistic_recv_info(guid, info, timeout);
}

vbsutil::elog::ReturnCode_t StatisticLookup::lookupWriterReaderQos(
    DomainId_t domain, const evbs::ertps::rtps::GUID_t& guid, evbs::edds::dds::builtin::StatisticWriterReaderQos* qos,
    uint32_t timeout) {
    if (qos == nullptr) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "param 'qos' is null.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if ((!guid.entityId.is_writer()) && (!guid.entityId.is_reader())) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, guid << " invalid.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (guid.is_builtin()) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, "Builtin " << guid << " not supported.");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (initDomain(domain) == false) {
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    // find from local reader first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid().guidPrefix != guid.guidPrefix) {
            continue;
        }

        return partcipant_it->get_statistic_writer_reader_qos(guid, qos);
    }

    // find from remote reader
    return domain_participants_map_[domain]->lookup_statistic_writer_reader_qos(guid, qos, timeout);
}

vbsutil::elog::Log::GuidMessageBriefsMap StatisticLookup::lookupMessageBrief(DomainId_t domain,
                                                                             const std::string& host_name, uint32_t pid,
                                                                             const std::string& topic_name,
                                                                             uint32_t timeout) {
    if (initDomain(domain) == false) {
        return vbsutil::elog::Log::GuidMessageBriefsMap();
    }

    if (static_cast<pid_t>(pid) == getpid()) {
        return vbsutil::elog::Log::GetMessageBrief(topic_name);
    }

    // find from remote process
    return domain_participants_map_[domain]->lookupMessageBrief(host_name, pid, topic_name, timeout);
}

evbs::edds::dds::builtin::StatisticProxyInfos StatisticLookup::lookupWriterReaderProxyInfos(DomainId_t domain,
                                                                                            const GUID_t& guid,
                                                                                            uint32_t timeout) {
    if ((!guid.entityId.is_writer()) && (!guid.entityId.is_reader())) {
        elogError(STATISTIC_LOOKUP, RetCode_t::RETCODE_ERROR, guid << " invalid.");
        return evbs::edds::dds::builtin::StatisticProxyInfos();
    }

    if (initDomain(domain) == false) {
        return evbs::edds::dds::builtin::StatisticProxyInfos();
    }

    // find from local reader first.
    const auto& partcipants = vbs::DomainParticipantFactoryInner::get_instance()->lookup_participants(domain);
    for (auto& partcipant_it : partcipants) {
        if (partcipant_it->guid().guidPrefix != guid.guidPrefix) {
            continue;
        }

        return partcipant_it->get_statistic_writer_reader_proxy_infos(guid);
    }

    // find from remote reader
    return domain_participants_map_[domain]->lookup_statistic_writer_reader_proxy_infos(guid, timeout);
}

vbsutil::elog::ReturnCode_t StatisticLookup::remoteConfig(DomainId_t domain, const std::string& host_name, uint32_t pid,
                                                          const std::string& params, uint32_t timeout) {
    if (initDomain(domain) == false) {
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    std::string local_hostname = vbs::SystemInfo::get_hostname();
    if (host_name == local_hostname && static_cast<pid_t>(pid) == getpid()) {
        return evbs::edds::dds::builtin::LookupManager::localConfig(params);
    } else {
        return domain_participants_map_[domain]->remoteConfig(host_name, pid, params, timeout);
    }
}

void StatisticLookup::release() {
    std::unique_lock<std::recursive_mutex> scoped(get_instance()->get_mutex());
    if (!registered_domains_.empty()) {
        for (auto& it : registered_domains_) {
            vbs::DomainParticipantFactoryInner::get_instance()->delete_participant_and_entities(it);
        }
        registered_domains_.clear();
    }
}

}  // namespace builtin
}  // namespace vbs
