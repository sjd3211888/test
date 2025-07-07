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

#ifndef _QOSSERIALIZERTAGS_HPP_
#define _QOSSERIALIZERTAGS_HPP_

namespace vbstoolsdk {

// Some generic tags
static constexpr const char* enabled_tag = "enabled";
static constexpr const char* kind_tag = "kind";
static constexpr const char* name_tag = "name";
static constexpr const char* value_tag = "value";

// For boolean serialization
static constexpr const char* false_tag = "false";
static constexpr const char* true_tag = "true";

// Primitive types tags
static constexpr const char* group_data_tag = "group_data";
static constexpr const char* topic_data_tag = "topic_data";
static constexpr const char* user_data_tag = "user_data";
static constexpr const char* partition_tag = "partition";
static constexpr const char* properties_tag = "properties";
static constexpr const char* vendor_id_tag = "vendor_id";
static constexpr const char* version_tag = "version";
static constexpr const char* product_version_tag = "product_version";
static constexpr const char* protocol_version_tag = "protocol_version";
static constexpr const char* available_builtin_endpoints_tag = "available_builtin_endpoints";

#include "discoverer/qos_serializer/DurabilityTags.ipp"
#include "discoverer/qos_serializer/DurationTags.ipp"
#include "discoverer/qos_serializer/LivelinessTags.ipp"
#include "discoverer/qos_serializer/OwnershipTags.ipp"
#include "discoverer/qos_serializer/PublishModeTags.ipp"
#include "discoverer/qos_serializer/ReliabilityTags.ipp"
#include "discoverer/qos_serializer/ResourceTags.ipp"
#include "discoverer/qos_serializer/XTypesTags.ipp"
#include "discoverer/qos_serializer/E2EProtectionTags.ipp"

}  // namespace vbstoolsdk

#endif
