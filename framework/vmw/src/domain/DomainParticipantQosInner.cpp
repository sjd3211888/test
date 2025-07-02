// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: (security): Security enabled by default
// ------------------------------------------------------------------

#include "domain/qos/DomainParticipantQosInner.hpp"

namespace vbs {

DomainParticipantQosInner::DomainParticipantQosInner() {
#if defined(HAVE_SECURITY) && HAVE_SECURITY && defined(VBSAUTH) && VBSAUTH
    // Activate DDS:Auth:PKI-DH plugin
    properties().properties().emplace_back("dds.sec.auth.plugin", "builtin.PKI-DH");

    // Configure DDS:Auth:PKI-DH plugin
    properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.identity_ca",
                                           "liddsauth://security/LiDDS_cacert.pem");
    properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.identity_certificate",
                                           "liddsauth://security/LiDDS_device_cert.pem");
    properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.private_key",
                                           "liddsauth://security/LiDDS_device_key.pem");
#endif
}

const DomainParticipantQosInner PARTICIPANT_QOS_DEFAULT;

void DomainParticipantQosInner::exportParticipantQosInfo() const {
    logDebug(
        PARTICIPANT,
        " Participant name: "
            << name_ << std::endl
            << " max_multicast_locators: " << allocation_.locators.max_multicast_locators << std::endl
            << " max_unicast_locators: " << allocation_.locators.max_unicast_locators << std::endl
            << " use_builtin_transports: " << transport_.use_builtin_transports << std::endl
            << " send_socket_buffer_size: " << transport_.send_socket_buffer_size << std::endl
            << " listen_socket_buffer_size: " << transport_.listen_socket_buffer_size << std::endl
            << " transport_type: " << transport_.transport_type << std::endl
            << " prefix " << wire_protocol_.prefix << std::endl
            << " participant_id: " << wire_protocol_.participant_id << std::endl
            << " default_unicast_locator_list: " << wire_protocol_.default_unicast_locator_list << std::endl
            << " default_multicast_locator_list: " << wire_protocol_.default_multicast_locator_list << std::endl
            << " leaseDuration: " << wire_protocol_.builtin.discovery_config.leaseDuration << std::endl
            << " leaseDuration_announcement period: "
            << wire_protocol_.builtin.discovery_config.leaseDuration_announcementperiod << std::endl
            << " initial_announcements count: " << wire_protocol_.builtin.discovery_config.initial_announcements.count
            << std::endl
            << " initial_announcements period: " << wire_protocol_.builtin.discovery_config.initial_announcements.period
            << std::endl
            << " metatraffic_external_unicast_locators: "
            << wire_protocol_.builtin.metatraffic_external_unicast_locators << std::endl
            << " metatrafficUnicastLocatorList: " << wire_protocol_.builtin.metatrafficUnicastLocatorList << std::endl
            << " metatrafficMulticastLocatorList: " << wire_protocol_.builtin.metatrafficMulticastLocatorList
            << std::endl
            << " initialPeersList: " << wire_protocol_.builtin.initialPeersList << std::endl
            << " avoid_builtin_multicast: " << wire_protocol_.builtin.avoid_builtin_multicast << std::endl
            << " readerHistoryMemoryPolicy: " << wire_protocol_.builtin.readerHistoryMemoryPolicy << std::endl
            << " readerPayloadSize: " << wire_protocol_.builtin.readerPayloadSize << std::endl
            << " writerHistoryMemoryPolicy: " << wire_protocol_.builtin.writerHistoryMemoryPolicy << std::endl
            << " writerPayloadSize: " << wire_protocol_.builtin.writerPayloadSize << std::endl
            << " avoid_builtin_multicast: " << wire_protocol_.builtin.avoid_builtin_multicast << std::endl
            << " lookup_config client: " << wire_protocol_.builtin.lookup_config.use_client << std::endl
            << " lookup_config server: " << wire_protocol_.builtin.lookup_config.use_server << std::endl
            << " discoveryProtocol: " << wire_protocol_.builtin.discovery_config.discoveryProtocol << std::endl
            << " flow_controllers_: " << flow_controllers_ << std::endl
            << " properties_: " << properties_.properties() << std::endl
            << " binary_properties_: " << properties_.binary_properties() << std::endl);
}

} /* namespace vbs */
