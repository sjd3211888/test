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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTLISTENER_HPP_
#define INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTLISTENER_HPP_

#include "edds/rtps/participant/ParticipantDiscoveryInfo.h"
#include "edds/rtps/reader/ReaderDiscoveryInfo.h"
#include "edds/rtps/writer/WriterDiscoveryInfo.h"
#include "ertps/types/TypeIdentifier.h"
#include "ertps/types/TypeObject.h"
#include "deps/type/DynamicTypePtr.h"
#include "pub/PublisherListener.hpp"
#include "sub/SubscriberListener.hpp"
#include "topic/TopicListener.hpp"

namespace vbs {
class DomainParticipantImpl;

/**
 * Class DomainParticipantListenerInner, overrides behaviour towards certain events.
 *
 * @ingroup EDDS_MODULE
 */
class DomainParticipantListenerInner : public PublisherListener, public SubscriberListener, public TopicListener {
 public:
    /**
     * @brief Constructor
     */
    DomainParticipantListenerInner() {}

    /**
     * @brief Destructor
     */
    virtual ~DomainParticipantListenerInner() {}

    /*!
     * This method is called when a new Participant is discovered, or a previously discovered
     * participant changes its QOS or is removed.
     *
     * @param participant Pointer to the Participant which discovered the remote participant.
     * @param info Remote participant information. User can take ownership of the object.
     */
    virtual void on_participant_discovery(vbs::DomainParticipantImpl* participant,
                                          evbs::ertps::rtps::ParticipantDiscoveryInfo&& info) {
        (void)participant, (void)info;
    }

#if HAVE_SECURITY
    /*!
     * This method is called when a new Participant is authenticated.
     *
     * @param participant Pointer to the authenticated Participant.
     * @param info Remote participant authentication information. User can take ownership of the
     * object.
     */
    virtual void onParticipantAuthentication(vbs::DomainParticipantImpl* participant,
                                             vbs::ParticipantAuthenticationInfo&& info) {
        (void)participant, (void)info;
    }

#endif  // if HAVE_SECURITY

    /*!
     * This method is called when a new Subscriber is discovered, or a previously discovered
     * subscriber changes its QOS or is removed.
     *
     * @param participant Pointer to the Participant which discovered the remote subscriber.
     * @param info Remote subscriber information. User can take ownership of the object.
     */
    virtual void on_subscriber_discovery(vbs::DomainParticipantImpl* participant,
                                         evbs::ertps::rtps::ReaderDiscoveryInfo&& info) {
        (void)participant, (void)info;
    }

    /*!
     * This method is called when a new Publisher is discovered, or a previously discovered
     * publisher changes its QOS or is removed.
     *
     * @param participant Pointer to the Participant which discovered the remote publisher.
     * @param info Remote publisher information. User can take ownership of the object.
     */
    virtual void on_publisher_discovery(vbs::DomainParticipantImpl* participant,
                                        evbs::ertps::rtps::WriterDiscoveryInfo&& info) {
        (void)participant, (void)info;
    }

    virtual void on_type_discovery(vbs::DomainParticipantImpl* participant, const std::string& type_name,
                                   vbs::common::types::DynamicType_ptr dyn_type) {
        static_cast<void>(participant);
        static_cast<void>(type_name);
        static_cast<void>(dyn_type);
    }
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_DOMAIN_DOMAINPARTICIPANTLISTENER_HPP_
