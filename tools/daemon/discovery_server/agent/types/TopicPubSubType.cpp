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

#include <agent/types/TopicPubSubType.hpp>
#include <ecdr/FastBuffer.h>
#include <ecdr/Cdr.h>

namespace discovery_server {
namespace agent {

TopicPubSubType::TopicPubSubType(bool with_key) {
    m_typeSize = 1500 + 4 /*encapsulation*/;
    m_isGetKeyDefined = with_key;
}

bool TopicPubSubType::serialize(void* data, vbs::SerializedPayload_t* payload) {
    return serialize(data, payload, vbs::DataRepresentationId_t::XCDR_DATA_REPRESENTATION);
}

bool TopicPubSubType::serialize(void* data, vbs::SerializedPayload_t* payload,
                                vbs::DataRepresentationId_t /* data_representation */) {
    bool rv = false;

    // Reinterpret cast back to a std::vector from data pointer
    void* non_const_data = const_cast<void*>(data);
    std::vector<unsigned char>* buffer = reinterpret_cast<std::vector<unsigned char>*>(non_const_data);

    // Representation header
    payload->data[0] = 0;
    payload->data[1] = 1;
    payload->data[2] = 0;
    payload->data[3] = 0;

    if (buffer->size() <= (payload->max_size - 4)) {
        memcpy(&payload->data[4], buffer->data(), buffer->size());
        payload->length = uint32_t(buffer->size() + 4);  // Get the serialized length
        rv = true;
    }

    return rv;
}

bool TopicPubSubType::deserialize(vbs::SerializedPayload_t* payload, void* data) {
    std::vector<unsigned char>* buffer = reinterpret_cast<std::vector<unsigned char>*>(data);
    buffer->assign(payload->data + 4, payload->data + payload->length);

    return true;
}

std::function<uint32_t()> TopicPubSubType::getSerializedSizeProvider(void* data) {
    return getSerializedSizeProvider(data, vbs::DataRepresentationId_t::XCDR_DATA_REPRESENTATION);
}

std::function<uint32_t()> TopicPubSubType::getSerializedSizeProvider(
    void* data, vbs::DataRepresentationId_t /* data_representation */) {
    return [data]() -> uint32_t {
        void* non_const_data = const_cast<void*>(data);
        std::vector<unsigned char>* buffer = reinterpret_cast<std::vector<unsigned char>*>(non_const_data);

        return static_cast<uint32_t>(buffer->size() + 4); /*encapsulation*/
    };
}

void* TopicPubSubType::createData() {
    return (void*)new std::vector<unsigned char>;
}

void TopicPubSubType::deleteData(void* data) {
    delete ((std::vector<unsigned char>*)data);
}

bool TopicPubSubType::getKey(void* /* data */, vbs::InstanceHandle_t* /* ihandle */, bool /* force_md5 */) {
    return false;
}

}  // namespace agent
}  // namespace discovery_server
