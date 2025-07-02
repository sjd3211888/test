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

#include "ertps/types/DynamicTypeBuilderFactory.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "vbs/types/VBSDynamicTypeBuilderFactory.h"
#include "core/Global.hpp"
#include "vbs/Utils.hpp"

namespace vbs {

VBSDynamicTypeBuilderFactory::VBSDynamicTypeBuilderFactory() {
    dynamic_type_builder_factory_ = evbs::ertps::types::DynamicTypeBuilderFactory::get_instance();
}

VBSDynamicTypeBuilderFactory::~VBSDynamicTypeBuilderFactory() {
    evbs::ertps::types::DynamicTypeBuilderFactory::delete_instance();
}

static VBSDynamicTypeBuilderFactory* instance_ = nullptr;
VBSDynamicTypeBuilderFactory* VBSDynamicTypeBuilderFactory::get_instance() {
    if (instance_ == nullptr) {
        instance_ = new VBSDynamicTypeBuilderFactory();
    }
    return instance_;
}

ReturnCode_t VBSDynamicTypeBuilderFactory::delete_instance() {
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
}

VBSDynamicType VBSDynamicTypeBuilderFactory::get_primitive_type(TypeKind kind) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(
        dynamic_type_builder_factory_->get_primitive_type(VBSDynamicType::ToEvbsTypeKind(kind)));
    return type;
}

ReturnCode_t VBSDynamicTypeBuilderFactory::delete_builder(VBSDynamicTypeBuilder builder) {
    dynamic_type_builder_factory_->delete_builder(builder.dynamic_type_builder_);
    builder.dynamic_type_builder_ = nullptr;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

ReturnCode_t VBSDynamicTypeBuilderFactory::delete_type(VBSDynamicType type) {
    DynamicType_ptr& dynamic_type_ = *(type.dynamic_type_);
    dynamic_type_builder_factory_->delete_type(dynamic_type_.get());
    type.dynamic_type_ = nullptr;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_builder_copy(const VBSDynamicTypeBuilder& type) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_builder_copy(type.dynamic_type_builder_);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_int32_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_int32_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_uint32_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_uint32_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_int16_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_int16_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_uint16_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_uint16_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_int64_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_int64_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_uint64_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_uint64_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_float32_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_float32_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_float64_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_float64_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_float128_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_float128_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_char8_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_char8_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_char16_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_char16_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_bool_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_bool_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_byte_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_byte_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_string_builder(uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_string_builder(bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_wstring_builder(uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_wstring_builder(bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_sequence_builder(const VBSDynamicTypeBuilder& element_type,
                                                                            uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_sequence_builder(element_type.dynamic_type_builder_, bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_sequence_builder(const VBSDynamicType& type,
                                                                            uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_sequence_builder(*(type.dynamic_type_), bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_array_builder(const VBSDynamicTypeBuilder& element_type,
                                                                         const std::vector<uint32_t>& bounds) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_array_builder(element_type.dynamic_type_builder_, bounds);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_array_builder(const VBSDynamicType& type,
                                                                         const std::vector<uint32_t>& bounds) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_array_builder(*(type.dynamic_type_), bounds);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_map_builder(VBSDynamicTypeBuilder key_element_type,
                                                                       VBSDynamicTypeBuilder element_type,
                                                                       uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_map_builder(
        key_element_type.dynamic_type_builder_, element_type.dynamic_type_builder_, bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_map_builder(VBSDynamicType key_type,
                                                                       VBSDynamicType value_type, uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_map_builder(
        *(key_type.dynamic_type_), *(value_type.dynamic_type_), bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_bitmask_builder(uint32_t bound) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_bitmask_builder(bound);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_bitset_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_bitset_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_alias_builder(VBSDynamicTypeBuilder base_type,
                                                                         const std::string& sName) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_alias_builder(base_type.dynamic_type_builder_, sName);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_alias_builder(VBSDynamicType base_type,
                                                                         const std::string& sName) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_alias_builder(*(base_type.dynamic_type_), sName);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_enum_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_enum_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_struct_builder() {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ = dynamic_type_builder_factory_->create_struct_builder();
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_child_struct_builder(VBSDynamicTypeBuilder parent_type) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_child_struct_builder(parent_type.dynamic_type_builder_);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_union_builder(VBSDynamicTypeBuilder discriminator_type) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_union_builder(discriminator_type.dynamic_type_builder_);
    return builder;
}

VBSDynamicTypeBuilder VBSDynamicTypeBuilderFactory::create_union_builder(VBSDynamicType discriminator_type) {
    VBSDynamicTypeBuilder builder;
    builder.dynamic_type_builder_ =
        dynamic_type_builder_factory_->create_union_builder(*(discriminator_type.dynamic_type_));
    return builder;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_annotation_primitive(const std::string& name) {
    VBSDynamicType type;
    type.dynamic_type_ =
        std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_annotation_primitive(name));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_type(const VBSDynamicTypeBuilder& other) {
    VBSDynamicType type;
    type.dynamic_type_ =
        std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_type(other.dynamic_type_builder_));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_alias_type(VBSDynamicTypeBuilder base_type,
                                                               const std::string& sName) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(
        dynamic_type_builder_factory_->create_alias_type(base_type.dynamic_type_builder_, sName));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_alias_type(VBSDynamicType base_type, const std::string& sName) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(
        dynamic_type_builder_factory_->create_alias_type(*(base_type.dynamic_type_), sName));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_int32_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_int32_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_uint32_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_uint32_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_int16_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_int16_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_uint16_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_uint16_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_int64_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_int64_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_uint64_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_uint64_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_float32_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_float32_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_float64_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_float64_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_float128_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_float128_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_char8_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_char8_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_char16_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_char16_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_bool_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_bool_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_byte_type() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_byte_type());
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_string_type(uint32_t bound) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_string_type(bound));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_wstring_type(uint32_t bound) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_wstring_type(bound));
    return type;
}

VBSDynamicType VBSDynamicTypeBuilderFactory::create_bitset_type(uint32_t bound) {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_factory_->create_bitset_type(bound));
    return type;
}

}  // namespace vbs
