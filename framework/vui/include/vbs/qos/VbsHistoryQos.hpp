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

#ifndef INCLUDE_QOS_HISTORYQOS_HPP_
#define INCLUDE_QOS_HISTORYQOS_HPP_

#include <utility>
#include <memory>

#include "deps/common/Types.h"

namespace vbsutil {
namespace xmlparser {
class HistoryQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using VbsHistoryQosPolicyKind = vbsutil::xmlparser::HistoryQosPolicyKind;
using EvbsHistoryQosPolicy = vbsutil::xmlparser::HistoryQosPolicy;
class VbsHistoryQos {
 public:
    VbsHistoryQos();

    VbsHistoryQos(const VbsHistoryQos& qos);

    VbsHistoryQos(VbsHistoryQos&& qos);

    VbsHistoryQos& operator=(const VbsHistoryQos& b);

    VbsHistoryQos& operator=(VbsHistoryQos&& b);

    bool operator==(const VbsHistoryQos& qos) const;

    void clear();

    VbsHistoryQosPolicyKind kind() const;

    void kind(const VbsHistoryQosPolicyKind& new_kind);

    int32_t depth() const;

    void depth(const int32_t& new_depth);

    void set_dds_history_qos(const EvbsHistoryQosPolicy& new_dds_history_qos);

    void get_dds_history_qos(EvbsHistoryQosPolicy& new_dds_history_qos) const;

 private:
    std::unique_ptr<EvbsHistoryQosPolicy> dds_history_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_HISTORYQOS_HPP_
