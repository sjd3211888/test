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

#ifndef BASE_DAT_REPLAYER_HPP_
#define BASE_DAT_REPLAYER_HPP_

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "replayer/VbsReplayer.hpp"
#include "utils/DbOperator.hpp"

namespace vbstoolsdk {

class BaseDatReplayer : public VbsReplayer {
 public:
    static constexpr int16_t SELECT_NUM_ONCE = 100;

    BaseDatReplayer(const std::string& path, const std::string& xml_file_name, const std::string& participant_profile,
                    const std::string& writer_profile, int domain_id, bool rti_flag)
        : VbsReplayer(path, xml_file_name, participant_profile, writer_profile, domain_id, rti_flag) {}
    virtual ~BaseDatReplayer() {}

 protected:
    bool init_db_();
    void process_msg_() override;
    void control_speed_(uint64_t last, uint64_t cur) override;
    virtual bool writer_(DbOperator::DbRawDataInfo& info) = 0;
    virtual void reset_() = 0;

    std::shared_ptr<DbOperator> db_;
    std::vector<DbOperator::DbMetaDataInfo> meta_info_;
    std::unordered_map<int, uint32_t> topicid_domain_;
};

}  // namespace vbstoolsdk

#endif
