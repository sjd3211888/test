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

#ifndef VBS_DAT_REPLAYER_HPP_
#define VBS_DAT_REPLAYER_HPP_

#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <unordered_map>

#include <vbs/DomainParticipant.hpp>
#include <vbs/DataReader.hpp>
#include <vbs/DataReaderListener.hpp>
#include <vbs/DataWriterListener.hpp>
#include <vbs/Global.hpp>
#include <vbs/DataReaderQos.hpp>
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/VBSDynamicTypeBuilderFactory.h"
#include "vbs/types/VBSDynamicDataFactory.h"
#include "vbs/types/VBSDynamicType.h"
#include "vbs/types/VBSDynamicTypeBuilder.h"

#include "replayer/BaseDatReplayer.hpp"

namespace vbstoolsdk {

class VbsDatReplayer : public BaseDatReplayer {
 public:
    VbsDatReplayer(const std::string& path, const std::string& xml_file_name, const std::string& participant_profile,
                   const std::string& writer_profile, int domain_id, bool rti_flag);
    ~VbsDatReplayer();
    bool reinit() override;
    bool reinit(const std::string& path) override;

 private:
    void reset_() override;
    bool init_(const std::string& xml_file_name, const std::string& participant_profile,
               const std::string& writer_profile);
    vbs::DomainParticipant* create_participant_(const uint32_t domain_id, const std::string& xml_file_name,
                                                const std::string& participant_profile);
    bool create_writer_(vbs::DomainParticipant* participant, DbOperator::DbMetaDataInfo& info,
                        const std::string& writer_profile);
    bool writer_(DbOperator::DbRawDataInfo& info) override;

    std::unordered_map<uint32_t, vbs::DomainParticipant*> participants_;
    std::unordered_map<int, vbs::VBSDynamicType> dyn_types_;
    std::unordered_map<int, vbs::Topic*> topics_;
    std::unordered_map<int, vbs::DataWriter*> writers_;

    class PubListener : public vbs::DataWriterListener {
     public:
        PubListener() {}
        ~PubListener() {}
        void on_publication_matched(vbs::DataWriter* writer, const vbs::PublicationMatchedStatus& info) override;
    };
    PubListener* writer_listener_;
};

}  // namespace vbstoolsdk

#endif
