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

#ifndef INCLUDE_QOS_DURABILITYQOS_HPP_
#define INCLUDE_QOS_DURABILITYQOS_HPP_

#include <utility>
#include <memory>

#include "deps/common/Types.h"

namespace vbsutil {
namespace xmlparser {
class DurabilityQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using VbsDurabilityKind = vbsutil::xmlparser::DurabilityKind_t;
using EvbsDurabilityQosPolicy = vbsutil::xmlparser::DurabilityQosPolicy;
class VbsDurabilityQos {
 public:
    /**
     * @brief Constructor
     */
    VbsDurabilityQos();

    VbsDurabilityQos(const VbsDurabilityQos& qos);

    VbsDurabilityQos(VbsDurabilityQos&& qos);

    VbsDurabilityQos& operator=(const VbsDurabilityQos& b);

    VbsDurabilityQos& operator=(VbsDurabilityQos&& b);

    bool operator==(const VbsDurabilityQos& b) const;

    void clear();

    VbsDurabilityKind kind() const;

    void kind(const VbsDurabilityKind& qos_durability);

    void set_dds_durability_qos(const EvbsDurabilityQosPolicy& new_dds_durability_qos);

    void get_dds_durability_qos(EvbsDurabilityQosPolicy& new_dds_durability_qos) const;

 private:
    std::unique_ptr<EvbsDurabilityQosPolicy> dds_durability_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_DURABILITYQOS_HPP_
