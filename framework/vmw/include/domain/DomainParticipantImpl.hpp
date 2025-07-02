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
// feature: VBS support SOA log collection.
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager
// ------------------------------------------------------------------

#ifndef INCLUDE_DOMAIN_DOMAINPARTICIPANT_HPP_
#define INCLUDE_DOMAIN_DOMAINPARTICIPANT_HPP_

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "edds/dds/builtin/lookup/LookupRequestListener.hpp"
#include "xmlparser/qos/XMLSubscriberQos.hpp"
#include "edds/rtps/interfaces/IContentFilterFactory.hpp"
#include "history/SubstrateParticipant.h"
#include "history/ParticipantListener.h"
#include "edds/rtps/reader/StatefulReader.h"
#include "dispatcher/DispatcherParticipant.hpp"
#include "ertps/rtps/attributes/EndpointAttributes.h"
#include "edds/dds/builtin/lookup/common/LookupTypes.hpp"
#include "edds/dds/builtin/topic/ParticipantBuiltinTopicData.hpp"
#include "edds/dds/builtin/topic/TopicBuiltinTopicData.hpp"
#include "deps/core/status/StatusMask.hpp"
#include "core/Entity.hpp"
#include "domain/qos/DomainParticipantQosInner.hpp"
#include "deps/common/TypeSupport.hpp"
#include "topic/TopicImpl.hpp"
#include "topic/TopicListener.hpp"
#include "topic/ContentFilteredTopicInner.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"
#include "xmlparser/attributes/XMLPublisherAttributes.h"
#include "xmlparser/attributes/XMLSubscriberAttributes.h"
#include "xmlparser/qos/XMLPublisherQos.hpp"
#include "xmlparser/qos/XMLSubscriberQos.hpp"
#include "edds/rtps/resources/ResourceEvent.h"
#include "discovery/database/DiscoveryEndpointInfo.hpp"
#include "discovery/database/DiscoveryParticipantInfo.hpp"
#include "deps/common/GuidPrefix_t.hpp"
#include "vbs/DomainParticipantListener.hpp"
#include "vbs/ContentFilteredTopic.hpp"

using evbs::ertps::Duration_t;
using namespace evbs::ertps::rtps::ddb;

namespace evbs {
namespace ertps {

class ParticipantAttributes;
using vbsutil::xmlparser::PublisherAttributes;
using vbsutil::xmlparser::SubscriberAttributes;
namespace rtps {
class WriterProxyData;
class ReaderProxyData;
}  // namespace rtps

}  // namespace ertps
}  // namespace evbs

namespace vbs {
namespace common {
// class ResourceEvent;
class ParticipantListener;
namespace types {
class TypeInformation;
}  // namespace types
}  // namespace common

using namespace evbs::ertps::rtps;
using namespace vbs::dispatcher;

using vbsutil::xmlparser::PublisherQos;
using vbsutil::xmlparser::SubscriberQos;
using DynamicType_ptr = vbs::common::types::DynamicType_ptr;

class TopicProxyFactory;
class Topic;
class DomainParticipant;
class DomainParticipantListenerInner;
class PublisherListener;
class SubscriberListener;
class Publisher;
class Subscriber;
class DomainParticipantFactoryInner;
class TopicCore;

/**
 * Class DomainParticipant used to group Publishers and Subscribers into a single working unit.
 *
 * @ingroup EDDS_MODULE
 */
class DomainParticipantImpl : public Entity {
    friend class DataWriterImpl;
    friend class DataReaderImpl;
    friend class TopicImpl;
    friend class ContentFilteredTopicImpl;
    friend class DomainParticipantTest;
    friend class NexusUnits;
    friend class DomainParticipant;
    friend class DomainParticipantFactory;

 public:
    /**
     * @brief Destructor
     */
    virtual ~DomainParticipantImpl();

    // Superclass methods

    /**
     * This operation returns the value of the DomainParticipant QoS policies
     *
     * @param qos DomainParticipantQosInner reference where the qos is going to be returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_qos(vbs::DomainParticipantQosInner& qos) const;

    /**
     * @brief This operation returns the value of the DomainParticipant QoS policies
     *
     * @return A reference to the DomainParticipantQosInner
     */
    RTPS_DllAPI const vbs::DomainParticipantQosInner& get_qos() const;

    /**
     * This operation sets the value of the DomainParticipant QoS policies.
     *
     * @param qos DomainParticipantQosInner to be set
     * @return RETCODE_IMMUTABLE_POLICY if any of the Qos cannot be changed,
     * RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the qos is
     * changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_qos(const vbs::DomainParticipantQosInner& qos);

    /**
     * Allows accessing the DomainParticipantListener.
     *
     * @return DomainParticipantListenerInner pointer
     */
    RTPS_DllAPI DomainParticipantListenerInner* get_listener() const;

    /**
     * Modifies the DomainParticipantListener, sets the mask to StatusMask::all()
     *
     * @param listener new value for the DomainParticipantListener
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DomainParticipantListenerInner* listener);

    /**
     * Modifies the DomainParticipantListener, sets the mask to StatusMask::all()
     *
     * @param listener New value for the DomainParticipantListener
     * @param timeout Maximum time to wait for executing callbacks to finish.
     * @return RETCODE_OK if successful, RETCODE_ERROR if failed (timeout expired).
     * @warning Do not call this method from a \c DomainParticipantListenerInner callback.
     */
    RTPS_DllAPI ReturnCode_t set_listener(DomainParticipantListenerInner* listener, const std::chrono::seconds timeout);

    /**
     * Modifies the DomainParticipantListener.
     *
     * @param listener new value for the DomainParticipantListener
     * @param mask StatusMask that holds statuses the listener responds to
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DomainParticipantListenerInner* listener,
                                          const evbs::edds::dds::StatusMask& mask);

    /**
     * Modifies the DomainParticipantListener.
     *
     * @param listener New value for the DomainParticipantListener
     * @param mask StatusMask that holds statuses the listener responds to
     * @param timeout Maximum time to wait for executing callbacks to finish.
     * @return RETCODE_OK if successful, RETCODE_ERROR if failed (timeout expired)
     * @warning Do not call this method from a \c DomainParticipantListenerInner callback.
     */
    RTPS_DllAPI ReturnCode_t set_listener(DomainParticipantListenerInner* const listener,
                                          const evbs::edds::dds::StatusMask& mask, const std::chrono::seconds timeout);

    /**
     * @brief This operation enables the DomainParticipant
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI vbsutil::elog::ReturnCode_t enable() override;

    // DomainParticipant specific methods from DDS API

    /**
     * Create a Publisher in this Participant.
     *
     * @param qos QoS of the Publisher.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created Publisher.
     */
    RTPS_DllAPI Publisher* create_publisher(
        const PublisherQos& qos, PublisherListener* const listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * Create a Publisher in this Participant.
     *
     * @param profile_name Publisher profile name.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created Publisher.
     */
    RTPS_DllAPI Publisher* create_publisher_with_profile(
        const std::string& profile_name, PublisherListener* const listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * Deletes an existing Publisher.
     *
     * @param publisher to be deleted.
     * @return RETCODE_PRECONDITION_NOT_MET if the publisher does not belong to this participant or
     * if it has active DataWriters, RETCODE_OK if it is correctly deleted and RETCODE_ERROR
     * otherwise.
     */
    RTPS_DllAPI ReturnCode_t delete_publisher(const Publisher* publisher);

    /**
     * Create a Subscriber in this Participant.
     *
     * @param qos QoS of the Subscriber.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created Subscriber.
     */
    RTPS_DllAPI Subscriber* create_subscriber(
        const SubscriberQos& qos, SubscriberListener* const listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * Create a Subscriber in this Participant.
     *
     * @param profile_name Subscriber profile name.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created Subscriber.
     */
    RTPS_DllAPI Subscriber* create_subscriber_with_profile(
        const std::string& profile_name, SubscriberListener* const listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * Deletes an existing Subscriber.
     *
     * @param subscriber to be deleted.
     * @return RETCODE_PRECONDITION_NOT_MET if the subscriber does not belong to this participant or
     * if it has active DataReaders, RETCODE_OK if it is correctly deleted and RETCODE_ERROR
     * otherwise.
     */
    RTPS_DllAPI ReturnCode_t delete_subscriber(Subscriber* subscriber);

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param profile_name topic profile name.
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic(const std::string& topic_name, const std::string& type_name,
                                        const std::string& profile_name);

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param local_type TypeSupport.
     * @param local_topic_qos topic qos.
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic(const std::string& topic_name, const std::string& type_name,
                                        const TypeSupport& local_type, const TopicQosImpl& local_topic_qos);

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param local_type TypeSupport.
     * @param profile_name topic profile name.
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic(const std::string& topic_name, const std::string& type_name,
                                        const TypeSupport& local_type, const std::string& profile_name);

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param qos QoS of the TopicImpl.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic(const std::string& topic_name, const std::string& type_name,
                                        const vbsutil::xmlparser::TopicQos& qos,
                                        TopicListener* const listener = nullptr,
                                        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param dyn_type DynamicType_ptr.
     * @param profile_name topic profile name.
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic(const std::string& topic_name, const std::string& type_name,
                                        DynamicType_ptr* dyn_type, const std::string& profile_name);

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param profile_name TopicImpl profile name.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic_with_profile(
        const std::string& topic_name, const std::string& type_name, const std::string& profile_name,
        TopicListener* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

    /**
     * Create a TopicImpl in this Participant.
     *
     * @param topic_name Name of the TopicImpl.
     * @param type_name Data type of the TopicImpl.
     * @param qos TopicImpl qos.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all)
     * @return Pointer to the created TopicImpl.
     */
    RTPS_DllAPI TopicImpl* create_topic_with_profile(
        const std::string& topic_name, const std::string& type_name, const vbsutil::xmlparser::TopicQos& qos,
        TopicListener* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

    /**
     * Deletes an existing TopicImpl.
     *
     * @param topic to be deleted.
     * @return RETCODE_BAD_PARAMETER if the topic passed is a nullptr, RETCODE_PRECONDITION_NOT_MET
     * if the topic does not belong to this participant or if it is referenced by any entity and
     * RETCODE_OK if the TopicImpl was deleted.
     */
    RTPS_DllAPI ReturnCode_t delete_topic(const TopicImpl* topic);

    /**
     * Create a ContentFilteredTopic in this Participant.
     *
     * @param name Name of the ContentFilteredTopic
     * @param related_topic Related TopicImpl to being subscribed
     * @param filter_expression Logic expression to create filter
     * @param expression_parameters Parameters to filter content
     * @return Pointer to the created ContentFilteredTopic.
     * @return nullptr if @c related_topic does not belong to this participant.
     * @return nullptr if a topic with the specified @c name has already been created.
     * @return nullptr if a filter cannot be created with the specified @c filter_expression and
     *                 @c expression_parameters.
     */
    RTPS_DllAPI vbs::ContentFilteredTopicInner* create_contentfilteredtopic(
        const std::string& name, TopicImpl* related_topic, const std::string& filter_expression,
        const std::vector<std::string>& expression_parameters);

    /**
     * Create a ContentFilteredTopic in this Participant using a custom filter.
     *
     * @param name Name of the ContentFilteredTopic
     * @param related_topic Related TopicImpl to being subscribed
     * @param filter_expression Logic expression to create filter
     * @param expression_parameters Parameters to filter content
     * @param filter_class_name Name of the filter class to use
     *
     * @return Pointer to the created ContentFilteredTopic.
     * @return nullptr if @c related_topic does not belong to this participant.
     * @return nullptr if a topic with the specified @c name has already been created.
     * @return nullptr if a filter cannot be created with the specified @c filter_expression and
     *                 @c expression_parameters.
     * @return nullptr if the specified @c filter_class_name has not been registered.
     */
    RTPS_DllAPI vbs::ContentFilteredTopicInner* create_contentfilteredtopic(
        const std::string& name, TopicImpl* related_topic, const std::string& filter_expression,
        const std::vector<std::string>& expression_parameters, const char* filter_class_name);

    /**
     * Deletes an existing ContentFilteredTopic.
     *
     * @param a_contentfilteredtopic ContentFilteredTopic to be deleted
     * @return RETCODE_BAD_PARAMETER if the topic passed is a nullptr, RETCODE_PRECONDITION_NOT_MET if the topic does not belong to
     * this participant or if it is referenced by any entity and RETCODE_OK if the ContentFilteredTopic was deleted.
     */
    RTPS_DllAPI ReturnCode_t delete_contentfilteredtopic(const vbs::ContentFilteredTopicInner* a_contentfilteredtopic);

    /**
     * Gives access to an existing (or ready to exist) enabled TopicImpl.
     * It should be noted that the returned TopicImpl is a local object that acts as a proxy to
     * designate the global concept of topic. TopicImpls obtained by means of find_topic, must also be
     * deleted by means of delete_topic so that the local resources can be released. If a TopicImpl is
     * obtained multiple times by means of find_topic or create_topic, it must also be deleted that
     * same number of times using delete_topic.
     *
     * @param topic_name TopicImpl name
     * @param timeout Maximum time to wait for the TopicImpl
     * @return Pointer to the existing TopicImpl, nullptr in case of error or timeout
     */
    RTPS_DllAPI TopicImpl* find_topic(const std::string& topic_name, const evbs::ertps::Duration_t& timeout);

    /**
     * Looks up an existing, locally created @ref TopicDescription, based on its name.
     * May be called on a disabled participant.
     *
     * @param topic_name Name of the @ref TopicDescription to search for.
     * @return Pointer to the topic description, if it has been created locally. Otherwise, nullptr
     * is returned.
     *
     * @remark UNSAFE. It is unsafe to lookup a topic description while another thread is creating a
     * topic.
     */
    RTPS_DllAPI vbs::TopicDescription* lookup_topicdescription(const std::string& topic_name) const;

    /**
     * Locally ignore a remote domain participant.
     *
     * @note This action is not reversible.
     *
     * @param handle Identifier of the remote participant to ignore
     * @return RETURN_OK code if everything correct, RETCODE_BAD_PARAMENTER otherwise
     *
     */
    RTPS_DllAPI ReturnCode_t ignore_participant(const InstanceHandle_t& handle);

    /**
     * Locally ignore a topic.
     *
     * @note This action is not reversible.
     *
     * @param handle Identifier of the topic to ignore
     * @return RETURN_OK code if everything correct, error code otherwise
     *
     * @warning Not supported yet. Currently returns RETCODE_UNSUPPORTED
     *
     */
    RTPS_DllAPI ReturnCode_t ignore_topic(const InstanceHandle_t& handle);

    /**
     * Locally ignore a remote datawriter.
     *
     * @note This action is not reversible.
     *
     * @param handle Identifier of the datawriter to ignore
     * @return RETURN_OK code if everything correct, error code otherwise
     *
     * @warning Not supported yet. Currently returns RETCODE_UNSUPPORTED
     *
     */
    RTPS_DllAPI ReturnCode_t ignore_publication(const InstanceHandle_t& handle);

    /**
     * Locally ignore a remote datareader.
     *
     * @note This action is not reversible.
     *
     * @param handle Identifier of the datareader to ignore
     * @return RETURN_OK code if everything correct, error code otherwise
     *
     * @warning Not supported yet. Currently returns RETCODE_UNSUPPORTED
     *
     */
    RTPS_DllAPI ReturnCode_t ignore_subscription(const InstanceHandle_t& handle);

    /**
     * This operation retrieves the domain_id used to create the DomainParticipant.
     * The domain_id identifies the DDS domain to which the DomainParticipant belongs.
     *
     * @return The Participant's domain_id
     */
    RTPS_DllAPI DomainId_t get_domain_id() const;

    /**
     * Deletes all the entities that were created by means of the “create” methods
     *
     * @return RETURN_OK code if everything correct, error code otherwise
     */
    RTPS_DllAPI ReturnCode_t delete_contained_entities();

    /**
     * This operation manually asserts the liveliness of the DomainParticipant.
     * This is used in combination with the LIVELINESS QoS policy to indicate to the Service that
     * the entity remains active.
     *
     * This operation needs to only be used if the DomainParticipant contains DataWriter entities
     * with the LIVELINESS set to MANUAL_BY_PARTICIPANT and it only affects the liveliness of those
     * DataWriter entities. Otherwise, it has no effect.
     *
     * @note Writing data via the write operation on a DataWriter asserts liveliness on the
     * DataWriter itself and its DomainParticipant. Consequently the use of assert_liveliness is
     * only needed if the application is not writing data regularly.
     *
     * @return RETCODE_OK if the liveliness was asserted, RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI RetCode_t assert_liveliness();

    /**
     * This operation sets a default value of the Publisher QoS policies which will be used for
     * newly created Publisher entities in the case where the QoS policies are defaulted in the
     * create_publisher operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return false.
     *
     * The special value PUBLISHER_QOS_DEFAULT may be passed to this operation to indicate that the
     * default QoS should be reset back to the initial values the factory would use, that is the
     * values that would be used if the set_default_publisher_qos operation had never been called.
     *
     * @param qos PublisherQos to be set
     * @return RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the
     * qos is changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_default_publisher_qos(const PublisherQos& qos);

    /**
     * This operation retrieves the default value of the Publisher QoS, that is, the QoS policies
     * which will be used for newly created Publisher entities in the case where the QoS policies
     * are defaulted in the create_publisher operation.
     *
     * The values retrieved get_default_publisher_qos will match the set of values specified on the
     * last successful call to set_default_publisher_qos, or else, if the call was never made, the
     * default values.
     *
     * @return Current default publisher qos.
     */
    RTPS_DllAPI const PublisherQos& get_default_publisher_qos() const;

    /**
     * This operation retrieves the default value of the Publisher QoS, that is, the QoS policies
     * which will be used for newly created Publisher entities in the case where the QoS policies
     * are defaulted in the create_publisher operation.
     *
     * The values retrieved get_default_publisher_qos will match the set of values specified on the
     * last successful call to set_default_publisher_qos, or else, if the call was never made, the
     * default values.
     *
     * @param qos PublisherQos reference where the default_publisher_qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_default_publisher_qos(PublisherQos& qos) const;

    /**
     * Fills the PublisherQos with the values of the XML profile.
     *
     * @param profile_name Publisher profile name.
     * @param qos PublisherQos object where the qos is returned.
     * @return RETCODE_OK if the profile exists. RETCODE_BAD_PARAMETER otherwise.
     */
    RTPS_DllAPI ReturnCode_t get_publisher_qos_from_profile(const std::string& profile_name, PublisherQos& qos) const;

    /**
     * This operation sets a default value of the Subscriber QoS policies that will be used for
     * newly created Subscriber entities in the case where the QoS policies are defaulted in the
     * create_subscriber operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return false.
     *
     * The special value SUBSCRIBER_QOS_DEFAULT may be passed to this operation to indicate that the
     * default QoS should be reset back to the initial values the factory would use, that is the
     * values that would be used if the set_default_subscriber_qos operation had never been called.
     *
     * @param qos SubscriberQos to be set
     * @return RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the
     * qos is changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_default_subscriber_qos(const SubscriberQos& qos);

    /**
     * This operation retrieves the default value of the Subscriber QoS, that is, the QoS policies
     * which will be used for newly created Subscriber entities in the case where the QoS policies
     * are defaulted in the create_subscriber operation.
     *
     * The values retrieved get_default_subscriber_qos will match the set of values specified on the
     * last successful call to set_default_subscriber_qos, or else, if the call was never made, the
     * default values.
     *
     * @return Current default subscriber qos.
     */
    RTPS_DllAPI const SubscriberQos& get_default_subscriber_qos() const;

    /**
     * This operation retrieves the default value of the Subscriber QoS, that is, the QoS policies
     * which will be used for newly created Subscriber entities in the case where the QoS policies
     * are defaulted in the create_subscriber operation.
     *
     * The values retrieved get_default_subscriber_qos will match the set of values specified on the
     * last successful call to set_default_subscriber_qos, or else, if the call was never made, the
     * default values.
     *
     * @param qos SubscriberQos reference where the default_subscriber_qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_default_subscriber_qos(SubscriberQos& qos) const;

    /**
     * Fills the SubscriberQos with the values of the XML profile.
     *
     * @param profile_name Subscriber profile name.
     * @param qos SubscriberQos object where the qos is returned.
     * @return RETCODE_OK if the profile exists. RETCODE_BAD_PARAMETER otherwise.
     */
    RTPS_DllAPI ReturnCode_t get_subscriber_qos_from_profile(const std::string& profile_name, SubscriberQos& qos) const;

    /**
     * This operation sets a default value of the TopicImpl QoS policies which will be used for newly
     * created TopicImpl entities in the case where the QoS policies are defaulted in the create_topic
     * operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return INCONSISTENT_POLICY.
     *
     * The special value TOPIC_QOS_DEFAULT may be passed to this operation to indicate that the
     * default QoS should be reset back to the initial values the factory would use, that is the
     * values that would be used if the set_default_topic_qos operation had never been called.
     *
     * @param qos TopicQos to be set
     * @return RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the
     * qos is changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_default_topic_qos(const vbsutil::xmlparser::TopicQos& qos);

    /**
     * This operation retrieves the default value of the TopicImpl QoS, that is, the QoS policies that
     * will be used for newly created TopicImpl entities in the case where the QoS policies are
     * defaulted in the create_topic operation.
     *
     * The values retrieved get_default_topic_qos will match the set of values specified on the last
     * successful call to set_default_topic_qos, or else, TOPIC_QOS_DEFAULT if the call was never
     * made.
     *
     * @return Current default topic qos.
     */
    RTPS_DllAPI const vbsutil::xmlparser::TopicQos& get_default_topic_qos() const;

    /**
     * This operation retrieves the default value of the TopicImpl QoS, that is, the QoS policies that
     * will be used for newly created TopicImpl entities in the case where the QoS policies are
     * defaulted in the create_topic operation.
     *
     * The values retrieved get_default_topic_qos will match the set of values specified on the last
     * successful call to set_default_topic_qos, or else, TOPIC_QOS_DEFAULT if the call was never
     * made.
     *
     * @param qos TopicQos reference where the default_topic_qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_default_topic_qos(vbsutil::xmlparser::TopicQos& qos) const;

    /**
     * Fills the TopicQos with the values of the XML profile.
     *
     * @param profile_name TopicImpl profile name.
     * @param qos TopicQos object where the qos is returned.
     * @return RETCODE_OK if the profile exists. RETCODE_BAD_PARAMETER otherwise.
     */
    RTPS_DllAPI ReturnCode_t get_topic_qos_from_profile(const std::string& profile_name,
                                                        vbsutil::xmlparser::TopicQos& qos) const;

    /**
     * This operation checks whether or not the given handle represents an Entity that was created
     * from the DomainParticipant.
     *
     * @param a_handle InstanceHandle of the entity to look for.
     * @param recursive The containment applies recursively. That is, it applies both to entities
     * (TopicDescription, Publisher, or Subscriber) created directly using the DomainParticipant as
     * well as entities created using a contained Publisher, or Subscriber as the factory, and so
     * forth. (default: true)
     * @return True if entity is contained. False otherwise.
     */
    RTPS_DllAPI bool contains_entity(const InstanceHandle_t& a_handle, bool recursive = true) const;

    /**
     * This operation returns the current value of the time that the service uses to time-stamp
     * data-writes and to set the reception-timestamp for the data-updates it receives.
     *
     * @param current_time Time_t reference where the current time is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_current_time(evbs::ertps::Time_t& current_time) const;

    // DomainParticipant methods specific from EDDS

    /**
     * Register a type in this participant.
     *
     * @param type TypeSupport.
     * @param type_name The name that will be used to identify the Type.
     * @return RETCODE_BAD_PARAMETER if the size of the name is 0, RERCODE_PRECONDITION_NOT_MET if
     * there is another TypeSupport with the same name and RETCODE_OK if it is correctly registered.
     */
    RTPS_DllAPI vbsutil::elog::ReturnCode_t register_type(TypeSupport type, const std::string& type_name);

    /**
     * Register a type in this participant.
     *
     * @param type TypeSupport.
     * @return RETCODE_BAD_PARAMETER if the size of the name is 0, RERCODE_PRECONDITION_NOT_MET if
     * there is another TypeSupport with the same name and RETCODE_OK if it is correctly registered.
     */
    RTPS_DllAPI vbsutil::elog::ReturnCode_t register_type(TypeSupport type);

    /**
     * Unregister a type in this participant.
     *
     * @param typeName Name of the type
     * @return RETCODE_BAD_PARAMETER if the size of the name is 0, RERCODE_PRECONDITION_NOT_MET if
     * there are entities using that TypeSupport and RETCODE_OK if it is correctly unregistered.
     */
    RTPS_DllAPI vbsutil::elog::ReturnCode_t unregister_type(const std::string& typeName);

    /**
     * This method gives access to a registered type based on its name.
     *
     * @param type_name Name of the type
     * @return TypeSupport corresponding to the type_name
     */
    RTPS_DllAPI TypeSupport find_type(const std::string& type_name) const;

    /**
     * Returns the DomainParticipant's handle.
     *
     * @return InstanceHandle of this DomainParticipant.
     */
    RTPS_DllAPI const InstanceHandle_t& get_instance_handle() const;

    // From here legacy RTPS methods.

    /**
     * @brief Getter for the Participant GUID
     *
     * @return A reference to the GUID
     */
    RTPS_DllAPI const evbs::ertps::rtps::GUID_t& guid() const;

    /**
     * @brief Getter for the participant names
     *
     * @return Vector with the names
     */
    RTPS_DllAPI std::vector<std::string> get_participant_names() const;

    /**
     * This method can be used when using a StaticEndpointDiscovery mechanism different that the one
     * included in ERTPS, for example when communicating with other implementations.
     * It indicates the Participant that an Endpoint from the XML has been discovered and
     * should be activated.
     *
     * @param partguid Participant GUID_t.
     * @param userId User defined ID as shown in the XML file.
     * @param kind EndpointKind (WRITER or READER)
     * @return True if correctly found and activated.
     */
    RTPS_DllAPI bool new_remote_endpoint_discovered(const evbs::ertps::rtps::GUID_t& partguid, const uint16_t userId,
                                                    evbs::ertps::rtps::EndpointKind_t kind);

    /**
     * @brief Getter for the resource event
     *
     * @pre The DomainParticipant is enabled.
     *
     * @return A reference to the resource event
     */
    RTPS_DllAPI evbs::ertps::rtps::ResourceEvent& get_resource_event() const;

    RTPS_DllAPI ReturnCode_t register_remote_type(const std::string& type_name);

    /**
     * @brief Check if the Participant has any Publisher, Subscriber or Topic
     *
     * @return true if any, false otherwise.
     */
    bool has_active_entities();

    /**
     * @brief Stop to send announcement messages with initial announcement period
     */
    void stop_initial_announcement();

    /**
     * @brief Start to send announcement messages with initial announcement count
     * and period in xml config profile
     * @return true
     */
    bool start_initial_announcement();

    /**
     * @brief Send announcement message with input parameters count and period
     *
     * @return true or false
     */
    bool start_initial_announcement(const uint32_t count, const Duration_t period);

    /**
     * @brief get remote evbs version
     *
     * @return evbsVersion
     */
    evbs::ertps::rtps::EvbsVersion_t get_remote_evbsversion();

    builtin::StatisticWrtierReaderInfos get_writer_reader_infos();

    builtin::StatisticWrtierReaderInfos lookup_writer_reader_infos(const evbs::ertps::rtps::GUID_t& guid,
                                                                   uint32_t timeout);

    builtin::StatisticMatchGuids get_remote_guids(const evbs::ertps::rtps::GUID_t& guid);

    builtin::StatisticMatchGuids lookup_remote_guids(const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout);

    RetCode_t get_statistic_send_info(const evbs::ertps::rtps::GUID_t& guid, builtin::StatisticSendInfo* info);

    RetCode_t lookup_statistic_send_info(const evbs::ertps::rtps::GUID_t& guid, builtin::StatisticSendInfo* info,
                                         uint32_t timeout);

    RetCode_t get_statistic_recv_info(const evbs::ertps::rtps::GUID_t& guid, builtin::StatisticRecvInfo* info);

    RetCode_t lookup_statistic_recv_info(const evbs::ertps::rtps::GUID_t& guid, builtin::StatisticRecvInfo* info,
                                         uint32_t timeout);

    RetCode_t get_statistic_writer_reader_qos(const evbs::ertps::rtps::GUID_t& guid,
                                              builtin::StatisticWriterReaderQos* qos);

    RetCode_t lookup_statistic_writer_reader_qos(const evbs::ertps::rtps::GUID_t& guid,
                                                 builtin::StatisticWriterReaderQos* qos, uint32_t timeout);

    builtin::StatisticProxyInfos get_statistic_writer_reader_proxy_infos(const evbs::ertps::rtps::GUID_t& guid);

    builtin::StatisticProxyInfos lookup_statistic_writer_reader_proxy_infos(const evbs::ertps::rtps::GUID_t& guid,
                                                                            uint32_t timeout);

    vbsutil::elog::Log::GuidMessageBriefsMap lookupMessageBrief(const std::string& host_name, uint32_t pid,
                                                                const std::string& topic_name, uint32_t timeout);

    RetCode_t remoteConfig(const std::string& host_name, uint32_t pid, const std::string& params, uint32_t timeout);

    // Get all the participants in DataBase
    bool get_participants_in_db(std::vector<vbsutil::xmlparser::GuidPrefix_t>& prefix_list);

    // Get the participant info by guid_prefix
    bool get_participant_info(const vbsutil::xmlparser::GuidPrefix_t& prefix, DiscoveryParticipantInfo& part_info);

    // Get the reader info by guid
    bool get_reader_info(const vbsutil::xmlparser::GUID_t& reader_guid, DiscoveryEndpointInfo& reader_info);

    // Get the writer info by guid
    bool get_writer_info(const vbsutil::xmlparser::GUID_t& writer_guid, DiscoveryEndpointInfo& writer_info);

    bool get_topic_names(std::vector<std::string>& topic_list);

    bool has_writer(const std::string& topic_name);

    bool has_reader(const std::string& topic_name);

    bool get_writer_of_topic(const std::string& topic_name, std::vector<DiscoveryEndpointInfo>& writer_info_list);

    bool get_reader_of_topic(const std::string& topic_name, std::vector<DiscoveryEndpointInfo>& reader_info_list);

    bool has_participant(const std::string& participant_name);

    bool get_participant_of_name(const std::string& participant_name, DiscoveryParticipantInfo& part_info);

    Topic* find_topic(const std::string& topic_name, const std::string& topic_type);

    void remove_topic(const Topic* topic_ptr);

    void add_topic(const std::string& topic_name, const std::string& topic_type, Topic* topic_ptr);

    void del_participant_infos();

    /**
     * @brief register remote topic name
     *
     * @param type_name Data type name bound to given Topic
     *
     * @return return RETCODE_OK when success
     */
    void register_remote_topic(const std::string& topic_name);

    /**
     * @brief register remote topic name
     *
     * @return return RETCODE_OK when success
     */
    void register_remote_topic();

    /**
     * @brief unregister remote topic name
     *
     * @return return RETCODE_OK when success
     */
    void unregister_remote_topic();

    void delete_topic(ContentFilteredTopic* topic_ptr);

    void remove_topic(ContentFilteredTopic* topic_ptr);

    void add_topic(ContentFilteredTopic* topic_ptr);

    void del_content_filtered_topics();

    bool type_discovery_enabled() { return qos_.wire_protocol().builtin.lookup_config.use_client; }

    vbs::common::types::DynamicType_ptr get_dynamic_type(const std::string& type_name);

 protected:
    DomainParticipantImpl(DomainId_t did, const vbs::DomainParticipantQosInner& qos,
                          const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(),
                          DomainParticipantListenerInner* listen = nullptr);

    friend class DomainParticipantFactoryInner;

    friend class ReaderFilterCollection;

 private:
    bool check_get_child_dependencies_request(const evbs::ertps::rtps::GUID_t& dst_guid,
                                              const evbs::ertps::rtps::SampleIdentity& request_id,
                                              const evbs::ertps::rtps::SampleIdentity& request_parent_id,
                                              const evbs::ertps::types::TypeIdentifierWithSizeSeq& dependencies);

    void type_request(const evbs::ertps::rtps::GUID_t& dst_guid,
                      const evbs::ertps::types::TypeInformation& type_information, const std::string& type_name);

    bool build_dynamic_type(std::string type_name);

 protected:
    evbs::ertps::rtps::IContentFilterFactory* find_content_filter_factory(const char* filter_class_name);

 public:
    /**
     * Implementation of Topic::set_listener that propagates the listener and mask to all the
     * TopicProxy objects held by the same TopicProxy factory in a thread-safe way.
     *
     * @param factory  TopicProxyFactory managing the topic on which the listener should be changed.
     * @param listener Listener to assign to all the TopicProxy objects owned by the factory.
     * @param mask     StatusMask to assign to all the TopicProxy objects owned by the factory.
     */
    void set_topic_listener(const TopicProxyFactory* factory, vbs::TopicCore* topic, TopicListener* const listener,
                            const evbs::edds::dds::StatusMask& mask);

    const vbs::dispatcher::DispatcherParticipant* get_dispatcher_participant() const {
        std::lock_guard<std::mutex> _(mtx_gs_);
        return dispatcher_participant_;
    }

    vbs::dispatcher::DispatcherParticipant* get_dispatcher_participant() {
        std::lock_guard<std::mutex> _(mtx_gs_);
        return dispatcher_participant_;
    }

    evbs::ertps::rtps::SampleIdentity get_type_dependencies(const evbs::ertps::rtps::GUID_t& dst_guid,
                                                            const evbs::ertps::types::TypeIdentifierSeq& in) const;

    evbs::ertps::rtps::SampleIdentity get_types(const evbs::ertps::rtps::GUID_t& dst_guid,
                                                const evbs::ertps::types::TypeIdentifierSeq& in) const;

    ReturnCode_t register_remote_type(const evbs::ertps::rtps::GUID_t& dst_guid,
                                      const evbs::ertps::types::TypeInformation& type_information,
                                      const std::string& type_name);

    //! Remove all listeners in the hierarchy to allow a quiet destruction
    virtual void disable();

    /**
     * Returns the most appropriate listener to handle the callback for the given status,
     * or nullptr if there is no appropriate listener.
     */
    DomainParticipantListenerInner* get_listener_for(const evbs::edds::dds::StatusMask& status);

    std::atomic<uint32_t>& id_counter() { return id_counter_; }

    void initToolHookGetInfoFunc();

    template <EndpointKind_t kind, octet no_key, octet with_key>
    static bool preprocess_endpoint_attributes(const EntityId_t& entity_id, std::atomic<uint32_t>& id_counter,
                                               EndpointAttributes& att, EntityId_t& entId) {
        const char* debug_label = (att.endpointKind == WRITER ? "writer" : "reader");

        if (!att.unicastLocatorList.isValid()) {
            elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                      "Unicast Locator List for " << debug_label << " contains invalid Locator");
            return false;
        }
        if (!att.multicastLocatorList.isValid()) {
            elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                      "Multicast Locator List for " << debug_label << " contains invalid Locator");
            return false;
        }
        if (!att.remoteLocatorList.isValid()) {
            elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                      "Remote Locator List for " << debug_label << " contains invalid Locator");
            return false;
        }

        if (entity_id == c_EntityId_Unknown) {
            if (att.topicKind == NO_KEY) {
                entId.value[3] =
                    ((-2 == att.getUserDefinedID()) && (0 < att.getEntityID())) ? (0x60U) | no_key : no_key;
            } else if (att.topicKind == WITH_KEY) {
                entId.value[3] =
                    ((-2 == att.getUserDefinedID()) && (0 < att.getEntityID())) ? (0x60) | with_key : with_key;
            }
            uint32_t idnum;
            if (att.getEntityID() > 0) {
                idnum = static_cast<uint32_t>(att.getEntityID());
            } else {
                idnum = ++id_counter;
            }

            entId.value[2] = octet(idnum);
            entId.value[1] = octet(idnum >> 8);
            entId.value[0] = octet(idnum >> 16);
        } else {
            entId = entity_id;
        }
        if (att.persistence_guid == c_Guid_Unknown) {
            // Try to load persistence_guid from property
            const std::string* persistence_guid_property =
                PropertyPolicyHelper::find_property(att.properties, "dds.persistence.guid");
            if (persistence_guid_property != nullptr) {
                // Load persistence_guid from property
                std::istringstream(persistence_guid_property->c_str()) >> att.persistence_guid;
                if (att.persistence_guid == c_Guid_Unknown) {
                    // Wrongly configured property
                    elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                              "Cannot configure " << debug_label << "'s persistence GUID from '"
                                                  << persistence_guid_property->c_str() << "'. Wrong input");
                    return false;
                }
            }
        }

        // Error log level can be disable. Avoid unused warning
        static_cast<void>(debug_label);

        return true;
    }

 protected:
    //! Domain id
    DomainId_t domain_id_;

    //! Participant id
    int32_t participant_id_ = -1;

    //! Pre-calculated guid
    evbs::ertps::rtps::GUID_t guid_;

    //! For instance handle creation
    std::atomic<uint32_t> next_instance_id_;

    //! Participant Qos
    vbs::DomainParticipantQosInner qos_;

    //! DispatcherParticipant
    vbs::dispatcher::DispatcherParticipant* dispatcher_participant_;

    //! Participant Listener
    DomainParticipantListenerInner* listener_;

    //! getter/setter mutex
    mutable std::mutex mtx_gs_;

    //! getter/setter condition variable
    std::condition_variable cv_gs_;

    //! Publisher maps
    std::map<InstanceHandle_t, Publisher*> publishers_by_handle_;
    mutable std::mutex mtx_pubs_;

    PublisherQos default_pub_qos_;

    //! Subscriber maps
    std::map<InstanceHandle_t, Subscriber*> subscribers_by_handle_;
    mutable std::mutex mtx_subs_;

    SubscriberQos default_sub_qos_;

    //! TopicDataType map
    std::map<std::string, TypeSupport> types_;
    mutable std::mutex mtx_types_;

    //! Topic map
    std::map<std::string, TopicProxyFactory*> topics_;
    std::map<InstanceHandle_t, TopicImpl*> topics_by_handle_;
    std::map<std::string, std::unique_ptr<vbs::ContentFilteredTopicInner>> filtered_topics_;
    std::map<std::string, evbs::ertps::rtps::IContentFilterFactory*> filter_factories_;
    mutable std::mutex mtx_topics_;
    std::condition_variable cond_topics_;

    vbsutil::xmlparser::TopicQos default_topic_qos_;

    // Mutex for requests and callbacks maps.
    std::mutex mtx_request_cb_;

    // [type_name, [type id, type obj]]
    std::map<std::string, evbs::ertps::types::TypeIdentifierTypeObjectPair> types_id_obj_;

    // register_remote_type parent request, type_name, callback relationship: [root, type_name]
    std::map<evbs::ertps::rtps::SampleIdentity, std::string> register_types_;

    // Relationship between child and parent request: [child1, parent], [child2, parent]
    std::map<evbs::ertps::rtps::SampleIdentity, evbs::ertps::rtps::SampleIdentity> child_requests_;

    // All parent's child requests:  [parent, [parent,child1,child2....]]
    std::map<evbs::ertps::rtps::SampleIdentity, std::vector<evbs::ertps::rtps::SampleIdentity>> parent_requests_;

    std::atomic<uint32_t> id_counter_;

    std::recursive_mutex mtx_type_discovery_;
    std::map<std::string, std::string> discovered_type_name_by_topic_;
    std::map<std::string, vbs::common::types::DynamicType_ptr> discovered_type_info_;
    std::map<std::string, std::set<std::string>> discovered_type_topic_;

    std::set<std::string> discovered_topic_names_;
    std::set<std::string> registered_type_names_;
    std::set<std::string> listener_topics_;

    // key is participant name, value is TOPIC_INFO, add 20231025 lyf
    std::map<std::string, Topic*> topic_participant_infos_;
    std::mutex mtx_topic_;

    // ContentFilteredTopic set
    std::set<ContentFilteredTopic*> content_filtered_topic_infos_;
    std::mutex mtx_content_filtered_topic_;

    bool receive_all_topic_ = false;  // for type discovery
    bool type_lookup_ = false;
    bool batch_free_ = false;

    class MyParticipantListener : public vbs::common::ParticipantListener {
        struct Sentry {
            Sentry(MyParticipantListener* listener) : listener_(listener), on_guard_(false) {
                if ((listener_ != nullptr) && (listener_->participant_ != nullptr)) {
                    std::lock_guard<std::mutex> _(listener_->participant_->mtx_gs_);
                    if (listener_->participant_->listener_ || listener_->participant_->user_listener_) {
                        if (listener_->callback_counter_ >= 0) {
                            ++listener_->callback_counter_;
                            on_guard_ = true;
                        }
                    }
                }
            }

            ~Sentry() {
                if (on_guard_) {
                    bool notify = false;
                    if (listener_ != nullptr && listener_->participant_ != nullptr &&
                        (listener_->participant_->listener_ || listener_->participant_->user_listener_)) {
                        std::lock_guard<std::mutex> lock(listener_->participant_->mtx_gs_);
                        --listener_->callback_counter_;
                        notify = !listener_->callback_counter_;
                    }
                    if (notify) {
                        listener_->participant_->cv_gs_.notify_all();
                    }
                }
            }

            operator bool() const { return on_guard_; }

            MyParticipantListener* listener_ = nullptr;
            bool on_guard_;
        };

     public:
        MyParticipantListener(DomainParticipantImpl* impl) : participant_(impl) {}

        ~MyParticipantListener() override { assert(!(callback_counter_ > 0)); }

        void onParticipantDiscovery(vbs::dispatcher::SubstrateParticipant* participant,
                                    vbs::BaseParticipantDiscoveryInfo&& info) override;

#if HAVE_SECURITY
        void onParticipantAuthentication(vbs::dispatcher::SubstrateParticipant* participant,
                                         vbs::ParticipantAuthenticationInfo&& info) override;
#endif  // if HAVE_SECURITY

        void onReaderDiscovery(vbs::dispatcher::SubstrateParticipant* const participant,
                               vbs::BaseReaderDiscoveryInfo&& info) override;

        void onWriterDiscovery(vbs::dispatcher::SubstrateParticipant* participant,
                               vbs::BaseWriterDiscoveryInfo&& info) override;

        void on_type_discovery(
            vbs::dispatcher::SubstrateParticipant* participant,
            const /*vbs::common::*/ SampleIdentity& request_sample_id,
            const std::vector<vbs::common::types::TypeIdentifierTypeObjectPair>& type_pairs) override;

        void on_type_dependencies_reply(const evbs::ertps::rtps::GUID_t& dst_guid,
                                        vbs::dispatcher::SubstrateParticipant* participant,
                                        const vbs::common::SampleIdentity& request_sample_id,
                                        const vbs::common::types::TypeIdentifierWithSizeSeq& dependencies) override;

        void on_type_information_received(const evbs::ertps::rtps::GUID_t& dst_guid, const std::string& type_name,
                                          const vbs::common::types::TypeInformation& type_information) override;

        DomainParticipantImpl* participant_;
        int callback_counter_ = 0;
    } rtps_listener_;

    vbs::DomainParticipantListener* user_listener_ {nullptr};

    vbs::DomainParticipant* user_participant_ {nullptr};

    void create_instance_handle(InstanceHandle_t& handle);

    ReturnCode_t register_dynamic_type(const std::string& type_name, evbs::ertps::types::DynamicType_ptr dyn_type);

    bool register_dynamic_type_to_factories(const TypeSupport& type) const;

    bool check_get_type_request(const evbs::ertps::rtps::SampleIdentity& requestId,
                                const std::vector<vbs::common::types::TypeIdentifierTypeObjectPair>& types);

    bool check_get_dependencies_request(const evbs::ertps::rtps::GUID_t& dst_guid,
                                        const evbs::ertps::rtps::SampleIdentity& requestId,
                                        const vbs::common::types::TypeIdentifierWithSizeSeq& dependencies);

    // Always call it with the mutex already taken
    void remove_parent_request(const evbs::ertps::rtps::SampleIdentity& request);

    // Always call it with the mutex already taken
    void remove_child_request(const evbs::ertps::rtps::SampleIdentity& request);

    // Always call it with the mutex already taken
    void on_child_requests_finished(const evbs::ertps::rtps::SampleIdentity& parent);

    void fill_pending_dependencies(const evbs::ertps::types::TypeIdentifierWithSizeSeq& dependencies,
                                   evbs::ertps::types::TypeIdentifierSeq& pending_identifiers,
                                   evbs::ertps::types::TypeIdentifierSeq& pending_objects) const;

    /**
     * Set the DomainParticipantQosInner checking if the Qos can be updated or not
     *
     * @param to DomainParticipantQosInner to be updated
     * @param from DomainParticipantQosInner desired
     * @param first_time Whether the DomainParticipant has been already initialized or not
     *
     * @return true if there has been a changed in one of the attributes that can be updated.
     * false otherwise.
     */
    static bool set_qos(vbs::DomainParticipantQosInner& to, const vbs::DomainParticipantQosInner& from,
                        bool first_time);

    static ReturnCode_t check_qos(const vbs::DomainParticipantQosInner& qos);

    static bool can_qos_be_updated(const vbs::DomainParticipantQosInner& to,
                                   const vbs::DomainParticipantQosInner& from);

    void set_user_listener(vbs::DomainParticipantListener* listener) { user_listener_ = listener; };

    void set_user_participant(vbs::DomainParticipant* user_part) { user_participant_ = user_part; }

 private:
    void reset_default_publisher_qos();
    void reset_default_subscriber_qos();
    void reset_default_topic_qos();
    void update_type_info(const std::string& topic_name, const std::string& type_name);
    void update_type_discovery(const std::string& type_name, evbs::ertps::types::DynamicType_ptr dyn_type);
    void notify_type_discovery(const std::string& topic_name, const std::string& type_name);
    void update_registered_topics();
};

} /* namespace vbs */

#endif  // INCLUDE_DOMAIN_DOMAINPARTICIPANT_HPP_
