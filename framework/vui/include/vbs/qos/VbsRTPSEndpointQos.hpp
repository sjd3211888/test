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

#ifndef INCLUDE_QOS_RTPSENDPOINTQOS_HPP_
#define INCLUDE_QOS_RTPSENDPOINTQOS_HPP_

#include <utility>
#include <memory>
#include <map>

#include "deps/common/Locator.h"

namespace vbsutil {
namespace xmlparser {
class LocatorList;
class LocatorWithMask;
class RTPSEndpointQos;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsLocatorList = vbsutil::xmlparser::LocatorList;
using EvbsExternalLocators =
    std::map<uint8_t, std::map<uint8_t, std::vector<vbsutil::xmlparser::LocatorWithMask>>, std::greater<uint8_t>>;
using EvbsRTPSEndpointQos = vbsutil::xmlparser::RTPSEndpointQos;

class VbsRTPSEndpointQos {
 public:
    VbsRTPSEndpointQos();

    VbsRTPSEndpointQos(const VbsRTPSEndpointQos& qos);

    VbsRTPSEndpointQos(VbsRTPSEndpointQos&& qos);

    VbsRTPSEndpointQos& operator=(const VbsRTPSEndpointQos& b);

    VbsRTPSEndpointQos& operator=(VbsRTPSEndpointQos&& b);

    bool operator==(const VbsRTPSEndpointQos& b) const;

    int16_t user_defined_id() const;

    void user_defined_id(const int16_t& new_d);

    int16_t entity_id() const;

    void entity_id(const int16_t& new_id);

    const EvbsLocatorList& get_unicast_locator_list() const;

    const EvbsLocatorList& get_multicast_locator_list() const;

    const EvbsLocatorList& get_remote_locator_list() const;

    const EvbsExternalLocators& get_external_unicast_locators() const;

    void set_dds_rtps_endpoint_qos(const EvbsRTPSEndpointQos& new_dds_rtps_endpoint_qos);

    void get_dds_rtps_endpoint_qos(EvbsRTPSEndpointQos& new_dds_rtps_endpoint_qos) const;

 private:
    std::unique_ptr<EvbsRTPSEndpointQos> dds_rtps_endpoint_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_RTPSENDPOINTQOS_HPP_
