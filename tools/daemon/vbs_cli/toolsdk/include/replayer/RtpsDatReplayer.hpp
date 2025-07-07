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

#ifndef COMMON_REPLAYER_HPP_
#define COMMON_REPLAYER_HPP_

#include <unordered_map>

#include <ertps/rtps/RTPSDomain.h>
#include <ertps/rtps/participant/RTPSParticipant.h>
#include <ertps/rtps/writer/RTPSWriter.h>
#include <ertps/rtps/writer/WriterListener.h>
#include <ertps/rtps/history/WriterHistory.h>

#include "replayer/BaseDatReplayer.hpp"

namespace vbstoolsdk {

namespace ERTPS = evbs::ertps;
namespace ERTPS_RTPS = evbs::ertps::rtps;

class RtpsDatReplayer : public BaseDatReplayer {
 public:
    RtpsDatReplayer(const std::string& path);
    ~RtpsDatReplayer();
    bool reinit() override;
    bool reinit(const std::string& path) override;

 private:
    void reset_() override;
    bool init_();
    ERTPS_RTPS::RTPSParticipant* create_participant_(uint32_t domain);
    bool create_writer_(ERTPS_RTPS::RTPSParticipant* participant, DbOperator::DbMetaDataInfo& info);
    bool writer_(DbOperator::DbRawDataInfo& info);

    std::unordered_map<uint32_t, ERTPS_RTPS::RTPSParticipant*> participants_;
    std::unordered_map<int, ERTPS_RTPS::RTPSWriter*> writers_;
    std::unordered_map<int, ERTPS_RTPS::WriterHistory*> historys_;
};

}  // namespace vbstoolsdk

#endif
