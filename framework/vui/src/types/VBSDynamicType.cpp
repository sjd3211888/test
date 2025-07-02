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

#include "ertps/xmlparser/XMLProfileManager.h"
#include "vbs/types/VBSDynamicType.h"
#include "vbs/types/VBSDynamicTypeBuilder.h"
#include "vbs/types/VBSDynamicTypeMember.h"
#include "vbs/types/GenerateXML.h"
#include "vbs/types/VBSGenerateIDL.h"
#include "topic/DynamicPubSubTypeImpl.hpp"
#include "core/Global.hpp"
#include "vbs/Utils.hpp"

namespace vbs {

VBSDynamicType::VBSDynamicType() {
    dynamic_type_ = std::make_shared<DynamicType_ptr>();
}

VBSDynamicType::VBSDynamicType(const VBSDynamicType& type) {
    dynamic_type_ = type.dynamic_type_;
}

VBSDynamicType& VBSDynamicType::operator=(const VBSDynamicType& data) {
    dynamic_type_ = data.dynamic_type_;
    return *this;
}

VBSDynamicType::~VBSDynamicType() {}

TypeKind VBSDynamicType::ToVbsTypeKind(EvbsTypeKind kind) {
    return static_cast<TypeKind>(kind);
}

EvbsTypeKind VBSDynamicType::ToEvbsTypeKind(TypeKind kind) {
    return static_cast<EvbsTypeKind>(kind);
}

EvbsTypeSupport VBSDynamicType::type() const {
    return EvbsTypeSupport(new vbs::VbsDynamicPubSubTypeImpl(*dynamic_type_));
}

void VBSDynamicType::get_all_members(std::map<uint32_t, std::shared_ptr<VBSDynamicTypeMember>>& members) const {
    std::map<uint32_t, EvbsDynamicTypeMember*> evbs_members;
    (*dynamic_type_)->get_all_members(evbs_members);
    for (const auto& member : evbs_members) {
        std::shared_ptr<VBSDynamicTypeMember> m = std::make_shared<VBSDynamicTypeMember>();
        m->dynamic_type_member_ = member.second;
        members[member.first] = std::move(m);
    }
}

void VBSDynamicType::get_all_members_by_name(
    std::map<std::string, std::shared_ptr<VBSDynamicTypeMember>>& members) const {
    std::map<std::string, EvbsDynamicTypeMember*> evbs_members;
    (*dynamic_type_)->get_all_members_by_name(evbs_members);
    for (const auto& member : evbs_members) {
        std::shared_ptr<VBSDynamicTypeMember> m = std::make_shared<VBSDynamicTypeMember>();
        m->dynamic_type_member_ = member.second;
        members[member.first] = std::move(m);
    }
}

VBSDynamicType VBSDynamicType::member_type(uint32_t memberId) const {
    VBSDynamicType member_type;
    std::map<uint32_t, std::shared_ptr<VBSDynamicTypeMember>> members;
    get_all_members(members);
    auto it = members.find(memberId);
    if (it == members.end()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << " can't get member_type with member id - " << memberId);
        return member_type;
    }
    std::shared_ptr<VBSDynamicTypeMember> member = it->second;
    member_type = (it->second)->get_type();
    return member_type;
}

TypeKind VBSDynamicType::get_kind() const {
    return VBSDynamicType::ToVbsTypeKind((*dynamic_type_)->get_kind());
}

std::string VBSDynamicType::get_name() const {
    return (*dynamic_type_)->get_name();
}

std::string VBSDynamicType::get_descriptor_name() const {
    return (*dynamic_type_)->get_descriptor()->get_name();
}

uint32_t VBSDynamicType::get_members_count() const {
    return (*dynamic_type_)->get_members_count();
}

bool VBSDynamicType::has_children() const {
    return (*dynamic_type_)->has_children();
}

bool VBSDynamicType::is_consistent() const {
    return (*dynamic_type_)->is_consistent();
}

bool VBSDynamicType::is_complex_kind() const {
    return (*dynamic_type_)->is_complex_kind();
}

uint32_t VBSDynamicType::get_total_bounds() const {
    return (*dynamic_type_)->get_total_bounds();
}

uint32_t VBSDynamicType::get_bounds(uint32_t index) const {
    return (*dynamic_type_)->get_bounds(index);
}

uint32_t VBSDynamicType::get_bounds_size() const {
    return (*dynamic_type_)->get_bounds_size();
}

VBSDynamicType VBSDynamicType::get_base_type() const {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>((*dynamic_type_)->get_base_type());
    return type;
}

VBSDynamicType VBSDynamicType::get_element_type() const {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>((*dynamic_type_)->get_element_type());
    return type;
}

VBSDynamicType VBSDynamicType::get_key_element_type() const {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>((*dynamic_type_)->get_key_element_type());
    return type;
}

VBSDynamicType VBSDynamicType::get_discriminator_type() const {
    VBSDynamicType type;
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>((*dynamic_type_)->get_discriminator_type());
    return type;
}

uint16_t VBSDynamicType::bitmask_bit_bound() const {
    return (*dynamic_type_)->get_descriptor()->get_bounds();
}

bool VBSDynamicType::has_base_type() const {
    DynamicType_ptr type = (*dynamic_type_)->get_base_type();
    return (type != nullptr);
}

void VBSDynamicType::generate_xml(const std::string& file_name) const {
    vbs::GenerateXML generate_xml;
    generate_xml.parse_dynamic(*this);
    generate_xml.save(file_name);
}

std::string VBSDynamicType::generate_xml() const {
    vbs::GenerateXML generate_xml;
    generate_xml.parse_dynamic(*this);
    return generate_xml.get();
}

bool VBSDynamicType::is_valid() const {
    return (*dynamic_type_) != nullptr;
}

void VBSDynamicType::print_idl(vbs::VBSDynamicType& type, std::string& idl) {
    vbs::VBSGenerateIDL generate_idl;
    generate_idl.print_idl(type, idl);
}

VBSDynamicType VBSDynamicType::getDynamicTypeFromXml(const std::string& xml_file, const std::string& type_name) {
    VBSDynamicType dynamic_type;
    evbs::ertps::xmlparser::XMLProfileManager::loadXMLFile(xml_file, false);
    EvbsDynamicType_ptr dyn_type = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name)->build();
    dynamic_type.dynamic_type_ = std::make_shared<EvbsDynamicType_ptr>(dyn_type);
    return dynamic_type;
}

size_t VBSDynamicType::member_count() const {
    std::map<uint32_t, EvbsDynamicTypeMember*> evbs_members;
    (*dynamic_type_)->get_all_members(evbs_members);
    return evbs_members.size();
}

bool VBSDynamicType::operator==(const VBSDynamicType& data) const {
    if (dynamic_type_ && *dynamic_type_ && data.dynamic_type_ && *data.dynamic_type_) {
        return (*dynamic_type_)->equals(data.dynamic_type_->get());
    }
    return false;
}

bool VBSDynamicType::operator!=(const VBSDynamicType& data) const {
    return !(*this == data);
}

bool VBSDynamicType::annotation_is_mutable() const {
    return (*dynamic_type_)->get_descriptor()->annotation_is_mutable();
}

bool VBSDynamicType::annotation_is_appendable() const {
    return (*dynamic_type_)->get_descriptor()->annotation_is_appendable();
}

bool VBSDynamicType::annotation_is_final() const {
    return (*dynamic_type_)->get_descriptor()->annotation_is_final();
}

}  // namespace vbs
