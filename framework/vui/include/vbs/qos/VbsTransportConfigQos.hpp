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

#ifndef INCLUDE_QOS_TRANSPORTCONFIGQOS_HPP_
#define INCLUDE_QOS_TRANSPORTCONFIGQOS_HPP_

#include <utility>
#include <memory>
#include <map>
#include <vector>

#include "deps/common/GuidPrefix_t.hpp"
#include "deps/common/Locator.h"
#include "deps/common/Types.h"

namespace vbsutil {
namespace xmlparser {
class LocatorList;
class LocatorWithMask;
class WireProtocolConfigQos;
class BuiltinAttributes;
class PortParameters;
}  // namespace xmlparser
}  // namespace vbsutil

namespace evbs {
namespace edds {
namespace dds {
class TransportConfigQos;
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EvbsTransportConfigQos = evbs::edds::dds::TransportConfigQos;
using TransportTypeKind = vbsutil::xmlparser::TransportTypeKind;
class VbsTransportConfigQos {
 public:
    VbsTransportConfigQos();

    VbsTransportConfigQos(const VbsTransportConfigQos& qos);

    VbsTransportConfigQos(VbsTransportConfigQos&& qos);

    virtual ~VbsTransportConfigQos();

    VbsTransportConfigQos& operator=(const VbsTransportConfigQos& b);

    VbsTransportConfigQos& operator=(VbsTransportConfigQos&& b);

    bool operator==(const VbsTransportConfigQos& qos) const;

    void clear();

    bool use_builtin_transports() const;

    void use_builtin_transports(bool flag);

    TransportTypeKind transport_type() const;

    void transport_type(TransportTypeKind kind);

    uint32_t send_socket_buffer_size() const;

    void send_socket_buffer_size(uint32_t sz);

    uint32_t listen_socket_buffer_size() const;

    void listen_socket_buffer_size(uint32_t sz);

    void set_dds_transport_config_qos(const EvbsTransportConfigQos& new_dds_transport_config_qos);

    void get_dds_transport_config_qos(EvbsTransportConfigQos& new_dds_transport_config_qos) const;

 private:
    std::unique_ptr<EvbsTransportConfigQos> dds_transport_config_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_TRANSPORTCONFIGQOS_HPP_
