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

#ifndef INCLUDE_QOS_OWNERSHIPQOS_HPP_
#define INCLUDE_QOS_OWNERSHIPQOS_HPP_

#include <utility>
#include <memory>

#include "deps/common/Types.h"

namespace vbsutil {
namespace xmlparser {
class OwnershipQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsOwnershipQosPolicy = vbsutil::xmlparser::OwnershipQosPolicy;
using VbsOwnershipQosPolicyKind = vbsutil::xmlparser::OwnershipQosPolicyKind;
class VbsOwnershipQos {
 public:
    VbsOwnershipQos();

    VbsOwnershipQos(const VbsOwnershipQos& qos);

    VbsOwnershipQos(VbsOwnershipQos&& qos);

    VbsOwnershipQos& operator=(const VbsOwnershipQos& b);

    VbsOwnershipQos& operator=(VbsOwnershipQos&& b);

    bool operator==(const VbsOwnershipQos& qos) const;

    void clear();

    VbsOwnershipQosPolicyKind kind() const;

    void kind(const VbsOwnershipQosPolicyKind& new_kind);

    void set_dds_ownership_qos(const EvbsOwnershipQosPolicy& new_dds_owner_ship_qos);

    void get_dds_ownership_qos(EvbsOwnershipQosPolicy& new_dds_owner_ship_qos) const;

 private:
    std::unique_ptr<EvbsOwnershipQosPolicy> dds_owner_ship_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_OWNERSHIPQOS_HPP_
