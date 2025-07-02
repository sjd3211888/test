// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#include <string>

#include "edds/rtps/common/Property.h"
#include "utils/QosConverters.hpp"

namespace vbs {
namespace utils {

using evbs::ertps::rtps::Property;
using std::string;

void set_qos_from_attributes(DataWriterQos& qos, const PublisherAttributes& attr) {
    qos.writer_resource_limits().matched_subscriber_allocation = attr.matched_subscriber_allocation;
    qos.properties() = attr.properties;
    qos.endpoint().unicast_locator_list = attr.unicastLocatorList;
    qos.endpoint().multicast_locator_list = attr.multicastLocatorList;
    qos.endpoint().remote_locator_list = attr.remoteLocatorList;
    qos.endpoint().external_unicast_locators = attr.external_unicast_locators;
    qos.endpoint().ignore_non_matching_locators = attr.ignore_non_matching_locators;
    qos.endpoint().history_memory_policy = attr.historyMemoryPolicy;
    qos.endpoint().user_defined_id = attr.getUserDefinedID();
    qos.endpoint().entity_id = attr.getEntityID();
    qos.reliable_writer_qos().times = attr.times;
    qos.reliable_writer_qos().disable_positive_acks = attr.qos.m_disablePositiveACKs;
    qos.durability() = attr.qos.m_durability;
    qos.deadline() = attr.qos.m_deadline;
    qos.liveliness() = attr.qos.m_liveliness;
    qos.reliability() = attr.qos.m_reliability;
    qos.lifespan() = attr.qos.m_lifespan;
    qos.ownership() = attr.qos.m_ownership;
    qos.ownership_strength() = attr.qos.m_ownershipStrength;
    qos.publish_mode() = attr.qos.m_publishMode;
    qos.history() = attr.qos.m_history;
    qos.resource_limits() = attr.qos.m_resourceLimits;
    qos.reliable_writer_qos().disable_heartbeat_piggyback = attr.qos.disable_heartbeat_piggyback;
    qos.reliable_writer_qos().heartbeat_per_max_samples = attr.qos.heartbeat_per_max_samples;
    qos.reliable_writer_qos().high_watermark = attr.qos.high_watermark;
    qos.reliable_writer_qos().low_watermark = attr.qos.low_watermark;
    qos.e2e_protection() = attr.qos.m_e2eProtecion;
    qos.batch_qos() = attr.qos.m_batch;
    qos.send_multi(attr.qos.m_isMultiSend);

    if (attr.qos.m_partition.size() > 0U) {
        Property property;
        property.name("partitions");
        string partitions;
        bool is_first_partition = true;

        for (auto& partition : attr.qos.m_partition.names()) {
            partitions += (is_first_partition ? "" : ";") + partition;
            is_first_partition = false;
        }

        property.value(std::move(partitions));
        qos.properties().properties().push_back(std::move(property));
    }
}

void set_qos_from_attributes(DataReaderQos& qos, const SubscriberAttributes& attr) {
    qos.reader_resource_limits().matched_publisher_allocation = attr.matched_publisher_allocation;
    qos.properties() = attr.properties;
    qos.endpoint().unicast_locator_list = attr.unicastLocatorList;
    qos.endpoint().multicast_locator_list = attr.multicastLocatorList;
    qos.endpoint().remote_locator_list = attr.remoteLocatorList;
    qos.endpoint().external_unicast_locators = attr.external_unicast_locators;
    qos.endpoint().ignore_non_matching_locators = attr.ignore_non_matching_locators;
    qos.endpoint().history_memory_policy = attr.historyMemoryPolicy;
    qos.endpoint().user_defined_id = attr.getUserDefinedID();
    qos.endpoint().entity_id = attr.getEntityID();
    qos.reliable_reader_qos().times = attr.times;
    qos.reliable_reader_qos().disable_positive_ACKs = attr.qos.m_disablePositiveACKs;
    qos.durability() = attr.qos.m_durability;
    qos.deadline() = attr.qos.m_deadline;
    qos.liveliness() = attr.qos.m_liveliness;
    qos.reliability() = attr.qos.m_reliability;
    qos.lifespan() = attr.qos.m_lifespan;
    qos.ownership() = attr.qos.m_ownership;
    qos.history() = attr.qos.m_history;
    qos.resource_limits() = attr.qos.m_resourceLimits;
    qos.e2e_protection() = attr.qos.m_e2eProtecion;
    qos.destination_order() = attr.qos.m_destinationOrder;
    qos.send_multi(attr.qos.m_isMultiSend);

    if (attr.qos.m_partition.size() > 0) {
        Property property;
        property.name("partitions");
        string partitions;
        bool is_first_partition = true;

        for (auto& partition : attr.qos.m_partition.names()) {
            partitions += (is_first_partition ? "" : ";") + partition;
            is_first_partition = false;
        }

        property.value(std::move(partitions));
        qos.properties().properties().push_back(std::move(property));
    }
}

void set_qos_from_attributes(vbs::DomainParticipantQosInner& qos,
                             const evbs::ertps::rtps::RTPSParticipantAttributes& attr) {
    qos.allocation() = attr.allocation;
    qos.wire_protocol().prefix = attr.prefix;
    qos.wire_protocol().participant_id = attr.participantID;
    qos.wire_protocol().builtin = attr.builtin;
    qos.wire_protocol().port = attr.port;
    qos.wire_protocol().default_unicast_locator_list = attr.defaultUnicastLocatorList;
    qos.wire_protocol().default_multicast_locator_list = attr.defaultMulticastLocatorList;
    qos.wire_protocol().default_external_unicast_locators = attr.default_external_unicast_locators;
    qos.wire_protocol().ignore_non_matching_locators = attr.ignore_non_matching_locators;
    qos.transport().user_transports = attr.userTransports;
    qos.transport().use_builtin_transports = attr.useBuiltinTransports;
    qos.transport().transport_type = attr.transport_type;
    qos.transport().send_socket_buffer_size = attr.sendSocketBufferSize;
    qos.transport().listen_socket_buffer_size = attr.listenSocketBufferSize;
    qos.name() = attr.getName();
    qos.flow_controllers() = attr.flow_controllers;

    // Merge attributes and qos properties
    for (auto& property : attr.properties.properties()) {
        string* const property_value =
            evbs::ertps::rtps::PropertyPolicyHelper::find_property(qos.properties(), property.name());
        if (nullptr == property_value) {
            qos.properties().properties().emplace_back(property);
        } else {
            *property_value = property.value();
        }
    }
    qos.properties().binary_properties() = attr.properties.binary_properties();
}

void set_attributes_from_qos(evbs::ertps::rtps::RTPSParticipantAttributes& attr,
                             const vbs::DomainParticipantQosInner& qos) {
    attr.allocation = qos.allocation();
    attr.properties = qos.properties();
    attr.setName(qos.name().c_str());
    attr.prefix = qos.wire_protocol().prefix;
    attr.participantID = qos.wire_protocol().participant_id;
    attr.builtin = qos.wire_protocol().builtin;
    attr.port = qos.wire_protocol().port;
    attr.defaultUnicastLocatorList = qos.wire_protocol().default_unicast_locator_list;
    attr.defaultMulticastLocatorList = qos.wire_protocol().default_multicast_locator_list;
    attr.default_external_unicast_locators = qos.wire_protocol().default_external_unicast_locators;
    attr.ignore_non_matching_locators = qos.wire_protocol().ignore_non_matching_locators;
    attr.userTransports = qos.transport().user_transports;
    attr.useBuiltinTransports = qos.transport().use_builtin_transports;
    attr.transport_type = qos.transport().transport_type;
    attr.sendSocketBufferSize = qos.transport().send_socket_buffer_size;
    attr.listenSocketBufferSize = qos.transport().listen_socket_buffer_size;
    attr.flow_controllers = qos.flow_controllers();
}

void set_qos_from_attributes(vbsutil::xmlparser::TopicQos& qos, const TopicAttributes& attr) {
    qos.history() = attr.qos.history();
    qos.resource_limits() = attr.qos.resource_limits();
    if (attr.qos.reliability_from_xml()) {
        qos.reliability().kind = attr.qos.reliability().kind;
        qos.reliability_from_xml(attr.qos.reliability_from_xml());
    }
    if (attr.qos.durability_from_xml()) {
        qos.durability().kind = attr.qos.durability().kind;
        qos.durability_from_xml(attr.qos.durability_from_xml());
    }
    if (attr.qos.deadline_from_xml()) {
        qos.deadline().period = attr.qos.deadline().period;
        qos.deadline_from_xml(attr.qos.deadline_from_xml());
    }
    if (attr.qos.lifespan_from_xml()) {
        qos.lifespan().duration = attr.qos.lifespan().duration;
        qos.lifespan_from_xml(attr.qos.lifespan_from_xml());
    }
    if (attr.qos.ownership_from_xml()) {
        qos.ownership().kind = attr.qos.ownership().kind;
        qos.ownership_from_xml(attr.qos.ownership_from_xml());
    }
    if (attr.qos.history_from_xml()) {
        qos.history_from_xml(attr.qos.history_from_xml());
    }
    if (attr.qos.resource_from_xml()) {
        qos.resource_from_xml(attr.qos.resource_from_xml());
    }
    qos.send_multi(attr.qos.send_multi());
}

void set_qos_from_attributes(vbsutil::xmlparser::SubscriberQos& qos, const SubscriberAttributes& attr) {
    qos.partition() = attr.qos.m_partition;
}

void set_qos_from_attributes(vbsutil::xmlparser::PublisherQos& qos, const PublisherAttributes& attr) {
    qos.partition() = attr.qos.m_partition;
}

} /* namespace utils */
} /* namespace vbs */
