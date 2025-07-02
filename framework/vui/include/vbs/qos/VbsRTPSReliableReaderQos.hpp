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

#ifndef INCLUDE_QOS_RTPSRELIABLEREADERQOS_HPP_
#define INCLUDE_QOS_RTPSRELIABLEREADERQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class RTPSReliableReaderQos;
class DisablePositiveACKsQosPolicy;
class ReaderTimes;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsRTPSReliableReaderQos = vbsutil::xmlparser::RTPSReliableReaderQos;
using EvbsDisablePositiveACKsQosPolicy = vbsutil::xmlparser::DisablePositiveACKsQosPolicy;
using EvbsReaderTimes = vbsutil::xmlparser::ReaderTimes;

class VbsRTPSReliableReaderQos {
 public:
    VbsRTPSReliableReaderQos();

    VbsRTPSReliableReaderQos(const VbsRTPSReliableReaderQos& qos);

    VbsRTPSReliableReaderQos(VbsRTPSReliableReaderQos&& qos);

    VbsRTPSReliableReaderQos& operator=(const VbsRTPSReliableReaderQos& b);

    VbsRTPSReliableReaderQos& operator=(VbsRTPSReliableReaderQos&& b);

    bool operator==(const VbsRTPSReliableReaderQos& qos) const;

    void clear();

    const EvbsReaderTimes& times() const;

    const EvbsDisablePositiveACKsQosPolicy& disable_positive_ACKs() const;

    void set_dds_rtps_reliable_reader_qos(const EvbsRTPSReliableReaderQos& new_dds_rtps_reliable_reader_qos);

    void get_dds_rtps_reliable_reader_qos(EvbsRTPSReliableReaderQos& new_dds_rtps_reliable_reader_qos) const;

 private:
    std::unique_ptr<EvbsRTPSReliableReaderQos> dds_rtps_reliable_reader_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_RTPSRELIABLEREADERQOS_HPP_
