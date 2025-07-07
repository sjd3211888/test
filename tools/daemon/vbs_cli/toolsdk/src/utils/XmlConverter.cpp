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

#include <fstream>
#include <filesystem>
#include <tinyxml2.h>
#include <elog/Log.hpp>

#include "utils/XmlConverter.hpp"

namespace vbstoolsdk {

bool rtixml_to_vbsxml_(tinyxml2::XMLDocument& rti_doc, tinyxml2::XMLDocument& vbs_doc);

bool process_rti_xml(const std::string& rti_xml, std::string& vbs_xml) {
    tinyxml2::XMLDocument rti_doc;
    tinyxml2::XMLDocument vbs_doc;

    if (tinyxml2::XML_SUCCESS != rti_doc.Parse(rti_xml.c_str())) {
        logError_("VBSTOOLSDK", "process_rti_xml parse error: " << rti_doc.ErrorStr());
        return false;
    }

    tinyxml2::XMLNode* firstNode = rti_doc.FirstChild();
    if (firstNode != nullptr && firstNode->ToDeclaration() == nullptr) {
        vbs_xml = rti_xml;
        return true;
    }

    if (false == rtixml_to_vbsxml_(rti_doc, vbs_doc)) {
        return false;
    }

    tinyxml2::XMLPrinter printer;
    vbs_doc.Print(&printer);
    vbs_xml = std::string(printer.CStr(), printer.CStrSize());
    return true;
}

bool rtixmlpath_to_vbsxml(const std::string& rti_xml_path, std::string& vbs_xml) {
    std::ifstream fileStream(rti_xml_path);
    if (!fileStream) {
        logError_("VBSTOOLSDK", "rtixmlpath_to_vbsxml cannot open file: " << rti_xml_path);
        return false;
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string fileContent = buffer.str();

    return process_rti_xml(fileContent, vbs_xml);
}

bool rtixml_to_vbsxml(const std::string& rti_xml, std::string& vbs_xml) {
    return process_rti_xml(rti_xml, vbs_xml);
}

bool rtixmlpath_to_vbsxmlpath(const std::string& rti_xml_path) {
    std::ifstream fileStream(rti_xml_path);
    if (!fileStream) {
        logError_("VBSTOOLSDK", "rtixmlpath_to_vbsxmlpath cannot open file: " << rti_xml_path);
        return false;
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string fileContent = buffer.str();

    std::string vbs_xml;
    if (!process_rti_xml(fileContent, vbs_xml)) {
        return false;
    }

    std::filesystem::path rti_xml_fs(rti_xml_path);
    std::string parent_path = rti_xml_fs.parent_path().string();
    std::string xml_filename = rti_xml_fs.stem().string() + "_vbs" + rti_xml_fs.extension().string();
    std::string vbs_xml_path = parent_path + "/" + xml_filename;

    std::ofstream outFile(vbs_xml_path);
    if (!outFile) {
        logError_("VBSTOOLSDK", "rtixmlpath_to_vbsxmlpath cannot write to file: " << vbs_xml_path);
        return false;
    }
    outFile << vbs_xml;
    return true;
}

bool rtixmlpath_to_vbsdyntype(const std::string& rti_xml_path, const std::string& type_name,
                              vbs::VBSDynamicType& type) {
    std::string vbs_xml;
    if (false == rtixmlpath_to_vbsxml(rti_xml_path, vbs_xml)) {
        return false;
    }

    vbs::VBSDynamicData::load_xml(vbs_xml.c_str(), vbs_xml.length());
    type = vbs::VBSDynamicData::get_dynamic_type(type_name);
    return true;
}

bool rtixml_to_vbsdyntype(const std::string& rti_xml, const std::string& type_name, vbs::VBSDynamicType& type) {
    std::string vbs_xml;
    if (false == rtixml_to_vbsxml(rti_xml, vbs_xml)) {
        return false;
    }

    vbs::VBSDynamicData::load_xml(vbs_xml.c_str(), vbs_xml.length());
    type = vbs::VBSDynamicData::get_dynamic_type(type_name);
    return true;
}

void union_discriminator_(tinyxml2::XMLElement* src_element) {
    for (tinyxml2::XMLElement* sub_element = src_element->FirstChildElement(); sub_element != nullptr;
         sub_element = sub_element->NextSiblingElement()) {
        if (std::string(sub_element->Value()) == "discriminator") {
            const char* basic_type_attr = sub_element->Attribute("nonBasicTypeName");
            if (basic_type_attr == nullptr) {
                continue;
            }
            sub_element->SetAttribute("type", basic_type_attr);
            sub_element->DeleteAttribute("nonBasicTypeName");
        }
    }
}

void extensible_to_appendable_(tinyxml2::XMLElement* src_element) {
    if (std::string(src_element->Value()) == "struct") {
        const char* extens_attr = src_element->Attribute("extensibility");
        if (extens_attr == nullptr) {
            return;
        }
        if (std::string(extens_attr) == "extensible") {
            src_element->SetAttribute("extensibility", "appendable");
        }
    }
}

void byte_to_uint8_(tinyxml2::XMLElement* src_element) {
    for (tinyxml2::XMLElement* sub_element = src_element->FirstChildElement(); sub_element != nullptr;
         sub_element = sub_element->NextSiblingElement()) {
        const char* name_attr = sub_element->Attribute("type");

        if (name_attr == nullptr) {
            continue;
        }
        if (std::string(name_attr) == "byte") {
            sub_element->SetAttribute("type", "uint8");
        }
    }
}

void add_type_to_element_(tinyxml2::XMLDocument& vbs_doc, tinyxml2::XMLElement* root_element,
                          tinyxml2::XMLElement* src_element) {
    tinyxml2::XMLElement* type_element = vbs_doc.NewElement("type");
    type_element->InsertEndChild(src_element);
    root_element->InsertEndChild(type_element);
}

bool module_to_element_(tinyxml2::XMLDocument& vbs_doc, tinyxml2::XMLElement* vbs_root_element,
                        tinyxml2::XMLElement* module_element, const std::string& module_name) {
    const char* name_attr = module_element->Attribute("name");
    std::string module_name_temp;
    module_name_temp = module_name + std::string(name_attr) + "::";
    for (tinyxml2::XMLElement* sub_element = module_element->FirstChildElement(); sub_element != nullptr;
         sub_element = sub_element->NextSiblingElement()) {
        if (std::string(sub_element->Value()) != "module") {
            tinyxml2::XMLElement* sub_element_clone =
                static_cast<tinyxml2::XMLElement*>(sub_element->DeepClone(&vbs_doc));

            if (std::string(sub_element_clone->Value()) == "union") {
                union_discriminator_(sub_element_clone);
            }
            byte_to_uint8_(sub_element_clone);
            extensible_to_appendable_(sub_element_clone);
            const char* name_attr = sub_element_clone->Attribute("name");
            std::string full_name = module_name_temp + std::string(name_attr);
            sub_element_clone->SetAttribute("name", full_name.c_str());

            add_type_to_element_(vbs_doc, vbs_root_element, sub_element_clone);
        } else {
            module_to_element_(vbs_doc, vbs_root_element, sub_element, module_name_temp);
        }
    }
    return true;
}

bool rtixml_to_vbsxml_(tinyxml2::XMLDocument& rti_doc, tinyxml2::XMLDocument& vbs_doc) {
    tinyxml2::XMLElement* rti_types_element = nullptr;
    tinyxml2::XMLElement* dds_element = rti_doc.FirstChildElement("dds");

    if (dds_element) {
        rti_types_element = dds_element->FirstChildElement("types");
    } else {
        rti_types_element = rti_doc.FirstChildElement("types");
    }
    if (!rti_types_element) {
        logError_("VBSTOOLSDK", "<types> element not found");
        return false;
    }

    tinyxml2::XMLElement* vbs_root_element = vbs_doc.NewElement("types");
    for (tinyxml2::XMLElement* rti_type_element = rti_types_element->FirstChildElement(); rti_type_element != nullptr;
         rti_type_element = rti_type_element->NextSiblingElement()) {
        if (std::string(rti_type_element->Value()) != "module") {
            tinyxml2::XMLElement* rti_type_element_clone =
                static_cast<tinyxml2::XMLElement*>(rti_type_element->DeepClone(&vbs_doc));
            if (std::string(rti_type_element_clone->Value()) == "union") {
                union_discriminator_(rti_type_element_clone);
            }
            byte_to_uint8_(rti_type_element_clone);
            extensible_to_appendable_(rti_type_element_clone);
            add_type_to_element_(vbs_doc, vbs_root_element, rti_type_element_clone);
        } else {
            module_to_element_(vbs_doc, vbs_root_element, rti_type_element, std::string());
        }
    }

    vbs_doc.LinkEndChild(vbs_root_element);
    return true;
}

}  // namespace vbstoolsdk
