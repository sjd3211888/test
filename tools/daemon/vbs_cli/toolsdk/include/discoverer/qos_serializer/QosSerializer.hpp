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

#ifndef _QOSSERIALIZER_HPP_
#define _QOSSERIALIZER_HPP_

#include <string>

#include <edds/rtps/builtin/data/ReaderProxyData.h>
#include <edds/rtps/builtin/data/WriterProxyData.h>
#include <edds/rtps/builtin/data/ParticipantProxyData.h>
#include <edds/dds/core/policy/QosPolicies.hpp>

#include <nlohmann/json.hpp>

namespace vbstoolsdk {

template <typename T>
void serialize(const T& qos, const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::DurabilityQosPolicy>(const evbs::edds::dds::DurabilityQosPolicy& qos,
                                                     const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::ertps::Duration_t>(const evbs::ertps::Duration_t& qos, const std::string& fieldname,
                                        nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::DeadlineQosPolicy>(const evbs::edds::dds::DeadlineQosPolicy& qos,
                                                   const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::LivelinessQosPolicy>(const evbs::edds::dds::LivelinessQosPolicy& qos,
                                                     const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::ReliabilityQosPolicy>(const evbs::edds::dds::ReliabilityQosPolicy& qos,
                                                      const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::OwnershipQosPolicy>(const evbs::edds::dds::OwnershipQosPolicy& qos,
                                                    const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<bool>(const bool& qos, const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::PartitionQosPolicy>(const evbs::edds::dds::PartitionQosPolicy& qos,
                                                    const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::LifespanQosPolicy>(const evbs::edds::dds::LifespanQosPolicy& qos,
                                                   const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::DisablePositiveACKsQosPolicy>(const evbs::edds::dds::DisablePositiveACKsQosPolicy& qos,
                                                              const std::string& fieldname, nlohmann::json& serialized);

template <>
void serialize<evbs::edds::dds::E2EProtectionQosPolicy>(const evbs::edds::dds::E2EProtectionQosPolicy& qos,
                                                        const std::string& fieldname, nlohmann::json& serialized);

nlohmann::json reader_proxy_data_to_backend_qos(const evbs::ertps::rtps::ReaderProxyData& reader_data);

nlohmann::json writer_proxy_data_to_backend_qos(const evbs::ertps::rtps::WriterProxyData& writer_data);

nlohmann::json participant_proxy_data_to_backend_qos(const evbs::ertps::rtps::ParticipantProxyData& participant_data);

}  // namespace vbstoolsdk

#endif
