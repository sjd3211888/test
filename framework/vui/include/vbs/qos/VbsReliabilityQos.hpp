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

#ifndef INCLUDE_QOS_RELIABILITYQOS_HPP_
#define INCLUDE_QOS_RELIABILITYQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class ReliabilityQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {

using EvbsReliabilityQosPolicy = vbsutil::xmlparser::ReliabilityQosPolicy;

enum VbsReliabilityQosPolicyKind { BEST_EFFORT_RELIABILITY_QOS = 0x01, RELIABLE_RELIABILITY_QOS = 0x02 };

class VbsReliabilityQos {
 public:
    VbsReliabilityQos();

    VbsReliabilityQos(const VbsReliabilityQos& qos);

    VbsReliabilityQos(VbsReliabilityQos&& qos);

    VbsReliabilityQos& operator=(const VbsReliabilityQos& b);

    VbsReliabilityQos& operator=(VbsReliabilityQos&& b);

    bool operator==(const VbsReliabilityQos& qos) const;

    void clear();

    VbsReliabilityQosPolicyKind kind() const;

    void kind(const VbsReliabilityQosPolicyKind& new_kind);

    int64_t max_blocking_time() const;

    void max_blocking_time(const int64_t& new_max_blocking_time);

    void set_dds_reliability_qos(const EvbsReliabilityQosPolicy& new_dds_reliability_qos);

    void get_dds_reliability_qos(EvbsReliabilityQosPolicy& new_dds_reliability_qos) const;

 private:
    int64_t vbs_max_blocking_time_;
    VbsReliabilityQosPolicyKind vbs_kind_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_RELIABILITYQOS_HPP_
