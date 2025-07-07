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

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace {
char dummy;
}
#endif

#include "Client.hpp"
#include "ClientTypeObject.hpp"
#include <utility>
#include <sstream>
#include <ertps/rtps/common/SerializedPayload.h>
#include <deps/common/md5.h>
#include <ertps/types/TypeObjectFactory.h>
#include <ertps/types/TypeNamesGenerator.h>
#include <ertps/types/AnnotationParameterValue.h>
#include <ecdr/FastBuffer.h>
#include <ecdr/Cdr.h>

using namespace evbs;
using namespace evbs::ertps::rtps;

void registerClientTypes() {
    TypeObjectFactory* factory = TypeObjectFactory::get_instance();
    factory->add_type_object("Client", GetClientIdentifier(), GetClientObject());
}

const TypeIdentifier* GetClientIdentifier() {
    const TypeIdentifier* c_identifier = TypeObjectFactory::get_instance()->get_type_identifier("Client");
    if (c_identifier != nullptr && (c_identifier->_d() == EK_COMPLETE)) {
        return c_identifier;
    }

    GetClientObject();  // Generated inside
    return TypeObjectFactory::get_instance()->get_type_identifier("Client");
}

const TypeObject* GetClientObject() {
    const TypeObject* c_type_object = TypeObjectFactory::get_instance()->get_type_object("Client");
    if (c_type_object != nullptr) {
        return c_type_object;
    }
    return GetCompleteClientObject();
}

const TypeObject* GetCompleteClientObject() {
    const TypeObject* c_type_object = TypeObjectFactory::get_instance()->get_type_object("Client");
    if (c_type_object != nullptr && c_type_object->_d() == EK_COMPLETE) {
        return c_type_object;
    }

    TypeObject* type_object = new TypeObject();
    type_object->_d(EK_COMPLETE);
    type_object->complete()._d(TK_STRUCTURE);

    type_object->complete().struct_type().struct_flags().IS_FINAL(false);
    type_object->complete().struct_type().struct_flags().IS_APPENDABLE(false);
    type_object->complete().struct_type().struct_flags().IS_MUTABLE(false);
    type_object->complete().struct_type().struct_flags().IS_NESTED(false);
    type_object->complete().struct_type().struct_flags().IS_AUTOID_HASH(false);  // Unsupported

    MemberId memberId = 0;
    CompleteStructMember cst_id;
    cst_id.common().member_id(memberId++);
    cst_id.common().member_flags().TRY_CONSTRUCT1(false);  // Unsupported
    cst_id.common().member_flags().TRY_CONSTRUCT2(false);  // Unsupported
    cst_id.common().member_flags().IS_EXTERNAL(false);     // Unsupported
    cst_id.common().member_flags().IS_OPTIONAL(false);
    cst_id.common().member_flags().IS_MUST_UNDERSTAND(false);
    cst_id.common().member_flags().IS_KEY(false);
    cst_id.common().member_flags().IS_DEFAULT(false);  // Doesn't apply
    cst_id.common().member_type_id(*TypeObjectFactory::get_instance()->get_type_identifier("uint32_t"));

    cst_id.detail().name("id");

    type_object->complete().struct_type().member_seq().emplace_back(cst_id);

    CompleteStructMember cst_msg;
    cst_msg.common().member_id(memberId++);
    cst_msg.common().member_flags().TRY_CONSTRUCT1(false);  // Unsupported
    cst_msg.common().member_flags().TRY_CONSTRUCT2(false);  // Unsupported
    cst_msg.common().member_flags().IS_EXTERNAL(false);     // Unsupported
    cst_msg.common().member_flags().IS_OPTIONAL(false);
    cst_msg.common().member_flags().IS_MUST_UNDERSTAND(false);
    cst_msg.common().member_flags().IS_KEY(false);
    cst_msg.common().member_flags().IS_DEFAULT(false);  // Doesn't apply
    cst_msg.common().member_type_id(*TypeObjectFactory::get_instance()->get_string_identifier(64, false));

    cst_msg.detail().name("msg");

    type_object->complete().struct_type().member_seq().emplace_back(cst_msg);

    // Header
    type_object->complete().struct_type().header().detail().type_name("Client");
    // TODO inheritance

    TypeIdentifier identifier;
    identifier._d(EK_COMPLETE);

    SerializedPayload_t payload(
        static_cast<uint32_t>(CompleteStructType::getCdrSerializedSize(type_object->complete().struct_type()) + 4));
    vbsutil::ecdr::FastBuffer fastbuffer((char*)payload.data, payload.max_size);
    // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for DDS document)
    vbsutil::ecdr::Cdr ser(fastbuffer, vbsutil::ecdr::Cdr::LITTLE_ENDIANNESS,
                           vbsutil::ecdr::Cdr::DDS_CDR);  // Object that serializes the data.
    payload.encapsulation = CDR_LE;

    type_object->serialize(ser);
    payload.length = (uint32_t)ser.getSerializedDataLength();  // Get the serialized length
    vbsutil::MD5 objectHash;
    objectHash.update((char*)payload.data, payload.length);
    objectHash.finalize();
    for (int i = 0; i < 14; ++i) {
        identifier.equivalence_hash()[i] = objectHash.digest[i];
    }

    TypeObjectFactory::get_instance()->add_type_object("Client", &identifier, type_object);
    delete type_object;
    return TypeObjectFactory::get_instance()->get_type_object("Client");
}
