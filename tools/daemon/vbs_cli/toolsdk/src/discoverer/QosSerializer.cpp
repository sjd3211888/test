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

#include <sstream>

#include "discoverer/qos_serializer/QosSerializer.hpp"
#include "discoverer/qos_serializer/QosSerializerTags.hpp"

#include <edds/dds/core/policy/QosPolicies.hpp>

namespace vbstoolsdk {

template <>
void serialize<evbs::edds::dds::DurabilityQosPolicy>(const evbs::edds::dds::DurabilityQosPolicy& qos,
                                                     const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json kind;
    switch (qos.durabilityKind()) {
        case evbs::edds::dds::VOLATILE_DURABILITY_QOS:
            kind[kind_tag] = durability_volatile_tag;
            break;
        case evbs::edds::dds::TRANSIENT_LOCAL_DURABILITY_QOS:
            kind[kind_tag] = durability_transient_local_tag;
            break;
        case evbs::edds::dds::TRANSIENT_DURABILITY_QOS:
            kind[kind_tag] = durability_transient_tag;
            break;
        case evbs::edds::dds::PERSISTENT_DURABILITY_QOS:
            kind[kind_tag] = durability_persistent_tag;
            break;
    }
    serialized[fieldname] = kind;
}

template <>
void serialize<evbs::ertps::Duration_t>(const evbs::ertps::Duration_t& qos, const std::string& fieldname,
                                        nlohmann::json& serialized) {
    nlohmann::json duration;
    duration[duration_seconds_tag] = qos.seconds;
    duration[duration_nanoseconds_tag] = qos.nanosec;
    serialized[fieldname] = duration;
}

template <>
void serialize<evbs::edds::dds::DeadlineQosPolicy>(const evbs::edds::dds::DeadlineQosPolicy& qos,
                                                   const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json deadline;
    serialize(qos.period, duration_period_tag, deadline);
    serialized[fieldname] = deadline;
}

template <>
void serialize<evbs::edds::dds::LivelinessQosPolicy>(const evbs::edds::dds::LivelinessQosPolicy& qos,
                                                     const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json liveliness;
    switch (qos.kind) {
        case evbs::edds::dds::AUTOMATIC_LIVELINESS_QOS:
            liveliness[kind_tag] = liveliness_automatic_tag;
            break;
        case evbs::edds::dds::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
            liveliness[kind_tag] = liveliness_manual_participant_tag;
            break;
        case evbs::edds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS:
            liveliness[kind_tag] = liveliness_manual_topic_tag;
            break;
    }
    serialize(qos.announcement_period, announcement_period_tag, liveliness);
    serialize(qos.lease_duration, lease_duration_tag, liveliness);
    serialized[fieldname] = liveliness;
}

template <>
void serialize<evbs::edds::dds::ReliabilityQosPolicy>(const evbs::edds::dds::ReliabilityQosPolicy& qos,
                                                      const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json reliability;
    switch (qos.kind) {
        case evbs::edds::dds::BEST_EFFORT_RELIABILITY_QOS:
            reliability[kind_tag] = reliability_best_effort_tag;
            break;
        case evbs::edds::dds::RELIABLE_RELIABILITY_QOS:
            reliability[kind_tag] = reliability_reliable_tag;
            break;
    }
    serialize(qos.max_blocking_time, max_blocking_time_tag, reliability);
    serialized[fieldname] = reliability;
}

template <>
void serialize<evbs::edds::dds::OwnershipQosPolicy>(const evbs::edds::dds::OwnershipQosPolicy& qos,
                                                    const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json ownership;
    switch (qos.kind) {
        case evbs::edds::dds::SHARED_OWNERSHIP_QOS:
            ownership[kind_tag] = ownership_shared_tag;
            break;
        case evbs::edds::dds::EXCLUSIVE_OWNERSHIP_QOS:
            ownership[kind_tag] = ownership_exclusive_tag;
            break;
    }
    serialized[fieldname] = ownership;
}

template <>
void serialize<bool>(const bool& qos, const std::string& fieldname, nlohmann::json& serialized) {
    if (qos) {
        serialized[fieldname] = true_tag;
    } else {
        serialized[fieldname] = false_tag;
    }
}

template <>
void serialize<evbs::edds::dds::PartitionQosPolicy>(const evbs::edds::dds::PartitionQosPolicy& qos,
                                                    const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json partition = nlohmann::json::array();
    for (const auto& p : qos) {
        partition.push_back(p.name());
    }
    serialized[fieldname] = partition;
}

template <>
void serialize<evbs::edds::dds::LifespanQosPolicy>(const evbs::edds::dds::LifespanQosPolicy& qos,
                                                   const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json lifespan;
    serialize(qos.duration, duration_tag, lifespan);
    serialized[fieldname] = lifespan;
}

template <>
void serialize<evbs::edds::dds::DisablePositiveACKsQosPolicy>(const evbs::edds::dds::DisablePositiveACKsQosPolicy& qos,
                                                              const std::string& fieldname,
                                                              nlohmann::json& serialized) {
    nlohmann::json disable_acks;
    serialize(qos.enabled, enabled_tag, disable_acks);
    serialize(qos.duration, duration_tag, disable_acks);
    serialized[fieldname] = disable_acks;
}

template <>
void serialize<evbs::edds::dds::OwnershipStrengthQosPolicy>(const evbs::edds::dds::OwnershipStrengthQosPolicy& qos,
                                                            const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json strength;
    strength[value_tag] = qos.value;
    serialized[fieldname] = strength;
}

template <>
void serialize<evbs::edds::dds::PublishModeQosPolicy>(const evbs::edds::dds::PublishModeQosPolicy& qos,
                                                      const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json publishmode;
    switch (qos.kind) {
        case evbs::edds::dds::SYNCHRONOUS_PUBLISH_MODE:
            publishmode[kind_tag] = publish_mode_sync_tag;
            break;
        case evbs::edds::dds::ASYNCHRONOUS_PUBLISH_MODE:
            publishmode[kind_tag] = publish_mode_async_tag;
            break;
    }
    serialized[fieldname] = publishmode;
}

template <>
void serialize<evbs::edds::dds::ParameterPropertyList_t>(const evbs::edds::dds::ParameterPropertyList_t& qos,
                                                         const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json properties = nlohmann::json::array();
    for (const auto& p : qos) {
        nlohmann::json property;
        property[name_tag] = p.first();
        property[value_tag] = p.second();
        properties.push_back(property);
    }
    serialized[fieldname] = properties;
}

template <>
void serialize<evbs::edds::dds::E2EProtectionQosPolicy>(const evbs::edds::dds::E2EProtectionQosPolicy& qos,
                                                        const std::string& fieldname, nlohmann::json& serialized) {
    nlohmann::json e2e;

    serialize(qos.enabled, enabled_tag, e2e);
    e2e[e2e_protection_need_check_tag] = qos.needCheckEnableFlag;
    // e2e[e2e_protection_data_id_tag] = qos.DataID;
    e2e[e2e_protection_min_data_len_tag] = qos.MinDataLength;
    e2e[e2e_protection_max_data_len_tag] = qos.MaxDataLength;
    e2e[e2e_protection_max_delta_cnt_tag] = qos.MaxDeltaCounter;
    serialized[fieldname] = e2e;
}

nlohmann::json reader_proxy_data_to_backend_qos(const evbs::ertps::rtps::ReaderProxyData& reader_data) {
    nlohmann::json reader;

    serialize(reader_data.m_qos.m_durability, durability_tag, reader);
    serialize(reader_data.m_qos.m_deadline, deadline_tag, reader);
    serialize(reader_data.m_qos.m_liveliness, liveliness_tag, reader);
    serialize(reader_data.m_qos.m_reliability, reliability_tag, reader);
    serialize(reader_data.m_qos.m_ownership, ownership_tag, reader);
    serialize(reader_data.m_qos.m_partition, partition_tag, reader);
    serialize(reader_data.m_qos.m_lifespan, lifespan_tag, reader);
    serialize(reader_data.m_qos.m_disablePositiveACKs, disable_positive_acks_tag, reader);
    serialize(reader_data.m_qos.m_e2eProtecion, e2e_protection_tag, reader);

    return reader;
}

void generate_vender_name(int vender_id, const std::string& name, std::stringstream& vendor_name) {
    vendor_name << name << " (0x" << std::setfill('0') << std::setw(sizeof(int)) << std::hex << vender_id << ')';
}

nlohmann::json writer_proxy_data_to_backend_qos(const evbs::ertps::rtps::WriterProxyData& writer_data) {
    nlohmann::json writer;

    serialize(writer_data.m_qos.m_durability, durability_tag, writer);
    serialize(writer_data.m_qos.m_deadline, deadline_tag, writer);
    serialize(writer_data.m_qos.m_liveliness, liveliness_tag, writer);
    serialize(writer_data.m_qos.m_reliability, reliability_tag, writer);
    serialize(writer_data.m_qos.m_lifespan, lifespan_tag, writer);
    serialize(writer_data.m_qos.m_ownership, ownership_tag, writer);
    serialize(writer_data.m_qos.m_ownershipStrength, ownership_strength_tag, writer);
    serialize(writer_data.m_qos.m_partition, partition_tag, writer);
    serialize(writer_data.m_qos.m_publishMode, publish_mode_tag, writer);
    serialize(writer_data.m_qos.m_disablePositiveACKs, disable_positive_acks_tag, writer);
    serialize(writer_data.m_qos.m_e2eProtecion, e2e_protection_tag, writer);

    return writer;
}

inline std::string generateEvbsVersion(const evbs::edds::rtps::EvbsVersion_t& version) {
    std::string v;
    for (auto it = version.begin(); it != version.end(); ++it) {
        v.append(std::to_string(*it));
        v.push_back('.');
    }
    v.pop_back();
    return v;
}

nlohmann::json participant_proxy_data_to_backend_qos(const evbs::ertps::rtps::ParticipantProxyData& participant_data) {
    nlohmann::json participant;

    participant[available_builtin_endpoints_tag] = participant_data.m_availableBuiltinEndpoints;
    serialize(participant_data.m_leaseDuration, lease_duration_tag, participant);
    serialize(participant_data.m_properties, properties_tag, participant);

    nlohmann::json version_json;
    int vendor_id = participant_data.m_VendorId[0] << 8 | participant_data.m_VendorId[1];
    std::stringstream vendor_name;
    std::string product_version;
    switch (vendor_id) {
        case 0x0f01:
            generate_vender_name(vendor_id, "VBS", vendor_name);
            product_version = generateEvbsVersion(participant_data.m_evbsRemoteVersion);
            break;
        case 0x0101:
            generate_vender_name(vendor_id, "RTI", vendor_name);
            break;
        case 0x010f:
            generate_vender_name(vendor_id, "FastDDS", vendor_name);
            break;
        default:
            generate_vender_name(vendor_id, "Unknown", vendor_name);
            break;
    }

    version_json[vendor_id_tag] = vendor_name.str();
    version_json[product_version_tag] = product_version;
    version_json[protocol_version_tag] = std::to_string(participant_data.m_protocolVersion.m_major) + "." +
                                         std::to_string(participant_data.m_protocolVersion.m_minor);
    participant[version_tag] = version_json;
    return participant;
}

}  // namespace vbstoolsdk
