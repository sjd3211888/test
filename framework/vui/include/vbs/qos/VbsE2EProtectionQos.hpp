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

#ifndef INCLUDE_QOS_E2EPROTECTIONQOS_HPP_
#define INCLUDE_QOS_E2EPROTECTIONQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class E2EProtectionQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsE2EProtectionQosPolicy = vbsutil::xmlparser::E2EProtectionQosPolicy;
class VbsE2EProtectionQos {
 public:
    VbsE2EProtectionQos();

    VbsE2EProtectionQos(const VbsE2EProtectionQos& qos);

    VbsE2EProtectionQos(VbsE2EProtectionQos&& qos);

    VbsE2EProtectionQos& operator=(const VbsE2EProtectionQos& b);

    VbsE2EProtectionQos& operator=(VbsE2EProtectionQos&& b);

    bool operator==(const VbsE2EProtectionQos& qos) const;

    void clear();

    bool enabled() const;

    void enabled(const bool& new_enable);

    uint16_t MinDataLength() const;

    void MinDataLength(const uint16_t& new_length);

    uint16_t MaxDataLength() const;

    void MaxDataLength(const uint16_t& new_length);

    uint16_t MaxDeltaCounter() const;

    void MaxDeltaCounter(const uint16_t& new_counter);

    void set_dds_e2e_protection_qos(const EvbsE2EProtectionQosPolicy& new_dds_e2e_protection_qos);

    void get_dds_e2e_protection_qos(EvbsE2EProtectionQosPolicy& new_dds_e2e_protection_qos) const;

 private:
    std::unique_ptr<EvbsE2EProtectionQosPolicy> dds_e2e_protection_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_E2EPROTECTIONQOS_HPP_
