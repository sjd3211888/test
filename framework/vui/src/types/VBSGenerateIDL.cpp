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

#include <iostream>
#include <sstream>

#include "elog/Log.hpp"
#include "vbs/types/VBSGenerateIDL.h"
#include "deps/type/TypesBase.h"

using namespace vbsutil::xmlparser;

namespace vbs {

std::unordered_map<std::string, std::string> VBSGenerateIDL::TypeHashMap_ = {
    {"int8", "int8"},       {"char8", "char"},           {"char16", "wchar"},         {"boolean", "boolean"},
    {"uint8", "octet"},     {"byte", "octet"},           {"int16", "short"},          {"uint16", "unsigned short"},
    {"int32", "long"},      {"uint32", "unsigned long"}, {"int64", "long long"},      {"uint64", "unsigned long long"},
    {"float32", "float"},   {"float64", "double"},       {"float128", "long double"}, {"string", "string"},
    {"wstring", "wstring"},
};

void VBSGenerateIDL::print_idl(const vbs::VBSDynamicType& type, std::string& idl) {
    if (!type.is_valid()) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "type is invalid ");
        return;
    }
    generate_xml->parse_dynamic(type);
    std::ostringstream oss;
    generate_xml->sort();
    ParseXML(generate_xml->xmlDoc_, oss);
    idl = oss.str();
}

int VBSGenerateIDL::ParseXML(tinyxml2::XMLDocument& xmlDoc, std::ostringstream& oss) {
    int ret = 0;
    tinyxml2::XMLElement* p_root = xmlDoc.FirstChildElement(xmlString::TYPES);
    if (p_root == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "failed with invalid xmlDocument") ret = -1;
    } else {
        ret = static_cast<int>(ParseXMLTypes(p_root, oss));
    }
    return ret;
}

int VBSGenerateIDL::ParseXMLTypes(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    tinyxml2::XMLElement* p_aux0 = nullptr;
    const char* name = nullptr;

    for (p_aux0 = p_root->FirstChildElement(); p_aux0 != nullptr; p_aux0 = p_aux0->NextSiblingElement()) {
        name = p_aux0->Name();
        if (strcmp(name, xmlString::TYPE) == 0) {
            ret = parseXMLDynamicType(p_aux0, oss);
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "Invalid element found into 'types'. Name: " << name);
            ret = -1;
        }
    }
    return ret;
}

int VBSGenerateIDL::parseXMLDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {

    int ret = 0;
    tinyxml2::XMLElement* p_aux0 = nullptr;
    for (p_aux0 = p_root->FirstChildElement(); p_aux0 != nullptr; p_aux0 = p_aux0->NextSiblingElement()) {
        const std::string type = p_aux0->Value();
        if (type.compare(xmlString::STRUCT) == 0) {
            ret = static_cast<int>(parseXMLStructDynamicType(p_aux0, oss));
        } else if (type.compare(xmlString::UNION) == 0) {
            ret = parseXMLUnionDynamicType(p_aux0, oss);
        } else if (type.compare(xmlString::ENUM) == 0) {
            ret = parseXMLEnumDynamicType(p_aux0, oss);
        } else if (type.compare(xmlString::TYPEDEF) == 0) {
            ret = parseXMLAliasDynamicType(p_aux0, oss);
        } else if (type.compare(xmlString::BITSET) == 0) {
            ret = parseXMLBitsetDynamicType(p_aux0, oss);
        } else if (type.compare(xmlString::BITMASK) == 0) {
            ret = parseXMLBitmaskDynamicType(p_aux0, oss);
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "Error parsing type: Type " << type << " not recognized.");
            ret = -1;
        }

        if (ret != 0) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "Error parsing type " << type << ".");
            break;
        }
    }
    return ret;
}

void inline VBSGenerateIDL::stringSplit(const char* name, std::vector<std::string>& substr_vec) {
    int len = strlen(name);
    if (len > 0) {
        std::string tmp = "";
        for (int i = 0; i < len - 1;) {
            if (name[i] == ':' && name[i + 1] == ':') {
                substr_vec.emplace_back(tmp);
                tmp = "";
                i = i + 2;
                if (i >= len)
                    break;
            } else {
                tmp += name[i];
                i = i + 1;
                if (i == len - 1) {
                    tmp += name[i];
                    substr_vec.emplace_back(tmp);
                    break;
                }
            }
        }
    }
}

void inline VBSGenerateIDL::insertPrefix(std::ostringstream& oss, std::vector<std::string> stuct_name_vec,
                                         std::string& tabs) {
    int vec_size = stuct_name_vec.size();
    for (int i = 0; i < vec_size; i++) {
        if (i == vec_size - 1) {
            oss << tabs << "struct " << stuct_name_vec[i] << " {" << std::endl;
        } else {
            oss << tabs << "module " << stuct_name_vec[i] << " {" << std::endl;
            tabs += "\t";
        }
    }
}

void inline VBSGenerateIDL::insertSuffix(std::ostringstream& oss, std::vector<std::string> stuct_name_vec,
                                         std::string& tabs) {
    int vec_size = stuct_name_vec.size();
    int expected_len = tabs.length();
    for (int i = 0; i < vec_size; i++) {
        oss << tabs << "};" << std::endl;
        expected_len--;
        tabs = tabs.substr(0, expected_len);
    }
}

void inline VBSGenerateIDL::appendArrayDimensions(std::ostringstream& oss, bool isArray, std::vector<int>& dimsVec) {
    if (isArray) {
        for (auto d : dimsVec)
            oss << "[" << d << "]";
        oss << ";" << std::endl;
    } else {
        oss << ";" << std::endl;
    }
}

tinyxml2::XMLElement* VBSGenerateIDL::SearchEnumTypeNode(tinyxml2::XMLElement* p_root, const char* name) {
    tinyxml2::XMLElement* enumTypeNode = nullptr;
    tinyxml2::XMLElement* prevTypeNode = p_root->PreviousSiblingElement(xmlString::TYPE);
    while (prevTypeNode) {
        tinyxml2::XMLElement* p_aux0 = nullptr;
        for (p_aux0 = prevTypeNode->FirstChildElement(); p_aux0 != nullptr; p_aux0 = p_aux0->NextSiblingElement()) {
            if (strcmp(p_aux0->Value(), xmlString::ENUM) == 0) {
                const char* enumName = p_aux0->Attribute(xmlString::NAME);
                if (enumName != nullptr && strcmp(name, enumName) == 0) {
                    enumTypeNode = p_aux0;
                    return enumTypeNode;
                }
            } else {
                continue;
            }
        }
        prevTypeNode = prevTypeNode->PreviousSiblingElement(xmlString::TYPE);
    }
    return enumTypeNode;
}

int VBSGenerateIDL::parseXMLBitvalueDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss,
                                                std::string&& tabs) {
    uint16_t field_position = 0;
    int ret = 0;
    if (p_root == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing bitmask: Node not found.");
        return -1;
    }

    const char* memberName = p_root->Attribute(xmlString::NAME);
    const char* position = p_root->Attribute(xmlString::POSITION);

    if (position != nullptr) {
        try {
            field_position = static_cast<uint16_t>(std::stoul(position));
        } catch (const std::exception&) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "Error parsing bit_value position: Invalid (must be an unsigned short).");
            return -1;
        }
        oss << tabs << "@position(" << field_position << ") ";
    }

    if (memberName == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing bit_value name: Not found.");
        return -1;
    }
    oss << memberName;

    return ret;
}
int VBSGenerateIDL::parseXMLBitmaskDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    uint16_t bit_bound = 32;
    std::string tabs = "";
    const char* anno_bit_bound = p_root->Attribute(xmlString::BIT_BOUND);
    if (anno_bit_bound != nullptr) {
        oss << "@bit_bound(" << anno_bit_bound << ")" << std::endl;
    } else {
        oss << "@bit_bound(" << bit_bound << ")" << std::endl;
    }

    const char* name = p_root->Attribute(xmlString::NAME);
    if (nullptr == name) {
        return -1;
    }
    oss << "bitmask " << name << " {" << std::endl;

    const char* element_name = nullptr;
    for (tinyxml2::XMLElement* p_element = p_root->FirstChildElement(); p_element != nullptr;
         p_element = p_element->NextSiblingElement()) {
        element_name = p_element->Name();
        if (strcmp(element_name, xmlString::BIT_VALUE) == 0) {
            ret = parseXMLBitvalueDynamicType(p_element, oss, tabs + "\t");
            if (ret != 0) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "Invalid BitValue element . Name: " << element_name);
                ret = -1;
                break;
            }
            if (p_element->NextSiblingElement() != nullptr) {
                oss << "," << std::endl;
            } else {
                oss << std::endl;
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "Invalid element found into 'bitmaskDcl'. Name: " << element_name);
            return -1;
        }
    }
    oss << "};" << std::endl;
    return ret;
}

int VBSGenerateIDL::parseXMLBitsetDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    std::string tabs = "";

    const char* name = p_root->Attribute(xmlString::NAME);
    if (nullptr == name) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                << "Error parsing 'bitsetDcl' type. No name attribute given.");
        return -1;
    }
    oss << "bitset " << name << " {" << std::endl;
    const char* baseType = p_root->Attribute(xmlString::BASE_TYPE);
    if (baseType != nullptr) {
        // todo
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "Bitset type: " << name << " get baseType: " << baseType);
        return -1;
    }

    const char* element_name = nullptr;
    tabs += "\t";
    for (tinyxml2::XMLElement* p_element = p_root->FirstChildElement(); p_element != nullptr;
         p_element = p_element->NextSiblingElement()) {
        element_name = p_element->Name();
        if (strcmp(element_name, xmlString::BITFIELD) == 0) {
            const char* bitfieldVal = p_element->Attribute(xmlString::BIT_BOUND);
            if (bitfieldVal != nullptr)
                oss << tabs << "bitfield<" << bitfieldVal;
            const char* bitfieldType = p_element->Attribute(xmlString::TYPE);
            if (bitfieldType != nullptr && strcmp(bitfieldType, "char8") != 0) {
                auto it = VBSGenerateIDL::TypeHashMap_.find(std::string(bitfieldType));
                if (it != VBSGenerateIDL::TypeHashMap_.end())
                    oss << ", " << it->second;
            }
            oss << "> ";
            const char* bitfieldName = p_element->Attribute(xmlString::NAME);
            if (bitfieldVal != nullptr)
                oss << bitfieldName << ";" << std::endl;
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "Invalid element found into 'bitsetDcl'. Name: " << element_name);
            return -1;
        }
    }
    oss << "};" << std::endl;
    return ret;
}

int VBSGenerateIDL::parseXMLAliasDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    std::string tabs = "";
    const char* typedefTypeName = nullptr;
    const char* ArrayDims = nullptr;
    std::vector<int> arrayDimsVec;
    const char* SequenceSize = nullptr;
    const char* MapSize = nullptr;
    const char* MapKeyType = nullptr;
    const char* memberNonBasicTypeName = nullptr;
    bool isArray = (p_root->Attribute(xmlString::ARRAY_DIMENSIONS) != nullptr) ? true : false;
    bool isMap = (p_root->Attribute(xmlString::MAP_MAXLENGTH) != nullptr) ? true : false;
    bool isSeq = (p_root->Attribute(xmlString::SEQ_MAXLENGTH) != nullptr) ? true : false;
    bool isNonBasicType = false;
    const char* type = p_root->Attribute(xmlString::TYPE);
    const char* name = p_root->Attribute(xmlString::NAME);

    if (type != nullptr) {
        isNonBasicType = (strcmp(type, xmlString::NON_BASIC_TYPE) == 0) ? true : false;
    }
    // VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "parseXMLAliasDynamicType type: " << type << ",  name: " << name);
    // VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "parseXMLAliasDynamicType isArray: " << isArray << ",  isMap: " << isMap << ", isSeq:" << isSeq);

    std::vector<std::string> module_struct_name;
    int module_count = 0;
    if (name != nullptr) {
        stringSplit(name, module_struct_name);
        module_count = module_struct_name.size() - 1;
        if (module_count >= 0)
            name = module_struct_name[module_count].c_str();
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                << "parseXMLAliasDynamicType: not found invalid typedef name");
    }

    for (int i = 0; i < module_count; i++) {
        oss << "module " << module_struct_name[i] << " {" << std::endl;
    }

    oss << "typedef"
        << " ";

    if (isArray) {
        ArrayDims = p_root->Attribute(xmlString::ARRAY_DIMENSIONS);
        int len = strlen(ArrayDims);
        std::string tmp = "";
        for (int i = 0; i < len; i++) {
            if (ArrayDims[i] == ',') {
                arrayDimsVec.emplace_back(std::stoi(tmp));
                tmp = "";
            } else {
                tmp += ArrayDims[i];
            }
            if (i == len - 1) {
                arrayDimsVec.emplace_back(std::stoi(tmp));
            }
        }
    }

    if (isMap) {
        MapSize = p_root->Attribute(xmlString::MAP_MAXLENGTH);
        MapKeyType = p_root->Attribute(xmlString::MAP_KEY_TYPE);
    }

    if (isSeq) {
        SequenceSize = p_root->Attribute(xmlString::SEQ_MAXLENGTH);
    }
    if (isNonBasicType) {
        memberNonBasicTypeName = p_root->Attribute(xmlString::NON_BASIC_TYPE_NAME);
        if (memberNonBasicTypeName != nullptr) {
            isNonBasicType = true;
            typedefTypeName = memberNonBasicTypeName;
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Can not get NonBasicType name");
            return -1;
        }
    } else {
        if (type == nullptr) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Can not get BasicType name");
            return -1;
        } else {
            auto it = VBSGenerateIDL::TypeHashMap_.find(std::string(type));
            if (it != VBSGenerateIDL::TypeHashMap_.end()) {
                typedefTypeName = it->second.c_str();
            } else {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "BasicType :" << type << "not found!");
                return -1;
            }
        }
    }

    if (isNonBasicType == false && isMap == false && isSeq == false) {
        if (strncmp(typedefTypeName, xmlString::STRING, 7) == 0 ||
            strncmp(typedefTypeName, xmlString::WSTRING, 8) == 0) {
            uint32_t bound = 0;
            const char* boundStr = p_root->Attribute(xmlString::STR_MAXLENGTH);
            if (boundStr != nullptr) {
                bound = static_cast<uint32_t>(std::atoi(boundStr));
                oss << typedefTypeName << "<" << bound << "> ";
            } else {
                oss << typedefTypeName << " ";
            }
        } else {
            oss << typedefTypeName << " ";
        }
        oss << name;
        appendArrayDimensions(oss, isArray, arrayDimsVec);
    } else if (isNonBasicType && isMap == false && isSeq == false) {
        oss << typedefTypeName << " " << name;
        appendArrayDimensions(oss, isArray, arrayDimsVec);
    } else {
        // 优先解决sequence, map特殊情况，然后判断array
        if (isSeq) {
            uint32_t length = 0;
            if (SequenceSize != nullptr) {
                try {
                    length = static_cast<uint32_t>(std::stoi(SequenceSize));
                } catch (const std::exception&) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "Error parsing member sequence length in line " << p_root->GetLineNum());
                    return -1;
                }
            }
            oss << "sequence<" << typedefTypeName;
            if (length > 0) {
                oss << ", " << length;
            }
            oss << " > " << name;
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        } else if (isMap) {
            auto key_type_it = VBSGenerateIDL::TypeHashMap_.find(std::string(MapKeyType));
            if (key_type_it != VBSGenerateIDL::TypeHashMap_.end()) {
                MapKeyType = (key_type_it->second).c_str();
            }
            uint32_t length = 0;
            if (MapSize != nullptr) {
                try {
                    length = static_cast<uint32_t>(std::stoi(MapSize));
                } catch (const std::exception&) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "Error parsing map member sequence length in line " << p_root->GetLineNum());
                    return -1;
                }
            }
            if (isNonBasicType)
                oss << "map<" << MapKeyType << ", " << memberNonBasicTypeName;
            else
                oss << "map<" << MapKeyType << ", " << type;
            if (length > 0) {
                oss << ", " << length;
            }
            oss << " > " << name;
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        }
    }

    for (int i = 0; i < module_count; i++) {
        oss << "};" << std::endl;
    }
    return ret;
}

int VBSGenerateIDL::parseXMLEnumDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    std::string tabs = "";
    const char* enumName = p_root->Attribute(xmlString::NAME);
    if (enumName == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing 'enum' type. No name attribute given.");
        return -1;
    }
    oss << "enum " << enumName << " {" << std::endl;
    int32_t currValue = 0;
    for (tinyxml2::XMLElement* literal = p_root->FirstChildElement(xmlString::ENUMERATOR); literal != nullptr;
         literal = literal->NextSiblingElement(xmlString::ENUMERATOR)) {
        tabs = "\t";
        oss << tabs;
        const char* name = literal->Attribute(xmlString::NAME);
        if (name == nullptr) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "Error parsing enum type: Literals must have name.");
            return -1;
        }
        oss << name;
        const char* value = literal->Attribute("value");
        if (value != nullptr) {
            currValue = static_cast<int32_t>(std::atoi(value));
            oss << " = " << currValue;
        }
        if (literal->NextSiblingElement(xmlString::ENUMERATOR) != nullptr) {
            oss << ",";
        }
        oss << std::endl;
    }
    oss << "};" << std::endl;
    return ret;
}

int VBSGenerateIDL::parseXMLUnionDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    tinyxml2::XMLElement* EnumTypeNode = nullptr;
    std::string tabs = "";
    const char* name = p_root->Attribute(xmlString::NAME);
    if (name == nullptr) {
        return -1;
    }
    oss << "union " << name;
    tinyxml2::XMLElement* p_element = p_root->FirstChildElement(xmlString::DISCRIMINATOR);
    if (p_element != nullptr) {
        const char* disc = p_element->Attribute(xmlString::TYPE);
        auto it = VBSGenerateIDL::TypeHashMap_.find(std::string(disc));
        if (it != VBSGenerateIDL::TypeHashMap_.end()) {
            oss << " switch (" << it->second << ") {" << std::endl;
        } else {
            oss << " switch (" << std::string(disc) << ") {" << std::endl;
            EnumTypeNode = SearchEnumTypeNode(static_cast<tinyxml2::XMLElement*>(p_root->Parent()), disc);
            if (EnumTypeNode == nullptr) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Not found Enum Type: " << disc);
                return -1;
            }
        }
        for (p_element = p_root->FirstChildElement(xmlString::CASE); p_element != nullptr;
             p_element = p_element->NextSiblingElement(xmlString::CASE)) {
            std::string valuesStr = "";
            tabs = "\t";
            for (tinyxml2::XMLElement* caseValue = p_element->FirstChildElement(xmlString::CASE_DISCRIMINATOR);
                 caseValue != nullptr; caseValue = caseValue->NextSiblingElement(xmlString::CASE_DISCRIMINATOR)) {
                const char* values = caseValue->Attribute(xmlString::VALUE);
                if (values == nullptr) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                            << "Error parsing union case value: Not found.");
                    return -1;
                }
                oss << tabs << "case ";
                if (EnumTypeNode != nullptr) {
                    const char* union_case_name = nullptr;
                    for (tinyxml2::XMLElement* literal = EnumTypeNode->FirstChildElement(xmlString::ENUMERATOR);
                         literal != nullptr; literal = literal->NextSiblingElement(xmlString::ENUMERATOR)) {
                        const char* enum_element_name = literal->Attribute(xmlString::NAME);
                        const char* enum_element_value = literal->Attribute(xmlString::VALUE);
                        if (enum_element_name == nullptr || enum_element_value == nullptr) {
                            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                                    << "Error parsing enum type: Literals must have name and value");
                            return -1;
                        }
                        if (strcmp(values, enum_element_value) == 0) {
                            union_case_name = enum_element_name;
                            break;
                        }
                    }
                    if (union_case_name == nullptr) {
                        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                                << "Not found enum element name who's value equal to " << values);
                        return -1;
                    }
                    oss << union_case_name << ":" << std::endl;
                } else {
                    oss << values << ":" << std::endl;
                }
            }

            tinyxml2::XMLElement* caseElement = p_element->FirstChildElement();
            while ((caseElement != nullptr) &&
                   (strncmp(caseElement->Value(), xmlString::CASE_DISCRIMINATOR, 10) == 0)) {
                caseElement = caseElement->NextSiblingElement();
            }
            if (caseElement != nullptr) {
                parseXMLMemberDynamicType(caseElement, oss, tabs + "\t");
            } else {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing union case member: Not found.");
                ret = -1;
            }
        }
        oss << "};" << std::endl;
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing union discriminator: Not found.");
        ret = -1;
    }
    return ret;
}

int VBSGenerateIDL::parseXMLStructDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss) {
    int ret = 0;
    std::string tabs = "";
    const char* name = p_root->Attribute(xmlString::NAME);
    const char* baseTypeName = p_root->Attribute(xmlString::BASE_TYPE);
    const char* extKeyWord = p_root->Attribute("extensibility");

    // std::vector<std::string> stuct_name_vec;
    // stringSplit(name, stuct_name_vec);
    // insertPrefix(oss, stuct_name_vec, tabs);
    if (extKeyWord != nullptr) {
        oss << tabs << "@" << extKeyWord << std::endl;
    }

    oss << tabs << "struct " << name;
    if (baseTypeName != nullptr) {
        oss << " : " << baseTypeName;
    }
    oss << " {" << std::endl;
    const char* element_name = nullptr;
    for (tinyxml2::XMLElement* p_element = p_root->FirstChildElement(); p_element != nullptr;
         p_element = p_element->NextSiblingElement()) {
        element_name = p_element->Name();
        if (strcmp(element_name, xmlString::MEMBER) == 0) {
            parseXMLMemberDynamicType(p_element, oss, tabs + "\t");
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "Invalid element found into 'structDcl'. Name: " << element_name);
            return -1;
        }
    }
    // insertSuffix(oss, stuct_name_vec, tabs);
    oss << tabs << "};" << std::endl;
    return ret;
}

void VBSGenerateIDL::parseXMLMemberDynamicType(tinyxml2::XMLElement* p_root, std::ostringstream& oss,
                                               std::string&& tabs) {
    if (p_root == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing member: Node not found.");
        return;
    }

    const char* memberType = p_root->Attribute(xmlString::TYPE);
    const char* memberName = p_root->Attribute(xmlString::NAME);
    const char* memberNonBasicTypeName = nullptr;
    bool isArray = false;
    bool isSequence = false;
    bool isMap = false;
    bool isNonBasicType = false;

    if (memberName == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing member name: Not found.");
        return;
    }

    if (memberType == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing member type: Not found.");
        return;
    }
    // VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "member type:" << memberType << ", member name:" << memberName);
    oss << tabs;
    const char* memberArray = nullptr;
    std::vector<int> arrayDimsVec;
    const char* memberSequence = nullptr;
    const char* memberMap = nullptr;
    const char* memberMapKeyType = nullptr;

    memberArray = p_root->Attribute(xmlString::ARRAY_DIMENSIONS);
    if (memberArray != nullptr) {
        isArray = true;
        int len = strlen(memberArray);
        std::string tmp = "";
        for (int i = 0; i < len; i++) {
            if (memberArray[i] == ',') {
                arrayDimsVec.emplace_back(std::stoi(tmp));
                tmp = "";
            } else {
                tmp += memberArray[i];
            }
            if (i == len - 1) {
                arrayDimsVec.emplace_back(std::stoi(tmp));
            }
        }
    }

    memberMap = p_root->Attribute(xmlString::MAP_MAXLENGTH);
    if (memberMap != nullptr) {
        memberMapKeyType = p_root->Attribute(xmlString::MAP_KEY_TYPE);
        if (memberMapKeyType != nullptr) {
            isMap = true;
        }
    }

    memberSequence = p_root->Attribute(xmlString::SEQ_MAXLENGTH);
    if (memberSequence != nullptr) {
        isSequence = true;
    }

    if (strcmp(memberType, xmlString::NON_BASIC_TYPE) == 0) {
        memberNonBasicTypeName = p_root->Attribute(xmlString::NON_BASIC_TYPE_NAME);
        if (memberNonBasicTypeName != nullptr) {
            isNonBasicType = true;
            memberType = memberNonBasicTypeName;
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "Error parsing member type: Not found.");
        }
    }

    std::string idl_member_type = "";
    if (isNonBasicType == false) {
        auto it = VBSGenerateIDL::TypeHashMap_.find(std::string(memberType));
        if (it != VBSGenerateIDL::TypeHashMap_.end())
            idl_member_type = it->second;
        else
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND) << "BasicType :" << memberType << "not found!");
    } else {
        idl_member_type = std::string(memberType);
    }

    const char* keyStr = p_root->Attribute(xmlString::KEY);
    if (keyStr != nullptr) {
        oss << "@key ";
    }

    const char* optionalStr = p_root->Attribute("optional");
    if (optionalStr != nullptr) {
        oss << "@optional ";
    }

    const char* memberIndex = p_root->Attribute("id");
    if (memberIndex != nullptr) {
        oss << "@id(" << memberIndex << ") ";
    }

    if (isNonBasicType == false && isMap == false && isSequence == false) {
        const char* defaultVal = p_root->Attribute(xmlString::DEFAULT);
        if (defaultVal != nullptr) {
            oss << "@default(" << defaultVal << ") ";
        }
        if (strncmp(memberType, xmlString::STRING, 7) == 0 || strncmp(memberType, xmlString::WSTRING, 8) == 0) {
            uint32_t bound = 0;
            const char* boundStr = p_root->Attribute(xmlString::STR_MAXLENGTH);
            if (boundStr != nullptr) {
                bound = static_cast<uint32_t>(std::atoi(boundStr));
                if (bound > 0)
                    oss << idl_member_type << "<" << bound << "> " << memberName;
                else
                    oss << idl_member_type << " " << memberName;
            } else {
                oss << idl_member_type << " " << memberName;
            }
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        } else {
            oss << idl_member_type << " " << memberName;
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        }
    } else if (isNonBasicType && isMap == false && isSequence == false) {
        oss << idl_member_type << " " << memberName;
        appendArrayDimensions(oss, isArray, arrayDimsVec);
    } else {
        // 优先解决sequence, map特殊情况，然后判断array
        if (isSequence) {
            const char* lengthStr = p_root->Attribute(xmlString::SEQ_MAXLENGTH);
            uint32_t length = vbs::common::types::MAX_ELEMENTS_COUNT;
            if (lengthStr != nullptr) {
                try {
                    length = static_cast<uint32_t>(std::stoi(lengthStr));
                } catch (const std::exception&) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "Error parsing member sequence length in line " << p_root->GetLineNum());
                    return;
                }
            }
            oss << "sequence<";
            if (strncmp(memberType, xmlString::STRING, 7) == 0 || strncmp(memberType, xmlString::WSTRING, 8) == 0) {
                uint32_t bound = 0;
                const char* boundStr = p_root->Attribute(xmlString::STR_MAXLENGTH);
                if (boundStr != nullptr) {
                    bound = static_cast<uint32_t>(std::atoi(boundStr));
                    if (bound > 0)
                        oss << memberType << "<" << bound << ">";
                    else
                        oss << memberType;
                } else {
                    oss << memberType;
                }
            } else {
                oss << idl_member_type;
            }
            if (length != 0)
                oss << ", " << length;
            oss << " > " << memberName;
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        } else if (isMap) {
            auto key_type_it = VBSGenerateIDL::TypeHashMap_.find(std::string(memberMapKeyType));
            if (key_type_it != VBSGenerateIDL::TypeHashMap_.end()) {
                memberMapKeyType = (key_type_it->second).c_str();
            }
            const char* lengthStr = p_root->Attribute(xmlString::MAP_MAXLENGTH);
            uint32_t length = vbs::common::types::MAX_ELEMENTS_COUNT;
            if (lengthStr != nullptr) {
                try {
                    length = static_cast<uint32_t>(std::stoi(lengthStr));
                } catch (const std::exception&) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "Error parsing map member sequence length in line " << p_root->GetLineNum());
                    return;
                }
            }
            oss << "map<" << memberMapKeyType << ", " << idl_member_type;
            if (length != 0)
                oss << ", " << length;
            oss << " > " << memberName;
            appendArrayDimensions(oss, isArray, arrayDimsVec);
        }
    }
}

}  // namespace vbs
