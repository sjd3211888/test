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

#ifndef INCLUDE_QOS_RTPSRELIABLEWRITERQOS_HPP_
#define INCLUDE_QOS_RTPSRELIABLEWRITERQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class RTPSReliableWriterQos;
class DisablePositiveACKsQosPolicy;
class WriterTimes;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsRTPSReliableWriterQos = vbsutil::xmlparser::RTPSReliableWriterQos;
using EvbsDisablePositiveACKsQosPolicy = vbsutil::xmlparser::DisablePositiveACKsQosPolicy;
using EvbsWriterTimes = vbsutil::xmlparser::WriterTimes;

class VbsRTPSReliableWriterQos {
 public:
    VbsRTPSReliableWriterQos();

    VbsRTPSReliableWriterQos(const VbsRTPSReliableWriterQos& qos);

    VbsRTPSReliableWriterQos(VbsRTPSReliableWriterQos&& qos);

    VbsRTPSReliableWriterQos& operator=(const VbsRTPSReliableWriterQos& b);

    VbsRTPSReliableWriterQos& operator=(VbsRTPSReliableWriterQos&& b);

    bool operator==(const VbsRTPSReliableWriterQos& qos) const;

    void clear();

    EvbsWriterTimes times() const;

    EvbsDisablePositiveACKsQosPolicy disable_positive_acks() const;

    bool disable_heartbeat_piggyback() const;

    void disable_heartbeat_piggyback(const bool& new_heartbeat);

    int32_t heartbeat_per_max_samples() const;

    void heartbeat_per_max_samples(const int32_t& new_heartbeat);

    void set_dds_rtps_reliability_writer_qos(const EvbsRTPSReliableWriterQos& new_dds_rtps_reliability_writer_qos);

    void get_dds_rtps_reliability_writer_qos(EvbsRTPSReliableWriterQos& new_dds_rtps_reliability_writer_qos) const;

 private:
    std::unique_ptr<EvbsRTPSReliableWriterQos> dds_rtps_reliability_writer_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_RTPSRELIABLEWRITERQOS_HPP_
