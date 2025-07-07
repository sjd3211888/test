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

#ifndef _XML_CONVERTER_HPP_
#define _XML_CONVERTER_HPP_

#include <string>

#include <vbs/types/VBSDynamicData.h>
#include <vbs/types/VBSDynamicType.h>

namespace vbstoolsdk {

bool rtixmlpath_to_vbsxml(const std::string& rti_xml_path, std::string& vbs_xml);
bool rtixml_to_vbsxml(const std::string& rti_xml, std::string& vbs_xml);
bool rtixmlpath_to_vbsxmlpath(const std::string& rti_xml_path);

bool rtixmlpath_to_vbsdyntype(const std::string& rti_xml_path, const std::string& type_name, vbs::VBSDynamicType& type);
bool rtixml_to_vbsdyntype(const std::string& rti_xml, const std::string& type_name, vbs::VBSDynamicType& type);

}  // namespace vbstoolsdk

#endif
