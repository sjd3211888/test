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

#ifndef VBS_TYPES_GENERATEXML_H_
#define VBS_TYPES_GENERATEXML_H_

#include <cstdint>
#include <cassert>
#include <map>
#include <string>
#include <set>

#include "tinyxml2.h"
#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class GenerateXMLNode {
 public:
    GenerateXMLNode() {}

    GenerateXMLNode(const std::string& name) : name_(name) {}

    ~GenerateXMLNode() {}

    std::set<GenerateXMLNode*> next_nodes_;
    std::set<GenerateXMLNode*> pre_nodes_;
    std::string name_;
};

class GenerateXML {
    friend class VBSGenerateIDL;

 public:
    /**
     * GenerateXML constructor
     */
    GenerateXML();

    /**
     * GenerateXML deconstructor
     */
    ~GenerateXML();

    /**
     * generate xml file
     *
     * @param file_name the xml file's name
     */
    void save(const std::string& file_name);

    /**
     * generate xml file
     *
     * @return string of xml content
     */
    std::string get();

    /**
     * parse dynamic type
     *
     * @param type dynamic type
     * @param name name of dynamic type which rely on type
     */
    void parse_dynamic(vbs::VBSDynamicType type, const std::string& name = "vbs_root");

    /**
     * parse dynamic type
     *
     * @param type dynamic type
     * @param type_struct type's info will insert into type_struct
     */
    void parse_dynamic_type(vbs::VBSDynamicType type, tinyxml2::XMLElement* type_struct);

    /**
     * create struct
     *
     * @param name name of dynamic type
     * @return XMLElement pointer
     */
    tinyxml2::XMLElement* create_struct(const std::string& name);

    /**
     * create struct
     *
     * @param name name of dynamic type
     * @param base_name base name of dynamic type
     * @return XMLElement pointer
     */
    tinyxml2::XMLElement* create_struct(const std::string& name, const std::string& base_name);

    /**
     * create struct
     *
     * @param name name of dynamic type
     * @param base_name basic name of dynamic type
     * @param extensibility extensibility
     * @return XMLElement pointer
     */
    tinyxml2::XMLElement* create_struct(const std::string& name, const std::string& base_name,
                                        const std::string& extensibility);

 private:
    void insert_to_struct(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                          const std::string& nonBasicTypeName, uint32_t id = VBS_MEMBER_ID_INVALID,
                          bool optional = false, bool key = false);

    tinyxml2::XMLElement* insert_union(const std::string& name, const std::string& type,
                                       const std::string& type_struct_name);

    tinyxml2::XMLElement* insert_union_element(tinyxml2::XMLElement* type_struct, const std::vector<uint64_t>& value);

    void insert_union_element(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                              const std::string& stringMaxLength);

    void insert_union_complex_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                      const std::string& type, const std::string& nonBasicTypeName);

    tinyxml2::XMLElement* insert_enum(const std::string& name, const std::string& type_struct_name);

    void insert_enum_element(tinyxml2::XMLElement* type_struct, const std::string& name, uint32_t index);

    tinyxml2::XMLElement* insert_bitset(std::string const& name, std::string const& type_struct_name,
                                        std::string const& base_type);

    tinyxml2::XMLElement* insert_bitset(const std::string& name, const std::string& type_struct_name);

    void insert_bitset_element(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                               const std::string& bit_bound);

    tinyxml2::XMLElement* insert_bitmask(const std::string& name, const std::string& bit_bound,
                                         const std::string& type_struct_name);

    void insert_bitmask_element(tinyxml2::XMLElement* type_struct, const std::string& name,
                                const std::string& position);

    void insert_base(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                     const std::string& defalut_value, uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false,
                     bool key = false);

    void insert_string(tinyxml2::XMLElement* type_struct, const std::string& name, uint32_t stringMaxLength,
                       bool is_string = true, uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false,
                       bool key = false);

    void insert_array(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                      const std::string& arrayDimensions, const std::string& stringMaxLength,
                      uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false, bool key = false);

    void insert_array(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& nonBasicTypeName,
                      const std::string& arrayDimensions, uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false,
                      bool key = false);

    void insert_seq(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                    uint32_t sequenceMaxLength, const std::string& stringMaxLength, uint32_t id = VBS_MEMBER_ID_INVALID,
                    bool optional = false, bool key = false);

    void insert_seq_nobasic(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                            uint32_t sequenceMaxLength, const std::string& nonBasicTypeName,
                            uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false, bool key = false);

    void insert_map(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                    const std::string& key_type, uint32_t mapMaxLength, uint32_t id = VBS_MEMBER_ID_INVALID,
                    bool optional = false, bool key = false);

    void insert_map(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                    const std::string& nonBasicTypeName, const std::string& key_type, uint32_t mapMaxLength,
                    uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false, bool key = false);

    void insert_alias(const std::string& name, const std::string& type, const std::string& nonBasicTypeName,
                      const std::string& type_struct_name);

    void insert_alias(tinyxml2::XMLElement* type_struct, const std::string& name, const std::string& type,
                      const std::string& nonBasicTypeName, uint32_t id = VBS_MEMBER_ID_INVALID, bool optional = false,
                      bool key = false);

    static std::string get_base_type(TypeKind kind);

    bool has_struct(const std::string& struct_name);

    GenerateXMLNode* get_node(const std::string& name);

    void parse_alias_type(vbs::VBSDynamicType type, const std::string& alias_name, const std::string& type_struct_name);

    void parse_bitset_type(vbs::VBSDynamicType type, const std::string& type_struct_name);

    void sort(void);

 private:
    tinyxml2::XMLDocument xmlDoc_;
    tinyxml2::XMLElement* types_;
    std::map<std::string, tinyxml2::XMLElement*> struct_elements_;
    GenerateXMLNode* root_node_;
    std::map<std::string, GenerateXMLNode*> all_nodes_;
};

}  // namespace vbs

#endif  // VBS_TYPES_GENERATEXML_H_
