// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef VBS_CPP_DISCOVERY_SERVER__HELPERS_HPP
#define VBS_CPP_DISCOVERY_SERVER__HELPERS_HPP

#include <edds/rtps/common/GuidPrefix_t.hpp>
#include <ertps/utils/LocatorAlias.h>

//! Transport kind enumeration
enum class TransportKind : uint8_t {
    UDPv4,
    UDPv6,
    TCPv4,
    TCPv6,
    SHM,
};

inline bool is_ip(const std::string ip_str) {
    return evbs::ertps::rtps::IPLocator::isIPv4(ip_str);
}

inline std::string get_ip_from_dns(const std::string& domain_name, int32_t kind) {
    std::pair<std::set<std::string>, std::set<std::string>> dns_response =
        evbs::ertps::rtps::IPLocator::resolveNameDNS(domain_name);

    if (kind == LOCATOR_KIND_UDPv4) {
        if (dns_response.first.empty()) {
            std::cout << "Not DNS found for IPv4 for " << domain_name << std::endl;
            return "";
        } else {
            std::string solution(*dns_response.first.begin());
            std::cout << "DNS found for " << domain_name << " => " << solution << std::endl;
            return solution;
        }
    }

    return domain_name;
}

#endif  // VBS_CPP_DISCOVERY_SERVER__HELPERS_HPP
