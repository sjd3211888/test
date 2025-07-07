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

#include <ecdr/FastBuffer.h>
#include <ecdr/Cdr.h>
#include <edds/rtps/common/CdrSerialization.hpp>
#include <elog/Log.hpp>
#include "RpcDiscoveryPubSubTypes.h"

using SerializedPayload_t = vbs::SerializedPayload_t;
using InstanceHandle_t = vbs::InstanceHandle_t;

RpcDiscoveryPubSubType::RpcDiscoveryPubSubType() {
    setName("RpcDiscovery");
    auto type_size = RpcDiscovery::getMaxCdrSerializedSize();
    type_size += vbsutil::ecdr::Cdr::alignment(type_size, 4); /* possible submessage alignment */
    m_typeSize = static_cast<uint32_t>(type_size) + 4;        /*encapsulation*/
    m_isGetKeyDefined = RpcDiscovery::isKeyDefined();
    size_t keyLength =
        RpcDiscovery::getKeyMaxCdrSerializedSize() > 16 ? RpcDiscovery::getKeyMaxCdrSerializedSize() : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
    setIdlCrc16("3cdc");
}

RpcDiscoveryPubSubType::~RpcDiscoveryPubSubType() {
    if (m_keyBuffer != nullptr) {
        free(m_keyBuffer);
    }
}

bool RpcDiscoveryPubSubType::serialize(void* data, SerializedPayload_t* payload) {
    RpcDiscovery* p_type = static_cast<RpcDiscovery*>(data);

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
    } catch (vbsutil::ecdr::exception::NotEnoughMemoryException& e) {
        logError_("EVBS serialize error", __func__ << "(): " << e.what());
        return false;
    }

    // Get the serialized length
    payload->length = static_cast<uint32_t>(ser.getSerializedDataLength());
    return true;
}
bool RpcDiscoveryPubSubType::deserialize(SerializedPayload_t* payload, void* data) {
    try {
        //Convert DATA to pointer of your type
        RpcDiscovery* p_type = static_cast<RpcDiscovery*>(data);

        // Object that manages the raw buffer.
        vbsutil::ecdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);

        // Object that deserializes the data.
        vbsutil::ecdr::Cdr deser(fastbuffer, vbsutil::ecdr::Cdr::DEFAULT_ENDIAN, vbsutil::ecdr::Cdr::DDS_CDR);

        // Deserialize encapsulation.
        deser.read_encapsulation();
        payload->encapsulation = deser.endianness() == vbsutil::ecdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;

        // Deserialize the object.
        p_type->deserialize(deser);
    } catch (vbsutil::ecdr::exception::NotEnoughMemoryException& e) {
        logError_("EVBS deserialize error", __func__ << "(): " << e.what());
        return false;
        ;
    }

    return true;
}
std::function<uint32_t()> RpcDiscoveryPubSubType::getSerializedSizeProvider(void* data) {
    return [data]() -> uint32_t {
        return static_cast<uint32_t>(type::getCdrSerializedSize(*static_cast<RpcDiscovery*>(data))) +
               4u /*encapsulation*/;
    };
}
void* RpcDiscoveryPubSubType::createData() {
    return reinterpret_cast<void*>(new RpcDiscovery());
}

void RpcDiscoveryPubSubType::deleteData(void* data) {
    delete (reinterpret_cast<RpcDiscovery*>(data));
}

bool RpcDiscoveryPubSubType::getKey(void* data, InstanceHandle_t* handle, bool force_md5) {
    if (!m_isGetKeyDefined) {
        return false;
    }

    RpcDiscovery* p_type = static_cast<RpcDiscovery*>(data);

    // Object that manages the raw buffer.
    vbsutil::ecdr::FastBuffer fastbuffer(reinterpret_cast<char*>(m_keyBuffer),
                                         RpcDiscovery::getKeyMaxCdrSerializedSize());

    // Object that serializes the data.
    vbsutil::ecdr::Cdr ser(fastbuffer, vbsutil::ecdr::Cdr::BIG_ENDIANNESS);
    p_type->serializeKey(ser);

    if (force_md5 || RpcDiscovery::getKeyMaxCdrSerializedSize() > 16) {
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
