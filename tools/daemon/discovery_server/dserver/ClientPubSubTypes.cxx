// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <ecdr/FastBuffer.h>
#include <ecdr/Cdr.h>

#include "ClientPubSubTypes.hpp"

using SerializedPayload_t = vbs::SerializedPayload_t;
using InstanceHandle_t = vbs::InstanceHandle_t;

ClientPubSubType::ClientPubSubType() {
    setName("Client");
    auto type_size = Client::getMaxCdrSerializedSize();
    type_size += vbsutil::ecdr::Cdr::alignment(type_size, 4); /* possible submessage alignment */
    m_typeSize = static_cast<uint32_t>(type_size) + 4;        /*encapsulation*/
    m_isGetKeyDefined = Client::isKeyDefined();
    size_t keyLength = Client::getKeyMaxCdrSerializedSize() > 16 ? Client::getKeyMaxCdrSerializedSize() : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
}

ClientPubSubType::~ClientPubSubType() {
    if (m_keyBuffer != nullptr) {
        free(m_keyBuffer);
    }
}

bool ClientPubSubType::serialize(void* data, SerializedPayload_t* payload) {
    Client* p_type = static_cast<Client*>(data);

    // Object that manages the raw buffer.
    vbsutil::ecdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->max_size);
    // Object that serializes the data.
    vbsutil::ecdr::Cdr ser(fastbuffer, vbsutil::ecdr::Cdr::DEFAULT_ENDIAN, vbsutil::ecdr::Cdr::DDS_CDR);
    payload->encapsulation = ser.endianness() == vbsutil::ecdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
    // Serialize encapsulation
    ser.serialize_encapsulation();

    try {
        // Serialize the object.
        p_type->serialize(ser);
    } catch (vbsutil::ecdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    // Get the serialized length
    payload->length = static_cast<uint32_t>(ser.getSerializedDataLength());
    return true;
}

bool ClientPubSubType::deserialize(SerializedPayload_t* payload, void* data) {
    try {
        // Convert DATA to pointer of your type
        Client* p_type = static_cast<Client*>(data);

        // Object that manages the raw buffer.
        vbsutil::ecdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);

        // Object that deserializes the data.
        vbsutil::ecdr::Cdr deser(fastbuffer, vbsutil::ecdr::Cdr::DEFAULT_ENDIAN, vbsutil::ecdr::Cdr::DDS_CDR);

        // Deserialize encapsulation.
        deser.read_encapsulation();
        payload->encapsulation = deser.endianness() == vbsutil::ecdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;

        // Deserialize the object.
        p_type->deserialize(deser);
    } catch (vbsutil::ecdr::exception::NotEnoughMemoryException& /*exception*/) {
        return false;
    }

    return true;
}

std::function<uint32_t()> ClientPubSubType::getSerializedSizeProvider(void* data) {
    return [data]() -> uint32_t {
        return static_cast<uint32_t>(type::getCdrSerializedSize(*static_cast<Client*>(data))) + 4u /*encapsulation*/;
    };
}

void* ClientPubSubType::createData() {
    return reinterpret_cast<void*>(new Client());
}

void ClientPubSubType::deleteData(void* data) {
    delete (reinterpret_cast<Client*>(data));
}

bool ClientPubSubType::getKey(void* data, InstanceHandle_t* handle, bool force_md5) {
    if (!m_isGetKeyDefined) {
        return false;
    }

    Client* p_type = static_cast<Client*>(data);

    // Object that manages the raw buffer.
    vbsutil::ecdr::FastBuffer fastbuffer(reinterpret_cast<char*>(m_keyBuffer), Client::getKeyMaxCdrSerializedSize());

    // Object that serializes the data.
    vbsutil::ecdr::Cdr ser(fastbuffer, vbsutil::ecdr::Cdr::BIG_ENDIANNESS);
    p_type->serializeKey(ser);
    if (force_md5 || Client::getKeyMaxCdrSerializedSize() > 16) {
        m_md5.init();
        m_md5.update(m_keyBuffer, static_cast<unsigned int>(ser.getSerializedDataLength()));
        m_md5.finalize();
        for (uint8_t i = 0; i < 16; ++i) {
            handle->value[i] = m_md5.digest[i];
        }
    } else {
        for (uint8_t i = 0; i < 16; ++i) {
            handle->value[i] = m_keyBuffer[i];
        }
    }
    return true;
}

size_t ClientPubSubType::getNonSerializedSize() {
    Client stData;
    return sizeof(stData) + 4u;
}
