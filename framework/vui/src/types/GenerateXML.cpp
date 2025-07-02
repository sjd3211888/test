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

#include "vbs/types/GenerateXML.h"
#include "vbs/types/VBSDynamicTypeMember.h"
#include "core/Global.hpp"

namespace vbs {

GenerateXML::GenerateXML() {
    types_ = xmlDoc_.NewElement("types");
    xmlDoc_.InsertFirstChild(types_);
    root_node_ = new GenerateXMLNode("vbs_root");
    all_nodes_["vbs_root"] = root_node_;
    struct_elements_["vbs_root"] = types_;
}

GenerateXML::~GenerateXML() {
    for (auto n = all_nodes_.begin(); n != all_nodes_.end(); n++) {
        delete n->second;
    }
    all_nodes_.clear();
    struct_elements_.clear();
}

tinyxml2::XMLElement* GenerateXML::create_struct(const std::string& name) {
    GenerateXMLNode* node = get_node(name);
    root_node_->next_nodes_.insert(node);
    node->pre_nodes_.insert(root_node_);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* type = xmlDoc_.NewElement("type");
    types_->InsertFirstChild(type);
    tinyxml2::XMLElement* struct_type = xmlDoc_.NewElement("struct");
    struct_type->SetAttribute("name", name.c_str());
    type->InsertEndChild(struct_type);
    struct_elements_[name] = type;

    return struct_type;
}

tinyxml2::XMLElement* GenerateXML::create_struct(const std::string& name, const std::string& base_name) {
    GenerateXMLNode* node = get_node(name);
    root_node_->next_nodes_.insert(node);
    node->pre_nodes_.insert(root_node_);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* type = xmlDoc_.NewElement("type");
    types_->InsertFirstChild(type);
    tinyxml2::XMLElement* struct_type = xmlDoc_.NewElement("struct");
    struct_type->SetAttribute("name", name.c_str());
    struct_type->SetAttribute("baseType", base_name.c_str());
    type->InsertEndChild(struct_type);
    struct_elements_[name] = type;

    return struct_type;
}

tinyxml2::XMLElement* GenerateXML::create_struct(const std::string& name, const std::string& base_name,
                                                 const std::string& extensibility) {

    GenerateXMLNode* node = get_node(name);
    root_node_->next_nodes_.insert(node);
    node->pre_nodes_.insert(root_node_);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* type = xmlDoc_.NewElement("type");
    types_->InsertFirstChild(type);
    tinyxml2::XMLElement* struct_type = xmlDoc_.NewElement("struct");
    struct_type->SetAttribute("name", name.c_str());
    struct_type->SetAttribute("extensibility", extensibility.c_str());
    if (base_name.length() != 0) {
        struct_type->SetAttribute("baseType", base_name.c_str());
    }
    type->InsertEndChild(struct_type);
    struct_elements_[name] = type;

    return struct_type;
}

void GenerateXML::insert_to_struct(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                                   const std::string& nonBasicTypeName, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* member_type = xmlDoc_.NewElement("member");
    member_type->SetAttribute("name", name.c_str());
    member_type->SetAttribute("type", type.c_str());
    member_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        member_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        member_type->SetAttribute("optional", "true");
    }
    if (key) {
        member_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(member_type);
}

tinyxml2::XMLElement* GenerateXML::insert_union(const std::string& name, const std::string& type,
                                                const std::string& type_struct_name) {

    GenerateXMLNode* node = get_node(name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* union_type = xmlDoc_.NewElement("union");
    union_type->SetAttribute("name", name.c_str());
    local_type->InsertEndChild(union_type);

    tinyxml2::XMLElement* discriminator_type = xmlDoc_.NewElement("discriminator");
    discriminator_type->SetAttribute("type", type.c_str());
    union_type->InsertEndChild(discriminator_type);
    struct_elements_[name] = local_type;

    return union_type;
}

tinyxml2::XMLElement* GenerateXML::insert_union_element(tinyxml2::XMLElement* type_struct,
                                                        const std::vector<uint64_t>& value) {
    tinyxml2::XMLElement* case_type = xmlDoc_.NewElement("case");
    for (auto& i : value) {
        tinyxml2::XMLElement* discriminator_type = xmlDoc_.NewElement("caseDiscriminator");
        discriminator_type->SetAttribute("value", std::to_string(i).c_str());
        case_type->InsertEndChild(discriminator_type);
    }
    type_struct->InsertEndChild(case_type);
    return case_type;
}

void GenerateXML::insert_union_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                       const std::string& type, const std::string& stringMaxLength) {

    tinyxml2::XMLElement* member_type = xmlDoc_.NewElement("member");
    member_type->SetAttribute("name", name.c_str());
    member_type->SetAttribute("type", type.c_str());
    if ((type == "string") || (type == "wstring")) {
        member_type->SetAttribute("stringMaxLength", stringMaxLength.c_str());
    }
    type_struct->InsertEndChild(member_type);
}

void GenerateXML::insert_union_complex_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                               const std::string& type, const std::string& nonBasicTypeName) {

    tinyxml2::XMLElement* member_type = xmlDoc_.NewElement("member");
    member_type->SetAttribute("name", name.c_str());
    member_type->SetAttribute("type", type.c_str());
    member_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    type_struct->InsertEndChild(member_type);
}

tinyxml2::XMLElement* GenerateXML::insert_enum(const std::string& name, const std::string& type_struct_name) {
    GenerateXMLNode* node = get_node(name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* enmu_type = xmlDoc_.NewElement("enum");
    enmu_type->SetAttribute("name", name.c_str());
    local_type->InsertEndChild(enmu_type);
    struct_elements_[name] = local_type;

    return enmu_type;
}

void GenerateXML::insert_enum_element(tinyxml2::XMLElement* type_struct, const std::string& name, uint32_t index) {
    tinyxml2::XMLElement* enumerator_type = xmlDoc_.NewElement("enumerator");
    enumerator_type->SetAttribute("name", name.c_str());
    enumerator_type->SetAttribute("value", std::to_string((int32_t)index).c_str());
    type_struct->InsertEndChild(enumerator_type);
}

tinyxml2::XMLElement* GenerateXML::insert_bitset(std::string const& name, std::string const& type_struct_name,
                                                 std::string const& base_type) {
    GenerateXMLNode* const node = get_node(name);
    (void)all_nodes_[type_struct_name]->next_nodes_.insert(node);
    (void)node->pre_nodes_.insert(all_nodes_[type_struct_name]);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* const local_type = xmlDoc_.NewElement("type");
    (void)types_->InsertEndChild(local_type);
    (void)types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* const bitset_type = xmlDoc_.NewElement("bitset");
    (void)bitset_type->SetAttribute("name", name.c_str());
    (void)bitset_type->SetAttribute("baseType", base_type.c_str());
    (void)local_type->InsertEndChild(bitset_type);
    struct_elements_[name] = local_type;

    return bitset_type;
}

tinyxml2::XMLElement* GenerateXML::insert_bitset(const std::string& name, const std::string& type_struct_name) {
    GenerateXMLNode* node = get_node(name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* bitset_type = xmlDoc_.NewElement("bitset");
    bitset_type->SetAttribute("name", name.c_str());
    local_type->InsertEndChild(bitset_type);
    struct_elements_[name] = local_type;

    return bitset_type;
}

void GenerateXML::insert_bitset_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                        const std::string& type, const std::string& bit_bound) {
    tinyxml2::XMLElement* bitfield_type = xmlDoc_.NewElement("bitfield");
    bitfield_type->SetAttribute("name", name.c_str());
    bitfield_type->SetAttribute("type", type.c_str());
    bitfield_type->SetAttribute("bit_bound", bit_bound.c_str());
    type_struct->InsertEndChild(bitfield_type);
}

tinyxml2::XMLElement* GenerateXML::insert_bitmask(const std::string& name, const std::string& bit_bound,
                                                  const std::string& type_struct_name) {

    GenerateXMLNode* node = get_node(name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);
    if (struct_elements_.find(name) != struct_elements_.end()) {
        return nullptr;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* bitmask_type = xmlDoc_.NewElement("bitmask");
    bitmask_type->SetAttribute("name", name.c_str());
    bitmask_type->SetAttribute("bit_bound", bit_bound.c_str());
    local_type->InsertEndChild(bitmask_type);
    struct_elements_[name] = local_type;

    return bitmask_type;
}

void GenerateXML::insert_bitmask_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                         const std::string& position) {
    tinyxml2::XMLElement* bit_value = xmlDoc_.NewElement("bit_value");
    bit_value->SetAttribute("name", name.c_str());
    bit_value->SetAttribute("position", position.c_str());
    type_struct->InsertEndChild(bit_value);
}

void GenerateXML::insert_base(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                              const std::string& defalut_value, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    if (defalut_value.length() != 0) {
        base_type->SetAttribute("default", defalut_value.c_str());
    }
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_string(tinyxml2::XMLElement* type_struct, const std::string& name, uint32_t stringMaxLength,
                                bool is_string, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* string_type = xmlDoc_.NewElement("member");
    string_type->SetAttribute("name", name.c_str());
    if (is_string) {
        string_type->SetAttribute("type", "string");
    } else {
        string_type->SetAttribute("type", "wstring");
    }
    string_type->SetAttribute("stringMaxLength", std::to_string(stringMaxLength).c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        string_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        string_type->SetAttribute("optional", "true");
    }
    if (key) {
        string_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(string_type);
}

void GenerateXML::insert_array(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                               const std::string& arrayDimensions, const std::string& stringMaxLength, uint32_t id,
                               bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("arrayDimensions", arrayDimensions.c_str());
    if ((type == "string") || (type == "wstring")) {
        base_type->SetAttribute("stringMaxLength", stringMaxLength.c_str());
    }
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_array(tinyxml2::XMLElement* type_struct, const std::string& name,
                               const std::string& nonBasicTypeName, const std::string& arrayDimensions, uint32_t id,
                               bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", "nonBasic");
    base_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    base_type->SetAttribute("arrayDimensions", arrayDimensions.c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_seq(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                             uint32_t sequenceMaxLength, const std::string& stringMaxLength, uint32_t id, bool optional,
                             bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("sequenceMaxLength", std::to_string(sequenceMaxLength).c_str());
    if ((type == "string") || (type == "wstring")) {
        base_type->SetAttribute("stringMaxLength", stringMaxLength.c_str());
    }
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_seq_nobasic(tinyxml2::XMLElement* type_struct, const std::string& name,
                                     const std::string& type, uint32_t sequenceMaxLength,
                                     const std::string& nonBasicTypeName, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("sequenceMaxLength", std::to_string(sequenceMaxLength).c_str());
    base_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_map(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                             const std::string& key_type, uint32_t mapMaxLength, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("key_type", key_type.c_str());
    base_type->SetAttribute("mapMaxLength", std::to_string(mapMaxLength).c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_map(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                             const std::string& nonBasicTypeName, const std::string& key_type, uint32_t mapMaxLength,
                             uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    base_type->SetAttribute("key_type", key_type.c_str());
    base_type->SetAttribute("mapMaxLength", std::to_string(mapMaxLength).c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::insert_alias(const std::string& name, const std::string& type, const std::string& nonBasicTypeName,
                               const std::string& type_struct_name) {

    GenerateXMLNode* node = get_node(name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);

    node = get_node(nonBasicTypeName);
    all_nodes_[name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[name]);

    if (struct_elements_.find(name) != struct_elements_.end()) {
        return;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* alias_type = xmlDoc_.NewElement("typedef");
    alias_type->SetAttribute("name", name.c_str());
    alias_type->SetAttribute("type", type.c_str());
    alias_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    local_type->InsertEndChild(alias_type);
    struct_elements_[name] = local_type;
}

void GenerateXML::insert_alias(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                               const std::string& nonBasicTypeName, uint32_t id, bool optional, bool key) {

    tinyxml2::XMLElement* base_type = xmlDoc_.NewElement("member");
    base_type->SetAttribute("name", name.c_str());
    base_type->SetAttribute("type", type.c_str());
    base_type->SetAttribute("nonBasicTypeName", nonBasicTypeName.c_str());
    if (id != VBS_MEMBER_ID_INVALID) {
        base_type->SetAttribute("id", std::to_string(id).c_str());
    }
    if (optional) {
        base_type->SetAttribute("optional", "true");
    }
    if (key) {
        base_type->SetAttribute("key", "true");
    }
    type_struct->InsertEndChild(base_type);
}

void GenerateXML::sort() {
    uint32_t num = all_nodes_.size();
    for (uint32_t index = 0; index < num; index++) {
        for (auto n = all_nodes_.begin(); n != all_nodes_.end(); n++) {
            if (n->second->next_nodes_.size() == 0) {
                if (n->second->name_ != "vbs_root") {
                    types_->InsertEndChild(struct_elements_[n->second->name_]);
                    for (auto& nx : n->second->pre_nodes_) {
                        nx->next_nodes_.erase(n->second);
                    }
                }
                delete n->second;
                all_nodes_.erase(n);
                break;
            }
        }
        if (all_nodes_.size() == 0) {
            break;
        }
    }
}

void GenerateXML::save(const std::string& file_name) {
    sort();
    xmlDoc_.SaveFile(file_name.c_str());
}

std::string GenerateXML::get() {
    sort();
    tinyxml2::XMLPrinter printer;
    xmlDoc_.Print(&printer);
    return std::string(printer.CStr(), printer.CStrSize());
}

std::string GenerateXML::get_base_type(TypeKind kind) {
    switch (kind) {
        case vbs::TypeKind::TK_BOOLEAN:
            return "boolean";
        case vbs::TypeKind::TK_BYTE:
            return "uint8";
        case vbs::TypeKind::TK_INT8:
            return "int8";
        case vbs::TypeKind::TK_INT16:
            return "int16";
        case vbs::TypeKind::TK_INT32:
            return "int32";
        case vbs::TypeKind::TK_INT64:
            return "int64";
        case vbs::TypeKind::TK_UINT16:
            return "uint16";
        case vbs::TypeKind::TK_UINT32:
            return "uint32";
        case vbs::TypeKind::TK_UINT64:
            return "uint64";
        case vbs::TypeKind::TK_FLOAT32:
            return "float32";
        case vbs::TypeKind::TK_FLOAT64:
            return "float64";
        case vbs::TypeKind::TK_FLOAT128:
            return "float128";
        case vbs::TypeKind::TK_CHAR8:
            return "char8";
        case vbs::TypeKind::TK_CHAR16:
            return "char16";
        case vbs::TypeKind::TK_STRING8:
            return "string";
        case vbs::TypeKind::TK_STRING16:
            return "wstring";
        case vbs::TypeKind::TK_ALIAS:
            return "";
        case vbs::TypeKind::TK_ENUM:
            return "";
        case vbs::TypeKind::TK_BITMASK:
            return "";
        case vbs::TypeKind::TK_ANNOTATION:
            return "annotation";
        case vbs::TypeKind::TK_UNION:
            return "";
        case vbs::TypeKind::TK_BITSET:
            return "";
        default:
            return "";
    }
}

void GenerateXML::parse_dynamic(vbs::VBSDynamicType type, const std::string& name) {
    if (!type.is_valid()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " type \'" << name << "\' is invalid");
        return;
    }
    tinyxml2::XMLElement* local_type_xml = nullptr;
    tinyxml2::XMLElement* local_union_type_xml = nullptr;
    std::map<uint32_t, std::shared_ptr<vbs::VBSDynamicTypeMember>> union_members;
    std::shared_ptr<vbs::VBSDynamicTypeMember> um;
    vbs::VBSDynamicType union_local_type;
    std::vector<uint64_t> union_lable;
    GenerateXMLNode* node = nullptr;
    std::string extensibility = "";
    switch (type.get_kind()) {
        case vbs::TypeKind::TK_ENUM:
            union_members.clear();
            type.get_all_members(union_members);
            local_type_xml = insert_enum(type.get_descriptor_name(), name);
            if (local_type_xml == nullptr) {
                break;
            }
            for (const auto& union_member : union_members) {
                um = union_member.second;
                insert_enum_element(local_type_xml, um->get_name(), um->get_index());
            }
            break;
        case vbs::TypeKind::TK_BITMASK:
            union_members.clear();
            type.get_all_members(union_members);
            local_type_xml = insert_bitmask(type.get_name(), std::to_string(type.bitmask_bit_bound()), name);
            if (local_type_xml == nullptr) {
                break;
            }
            for (const auto& union_member : union_members) {
                um = union_member.second;
                insert_bitmask_element(local_type_xml, um->get_name(), std::to_string(um->annotation_get_position()));
            }
            break;
        case vbs::TypeKind::TK_STRUCTURE:
            if (has_struct(type.get_name())) {
                auto& node1 = all_nodes_[type.get_name()];
                all_nodes_[name]->next_nodes_.insert(node1);
                auto& node2 = all_nodes_[name];
                all_nodes_[type.get_name()]->pre_nodes_.insert(node2);
                return;
            }

            node = get_node(type.get_name());
            all_nodes_[name]->next_nodes_.insert(node);
            node->pre_nodes_.insert(all_nodes_[name]);

            if (type.annotation_is_mutable()) {
                extensibility = "mutable";
            }
            if (type.annotation_is_appendable()) {
                extensibility = "appendable";
            }
            if (type.annotation_is_final()) {
                extensibility = "final";
            }
            if (extensibility.length() != 0) {
                if (type.has_base_type()) {
                    local_type_xml = create_struct(type.get_name(), type.get_base_type().get_name(), extensibility);
                    parse_dynamic(type.get_base_type(), type.get_name());
                } else {
                    local_type_xml = create_struct(type.get_name(), "", extensibility);
                }
            } else {
                if (type.has_base_type()) {
                    local_type_xml = create_struct(type.get_name(), type.get_base_type().get_name());
                    parse_dynamic(type.get_base_type(), type.get_name());
                } else {
                    local_type_xml = create_struct(type.get_name());
                }
            }

            if (local_type_xml == nullptr) {
                return;
            }
            parse_dynamic_type(type, local_type_xml);
            break;
        case vbs::TypeKind::TK_UNION:
            union_members.clear();
            union_lable.clear();
            type.get_all_members(union_members);
            if (get_base_type(type.get_discriminator_type().get_kind()) != "") {
                local_type_xml =
                    insert_union(type.get_name(), get_base_type(type.get_discriminator_type().get_kind()), name);
            } else {
                local_type_xml = insert_union(type.get_name(), type.get_discriminator_type().get_name(), name);
            }
            if (local_type_xml == nullptr) {
                break;
            }
            for (const auto& union_member : union_members) {
                um = union_member.second;
                union_local_type = um->get_type();
                union_lable = um->get_union_labels();
                local_union_type_xml = insert_union_element(local_type_xml, um->get_union_labels());
                if (get_base_type(union_local_type.get_kind()) != "") {
                    insert_union_element(local_union_type_xml, um->get_name(),
                                         get_base_type(union_local_type.get_kind()),
                                         std::to_string(union_local_type.get_total_bounds()));
                } else {
                    insert_union_complex_element(local_union_type_xml, um->get_name(), "nonBasic",
                                                 union_local_type.get_name());
                    parse_dynamic(union_local_type, type.get_name());
                }
            }
            break;
        case vbs::TypeKind::TK_BITSET:
            union_members.clear();
            type.get_all_members(union_members);
            local_type_xml = insert_bitset(type.get_name(), name);
            if (local_type_xml == nullptr) {
                break;
            }
            for (const auto& union_member : union_members) {
                um = union_member.second;
                insert_bitset_element(local_type_xml, um->get_name(), get_base_type(um->get_type().get_kind()),
                                      std::to_string(um->annotation_get_bit_bound()));
            }
            break;
        case vbs::TypeKind::TK_ALIAS:
            parse_alias_type(type.get_base_type(), type.get_name(), name);
            break;
        default:
            break;
    }
}

void GenerateXML::parse_dynamic_type(vbs::VBSDynamicType type, tinyxml2::XMLElement* type_struct) {
    if (!type.is_valid()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " parse_dynamic_type type is invalid ");
        return;
    }
    std::map<uint32_t, std::shared_ptr<vbs::VBSDynamicTypeMember>> members_id;
    type.get_all_members(members_id);
    tinyxml2::XMLElement* local_type_xml = nullptr;
    tinyxml2::XMLElement* local_union_type_xml = nullptr;
    std::map<uint32_t, std::shared_ptr<vbs::VBSDynamicTypeMember>> union_members;
    std::shared_ptr<vbs::VBSDynamicTypeMember> um;
    vbs::VBSDynamicType union_local_type;
    std::vector<uint64_t> union_lable;
    std::string arrayDimensions = "";
    std::string val_type = "";
    std::string key_type = "";
    uint32_t mutable_id = VBS_MEMBER_ID_INVALID;

    for (const auto& member : members_id) {
        std::shared_ptr<vbs::VBSDynamicTypeMember> m = member.second;
        vbs::VBSDynamicType local_type = m->get_type();
        if (type.annotation_is_mutable()) {
            mutable_id = m->get_cdr_id();
        }
        switch (m->get_kind()) {
            case vbs::TypeKind::TK_BOOLEAN:
                insert_base(type_struct, m->get_name(), "boolean", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_BYTE:
                insert_base(type_struct, m->get_name(), "uint8", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_INT8:
                insert_base(type_struct, m->get_name(), "int8", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_INT16:
                insert_base(type_struct, m->get_name(), "int16", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_INT32:
                insert_base(type_struct, m->get_name(), "int32", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_INT64:
                insert_base(type_struct, m->get_name(), "int64", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_UINT16:
                insert_base(type_struct, m->get_name(), "uint16", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_UINT32:
                insert_base(type_struct, m->get_name(), "uint32", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_UINT64:
                insert_base(type_struct, m->get_name(), "uint64", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_FLOAT32:
                insert_base(type_struct, m->get_name(), "float32", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_FLOAT64:
                insert_base(type_struct, m->get_name(), "float64", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_FLOAT128:
                insert_base(type_struct, m->get_name(), "float128", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_CHAR8:
                insert_base(type_struct, m->get_name(), "char8", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_CHAR16:
                insert_base(type_struct, m->get_name(), "char16", m->get_default_value(), mutable_id,
                            m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_STRING8:
                insert_string(type_struct, m->get_name(), local_type.get_total_bounds(), true, mutable_id,
                              m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_STRING16:
                insert_string(type_struct, m->get_name(), local_type.get_total_bounds(), false, mutable_id,
                              m->annotation_is_optional(), m->annotation_is_key());
                break;
            case vbs::TypeKind::TK_ALIAS:
                insert_alias(type_struct, m->get_name(), "nonBasic", m->get_type().get_name(), mutable_id,
                             m->annotation_is_optional(), m->annotation_is_key());
                parse_alias_type(m->get_type().get_base_type(), m->get_type().get_name(), type.get_name());
                break;
            case vbs::TypeKind::TK_ENUM:
                union_members.clear();
                local_type.get_all_members(union_members);
                insert_to_struct(type_struct, m->get_name(), "nonBasic", m->get_type().get_descriptor_name(),
                                 mutable_id, m->annotation_is_optional(), m->annotation_is_key());
                local_type_xml = insert_enum(m->get_type().get_descriptor_name(), type.get_name());
                if (local_type_xml == nullptr) {
                    break;
                }
                for (const auto& union_member : union_members) {
                    um = union_member.second;
                    insert_enum_element(local_type_xml, um->get_name(), um->get_index());
                }
                break;
            case vbs::TypeKind::TK_BITMASK:
                union_members.clear();
                local_type.get_all_members(union_members);
                insert_to_struct(type_struct, m->get_name(), "nonBasic", m->get_type().get_name(), mutable_id,
                                 m->annotation_is_optional(), m->annotation_is_key());
                local_type_xml = insert_bitmask(m->get_type().get_name(),
                                                std::to_string(local_type.bitmask_bit_bound()), type.get_name());
                if (local_type_xml == nullptr) {
                    break;
                }
                for (const auto& union_member : union_members) {
                    um = union_member.second;
                    insert_bitmask_element(local_type_xml, um->get_name(),
                                           std::to_string(um->annotation_get_position()));
                }
                break;
            case vbs::TypeKind::TK_ANNOTATION:
                insert_base(type_struct, m->get_name(), "annotation", m->get_default_value());
                break;
            case vbs::TypeKind::TK_STRUCTURE:
                insert_to_struct(type_struct, m->get_name(), "nonBasic", m->get_type().get_name(), mutable_id,
                                 m->annotation_is_optional(), m->annotation_is_key());
                parse_dynamic(m->get_type(), type.get_name());
                break;
            case vbs::TypeKind::TK_UNION:
                union_members.clear();
                union_lable.clear();
                local_type.get_all_members(union_members);
                insert_to_struct(type_struct, m->get_name(), "nonBasic", m->get_type().get_name(), mutable_id,
                                 m->annotation_is_optional(), m->annotation_is_key());
                if (get_base_type(local_type.get_discriminator_type().get_kind()) != "") {
                    local_type_xml =
                        insert_union(local_type.get_name(),
                                     get_base_type(local_type.get_discriminator_type().get_kind()), type.get_name());
                } else {
                    local_type_xml =
                        insert_union(local_type.get_name(), local_type.get_discriminator_type().get_descriptor_name(),
                                     type.get_name());
                    parse_dynamic(local_type.get_discriminator_type(), local_type.get_name());
                }
                if (local_type_xml == nullptr) {
                    break;
                }
                for (const auto& union_member : union_members) {
                    um = union_member.second;
                    union_local_type = um->get_type();
                    union_lable = um->get_union_labels();
                    local_union_type_xml = insert_union_element(local_type_xml, um->get_union_labels());
                    if (get_base_type(union_local_type.get_kind()) != "") {
                        insert_union_element(local_union_type_xml, um->get_name(),
                                             get_base_type(union_local_type.get_kind()),
                                             std::to_string(union_local_type.get_total_bounds()));
                    } else {
                        insert_union_complex_element(local_union_type_xml, um->get_name(), "nonBasic",
                                                     union_local_type.get_name());
                        parse_dynamic(union_local_type, local_type.get_name());
                    }
                }
                break;
            case vbs::TypeKind::TK_BITSET:
                union_members.clear();
                local_type.get_all_members(union_members);
                insert_to_struct(type_struct, m->get_name(), "nonBasic", m->get_type().get_name(), mutable_id,
                                 m->annotation_is_optional(), m->annotation_is_key());
                parse_bitset_type(local_type, type.get_name());
                break;
            case vbs::TypeKind::TK_SEQUENCE:
                if (get_base_type(local_type.get_element_type().get_kind()) != "") {
                    insert_seq(type_struct, m->get_name(), get_base_type(local_type.get_element_type().get_kind()),
                               local_type.get_total_bounds(),
                               std::to_string(local_type.get_element_type().get_total_bounds()), mutable_id,
                               m->annotation_is_optional(), m->annotation_is_key());
                } else {
                    if (local_type.get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
                        insert_seq_nobasic(type_struct, m->get_name(), "nonBasic", local_type.get_total_bounds(),
                                           local_type.get_element_type().get_descriptor_name(), mutable_id,
                                           m->annotation_is_optional(), m->annotation_is_key());
                    } else {
                        insert_seq_nobasic(type_struct, m->get_name(), "nonBasic", local_type.get_total_bounds(),
                                           local_type.get_element_type().get_name(), mutable_id,
                                           m->annotation_is_optional(), m->annotation_is_key());
                    }
                    parse_dynamic(local_type.get_element_type(), type.get_name());
                }
                break;
            case vbs::TypeKind::TK_ARRAY:
                arrayDimensions = "";
                for (uint32_t arr_index = 0; arr_index < local_type.get_bounds_size(); arr_index++) {
                    if (arr_index != 0) {
                        arrayDimensions += ",";
                    }
                    arrayDimensions += std::to_string(local_type.get_bounds(arr_index));
                }
                if (get_base_type(local_type.get_element_type().get_kind()) != "") {
                    insert_array(type_struct, m->get_name(), get_base_type(local_type.get_element_type().get_kind()),
                                 arrayDimensions, std::to_string(local_type.get_element_type().get_total_bounds()),
                                 mutable_id, m->annotation_is_optional(), m->annotation_is_key());
                } else {
                    if (local_type.get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
                        insert_array(type_struct, m->get_name(), local_type.get_element_type().get_descriptor_name(),
                                     arrayDimensions, mutable_id, m->annotation_is_optional(), m->annotation_is_key());
                        parse_dynamic(local_type.get_element_type(), type.get_name());
                    } else {
                        insert_array(type_struct, m->get_name(), local_type.get_element_type().get_name(),
                                     arrayDimensions, mutable_id, m->annotation_is_optional(), m->annotation_is_key());
                        parse_dynamic(local_type.get_element_type(), type.get_name());
                    }
                }
                break;
            case vbs::TypeKind::TK_MAP:
                val_type = get_base_type(local_type.get_element_type().get_kind());
                key_type = get_base_type(local_type.get_key_element_type().get_kind());
                if (key_type == "") {
                    key_type = local_type.get_key_element_type().get_name();
                    parse_dynamic(local_type.get_key_element_type(), type.get_name());
                }
                if (val_type == "") {
                    if (local_type.get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
                        insert_map(type_struct, m->get_name(), "nonBasic",
                                   local_type.get_element_type().get_descriptor_name(), key_type,
                                   local_type.get_total_bounds(), mutable_id, m->annotation_is_optional(),
                                   m->annotation_is_key());
                        parse_dynamic(local_type.get_element_type(), type.get_name());
                    } else {
                        insert_map(type_struct, m->get_name(), "nonBasic", local_type.get_element_type().get_name(),
                                   key_type, local_type.get_total_bounds(), mutable_id, m->annotation_is_optional(),
                                   m->annotation_is_key());
                        parse_dynamic(local_type.get_element_type(), type.get_name());
                    }
                } else {
                    insert_map(type_struct, m->get_name(), val_type, key_type, local_type.get_total_bounds(),
                               mutable_id, m->annotation_is_optional(), m->annotation_is_key());
                }
                break;
            default:
                break;
        }
    }
}

void GenerateXML::parse_alias_type(vbs::VBSDynamicType type, const std::string& alias_name,
                                   const std::string& type_struct_name) {
    if (!type.is_valid()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " type is invalidm alias_name: \'" << alias_name << "\' type_struct_name: \'" << type_struct_name
                << "\'");
        return;
    }
    std::string type_name = type.get_name();
    GenerateXMLNode* node = get_node(alias_name);
    all_nodes_[type_struct_name]->next_nodes_.insert(node);
    node->pre_nodes_.insert(all_nodes_[type_struct_name]);

    if (struct_elements_.find(alias_name) != struct_elements_.end()) {
        return;
    }
    tinyxml2::XMLElement* local_type = xmlDoc_.NewElement("type");
    types_->InsertEndChild(local_type);
    types_->InsertAfterChild(local_type, struct_elements_[type_struct_name]);
    tinyxml2::XMLElement* alias_type = xmlDoc_.NewElement("typedef");

    alias_type->SetAttribute("name", alias_name.c_str());
    local_type->InsertEndChild(alias_type);
    struct_elements_[alias_name] = local_type;

    std::string arrayDimensions = "";
    std::string val_type = "";

    switch (type.get_kind()) {
        case vbs::TypeKind::TK_BOOLEAN:
        case vbs::TypeKind::TK_BYTE:
        case vbs::TypeKind::TK_INT8:
        case vbs::TypeKind::TK_INT16:
        case vbs::TypeKind::TK_INT32:
        case vbs::TypeKind::TK_INT64:
        case vbs::TypeKind::TK_UINT16:
        case vbs::TypeKind::TK_UINT32:
        case vbs::TypeKind::TK_UINT64:
        case vbs::TypeKind::TK_FLOAT32:
        case vbs::TypeKind::TK_FLOAT64:
        case vbs::TypeKind::TK_FLOAT128:
        case vbs::TypeKind::TK_CHAR8:
        case vbs::TypeKind::TK_CHAR16:
            alias_type->SetAttribute("type", get_base_type(type.get_kind()).c_str());
            break;
        case vbs::TypeKind::TK_STRING8:
        case vbs::TypeKind::TK_STRING16:
            alias_type->SetAttribute("type", get_base_type(type.get_kind()).c_str());
            alias_type->SetAttribute("stringMaxLength", std::to_string(type.get_total_bounds()).c_str());
            break;
        case vbs::TypeKind::TK_ALIAS:
            alias_type->SetAttribute("type", "nonBasic");
            alias_type->SetAttribute("nonBasicTypeName", type_name.c_str());
            parse_alias_type(type.get_base_type(), type_name, alias_name);
            break;
        case vbs::TypeKind::TK_ENUM:
            node = get_node(type.get_descriptor_name());
            all_nodes_[alias_name]->next_nodes_.insert(node);
            node->pre_nodes_.insert(all_nodes_[alias_name]);
            alias_type->SetAttribute("type", "nonBasic");
            alias_type->SetAttribute("nonBasicTypeName", type.get_descriptor_name().c_str());
            parse_dynamic(type, alias_name);
            break;
        case vbs::TypeKind::TK_BITMASK:
        case vbs::TypeKind::TK_STRUCTURE:
        case vbs::TypeKind::TK_UNION:
        case vbs::TypeKind::TK_BITSET:
            node = get_node(type_name);
            all_nodes_[alias_name]->next_nodes_.insert(node);
            node->pre_nodes_.insert(all_nodes_[alias_name]);
            alias_type->SetAttribute("type", "nonBasic");
            alias_type->SetAttribute("nonBasicTypeName", type_name.c_str());
            parse_dynamic(type, alias_name);
            break;
        case vbs::TypeKind::TK_SEQUENCE:
            if (get_base_type(type.get_element_type().get_kind()) != "") {
                alias_type->SetAttribute("type", get_base_type(type.get_element_type().get_kind()).c_str());
                alias_type->SetAttribute("sequenceMaxLength", std::to_string(type.get_total_bounds()).c_str());
                if ((get_base_type(type.get_element_type().get_kind()) == "string") ||
                    (get_base_type(type.get_element_type().get_kind()) == "wstring")) {

                    alias_type->SetAttribute("stringMaxLength",
                                             std::to_string(type.get_element_type().get_total_bounds()).c_str());
                }
            } else {
                alias_type->SetAttribute("type", "nonBasic");
                alias_type->SetAttribute("nonBasicTypeName", type.get_element_type().get_name().c_str());
                alias_type->SetAttribute("sequenceMaxLength", std::to_string(type.get_total_bounds()).c_str());
                parse_dynamic(type.get_element_type(), alias_name);
            }
            break;
        case vbs::TypeKind::TK_ARRAY:
            arrayDimensions = "";
            for (uint32_t arr_index = 0; arr_index < type.get_bounds_size(); arr_index++) {
                if (arr_index != 0) {
                    arrayDimensions += ",";
                }
                arrayDimensions += std::to_string(type.get_bounds(arr_index));
            }
            if (get_base_type(type.get_element_type().get_kind()) != "") {
                alias_type->SetAttribute("type", get_base_type(type.get_element_type().get_kind()).c_str());
                alias_type->SetAttribute("arrayDimensions", arrayDimensions.c_str());
            } else {
                alias_type->SetAttribute("type", "nonBasic");
                alias_type->SetAttribute("nonBasicTypeName", type.get_element_type().get_name().c_str());
                alias_type->SetAttribute("arrayDimensions", arrayDimensions.c_str());
                parse_dynamic(type.get_element_type(), alias_name);
            }
            break;
        case vbs::TypeKind::TK_MAP:
            val_type = get_base_type(type.get_element_type().get_kind());
            if (get_base_type(type.get_element_type().get_kind()) != "") {
                alias_type->SetAttribute("type", get_base_type(type.get_element_type().get_kind()).c_str());
                alias_type->SetAttribute("key_type", get_base_type(type.get_key_element_type().get_kind()).c_str());
                alias_type->SetAttribute("mapMaxLength", std::to_string(type.get_total_bounds()).c_str());
            } else {
                alias_type->SetAttribute("type", "nonBasic");
                alias_type->SetAttribute("nonBasicTypeName", type.get_element_type().get_name().c_str());
                alias_type->SetAttribute("key_type", get_base_type(type.get_key_element_type().get_kind()).c_str());
                alias_type->SetAttribute("mapMaxLength", std::to_string(type.get_total_bounds()).c_str());
                parse_dynamic(type.get_element_type(), alias_name);
            }
            break;
        default:
            break;
    }
}

void GenerateXML::parse_bitset_type(vbs::VBSDynamicType type, const std::string& type_struct_name) {
    if (!type.is_valid()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " type is invalid type_struct_name: \'" << type_struct_name << "\'");
        return;
    }

    if (type.get_kind() != vbs::TypeKind::TK_BITSET) {
        return;
    }

    std::string type_name = type.get_name();
    if (struct_elements_.find(type_name) != struct_elements_.end()) {
        return;
    }

    std::map<uint32_t, std::shared_ptr<vbs::VBSDynamicTypeMember>> union_members;
    std::shared_ptr<vbs::VBSDynamicTypeMember> um;
    tinyxml2::XMLElement* type_xml = nullptr;
    if (type.has_base_type()) {
        type_xml = insert_bitset(type_name, type_struct_name, type.get_base_type().get_name());
        parse_bitset_type(type.get_base_type(), type_name);
    } else {
        type_xml = insert_bitset(type_name, type_struct_name);
    }
    if (type_xml == nullptr) {
        return;
    }

    type.get_all_members(union_members);
    for (const auto& union_member : union_members) {
        um = union_member.second;
        insert_bitset_element(type_xml, um->get_name(), get_base_type(um->get_type().get_kind()),
                              std::to_string(um->annotation_get_bit_bound()));
    }
}

bool GenerateXML::has_struct(const std::string& struct_name) {
    if (struct_elements_.find(struct_name) != struct_elements_.end()) {
        return true;
    }
    return false;
}

GenerateXMLNode* GenerateXML::get_node(const std::string& name) {
    GenerateXMLNode* node = nullptr;
    if (all_nodes_.find(name) == all_nodes_.end()) {
        node = new GenerateXMLNode(name);
        all_nodes_[name] = node;
    } else {
        node = all_nodes_[name];
    }
    return node;
}

}  // namespace vbs
