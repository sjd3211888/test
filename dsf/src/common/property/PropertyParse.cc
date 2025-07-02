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

#include "PropertyParse.h"

namespace li {
namespace dsfdds {

uint32_t PropertyParse::GetValueFromStr(const std::string* p_value,
                                        const std::unordered_map<std::string, uint32_t>& default_map,
                                        const std::string& field) {
    if (p_value == nullptr) {
        DSF_LOG_DEBUG(PropertyParse, field << " value string is nullptr,use default:" << default_map.at(field));
        return default_map.at(field);
    }
    try {
        size_t pos;
        uint64_t res = std::stoul(*p_value, &pos);
        if ((res > std::numeric_limits<uint32_t>::max()) || (pos < p_value->size())) {
            DSF_LOG_WARNING(PropertyParse, field << " value:" << *p_value << " is invalid conversion error");
            return 0;
        }
        return static_cast<uint32_t>(res);
    } catch (const std::exception& e) {
        DSF_LOG_WARNING(PropertyParse, field << " value:" << *p_value << " is invalid,exception e:" << e.what());
        return 0;
    }
}

double PropertyParse::GetValueFromStr(const std::string* p_value,
                                      const std::unordered_map<std::string, double>& default_map,
                                      const std::string& field) {
    if (p_value == nullptr) {
        DSF_LOG_DEBUG(PropertyParse, field << " value string is nullptr,use default:" << default_map.at(field));
        return default_map.at(field);
    }
    try {
        size_t pos;
        double res = std::stod(*p_value, &pos);
        if (pos < p_value->size()) {
            DSF_LOG_WARNING(PropertyParse, field << " value:" << *p_value << " is invalid conversion error");
            return 0.0;
        }
        return res;
    } catch (const std::exception& e) {
        DSF_LOG_WARNING(PropertyParse, field << " value:" << *p_value << " is invalid,exception e:" << e.what());
        return 0.0;
    }
}

}  // namespace dsfdds
}  // namespace li