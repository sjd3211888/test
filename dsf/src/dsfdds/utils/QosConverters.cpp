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
// feature: adapt dispatcher for vbs2.0
// ------------------------------------------------------------------

#include <iostream>
#include <string>
#include "DsfLog.hpp"
#include "dsfdds/utils/QosConverters.hpp"
#include "deps/attributes/BaseWriterAttributes.h"

namespace li {
namespace dsfdds {
namespace utils {
void set_qos_from_attributes(DataWriterQos& qos, const vbs::BaseWriterAttributes& attr) {
    int16_t entity_id = attr.endpoint.getEntityID();
    int16_t user_def_id = attr.endpoint.getUserDefinedID();
    // 对应AR_DSF-Global-Profile-05，当xml中同时配置了user_def_id与entity_id，但两者值不相同时，输出警告日志，并且随后会将entity_id配置为user_def_id
    if ((entity_id != -1) && (entity_id != user_def_id)) {
        DSF_LOG_WARNING(QosConverters,
                        "attr entityID invalid, entityID:" << entity_id << ", userDefinedID:" << user_def_id);
    }
    qos.endpoint().user_defined_id = user_def_id;
    qos.endpoint().entity_id = user_def_id;

    // qos.deadline() = attr.deadline;
    qos.properties() = attr.endpoint.properties;
    if (attr.endpoint.reliabilityKind == vbsutil::xmlparser::ReliabilityKind_t::BEST_EFFORT) {
        qos.reliability().kind = vbsutil::xmlparser::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS;
    } else if (attr.endpoint.reliabilityKind == vbsutil::xmlparser::ReliabilityKind_t::RELIABLE) {
        qos.reliability().kind = vbsutil::xmlparser::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
    }

    if (attr.endpoint.durabilityKind == vbsutil::xmlparser::DurabilityKind_t::VOLATILE) {
        qos.durability().kind = vbsutil::xmlparser::DurabilityQosPolicyKind::VOLATILE_DURABILITY_QOS;
    } else if (attr.endpoint.durabilityKind == vbsutil::xmlparser::DurabilityKind_t::TRANSIENT_LOCAL) {
        qos.durability().kind = vbsutil::xmlparser::DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS;
    } else if (attr.endpoint.durabilityKind == vbsutil::xmlparser::DurabilityKind_t::TRANSIENT) {
        qos.durability().kind = vbsutil::xmlparser::DurabilityQosPolicyKind::TRANSIENT_DURABILITY_QOS;
    } else if (attr.endpoint.durabilityKind == vbsutil::xmlparser::DurabilityKind_t::PERSISTENT) {
        qos.durability().kind = vbsutil::xmlparser::DurabilityQosPolicyKind::PERSISTENT_DURABILITY_QOS;
    }

    DSF_LOG_INFO(WriterQosConverters, "entityID:" << entity_id << ", userDefinedID:" << user_def_id
                                                  << ", reliability_kind: " << static_cast<int>(qos.reliability().kind)
                                                  << ", DurabilityKind_t: " << static_cast<int>(qos.durability().kind));
}

void set_qos_from_attributes(DataReaderQos& qos, const vbs::BaseReaderAttributes& attr) {
    int16_t entity_id = attr.endpoint.getEntityID();
    int16_t user_def_id = attr.endpoint.getUserDefinedID();
    // 对应AR_DSF-Global-Profile-05，当xml中同时配置了user_def_id与entity_id，但两者值不相同时，输出警告日志，并且随后会将entity_id配置为user_def_id
    if ((entity_id != -1) && (entity_id != user_def_id)) {
        DSF_LOG_WARNING(QosConverters, "SubscriberAttributes entityID invalid, entityID:"
                                           << entity_id << ", userDefinedID:" << user_def_id);
    }

    qos.endpoint().user_defined_id = user_def_id;
    qos.endpoint().entity_id = entity_id;

    qos.properties() = attr.endpoint.properties;
    if (attr.endpoint.reliabilityKind == vbsutil::xmlparser::ReliabilityKind_t::BEST_EFFORT) {
        qos.reliability().kind = vbsutil::xmlparser::ReliabilityQosPolicyKind::BEST_EFFORT_RELIABILITY_QOS;
    } else if (attr.endpoint.reliabilityKind == vbsutil::xmlparser::ReliabilityKind_t::RELIABLE) {
        qos.reliability().kind = vbsutil::xmlparser::ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
    }
    DSF_LOG_INFO(ReaderQosConverters, "entityID:" << entity_id << ", userDefinedID:" << user_def_id
                                                  << ", reliability_kind: " << static_cast<int>(qos.reliability().kind)
                                                  << ", DurabilityKind_t: " << attr.endpoint.durabilityKind);
}

} /* namespace utils */
} /* namespace dsfdds */
} /* namespace li */
