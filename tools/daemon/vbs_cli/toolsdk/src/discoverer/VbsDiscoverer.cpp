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

#include <chrono>

#include <domain/DomainParticipantFactoryInner.hpp>
#include <ertps/types/DynamicDataFactory.h>

#include "discoverer/VbsDiscoverer.hpp"
#include "discoverer/qos_serializer/QosSerializer.hpp"

#include <ertps/types/DynamicDataHelper.hpp>
#include <deps/common/IPLocator.h>
#include <deps/core/status/BaseStatus.hpp>

namespace vbstoolsdk {

VbsDiscoverer::VbsDiscoverer(uint32_t domain_id, const std::string& xml_file_name,
                             const std::string& participant_profile, bool type_discovery_flag)
    : BaseDiscoverer(domain_id, type_discovery_flag), participant_listener_(nullptr) {
    create_participant_(domain_id, xml_file_name, participant_profile);
}

VbsDiscoverer::~VbsDiscoverer() {
    if (participant_ != nullptr) {
        if (subscriber_ != nullptr) {
            for (auto& reader : readers_dyndatas_) {
                if (reader.first != nullptr) {
                    subscriber_->delete_datareader(reader.first);
                }
            }
            participant_->delete_subscriber(subscriber_);
        }

        for (auto& topic : topics_) {
            participant_->delete_topic(topic);
        }
        vbs::DomainParticipantFactoryInner::get_instance()->delete_participant(participant_);
    }

    if (participant_listener_ != nullptr) {
        delete participant_listener_;
    }
}

bool VbsDiscoverer::create_participant_(uint32_t domain_id, const std::string& xml_file_name,
                                        const std::string& participant_profile) {
    participant_listener_ = new ParticipantListener(this);
    // reader_listener_ = new ReaderListener(this);

    if (xml_file_name == "") {
        vbs::DomainParticipantQosInner pqos;
        pqos.name("vbsdiscoverer");
        pqos.wire_protocol().builtin.lookup_config.use_client = true;
        pqos.wire_protocol().builtin.avoid_builtin_multicast = false;
        try {
            participant_ = vbs::DomainParticipantFactoryInner::get_instance()->create_participant(
                domain_id, pqos, participant_listener_);
        } catch (...) {
            logError_("VBSTOOLSDK", "VbsDiscoverer participant_ create exception");
            return false;
        }

    } else {
        vbs::DomainParticipantFactoryInner::get_instance()->load_XML_profiles_file(xml_file_name);
        try {
            participant_ = vbs::DomainParticipantFactoryInner::get_instance()->create_participant_with_profile(
                domain_id, participant_profile, participant_listener_);
        } catch (...) {
            logError_("VBSTOOLSDK", "VbsDiscoverer participant_ create exception");
            return false;
        }
    }
    if (participant_ == nullptr) {
        logError_("VBSTOOLSDK", "VbsDiscoverer participant_ is nullptr");
        return false;
    }

    subscriber_ = participant_->create_subscriber(vbsutil::xmlparser::getDefaultSubscriberQos(), nullptr);
    if (subscriber_ == nullptr) {
        logError_("VBSTOOLSDK", "VbsDiscoverer subscriber_ is nullptr");
        return false;
    }
    return true;
}

void VbsDiscoverer::ParticipantListener::on_participant_discovery(vbs::DomainParticipantImpl* participant,
                                                                  ERTPS_RTPS::ParticipantDiscoveryInfo&& info) {

    (void)participant;
    ParticipantInfo participant_info;
    participant_info.domain_id = parent_->domain_id_;
    participant_info.guid = info.info.m_guid;
    participant_info.qos = participant_proxy_data_to_backend_qos(info.info);
    participant_info.address = parent_->get_address_(info.info);
    participant_info.participant_name = info.info.m_participantName.to_string();

    for (auto it = info.info.m_properties.begin(); it != info.info.m_properties.end(); ++it) {
        if (it->first().rfind(HOST_ENTITY_TAG) != std::string::npos)
            participant_info.host = it->second();
        if (it->first().rfind(USER_ENTITY_TAG) != std::string::npos)
            participant_info.user = it->second();
        if (it->first().rfind(PROCESS_ENTITY_TAG) != std::string::npos)
            participant_info.process = it->second();
    }

    std::vector<unsigned char> temp = parent_->guid_to_vector_(info.info.m_guid);
    if (info.status == ERTPS_RTPS::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT ||
        info.status == ERTPS_RTPS::ParticipantDiscoveryInfo::CHANGED_QOS_PARTICIPANT) {
        parent_->participants_map_[temp] = std::move(participant_info);
    } else {
        parent_->participants_map_.erase(temp);
    }
}

void VbsDiscoverer::ParticipantListener::on_subscriber_discovery(vbs::DomainParticipantImpl* participant,
                                                                 ERTPS_RTPS::ReaderDiscoveryInfo&& info) {

    (void)participant;
    RWInfo reader_info(RWKind::DATAREADER);
    reader_info.domain_id = parent_->domain_id_;
    reader_info.guid = info.info.guid();
    reader_info.qos = reader_proxy_data_to_backend_qos(info.info);
    reader_info.topic_name = info.info.topicName().to_string();
    reader_info.type_name = info.info.typeName().to_string();
    reader_info.locators = info.info.remote_locators();

    std::vector<unsigned char> guid_temp = parent_->guid_to_vector_(info.info.guid());
    std::vector<unsigned char> prefix_temp = parent_->prefix_guid_to_vector_(info.info.guid());
    std::vector<unsigned char> entity_temp = parent_->entity_guid_to_vector_(info.info.guid());
    std::string topic_type = info.info.topicName().to_string() + "&&" + info.info.typeName().to_string();
    if (info.status == EDDS_DDS::READER_DISCOVERY_STATUS::DISCOVERED_READER ||
        info.status == EDDS_DDS::READER_DISCOVERY_STATUS::CHANGED_QOS_READER) {
        parent_->readers_map_[prefix_temp].emplace(entity_temp, reader_info);
        parent_->topics_readers_map_[topic_type].emplace(guid_temp, info.info.guid());
    } else {
        parent_->readers_map_[prefix_temp].erase(entity_temp);
        parent_->topics_readers_map_[topic_type].erase(guid_temp);
    }
}

void VbsDiscoverer::ParticipantListener::on_publisher_discovery(vbs::DomainParticipantImpl* participant,
                                                                ERTPS_RTPS::WriterDiscoveryInfo&& info) {

    (void)participant;
    RWInfo writer_info(RWKind::DATAWRITER);
    writer_info.domain_id = parent_->domain_id_;
    writer_info.guid = info.info.guid();
    writer_info.qos = writer_proxy_data_to_backend_qos(info.info);
    writer_info.topic_name = info.info.topicName().to_string();
    writer_info.type_name = info.info.typeName().to_string();
    writer_info.locators = info.info.remote_locators();

    std::vector<unsigned char> guid_temp = parent_->guid_to_vector_(info.info.guid());
    std::vector<unsigned char> prefix_temp = parent_->prefix_guid_to_vector_(info.info.guid());
    std::vector<unsigned char> entity_temp = parent_->entity_guid_to_vector_(info.info.guid());
    std::string topic_type = writer_info.topic_name + "&&" + writer_info.type_name;
    if (info.status == EDDS_DDS::WRITER_DISCOVERY_STATUS::DISCOVERED_WRITER ||
        info.status == EDDS_DDS::WRITER_DISCOVERY_STATUS::CHANGED_QOS_WRITER) {
        parent_->register_remote_type_(writer_info.type_name);
        parent_->writers_map_[prefix_temp].emplace(entity_temp, writer_info);
        parent_->topics_writers_map_[topic_type].emplace(guid_temp, info.info.guid());
        parent_->discovered_types_topics_[writer_info.type_name].emplace(writer_info.topic_name);

    } else {
        parent_->writers_map_[prefix_temp].erase(entity_temp);
        parent_->topics_writers_map_[topic_type].erase(guid_temp);
    }
}

void VbsDiscoverer::ParticipantListener::on_type_discovery(vbs::DomainParticipantImpl* participant,
                                                           const std::string& type_name,
                                                           ERTPS::types::DynamicType_ptr dyn_type) {
    if (!parent_->is_type_discovered_(type_name, dyn_type)) {
        parent_->register_type_(participant, dyn_type);
    }
}

void VbsDiscoverer::register_remote_type_(const std::string& type_name) {
    if (discovered_types_topics_.find(type_name) != discovered_types_topics_.end()) {
        return;
    }
    participant_->register_remote_type(type_name);
}

std::string VbsDiscoverer::get_address_(const ERTPS_RTPS::ParticipantProxyData& info) {
    // The IP is obtained from the announced locators
    // Search for a locator with an IP different from localhost
    std::string address;

    // 1. default_locators.unicast
    if (search_address_in_locators_(info.default_locators.unicast, address)) {
        return address;
    }

    // 2. metatraffic_locators.unicast
    if (search_address_in_locators_(info.metatraffic_locators.unicast, address)) {
        return address;
    }

    // 3. default_locators.multicast
    if (search_address_in_locators_(info.default_locators.multicast, address)) {
        return address;
    }

    // 4. metatraffic_locators.multicast
    if (search_address_in_locators_(info.metatraffic_locators.multicast, address)) {
        return address;
    }

    // The only option is for localhost to be the only valid IP
    return "localhost";
}

bool VbsDiscoverer::search_address_in_locators_(const ERTPS::ResourceLimitedVector<ERTPS_RTPS::Locator_t>& locators,
                                                std::string& address) {
    for (auto& locator : locators) {
        // if the address is not localhost
        if (!ERTPS_RTPS::IPLocator::isLocal(locator)) {
            // Convert the locator to an address with IP format
            address = ERTPS_RTPS::IPLocator::ip_to_string(locator);
            return true;
        }
    }
    return false;
}

void VbsDiscoverer::register_type_(vbs::DomainParticipantImpl* participant, ERTPS::types::DynamicType_ptr type) {
    ERTPS::types::DynamicPubSubType pub_sub_type(type);
    vbs::TypeSupport(pub_sub_type).register_type(participant);
}

bool VbsDiscoverer::is_type_discovered_(const std::string& type_name, ERTPS::types::DynamicType_ptr type) {
    if (discovered_types_.find(type_name) != discovered_types_.end()) {
        return true;
    }

    discovered_types_.emplace(type_name, type);
    type_discovered_types_.emplace(type_name);
    return false;
}

}  // namespace vbstoolsdk
