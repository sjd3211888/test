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
#include <sstream>
#include <elog/Log.hpp>
#include "utils/DbOperator.hpp"
#include "utils/DbUser.hpp"

namespace vbstoolsdk {

bool get_meta_info_(const std::string& db_path, std::vector<DbOperator::DbMetaDataInfo>& meta_infos);

bool db_xml_to_path(const std::string& db_path) {
    std::vector<DbOperator::DbMetaDataInfo> meta_infos;
    if (get_meta_info_(db_path, meta_infos)) {
        for (auto& meta_info : meta_infos) {
            std::string file_name = meta_info.topic_name + ".xml";
            std::ofstream out_file(file_name);
            if (out_file.is_open()) {
                out_file << meta_info.topic_idl_xml;
                out_file.close();
            }
        }
        return true;
    }
    return false;
}

bool db_idl_to_path(const std::string& db_path) {
    std::vector<DbOperator::DbMetaDataInfo> meta_infos;
    if (get_meta_info_(db_path, meta_infos)) {
        for (auto& meta_info : meta_infos) {
            std::string file_name = meta_info.topic_name + ".idl";
            std::ofstream out_file(file_name);
            if (out_file.is_open()) {
                out_file << meta_info.topic_idl;
                out_file.close();
            }
        }
        return true;
    }
    return false;
}

bool get_meta_info_(const std::string& db_path, std::vector<DbOperator::DbMetaDataInfo>& meta_infos) {
    std::shared_ptr<DbOperator> db = std::make_shared<DbOperator>(db_path);
    if (nullptr == db) {
        logError_("VBSTOOLSDK", "Utils db init failed, full_path: " << db_path);
        return false;
    }

    if (false == db->get_metadata(meta_infos)) {
        logError_("VBSTOOLSDK", "Utils db get_metadata failed");
        return false;
    }

    for (auto& meta_info : meta_infos) {
        for (char& ch : meta_info.topic_name) {
            if (ch == '/') {
                ch = '_';
            }
        }
    }
    return true;
}

}  // namespace vbstoolsdk
