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

#include "topic/DynamicPubSubTypeImpl.hpp"

namespace vbs {
VbsDynamicPubSubTypeImpl::VbsDynamicPubSubTypeImpl() : evbs::ertps::types::DynamicPubSubType(DynamicType_ptr()) {}

VbsDynamicPubSubTypeImpl::VbsDynamicPubSubTypeImpl(vbs::DynamicType_ptr pType)
    : evbs::ertps::types::DynamicPubSubType(std::move(pType)) {}

VbsFlatDataPubSubType::VbsFlatDataPubSubType(const std::string& type_name) {
    setName(type_name.c_str());
    m_typeSize = 10000000;
    m_isGetKeyDefined = false;
}

bool VbsFlatDataPubSubType::is_bounded() const {
    return true;
}

bool VbsFlatDataPubSubType::is_plain() const {
    return true;
}

bool VbsFlatDataPubSubType::serialize(void* data, vbs::SerializedPayload_t* payload) {
    (void)data;
    (void)payload;
    return true;
}

bool VbsFlatDataPubSubType::deserialize(vbs::SerializedPayload_t* payload, void* data) {
    (void)data;
    (void)payload;
    return true;
}

std::function<uint32_t()> VbsFlatDataPubSubType::getSerializedSizeProvider(void* data) {
    (void)data;
    return [this]() -> uint32_t {
        return m_typeSize;
    };
}

bool VbsFlatDataPubSubType::getKey(void* data, vbs::InstanceHandle_t* ihandle, bool force_md5) {
    (void)data;
    (void)ihandle;
    (void)force_md5;
    return false;
}

void* VbsFlatDataPubSubType::createData() {
    return nullptr;
}

void VbsFlatDataPubSubType::deleteData(void* data) {
    (void)data;
    return;
}

}  // namespace vbs
