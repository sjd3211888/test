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

#ifndef VBS_TYPES_VBSGENERATEIDL_H_
#define VBS_TYPES_VBSGENERATEIDL_H_

#include <cstdint>
#include <cassert>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "xmlparser/XMLParserCommonPart.h"
#include "vbs/types/GenerateXML.h"
#include "tinyxml2.h"
#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class VBSGenerateIDL {
 public:
    /**
     * VBSGenerateIDL constructor
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    VBSGenerateIDL() : generate_xml(new vbs::GenerateXML()) {};

    /**
     * VBSGenerateIDL deconstructor
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    ~VBSGenerateIDL() {
        if (generate_xml)
            delete generate_xml;
    };

    /**
     * print idl
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type dynamic type
     * @param idl idl string
     */
    void print_idl(const vbs::VBSDynamicType& type, std::string& idl);

 private:
    vbs::GenerateXML* generate_xml;
    tinyxml2::XMLElement* SearchEnumTypeNode(tinyxml2::XMLElement* p_root, const char* name);

    void inline stringSplit(const char* name, std::vector<std::string>& substr_vec);

    void inline insertPrefix(std::ostringstream& oss, std::vector<std::string> stuct_name_vec, std::string& tabs);

    void inline insertSuffix(std::ostringstream& oss, std::vector<std::string> stuct_name_vec, std::string& tabs);

    void inline appendArrayDimensions(std::ostringstream& oss, bool isArray, std::vector<int>& dimsVec);

    int ParseXML(tinyxml2::XMLDocument& xmlDoc, std::ostringstream& oss);

    int ParseXMLTypes(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLStructDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLUnionDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLEnumDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLAliasDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLBitsetDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLBitmaskDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss);

    int parseXMLBitvalueDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss, std::string&& tabs);

    void parseXMLMemberDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss, std::string&& tabs);

    static std::unordered_map<std::string, std::string> TypeHashMap_;
};

}  // namespace vbs

#endif  // VBS_TYPES_VBSGENERATEIDL_H_
