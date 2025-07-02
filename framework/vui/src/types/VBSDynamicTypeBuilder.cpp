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

#include "ertps/types/DynamicDataFactory.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "vbs/types/VBSDynamicTypeBuilder.h"
#include "core/Global.hpp"
#include "vbs/Utils.hpp"

namespace vbs {

VBSDynamicTypeBuilder::VBSDynamicTypeBuilder() {
    dynamic_type_builder_ = nullptr;
}

VBSDynamicTypeBuilder::VBSDynamicTypeBuilder(const VBSDynamicTypeBuilder& builder) {
    dynamic_type_builder_ = builder.dynamic_type_builder_;
}

VBSDynamicTypeBuilder::~VBSDynamicTypeBuilder() {}

ReturnCode_t VBSDynamicTypeBuilder::add_empty_member(uint32_t id, const std::string& name) {
    return ReturnCode_t(dynamic_type_builder_->add_empty_member(id, name)());
}

ReturnCode_t VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type) {
    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, type.dynamic_type_builder_)());
}

ReturnCode_t VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type,
                                               const std::string& defaultValue) {
    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, type.dynamic_type_builder_, defaultValue)());
}

ReturnCode_t VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type,
                                               const std::string& defaultValue,
                                               const std::vector<uint64_t>& unionLabels, bool isDefaultLabel) {
    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, type.dynamic_type_builder_, defaultValue,
                                                          unionLabels, isDefaultLabel)());
}

ReturnCode_t VBSDynamicTypeBuilder::VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name,
                                                                      VBSDynamicType type = VBSDynamicType()) {

    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, *(type.dynamic_type_))());
}

ReturnCode_t VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name, VBSDynamicType type,
                                               const std::string& defaultValue) {
    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, *(type.dynamic_type_), defaultValue)());
}

ReturnCode_t VBSDynamicTypeBuilder::add_member(uint32_t id, const std::string& name, VBSDynamicType type,
                                               const std::string& defaultValue,
                                               const std::vector<uint64_t>& unionLabels, bool isDefaultLabel) {
    return ReturnCode_t(dynamic_type_builder_->add_member(id, name, *(type.dynamic_type_), defaultValue, unionLabels,
                                                          isDefaultLabel)());
}

VBSDynamicType VBSDynamicTypeBuilder::build() {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dynamic_type_builder_->build());
    return type;
}

ReturnCode_t VBSDynamicTypeBuilder::copy_from(const VBSDynamicTypeBuilder& other) {
    return ReturnCode_t(dynamic_type_builder_->copy_from(other.dynamic_type_builder_)());
}

TypeKind VBSDynamicTypeBuilder::get_kind() const {
    return VBSDynamicType::ToVbsTypeKind(dynamic_type_builder_->get_kind());
}

std::string VBSDynamicTypeBuilder::get_name() const {
    return dynamic_type_builder_->get_name();
}

uint32_t VBSDynamicTypeBuilder::get_member_id_by_name(const std::string& name) const {
    return dynamic_type_builder_->get_member_id_by_name(name);
}

bool VBSDynamicTypeBuilder::is_consistent() const {
    return dynamic_type_builder_->is_consistent();
}

bool VBSDynamicTypeBuilder::is_discriminator_type() const {
    return dynamic_type_builder_->is_discriminator_type();
}

ReturnCode_t VBSDynamicTypeBuilder::set_name(const std::string& name) {
    return ReturnCode_t(dynamic_type_builder_->set_name(name)());
}

}  // namespace vbs
