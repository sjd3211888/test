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

#ifndef INCLUDE_QOS_OWNERSHIPSTRENGTHQOS_HPP_
#define INCLUDE_QOS_OWNERSHIPSTRENGTHQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class OwnershipStrengthQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsOwnershipStrengthQosPolicy = vbsutil::xmlparser::OwnershipStrengthQosPolicy;
class VbsOwnershipStrengthQos {
 public:
    VbsOwnershipStrengthQos();

    VbsOwnershipStrengthQos(const VbsOwnershipStrengthQos& qos);

    VbsOwnershipStrengthQos(VbsOwnershipStrengthQos&& qos);

    VbsOwnershipStrengthQos& operator=(const VbsOwnershipStrengthQos& b);

    VbsOwnershipStrengthQos& operator=(VbsOwnershipStrengthQos&& b);

    bool operator==(const VbsOwnershipStrengthQos& qos) const;

    void clear();

    uint32_t value() const;

    void value(const uint32_t& new_value);

    void set_dds_ownership_strength_qos(const EvbsOwnershipStrengthQosPolicy& new_dds_ownership_strength_qos);

    void get_dds_ownership_strength_qos(EvbsOwnershipStrengthQosPolicy& new_dds_ownership_strength_qos) const;

 private:
    std::unique_ptr<EvbsOwnershipStrengthQosPolicy> dds_ownership_strength_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_OWNERSHIPSTRENGTHQOS_HPP_
