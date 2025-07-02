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

#include "vbs/types/VBSDynamicTypeMember.h"
#include "topic/DynamicPubSubTypeImpl.hpp"
#include "core/Global.hpp"
#include "ertps/types/DynamicDataFactory.h"
#include "core/Global.hpp"

namespace vbs {

VBSDynamicTypeMember::VBSDynamicTypeMember() : dynamic_type_member_(nullptr) {}

VBSDynamicTypeMember::VBSDynamicTypeMember(const VBSDynamicTypeMember& other) {
    dynamic_type_member_ = other.dynamic_type_member_;
}

VBSDynamicTypeMember& VBSDynamicTypeMember::operator=(VBSDynamicTypeMember&& other) {
    dynamic_type_member_ = other.dynamic_type_member_;
    return *this;
}

VBSDynamicTypeMember::~VBSDynamicTypeMember() {}

std::string VBSDynamicTypeMember::get_name() const {
    if (dynamic_type_member_ == nullptr) {
        return "";
    }
    return dynamic_type_member_->get_descriptor()->get_name();
}

uint32_t VBSDynamicTypeMember::get_id() const {
    if (dynamic_type_member_ == nullptr) {
        return VBS_MEMBER_ID_INVALID;
    }
    return dynamic_type_member_->get_descriptor()->get_id();
}

TypeKind VBSDynamicTypeMember::get_kind() const {
    if (dynamic_type_member_ == nullptr) {
        return TypeKind::TK_NONE;
    }
    return VBSDynamicType::ToVbsTypeKind(dynamic_type_member_->get_descriptor()->get_kind());
}

uint32_t VBSDynamicTypeMember::get_index() const {
    if (dynamic_type_member_ == nullptr) {
        return 0;
    }
    return dynamic_type_member_->get_descriptor()->get_index();
}

VBSDynamicType VBSDynamicTypeMember::get_type() const {
    VBSDynamicType type;
    if (dynamic_type_member_ != nullptr) {
        type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_member_->get_descriptor()->get_type());
    }
    return type;
}

std::vector<uint64_t> VBSDynamicTypeMember::get_union_labels() const {
    if (dynamic_type_member_ == nullptr) {
        return std::vector<uint64_t>();
    }
    return dynamic_type_member_->get_descriptor()->get_union_labels();
}

uint16_t VBSDynamicTypeMember::annotation_get_bit_bound() const {
    if (dynamic_type_member_ == nullptr) {
        return 0;
    }
    return dynamic_type_member_->get_descriptor()->annotation_get_bit_bound();
}

uint16_t VBSDynamicTypeMember::annotation_get_position() const {
    if (dynamic_type_member_ == nullptr) {
        return 0;
    }
    return dynamic_type_member_->get_descriptor()->annotation_get_position();
}

std::string VBSDynamicTypeMember::get_default_value() const {
    if (dynamic_type_member_ == nullptr) {
        return "";
    }
    return dynamic_type_member_->get_descriptor()->get_default_value();
}

uint32_t VBSDynamicTypeMember::get_cdr_id() const {
    if (dynamic_type_member_ == nullptr) {
        return VBS_MEMBER_ID_INVALID;
    }
    return dynamic_type_member_->get_cdr_id();
}

bool VBSDynamicTypeMember::annotation_is_optional() const {
    if (dynamic_type_member_ == nullptr) {
        return false;
    }
    return dynamic_type_member_->get_descriptor()->annotation_is_optional();
}

bool VBSDynamicTypeMember::annotation_is_key() const {
    if (dynamic_type_member_ == nullptr) {
        return false;
    }
    return dynamic_type_member_->get_descriptor()->annotation_is_key();
}

}  // namespace vbs
