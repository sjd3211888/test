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

#ifndef INCLUDE_QOS_WIREPROTOCOLCONFIGQOS_HPP_
#define INCLUDE_QOS_WIREPROTOCOLCONFIGQOS_HPP_

#include <utility>
#include <memory>
#include <map>
#include <vector>

#include "deps/common/GuidPrefix_t.hpp"
#include "deps/common/Locator.h"

namespace vbsutil {
namespace xmlparser {
class LocatorList;
class LocatorWithMask;
class WireProtocolConfigQos;
class BuiltinAttributes;
class PortParameters;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {

using EvbsLocatorList = vbsutil::xmlparser::LocatorList;
using EvbsExternalLocators =
    std::map<uint8_t, std::map<uint8_t, std::vector<vbsutil::xmlparser::LocatorWithMask>>, std::greater<uint8_t>>;
using EvbsWireProtocolConfigQos = vbsutil::xmlparser::WireProtocolConfigQos;
using EvbsBuiltinAttributes = vbsutil::xmlparser::BuiltinAttributes;
using EvbsPortParameters = vbsutil::xmlparser::PortParameters;
using GuidPrefix_t = vbsutil::xmlparser::GuidPrefix_t;
class VbsWireProtocolConfigQos {
 public:
    VbsWireProtocolConfigQos();

    VbsWireProtocolConfigQos(const VbsWireProtocolConfigQos& qos);

    VbsWireProtocolConfigQos(VbsWireProtocolConfigQos&& qos);

    virtual ~VbsWireProtocolConfigQos();

    VbsWireProtocolConfigQos& operator=(const VbsWireProtocolConfigQos& b);

    VbsWireProtocolConfigQos& operator=(VbsWireProtocolConfigQos&& b);

    bool operator==(const VbsWireProtocolConfigQos& qos) const;

    void clear();

    GuidPrefix_t prefix() const;

    void prefix(const GuidPrefix_t& prefix);

    int32_t participant_id() const;

    void participant_id(int32_t id);

    const EvbsBuiltinAttributes& builtin() const;

    const EvbsPortParameters& port() const;

    const EvbsLocatorList& default_unicast_locator_list() const;

    const EvbsLocatorList& default_multicast_locator_list() const;

    const EvbsExternalLocators& default_external_unicast_locators() const;

    bool ignore_non_matching_locators() const;

    void ignore_non_matching_locators(bool flag) const;

    void set_dds_wire_protocol_config_qos(const EvbsWireProtocolConfigQos& new_dds_wire_protocol_config_qos);

    void get_dds_wire_protocol_config_qos(EvbsWireProtocolConfigQos& new_dds_wire_protocol_config_qos) const;

 private:
    std::unique_ptr<EvbsWireProtocolConfigQos> dds_wire_protocol_config_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_WIREPROTOCOLCONFIGQOS_HPP_
