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

#ifndef VBS_DISCOVERER_HPP_
#define VBS_DISCOVERER_HPP_

#include <unordered_map>
#include <vector>

#include "domain/DomainParticipantImpl.hpp"
#include "topic/TopicImpl.hpp"
#include <domain/DomainParticipantListenerInner.hpp>
#include <sub/Subscriber.hpp>
#include <sub/DataReaderImpl.hpp>
#include <sub/DataReaderListenerInner.hpp>

#include <discoverer/BaseDiscoverer.hpp>

namespace vbstoolsdk {

namespace EDDS = evbs::edds;
namespace EDDS_DDS = evbs::edds::dds;

class VbsDiscoverer : public BaseDiscoverer {
    friend class ParticipantListener;
    friend class ReaderListener;

 public:
    VbsDiscoverer(uint32_t domain_id, const std::string& xml_file_name = "",
                  const std::string& participant_profile = "", bool type_discovery_flag = false);
    ~VbsDiscoverer();

 private:
    bool create_participant_(uint32_t domain_id, const std::string& xml_file_name,
                             const std::string& participant_profile);
    bool is_type_discovered_(const std::string&, ERTPS::types::DynamicType_ptr);
    void register_remote_type_(const std::string& type_name);
    void register_type_(vbs::DomainParticipantImpl* participant, ERTPS::types::DynamicType_ptr type);
    std::string get_address_(const ERTPS_RTPS::ParticipantProxyData& info);
    bool search_address_in_locators_(const ERTPS::ResourceLimitedVector<ERTPS_RTPS::Locator_t>& locators,
                                     std::string& address);

    std::unordered_map<std::string, ERTPS::types::DynamicType_ptr> discovered_types_;
    vbs::DomainParticipantImpl* participant_;
    vbs::Subscriber* subscriber_;
    std::vector<vbs::TopicImpl*> topics_;
    std::unordered_map<vbs::DataReaderImpl*, ERTPS::types::DynamicData_ptr> readers_dyndatas_;

    class ParticipantListener : public vbs::DomainParticipantListenerInner {
     public:
        ParticipantListener() {}

        ParticipantListener(VbsDiscoverer* parent) : parent_(parent) {}

        ~ParticipantListener() {}

        void on_participant_discovery(vbs::DomainParticipantImpl* participant,
                                      ERTPS_RTPS::ParticipantDiscoveryInfo&& info) override;

        void on_subscriber_discovery(vbs::DomainParticipantImpl* participant,
                                     ERTPS_RTPS::ReaderDiscoveryInfo&& info) override;

        void on_publisher_discovery(vbs::DomainParticipantImpl* participant,
                                    ERTPS_RTPS::WriterDiscoveryInfo&& info) override;

        void on_type_discovery(vbs::DomainParticipantImpl* participant, const std::string& type_name,
                               ERTPS::types::DynamicType_ptr dyn_type) override;

     private:
        VbsDiscoverer* parent_;
    };

    ParticipantListener* participant_listener_;
    // ReaderListener* reader_listener_;
};

}  // namespace vbstoolsdk

#endif
