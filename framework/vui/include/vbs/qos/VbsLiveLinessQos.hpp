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

#ifndef INCLUDE_QOS_LIVELINESSQOS_HPP_
#define INCLUDE_QOS_LIVELINESSQOS_HPP_

#include <utility>
#include <memory>

#include "deps/common/Types.h"

namespace vbsutil {
namespace xmlparser {
class LivelinessQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsLivelinessQosPolicy = vbsutil::xmlparser::LivelinessQosPolicy;
using VbsLivelinessQosPolicyKind = vbsutil::xmlparser::LivelinessQosPolicyKind;
class VbsLiveLinessQos {
 public:
    VbsLiveLinessQos();

    VbsLiveLinessQos(const VbsLiveLinessQos& qos);

    VbsLiveLinessQos(VbsLiveLinessQos&& qos);

    VbsLiveLinessQos& operator=(const VbsLiveLinessQos& b);

    VbsLiveLinessQos& operator=(VbsLiveLinessQos&& b);

    bool operator==(const VbsLiveLinessQos& qos) const;

    void clear();

    VbsLivelinessQosPolicyKind kind() const;

    void kind(const VbsLivelinessQosPolicyKind& new_kind);

    int64_t lease_duration() const;

    void lease_duration(const int64_t& new_lease_duration);

    int64_t announcement_period() const;

    void announcement_period(const int64_t& new_announcement_period);

    void set_dds_liveliness_qos(const EvbsLivelinessQosPolicy& new_dds_liveliness_qos);

    void get_dds_liveliness_qos(EvbsLivelinessQosPolicy& new_dds_liveliness_qos) const;

 private:
    std::unique_ptr<EvbsLivelinessQosPolicy> dds_liveliness_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_LIVELINESSQOS_HPP_
