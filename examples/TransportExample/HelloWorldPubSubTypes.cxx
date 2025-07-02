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
// feature: Modify test cases
// ------------------------------------------------------------------
#include "HelloWorldPubSubTypes.h"

HelloWorldPubSubType::HelloWorldPubSubType() {
    setName("HelloWorld");
    auto type_size = HelloWorld::getMaxCdrSerializedSize();
    type_size += vbs::Cdr::alignment(type_size, 4);    /* possible submessage alignment */
    m_typeSize = static_cast<uint32_t>(type_size) + 4; /*encapsulation*/
    m_isGetKeyDefined = HelloWorld::isKeyDefined();
    size_t keyLength = HelloWorld::getKeyMaxCdrSerializedSize() > 16 ? HelloWorld::getKeyMaxCdrSerializedSize() : 16;
    m_keyBuffer = reinterpret_cast<unsigned char*>(malloc(keyLength));
    memset(m_keyBuffer, 0, keyLength);
    setIdlCrc16("847E");
}

HelloWorldPubSubType::~HelloWorldPubSubType() {
    if (m_keyBuffer != nullptr) {
        free(m_keyBuffer);
    }
}

bool HelloWorldPubSubType::serialize(void* data, vbs::SerializedPayload_t* payload) {
    HelloWorld* p_type = static_cast<HelloWorld*>(data);

    // Object that manages the raw buffer.
    vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(payload->data), payload->max_size);
    // Object that serializes the data.
    vbs::Cdr ser(vbsbuffer, vbs::Cdr::DEFAULT_ENDIAN, vbs::Cdr::DDS_CDR);
    payload->encapsulation = ser.endianness() == vbs::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
    // Serialize encapsulation
    ser.serialize_encapsulation();

    try {
        // Serialize the object.
        p_type->serialize(ser);
    } catch (vbs::NotEnoughMemoryException& e) {
        VbsLogE(e.what() << getName());
        return false;
    }

    // Get the serialized length
    payload->length = static_cast<uint32_t>(ser.getSerializedDataLength());
    return true;
}
bool HelloWorldPubSubType::deserialize(vbs::SerializedPayload_t* payload, void* data) {
    try {
        //Convert DATA to pointer of your type
        HelloWorld* p_type = static_cast<HelloWorld*>(data);

        // Object that manages the raw buffer.
        vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(payload->data), payload->length);

        // Object that deserializes the data.
        vbs::Cdr deser(vbsbuffer, vbs::Cdr::DEFAULT_ENDIAN, vbs::Cdr::DDS_CDR);

        // Deserialize encapsulation.
        deser.read_encapsulation();
        payload->encapsulation = deser.endianness() == vbs::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;

        // Deserialize the object.
        p_type->deserialize(deser);
    } catch (vbs::NotEnoughMemoryException& e) {
        VbsLogE(e.what() << getName());
        return false;
        ;
    }

    return true;
}
std::function<uint32_t()> HelloWorldPubSubType::getSerializedSizeProvider(void* data) {
    return [data]() -> uint32_t {
        return static_cast<uint32_t>(type::getCdrSerializedSize(*static_cast<HelloWorld*>(data))) +
               4u /*encapsulation*/;
    };
}
void* HelloWorldPubSubType::createData() {
    return reinterpret_cast<void*>(new HelloWorld());
}

void HelloWorldPubSubType::deleteData(void* data) {
    delete (reinterpret_cast<HelloWorld*>(data));
}

bool HelloWorldPubSubType::getKey(void* data, vbs::InstanceHandle_t* handle, bool force_md5) {
    if (!m_isGetKeyDefined) {
        return false;
    }

    std::cout << "HelloWorldPubSubType::getKey " << std::endl;
    HelloWorld* p_type = static_cast<HelloWorld*>(data);

    // Object that manages the raw buffer.
    vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(m_keyBuffer), HelloWorld::getKeyMaxCdrSerializedSize());

    // Object that serializes the data.
    vbs::Cdr ser(vbsbuffer, vbs::Cdr::BIG_ENDIANNESS);
    p_type->serializeKey(ser);

    if (force_md5 || HelloWorld::getKeyMaxCdrSerializedSize() > 16) {
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
namespace vbs {

bool topic_type_support<HelloWorld>::ToBuffer(const HelloWorld& sample, std::vector<char>& buffer) {
    HelloWorldPubSubType subType;
    vbs::SerializedPayload_t payload;
    uint32_t len = subType.getSerializedSizeProvider(const_cast<HelloWorld*>(&sample))();
    buffer.resize(len);
    payload.data = reinterpret_cast<vbs::octet*>(buffer.data());
    payload.max_size = len;
    const bool res = subType.serialize(static_cast<void*>(const_cast<HelloWorld*>(&sample)), &payload);

    payload.data = nullptr;
    return res;
}

bool topic_type_support<HelloWorld>::FromBuffer(HelloWorld& sample, const std::vector<char>& buffer) {
    HelloWorldPubSubType subType;
    vbs::SerializedPayload_t payload;
    payload.data = reinterpret_cast<vbs::octet*>(const_cast<char*>(buffer.data()));
    payload.length = buffer.size();
    payload.max_size = buffer.size();
    const bool res = subType.deserialize(&payload, static_cast<void*>(&sample));
    payload.data = nullptr;
    return res;
}

std::string topic_type_support<HelloWorld>::data_to_json(const HelloWorld& sample, std::ostringstream* o_ptr,
                                                         const bool is_need_delete, const bool include_braces) {
    (void)is_need_delete;
    (void)include_braces;
    (void)o_ptr;
    return vbs::data_to_json_string(sample);
}
}  // namespace vbs
