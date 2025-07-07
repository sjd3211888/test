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

#ifndef _DB_OPERATOR_HPP_
#define _DB_OPERATOR_HPP_

#include <string>
#include <vector>
#include <memory>

namespace vbstoolsdk {

class DbOperator {

 public:
    struct DbMetaDataInfo {
        int topic_id;
        std::string topic_name;
        std::string data_type;
        std::string topic_type_name;
        std::string topic_idl;
        std::string topic_idl_xml;
        int64_t start_timestamp;
        int64_t end_timestamp;
        std::string description;
        int domain_id;
    };

    struct UpdateTimestampInfo {
        std::string topic_name;
        std::string key;
        int64_t value;
    };

    struct DbRawDataInfo {
        int topic_id;
        int64_t raw_timestamp;
        int64_t local_timestamp;
        std::vector<uint8_t> raw_data;
        int sequence_number;
        std::string raw_json;
    };

    DbOperator(const std::string& full_path, bool need_create = false);
    ~DbOperator();
    bool add_metadata(DbMetaDataInfo& info);
    bool add_metadata(std::vector<DbMetaDataInfo>& info_vec);
    bool get_metadata(std::vector<DbMetaDataInfo>& info_vec);
    bool update_metadata_timestamp(UpdateTimestampInfo& info);
    bool add_rawdata(DbRawDataInfo& info);
    bool add_rawdata(std::vector<DbRawDataInfo>& info_vec);
    bool is_topic_name_existed(const std::string& topic_name, int& topic_id);
    int get_rawdata(std::vector<DbRawDataInfo>& info_vec, int begin, int end);
    int get_rawdata_count();

 private:
    void init_db_();
    void db_exec_cmd_(const std::string& command);

 private:
    bool if_new_tables_;
    std::string db_path_;
    void* db_;
};

}  // namespace vbstoolsdk

#endif
