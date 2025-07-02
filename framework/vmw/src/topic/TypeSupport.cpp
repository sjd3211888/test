// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "deps/common/TypeSupport.hpp"
#include "ertps/types/DynamicPubSubType.h"
#include "ertps/types/TypesBase.h"
#include "domain/DomainParticipantImpl.hpp"
#include "ecdr/exceptions/Exception.h"

namespace vbs {

const InstanceHandle_t HANDLE_NIL;

TypeSupport::TypeSupport(const evbs::ertps::types::DynamicPubSubType& ptr)
    : std::shared_ptr<vbs::TopicDataType>(std::make_shared<evbs::ertps::types::DynamicPubSubType>(ptr)) {}

vbsutil::elog::ReturnCode_t TypeSupport::register_type(DomainParticipantImpl* participant,
                                                       std::string type_name) const {
    return participant->register_type(*this, type_name.empty() ? get_type_name() : type_name);
}

vbsutil::elog::ReturnCode_t TypeSupport::register_type(DomainParticipantImpl* participant) const {
    return participant->register_type(*this, get_type_name());
}

bool TypeSupport::serialize(void* data, vbsutil::xmlparser::SerializedPayload_t* payload) {
    bool result = false;
    try {
        result = get()->serialize(data, payload);
    } catch (vbsutil::ecdr::exception::Exception&) {
        result = false;
    }

    return result;
}

bool TypeSupport::deserialize(vbsutil::xmlparser::SerializedPayload_t* payload, void* data) {
    bool result = false;
    try {
        result = get()->deserialize(payload, data);
    } catch (vbsutil::ecdr::exception::Exception&) {
        result = false;
    }

    return result;
}

}  // namespace vbs
