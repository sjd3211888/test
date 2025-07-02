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
// feature: framework fit QosManager
// 1. create topic、reader、writer with profile use QosManager API
// 2. QosManager add clear xml file API 3. UT fit QosManager
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBER_HPP_
#define INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBER_HPP_

#include <mutex>
#include <map>

#include "core/Entity.hpp"
#include "sub/DataReaderListenerInner.hpp"
#include "sub/InstanceState.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "xmlparser/qos/XMLSubscriberQos.hpp"
#include "sub/SampleState.hpp"
#include "sub/ViewState.hpp"
#include "deps/common/TypeSupport.hpp"
#include "ertps/types/TypesBase.h"
#include "xmlparser/attributes/XMLTopicAttributes.h"
#include "ertps/attributes/SubscriberAttributes.h"
#include "dispatcher/DispatcherParticipant.hpp"
#include "deps/core/status/StatusMask.hpp"

using evbs::ReturnCode_t;

namespace vbs {
class TypeSupport;
class DomainParticipantImpl;
class TopicImpl;
class SubscriberListener;
class DataReaderListenerInner;
}  // namespace vbs

namespace evbs {
namespace ertps {
namespace rtps {
class RTPSParticipant;
}  // namespace rtps
using vbsutil::xmlparser::TopicAttributes;

}  // namespace ertps

} /* namespace evbs */

namespace vbs {

class DataReaderImpl;
class TopicDescription;

/**
 * Class Subscriber, contains the public API that allows the user to control the reception of
 * messages. This class should not be instantiated directly. DomainRTPSParticipant class should be
 * used to correctly create this element.
 *
 * @ingroup EDDS_MODULE
 */
class Subscriber : public DomainEntity {
 private:
    vbs::TopicImpl* get_topic_from_desc(vbs::TopicDescription* topic_desc);

 protected:
    friend class vbs::DataReaderImpl;
    friend class vbs::DomainParticipantImpl;

    /**
     * Create a subscriber, assigning its pointer to the associated implementation.
     * Don't use directly, create Subscriber using create_subscriber from DomainParticipant.
     */
    Subscriber(vbs::DomainParticipantImpl* dp,
               const vbsutil::xmlparser::SubscriberQos& qos = vbsutil::xmlparser::getDefaultSubscriberQos(),
               SubscriberListener* listener = nullptr,
               const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

    //! Participant
    vbs::DomainParticipantImpl* participant_;

    vbsutil::xmlparser::SubscriberQos qos_;

    //! Map of Pointer to associated DataReaders. Topic name is the key.
    std::map<std::string, std::vector<DataReaderImpl*>> readers_;

    mutable std::mutex mtx_readers_;

    //! Listener
    SubscriberListener* listener_;

    class SubscriberReaderListener : public vbs::DataReaderListenerInner {
     public:
        SubscriberReaderListener(Subscriber* s) : subscriber_(s) {}

        ~SubscriberReaderListener() override {}

        void on_data_available(DataReaderImpl* reader) override;

        void on_subscription_matched(DataReaderImpl* reader,
                                     const evbs::edds::dds::SubscriptionMatchedStatus& info) override;

        void on_requested_deadline_missed(DataReaderImpl* reader,
                                          const evbs::ertps::RequestedDeadlineMissedStatus& status) override;

        void on_liveliness_changed(DataReaderImpl* reader, const evbs::ertps::LivelinessChangedStatus& status) override;

        void on_sample_rejected(DataReaderImpl* reader, const evbs::ertps::SampleRejectedStatus& status) override;

        void on_requested_incompatible_qos(DataReaderImpl* reader,
                                           const evbs::edds::dds::RequestedIncompatibleQosStatus& status) override;

        void on_sample_lost(DataReaderImpl* reader, const evbs::edds::dds::SampleLostStatus& status) override;

        Subscriber* subscriber_;
    } subscriber_listener_;

    vbs::dispatcher::DispatcherParticipant* dispatcher_participant_;

    vbsutil::xmlparser::DataReaderQos default_datareader_qos_;

    evbs::ertps::rtps::InstanceHandle_t handle_;

    bool batch_free_ = false;

 public:
    /**
     * @brief Destructor
     */
    ~Subscriber();

    /**
     * @brief This operation enables the Subscriber
     *
     * @return RETCODE_OK is successfully enabled. RETCODE_PRECONDITION_NOT_MET if the participant
     * creating this Subscriber is not enabled.
     */
    RTPS_DllAPI evbs::ReturnCode_t enable() override;

    /**
     * Allows accessing the Subscriber Qos.
     *
     * @return SubscriberQos reference
     */
    RTPS_DllAPI const vbsutil::xmlparser::SubscriberQos& get_qos() const;

    /**
     * Retrieves the Subscriber Qos.
     *
     * @param qos SubscriberQos where the qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_qos(vbsutil::xmlparser::SubscriberQos& qos) const;

    /**
     * Allows modifying the Subscriber Qos.
     * The given Qos must be supported by the SubscriberQos.
     *
     * @param qos new value for SubscriberQos
     * @return RETCODE_IMMUTABLE_POLICY if any of the Qos cannot be changed,
     * RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the qos is
     * changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_qos(const vbsutil::xmlparser::SubscriberQos& qos);

    /**
     * Retrieves the attached SubscriberListener.
     *
     * @return Pointer to the SubscriberListener
     */
    RTPS_DllAPI const SubscriberListener* get_listener() const;

    /**
     * Modifies the SubscriberListener, sets the mask to StatusMask::all()
     *
     * @param listener new value for SubscriberListener
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(SubscriberListener* listener);

    /**
     * Modifies the SubscriberListener.
     *
     * @param listener new value for the SubscriberListener
     * @param mask StatusMask that holds statuses the listener responds to.
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(SubscriberListener* listener, const evbs::edds::dds::StatusMask& mask);
    /**
     * This operation creates a DataReaderImpl. The returned DataReaderImpl will be attached and belong to
     * the Subscriber.
     *
     * @param topic Topic the DataReaderImpl will be listening.
     * @param reader_qos QoS of the DataReaderImpl.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all).
     * @return Pointer to the created DataReaderImpl. nullptr if failed.
     */
    RTPS_DllAPI DataReaderImpl* create_datareader(
        vbs::TopicDescription* topic, const vbsutil::xmlparser::DataReaderQos& reader_qos,
        DataReaderListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * This operation creates a DataReaderImpl. The returned DataReaderImpl will be attached and belongs to
     * the Subscriber.
     *
     * @param topic Topic the DataReaderImpl will be listening.
     * @param profile_name DataReaderImpl profile name.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all).
     * @return Pointer to the created DataReaderImpl. nullptr if failed.
     */
    RTPS_DllAPI DataReaderImpl* create_datareader_with_profile(
        vbs::TopicDescription* topic, const std::string& profile_name, DataReaderListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * This operation creates a DataReaderImpl. The returned DataReaderImpl will be attached and belongs to
     * the Subscriber.
     *
     * @param topic Topic the DataReaderImpl will be listening.
     * @param qos DataReaderImpl qos.
     * @param listener Pointer to the listener (default: nullptr)
     * @param mask StatusMask that holds statuses the listener responds to (default: all).
     * @return Pointer to the created DataReaderImpl. nullptr if failed.
     */
    RTPS_DllAPI DataReaderImpl* create_datareader_with_profile(
        vbs::TopicDescription* topic, const vbsutil::xmlparser::DataReaderQos& qos,
        DataReaderListenerInner* listener = nullptr,
        const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all(), bool auto_enable = true);

    /**
     * This operation deletes a DataReaderImpl that belongs to the Subscriber.
     *
     * The delete_datareader operation must be called on the same Subscriber object used to create
     * the DataReaderImpl. If delete_datareader is called on a different Subscriber, the operation will
     * have no effect and it will return an error.
     *
     * @param reader DataReaderImpl to delete
     * @return RETCODE_PRECONDITION_NOT_MET if the datareader does not belong to this subscriber,
     * RETCODE_OK if it is correctly deleted and RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t delete_datareader(const DataReaderImpl* reader, bool free = true);
    RTPS_DllAPI ReturnCode_t add_datareader(const vbs::TopicImpl* topic, DataReaderImpl* reader);

    /**
     * This operation retrieves a previously-created DataReaderImpl belonging to the Subscriber that is
     * attached to a Topic with a matching topic_name. If no such DataReaderImpl exists, the operation
     * will return nullptr.
     *
     * If multiple DataReaders attached to the Subscriber satisfy this condition, then the operation
     * will return one of them. It is not specified which one.
     *
     * @param topic_name Name of the topic associated to the DataReaderImpl
     * @return Pointer to a previously created DataReaderImpl created on a Topic with that topic_name
     */
    RTPS_DllAPI DataReaderImpl* lookup_datareader(const std::string& topic_name) const;

    bool contains_entity(const evbs::ertps::rtps::InstanceHandle_t& handle) const;

    /**
     * This operation allows the application to access the DataReaderImpl objects.
     *
     * @param readers Vector of DataReaderImpl where the list of existing readers is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_datareaders(std::vector<DataReaderImpl*>& readers) const;

    /**
     * This operation checks if the subscriber has DataReaders
     *
     * @return true if the subscriber has one or several DataReaders, false in other case
     */
    RTPS_DllAPI bool has_datareaders() const;

    /**
     * This operation invokes the operation on_data_available on the DataReaderListenerInner objects
     * attached to contained DataReaderImpl entities.
     *
     * This operation is typically invoked from the on_data_on_readers operation in the
     * SubscriberListener. That way the SubscriberListener can delegate to the DataReaderListenerInner
     * objects the handling of the data.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t notify_datareaders() const;

    /**
     * @brief Deletes all contained DataReaders. If the DataReaders have any QueryCondition or
     * ReadCondition, they are deleted before the DataReaderImpl itself.
     *
     * @return RETCODE_OK if successful, an error code otherwise
     */
    RTPS_DllAPI ReturnCode_t delete_contained_entities();

    /**
     * This operation sets a default value of the DataReaderImpl QoS policies which will be used for
     * newly created DataReaderImpl entities in the case where the QoS policies are defaulted in the
     * create_datareader operation.
     *
     * This operation will check that the resulting policies are self consistent; if they are not,
     * the operation will have no effect and return false.
     *
     * The special value DATAREADER_QOS_DEFAULT may be passed to this operation to indicate that the
     * default QoS should be reset back to the initial values the factory would use, that is the
     * values that would be used if the set_default_datareader_qos operation had never been called.
     *
     * @param qos new value for DataReaderQos to set as default
     * @return RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the
     * qos is changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_default_datareader_qos(const vbsutil::xmlparser::DataReaderQos& qos);

    /**
     * This operation returns the default value of the DataReaderImpl QoS, that is, the QoS policies
     * which will be used for newly created DataReaderImpl entities in the case where the QoS policies
     * are defaulted in the create_datareader operation.
     *
     * The values retrieved get_default_datareader_qos will match the set of values specified on the
     * last successful call to get_default_datareader_qos, or else, if the call was never made, the
     * default values.
     *
     * @return Current default DataReaderQos.
     */
    RTPS_DllAPI const vbsutil::xmlparser::DataReaderQos& get_default_datareader_qos() const;

    /**
     * This operation returns the default value of the DataReaderImpl QoS, that is, the QoS policies
     * which will be used for newly created DataReaderImpl entities in the case where the QoS policies
     * are defaulted in the create_datareader operation.
     *
     * The values retrieved get_default_datareader_qos will match the set of values specified on the
     * last successful call to get_default_datareader_qos, or else, if the call was never made, the
     * default values.
     *
     * @return Current default DataReaderQos.
     */
    RTPS_DllAPI vbsutil::xmlparser::DataReaderQos& get_default_datareader_qos();

    /**
     * This operation retrieves the default value of the DataReaderImpl QoS, that is, the QoS policies
     * which will be used for newly created DataReaderImpl entities in the case where the QoS policies
     * are defaulted in the create_datareader operation.
     *
     * The values retrieved get_default_datareader_qos will match the set of values specified on the
     * last successful call to get_default_datareader_qos, or else, if the call was never made, the
     * default values.
     *
     * @param qos DataReaderQos where the default_qos is returned
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_default_datareader_qos(vbsutil::xmlparser::DataReaderQos& qos) const;

    /**
     * Fills the DataReaderQos with the values of the XML profile.
     *
     * @param profile_name DataReaderImpl profile name.
     * @param qos DataReaderQos object where the qos is returned.
     * @return RETCODE_OK if the profile exists. RETCODE_BAD_PARAMETER otherwise.
     */
    RTPS_DllAPI ReturnCode_t get_datareader_qos_from_profile(const std::string& profile_name,
                                                             vbsutil::xmlparser::DataReaderQos& qos) const;

    /**
     * This operation returns the DomainParticipant to which the Subscriber belongs.
     *
     * @return DomainParticipant Pointer
     */
    RTPS_DllAPI vbs::DomainParticipantImpl* get_participant() const;

    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant() const { return dispatcher_participant_; }

    vbs::dispatcher::DispatcherParticipant* dispatcher_participant() { return dispatcher_participant_; }

    /**
     * Returns the Subscriber's handle.
     *
     * @return InstanceHandle of this Subscriber.
     */
    RTPS_DllAPI const InstanceHandle_t& get_instance_handle() const;

    //! Remove all listeners in the hierarchy to allow a quiet destruction
    void disable();

    //! Check if any reader uses the given type name
    bool type_in_use(const std::string& type_name) const;

    /**
     * Check if the Qos values are compatible between each other.
     * @param qos Reference to the qos instance to check.
     * @return True if correct.
     */
    static ReturnCode_t check_qos(const vbsutil::xmlparser::SubscriberQos& qos);

    /**
     * Check if the Qos can be update with the values provided. This method DOES NOT update
     * anything.
     * @param to Reference to the qos instance to be changed.
     * @param from Reference to the qos instance with the new values.
     * @return True if they can be updated.
     */
    static bool can_qos_be_updated(const vbsutil::xmlparser::SubscriberQos& to,
                                   const vbsutil::xmlparser::SubscriberQos& from);

    bool can_be_deleted() const;

    /**
     * Set Qos from another instance
     * @param to Reference to the qos instance to be changed.
     * @param from Reference to the qos instance with the new values.
     * @param first_time Boolean indicating whether is the first time (If not some parameters cannot
     * be set).
     */
    static void set_qos(vbsutil::xmlparser::SubscriberQos& to, const vbsutil::xmlparser::SubscriberQos& from,
                        bool first_time);

    SubscriberListener* get_listener_for(const evbs::edds::dds::StatusMask& status);

    void reset_default_datareader_qos();
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBER_HPP_
