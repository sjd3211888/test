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
// feature: remove system discovery
// feature: support e2e deadline and liveliness 1. config deadline sendPeriod 2. enable deadline event
// feature: change name NewusContext to NexusContext
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: Reduce level of DomainParticipantListener
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// feature: use environment variable to control system discovery.
// feature: Enable trace and log control with udp
// feature: separating interface and implementation about DataWriter
// feature: clean do_coverity
// feature: VBSDynamicData interface constness support
// feature: VBS framework add performance test
// feature: activate global discovery in vbs
// ------------------------------------------------------------------

#ifndef INCLUDE_IMPLEMENTATION_PUB_DATAWRITERIMPL_HPP_
#define INCLUDE_IMPLEMENTATION_PUB_DATAWRITERIMPL_HPP_

#include <memory>

#include "edds/rtps/common/Time_t.h"
#include "context/NexusContext.hpp"
#include "context/NexusUnits.hpp"
#include "context/QosManager.hpp"
#include "ertps/ertps_dll.h"
#include "ertps/qos/DeadlineMissedStatus.h"
#include "ertps/types/TypesBase.h"
#include "edds/dds/builtin/topic/SubscriptionBuiltinTopicData.hpp"
#include "core/Entity.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "deps/core/status/IncompatibleQosStatus.hpp"
#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "deps/core/status/StatusMask.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"
#include "deps/common/LocatorList.hpp"
#include "edds/rtps/common/Guid.h"
#include "core/WriteParams.hpp"
#include "history/IChangePool.h"
#include "history/IPayloadPool.h"
#include "history/RemoteEndpointInfo.h"
#include "history/WriterListener.h"
#include "dispatcher/DispatcherWriter.hpp"
#include "dispatcher/DispatcherParticipant.hpp"
#include "ertps/qos/LivelinessLostStatus.h"
#include "history/ITopicPayloadPool.h"
#include "edds/rtps/attributes/WriterAttributes.h"
#include "pub/DataWriterListenerInner.hpp"
#include "topic/TopicCore.hpp"
#include "deps/common/TypeSupport.hpp"
#include "edds/dds/builtin/lookup/common/LookupTypes.hpp"
#include "deps/common/LocatorList.hpp"
#include "pub/DataWriterHistory.hpp"
#include "pub/ReaderFilterCollection.hpp"
#include "ertps/common/PayloadInfo_t.hpp"
#include "edds/rtps/interfaces/IReaderDataFilter.hpp"
#include "qos/DataWriterQosImpl.hpp"
#include "vbs/DataWriter.hpp"

using evbs::ReturnCode_t;

namespace evbs {
namespace ertps {
namespace rtps {

using vbsutil::xmlparser::GUID_t;

}  // namespace rtps
}  // namespace ertps
}  // namespace evbs

namespace vbs {

namespace common {
class TimedEvent;
class WriteParams;
}  // namespace common

class PublisherListener;
class Publisher;
class DataWriterListenerInner;
class Topic;

/**
 * Class DataWriterImpl, contains the actual implementation of the behaviour of the DataWriter.
 *
 * @ingroup EDDS_MODULE
 */
class DataWriterImpl : public DomainEntity, protected evbs::ertps::rtps::IReaderDataFilter {
    friend class DataWriterListenerInner;
    friend class DataWriter;

 public:
    enum class LoanInitializationKind {
        /**
         * @brief Do not perform initialization of sample.
         *
         * This is the default initialization scheme of loaned samples.
         * It is the fastest scheme, but implies the user should take care of writing
         * every field on the data type before calling @ref write on the loaned sample.
         */
        NO_LOAN_INITIALIZATION,

        /**
         * @brief Initialize all memory with zero-valued bytes.
         *
         * The contents of the loaned sample will be zero-initialized upon return of
         * @ref loan_sample.
         */
        ZERO_LOAN_INITIALIZATION,

        /**
         * @brief Use in-place constructor initialization.
         *
         * This will call the constructor of the data type over the memory space being
         * returned by @ref loan_sample.
         */
        CONSTRUCTED_LOAN_INITIALIZATION
    };

    using LoanInitializationKind = DataWriterImpl::LoanInitializationKind;
    using PayloadInfo_t = evbs::ertps::rtps::detail::PayloadInfo_t;
    using CacheChange_t = vbs::common::CacheChange_t;
    class LoanCollection;

 private:
    vbs::dispatcher::DispatcherWriter* create_dispatcher_writer(evbs::ertps::rtps::WriterAttributes& watt,
                                                                TopicAttributes& topic_att, WriterQos& wqos);

    bool sample_init(void*& sample, uint32_t real_size, LoanInitializationKind initialization);

    void add_topic_id_to_inline_qos(SerializedPayload_t& payload, uint32_t topic_id);

 protected:
    friend class Publisher;

    /**
     * Create a data writer, assigning its pointer to the associated implementation.
     * Don't use directly, create DataWriterImpl using create_datawriter from Publisher.
     */
    DataWriterImpl(Publisher* pub, TypeSupport type, vbs::TopicImpl* topic,
                   const vbsutil::xmlparser::DataWriterQos& qos = vbsutil::xmlparser::getDefaultDataWriteQos(),
                   DataWriterListenerInner* listener = nullptr,
                   const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

 public:
    /**
     * How to initialize samples loaned with @ref loan_sample
     */

    virtual ~DataWriterImpl();

    /**
     * @brief This operation enables the DataWriterImpl
     *
     * @return RETCODE_OK is successfully enabled. RETCODE_PRECONDITION_NOT_MET if the Publisher
     * creating this DataWriterImpl is not enabled.
     */
    RTPS_DllAPI evbs::ReturnCode_t enable() override;

    RTPS_DllAPI void disable();

    /**
     * Flushes the batch in progress in the context of the calling thread.
     */
    RTPS_DllAPI bool flush();

    /**
     * Write data to the topic.
     *
     * @param data Pointer to the data
     * @return True if correct, false otherwise
     */
    RTPS_DllAPI bool write(void* data);

    /**
     * Write data with params to the topic.
     *
     * @param data Pointer to the data
     * @param params Extra write parameters.
     * @return True if correct, false otherwise
     */
    RTPS_DllAPI bool write(void* data, vbs::common::WriteParams& params);

    /**
     * Write data with params to the topic.
     *
     * @param data Pointer to the data
     * @param params Extra write parameters.
     * @return True if correct, false otherwise
     */
    RTPS_DllAPI bool write_buffer(void* data, vbs::common::WriteParams& params, const uint32_t len);

    /**
     * Write data with handle.
     *
     * The special value HANDLE_NIL can be used for the parameter handle.This indicates that the
     * identity of the instance should be automatically deduced from the instance_data (by means of
     * the key).
     *
     * @param data Pointer to the data
     * @param handle InstanceHandle_t.
     * @return RETCODE_PRECONDITION_NOT_MET if the handle introduced does not match with the one
     * associated to the data, RETCODE_OK if the data is correctly sent and RETCODE_ERROR otherwise.
     */

    RTPS_DllAPI ReturnCode_t write(void* data, const InstanceHandle_t& handle);

    /**
     * @brief This operation performs the same function as write except that it also provides the
     * value for the
     * @ref vbs::SampleInfoInner::source_timestamp "source_timestamp" that is made available
     * to DataReader objects by means of the @ref vbs::SampleInfoInner::source_timestamp
     * attribute "source_timestamp" inside the SampleInfo. The constraints on the values of the @c
     * handle parameter and the corresponding error behavior are the same specified for the @ref
     * write operation. This operation may block and return RETCODE_TIMEOUT under the same
     * circumstances described for the @ref write operation.
     * This operation may return RETCODE_OUT_OF_RESOURCES, RETCODE_PRECONDITION_NOT_MET or
     * RETCODE_BAD_PARAMETER under the same circumstances described for the write operation.
     *
     * @param data Pointer to the data
     * @param handle InstanceHandle_t
     * @param timestamp Time_t used to set the source_timestamp.
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t write_w_timestamp(void* data, const InstanceHandle_t& handle,
                                               const evbs::ertps::Time_t& timestamp);

    /*!
     * @brief Informs that the application will be modifying a particular instance.
     * It gives an opportunity to the middleware to pre-configure itself to improve performance.
     *
     * @param[in] instance Sample used to get the instance's key.
     * @return Handle containing the instance's key.
     * This handle could be used in successive `write` or `dispose` operations.
     * In case of error, HANDLE_NIL will be returned.
     */
    RTPS_DllAPI InstanceHandle_t register_instance(void* instance);

    /**
     * @brief This operation performs the same function as register_instance and can be used instead
     * of
     * @ref register_instance in the cases where the application desires to specify the value for
     * the
     * @ref vbs::SampleInfoInner::source_timestamp "source_timestamp".
     * The @ref vbs::SampleInfoInner::source_timestamp "source_timestamp" potentially affects
     * the relative order in which readers observe events from multiple writers. See the QoS policy
     * vbsutil::xmlparser::DataWriterQos::destination_order "DESTINATION_ORDER".
     *
     * This operation may block and return RETCODE_TIMEOUT under the same circumstances described
     * for the @ref write operation.
     *
     * This operation may return RETCODE_OUT_OF_RESOURCES under the same circumstances described for
     * the
     * @ref write operation.
     *
     * @param instance  Sample used to get the instance's key.
     * @param timestamp Time_t used to set the source_timestamp.
     * @return Handle containing the instance's key.
     */
    RTPS_DllAPI InstanceHandle_t register_instance_w_timestamp(void* instance, const evbs::ertps::Time_t& timestamp);

    /*!
     * @brief This operation reverses the action of `register_instance`.
     * It should only be called on an instance that is currently registered.
     * Informs the middleware that the DataWriterImpl is not intending to modify any more of that data
     * instance. Also indicates that the middleware can locally remove all information regarding
     * that instance.
     *
     * @param[in] instance Sample used to deduce instance's key in case of `handle` parameter is
     * HANDLE_NIL.
     * @param[in] handle Instance's key to be unregistered.
     * @return Returns the operation's result.
     * If the operation finishes successfully, ReturnCode_t::RETCODE_OK is returned.
     */
    RTPS_DllAPI ReturnCode_t unregister_instance(void* instance, const InstanceHandle_t& handle, bool dispose = false);

    /**
     * @brief This operation performs the same function as @ref unregister_instance and can be used
     * instead of
     * @ref unregister_instance in the cases where the application desires to specify the value for
     * the
     * @ref vbs::SampleInfoInner::source_timestamp "source_timestamp".
     * The @ref vbs::SampleInfoInner::source_timestamp "source_timestamp" potentially affects
     * the relative order in which readers observe events from multiple writers. See the QoS policy
     * vbsutil::xmlparser::DataWriterQos::destination_order "DESTINATION_ORDER".
     *
     * The constraints on the values of the @c handle parameter and the corresponding error behavior
     * are the same specified for the @ref unregister_instance operation.
     *
     * This operation may block and return RETCODE_TIMEOUT under the same circumstances described
     * for the write operation
     *
     * @param instance  Sample used to deduce instance's key in case of `handle` parameter is
     * HANDLE_NIL.
     * @param handle Instance's key to be unregistered.
     * @param timestamp Time_t used to set the source_timestamp.
     * @return Handle containing the instance's key.
     */
    RTPS_DllAPI ReturnCode_t unregister_instance_w_timestamp(void* instance, const InstanceHandle_t& handle,
                                                             const evbs::ertps::Time_t& timestamp,
                                                             bool dispose = false);

    /**
     * This operation can be used to retrieve the instance key that corresponds to an
     * @ref vbs::Entity::instance_handle_ "instance_handle".
     * The operation will only fill the fields that form the key inside the key_holder instance.
     *
     * This operation may return BAD_PARAMETER if the InstanceHandle_t handle does not correspond to
     * an existing data-object known to the DataWriterImpl. If the implementation is not able to check
     * invalid handles then the result in this situation is unspecified.
     *
     * @param[in,out] key_holder  Sample where the key fields will be returned.
     * @param[in] handle          Handle to the instance to retrieve the key values from.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

    /**
     * Returns the DataWriterImpl's GUID
     *
     * @return Reference to the DataWriterImpl GUID
     */
    RTPS_DllAPI const evbs::ertps::rtps::GUID_t& guid() const;

    /**
     * Returns the DataWriterImpl's InstanceHandle
     *
     * @return Copy of the DataWriterImpl InstanceHandle
     */
    RTPS_DllAPI InstanceHandle_t get_instance_handle() const;

    /**
     * Waits the current thread until all writers have received their acknowledgments.
     *
     * @param max_wait Maximum blocking time for this operation
     * @return RETCODE_OK if the DataWriterImpl receive the acknowledgments before the time expires and
     * RETCODE_ERROR otherwise
     */
    RTPS_DllAPI ReturnCode_t wait_for_acknowledgments(const evbs::ertps::Duration_t& max_wait);

    /**
     * @brief Returns the offered deadline missed status
     *
     * @param[out] status Deadline missed status struct
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_offered_deadline_missed_status(evbs::edds::dds::OfferedDeadlineMissedStatus& status);

    /**
     * @brief Returns the offered incompatible qos status
     *
     * @param[out] status Offered incompatible qos status struct
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_offered_incompatible_qos_status(evbs::edds::dds::OfferedIncompatibleQosStatus& status);

    /**
     * @brief Returns the publication matched status
     *
     * @param[out] status publication matched status struct
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_publication_matched_status(evbs::edds::dds::PublicationMatchedStatus& status) const;

    /**
     * Establishes the DataWriterQos for this DataWriterImpl.
     *
     * @param qos DataWriterQos to be set
     * @return RETCODE_IMMUTABLE_POLICY if any of the Qos cannot be changed,
     * RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the qos is
     * changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_qos(const vbsutil::xmlparser::DataWriterQos& qos);

    /**
     * Retrieves the DataWriterQos for this DataWriterImpl.
     *
     * @return Reference to the current DataWriterQos
     */
    RTPS_DllAPI const vbsutil::xmlparser::DataWriterQos& get_dds_qos() const;

    /**
     * Fills the DataWriterQos with the values of this DataWriterImpl.
     *
     * @param qos DataWriterQos object where the qos is returned.
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_dds_qos(vbsutil::xmlparser::DataWriterQos& qos) const;

    /**
     * Retrieves the topic for this DataWriterImpl.
     *
     * @return Pointer to the associated Topic
     */
    RTPS_DllAPI vbs::TopicImpl* get_topic() const;

    /**
     * Retrieves the listener for this DataWriterImpl.
     *
     * @return Pointer to the DataWriterListenerInner
     */
    RTPS_DllAPI const DataWriterListenerInner* get_listener() const;

    /**
     * Modifies the DataWriterListenerInner, sets the mask to StatusMask::all()
     *
     * @param listener new value for the DataWriterListenerInner
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DataWriterListenerInner* listener);

    /**
     * Modifies the DataWriterListenerInner.
     *
     * @param listener new value for the DataWriterListenerInner
     * @param mask StatusMask that holds statuses the listener responds to (default: all).
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DataWriterListenerInner* listener, const evbs::edds::dds::StatusMask& mask);

    /**
     * @brief This operation requests the middleware to delete the data (the actual deletion is
     * postponed until there is no more use for that data in the whole system). In general,
     * applications are made aware of the deletion by means of operations on the DataReader objects
     * that already knew that instance. This operation does not modify the value of the instance.
     * The instance parameter is passed just for the purposes of identifying the instance. When this
     * operation is used, the Service will automatically supply the value of the source_timestamp
     * that is made available to DataReader objects by means of the source_timestamp attribute
     * inside the SampleInfo. The constraints on the values of the handle parameter and the
     * corresponding error behavior are the same specified for the unregister_instance operation.
     *
     * @param[in] data Sample used to deduce instance's key in case of `handle` parameter is
     * HANDLE_NIL.
     * @param[in] handle InstanceHandle of the data
     * @return RETCODE_PRECONDITION_NOT_MET if the handle introduced does not match with the one
     * associated to the data, RETCODE_OK if the data is correctly sent and RETCODE_ERROR otherwise.
     */
    RTPS_DllAPI ReturnCode_t dispose(void* data, const InstanceHandle_t& handle);

    /**
     * @brief This operation performs the same functions as @ref dispose except that the application
     * provides the value for the @ref vbs::SampleInfoInner::source_timestamp
     * "source_timestamp" that is made available to DataReader objects by means of the @ref
     * vbs::SampleInfoInner::source_timestamp "source_timestamp" attribute inside the
     * SampleInfo.
     *
     * The constraints on the values of the @c handle parameter and the corresponding error behavior
     * are the same specified for the @ref dispose operation.
     *
     * This operation may return RETCODE_PRECONDITION_NOT_MET and RETCODE_BAD_PARAMETER under the
     * same circumstances described for the @ref dispose operation.
     *
     * This operation may return RETCODE_TIMEOUT and RETCODE_OUT_OF_RESOURCES under the same
     * circumstances described for the @ref write operation.
     *
     * @param instance  Sample used to deduce instance's key in case of `handle` parameter is
     * HANDLE_NIL.
     * @param handle Instance's key to be disposed.
     * @param timestamp Time_t used to set the source_timestamp.
     * @return RTPS_DllAPI
     */
    RTPS_DllAPI ReturnCode_t dispose_w_timestamp(void* instance, const InstanceHandle_t& handle,
                                                 const evbs::ertps::Time_t& timestamp);
    /**
     * @brief Returns the liveliness lost status
     *
     * @param status Liveliness lost status struct
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_liveliness_lost_status(evbs::edds::dds::LivelinessLostStatus& status);

    /**
     * @brief Getter for the Publisher that creates this DataWriterImpl
     *
     * @return Pointer to the Publisher
     */
    RTPS_DllAPI Publisher* get_publisher();

    RTPS_DllAPI const Publisher* get_publisher() const;

    /**
     * @brief This operation manually asserts the liveliness of the DataWriterImpl. This is used in
     * combination with the LivelinessQosPolicy to indicate to the Service that the entity remains
     * active. This operation need only be used if the LIVELINESS setting is either
     * MANUAL_BY_PARTICIPANT or MANUAL_BY_TOPIC. Otherwise, it has no effect.
     *
     * @note Writing data via the write operation on a DataWriterImpl asserts liveliness on the
     * DataWriterImpl itself and its DomainParticipantImpl. Consequently the use of assert_liveliness is
     * only needed if the application is not writing data regularly.
     *
     * @return RETCODE_OK if asserted, RETCODE_ERROR otherwise
     */
    RTPS_DllAPI bool assert_liveliness();

    /**
     * @brief Clears the DataWriterImpl history
     *
     * @param removed size_t pointer to return the size of the data removed
     * @return RETCODE_OK if the samples are removed and RETCODE_ERROR otherwise
     */
    RTPS_DllAPI ReturnCode_t clear_history(size_t* removed);

    /**
     * @brief Get a pointer to the internal pool where the user could directly write.
     *
     * This method can only be used on a DataWriterImpl for a plain data type. It will provide the
     * user with a pointer to an internal buffer where the data type can be prepared for sending.
     *
     * When using NO_LOAN_INITIALIZATION on the initialization parameter, which is the default,
     * no assumptions should be made on the contents where the pointer points to, as it may be an
     * old pointer being reused. See @ref LoanInitializationKind for more details.
     *
     * Once the sample has been prepared, it can then be published by calling @ref write.
     * After a successful call to @ref write, the middleware takes ownership of the loaned pointer
     * again, and the user should not access that memory again.
     *
     * If, for whatever reason, the sample is not published, the loan can be returned by calling
     * @ref discard_loan.
     *
     * @param [out] sample          Pointer to the sample on the internal pool.
     * @param [in]  initialization  How to initialize the loaned sample.
     *
     * @return ReturnCode_t::RETCODE_ILLEGAL_OPERATION when the data type does not support loans.
     * @return ReturnCode_t::RETCODE_NOT_ENABLED if the writer has not been enabled.
     * @return ReturnCode_t::RETCODE_OUT_OF_RESOURCES if the pool has been exhausted.
     * @return ReturnCode_t::RETCODE_OK if a pointer to a sample is successfully obtained.
     */
    RTPS_DllAPI ReturnCode_t
    loan_sample(void*& sample, LoanInitializationKind initialization = LoanInitializationKind::NO_LOAN_INITIALIZATION);

    /**
     * @brief Get a pointer to the internal pool where the user could directly write.
     *
     * This method can only be used on a DataWriterImpl for a plain data type. It will provide the
     * user with a pointer to an internal buffer where the data type can be prepared for sending.
     *
     * When using NO_LOAN_INITIALIZATION on the initialization parameter, which is the default,
     * no assumptions should be made on the contents where the pointer points to, as it may be an
     * old pointer being reused. See @ref LoanInitializationKind for more details.
     *
     * Once the sample has been prepared, it can then be published by calling @ref write.
     * After a successful call to @ref write, the middleware takes ownership of the loaned pointer
     * again, and the user should not access that memory again.
     *
     * If, for whatever reason, the sample is not published, the loan can be returned by calling
     * @ref discard_loan.
     *
     * @param [out] sample          Pointer to the sample on the internal pool.
     * @param [in]  size            size of sample, if equals 0, use type`size
     * @param [in]  initialization  How to initialize the loaned sample.
     *
     * @return ReturnCode_t::RETCODE_ILLEGAL_OPERATION when the data type does not support loans.
     * @return ReturnCode_t::RETCODE_NOT_ENABLED if the writer has not been enabled.
     * @return ReturnCode_t::RETCODE_OUT_OF_RESOURCES if the pool has been exhausted.
     * @return ReturnCode_t::RETCODE_OK if a pointer to a sample is successfully obtained.
     */
    RTPS_DllAPI ReturnCode_t
    loan_sample(void*& sample, uint32_t size,
                LoanInitializationKind initialization = LoanInitializationKind::NO_LOAN_INITIALIZATION);

    /**
     * @brief Discards a loaned sample pointer.
     *
     * See the description on @ref loan_sample for how and when to call this method.
     *
     * @param [in,out] sample  Pointer to the previously loaned sample.
     *
     * @return ReturnCode_t::RETCODE_ILLEGAL_OPERATION when the data type does not support loans.
     * @return ReturnCode_t::RETCODE_NOT_ENABLED if the writer has not been enabled.
     * @return ReturnCode_t::RETCODE_BAD_PARAMETER if the pointer does not correspond to a loaned
     * sample.
     * @return ReturnCode_t::RETCODE_OK if the loan is successfully discarded.
     */
    RTPS_DllAPI ReturnCode_t discard_loan(void*& sample);

    /**
     * Block the current thread until the writer has received the acknowledgment corresponding to
     * the given instance. Operations performed on the same instance while the current thread is
     * waiting will not be taken into consideration, i.e. this method may return `RETCODE_OK` with
     * those operations unacknowledged.
     *
     * @param instance Sample used to deduce instance's key in case of `handle` parameter is
     * HANDLE_NIL.
     * @param handle Instance handle of the data.
     * @param max_wait Maximum blocking time for this operation.
     *
     * @return RETCODE_NOT_ENABLED if the writer has not been enabled.
     * @return RETCODE_BAD_PARAMETER if `instance` is not a valid pointer.
     * @return RETCODE_PRECONDITION_NOT_MET if the topic does not have a key, the key is unknown to
     * the writer, or the key is not consistent with `handle`.
     * @return RETCODE_OK if the DataWriterImpl received the acknowledgments before the time expired.
     * @return RETCODE_TIMEOUT otherwise.
     */
    RTPS_DllAPI ReturnCode_t wait_for_acknowledgments(void* instance, const InstanceHandle_t& handle,
                                                      const evbs::ertps::Duration_t& max_wait);

    /**
     * Check if the preconditions to delete this object are met.
     *
     * @return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET if the preconditions to delete this object
     * are not met.
     * @return ReturnCode_t::RETCODE_OK if it is safe to delete this object.
     */
    ReturnCode_t check_delete_preconditions();

#ifdef BATCH_SEND_ENABLE
    /**
     * Flushes the batch in progress in the context of the calling thread.
     */
    bool flush();
#endif
    /**
     *
     * @param kind
     * @param  data
     * @param wparams
     * @return
     */
    ReturnCode_t create_new_change_with_params(vbs::common::ChangeKind_t kind, void* data,
                                               vbs::common::WriteParams& wparams, const uint32_t len);

    /**
     *
     * @param kind
     * @param  data
     * @param wparams
     * @param handle
     * @return
     */
    ReturnCode_t create_new_change_with_params(vbs::common::ChangeKind_t kind, void* data,
                                               vbs::common::WriteParams& wparams, const InstanceHandle_t& handle);

    ReturnCode_t check_write_preconditions(void* data, const InstanceHandle_t& handle,
                                           InstanceHandle_t& instance_handle);

    TypeSupport get_type() const { return type_; }

    ReturnCode_t get_publication_matched_status(evbs::edds::dds::PublicationMatchedStatus& status);

    void get_statistic_send_info(builtin::StatisticSendInfo* info);

    /**c
     * Called from the DomainParticipantImpl when a filter factory is being unregistered.
     *
     * @param filter_class_name  The class name under which the factory was registered.
     */
    // void filter_is_being_removed(const char* filter_class_name);
 protected:
    using IChangePool = vbs::common::IChangePool;
    using IPayloadPool = vbs::common::IPayloadPool;
    using ITopicPayloadPool = vbs::common::ITopicPayloadPool;

    Publisher* publisher_ = nullptr;

    //! Pointer to the associated Data Writer.
    vbs::dispatcher::DispatcherWriter* writer_ = nullptr;

    //! Pointer to the TopicDataType object.
    TypeSupport type_;

    vbsutil::xmlparser::DataWriterQos qos_;

    //! History
    DataWriterHistory* history_ = nullptr;

    //! DataWriterListenerInner
    vbs::DataWriterListener* user_listener_ = nullptr;
    DataWriterListenerInner* inner_listener_ = nullptr;

    //! Listener to capture the events of the Writer
    class InnerDataWriterListener : public vbs::common::WriterListener {
     public:
        InnerDataWriterListener(DataWriterImpl* w) : data_writer_(w) {}

        ~InnerDataWriterListener() override {}

        void onWriterMatched(const evbs::ertps::rtps::MatchingInfo& info) override;

        void on_offered_incompatible_qos(evbs::edds::dds::PolicyMask qos) override;

        void onWriterChangeReceivedByAll(vbs::dispatcher::SubstrateWriter* const writer,
                                         vbs::common::CacheChange_t* change) override;

        void on_liveliness_lost(const evbs::edds::dds::LivelinessLostStatus& status) override;

        void on_endpoint_crash(const evbs::ertps::rtps::GUID_t& reader_guid) override;

        void on_reader_discovery(evbs::edds::dds::READER_DISCOVERY_STATUS reason,
                                 const evbs::ertps::rtps::GUID_t& reader_guid,
                                 vbs::RemoteEndpointInfo& reader_info) override;

        void on_non_plain_reader_discovery() override;

        DataWriterImpl* data_writer_;
    } listener_wrapper_;

    //! A timer used to check for deadlines
    vbs::common::TimedEvent* deadline_timer_ = nullptr;

    //! Deadline duration in microseconds
    std::chrono::duration<double_t, std::ratio<1, 1000000>> deadline_duration_us_ {0.0};

    //! The current timer owner, i.e. the instance which started the deadline timer
    InstanceHandle_t timer_owner_;

    //! The publication matched status
    evbs::edds::dds::PublicationMatchedStatus publication_matched_status_;

    //! The offered deadline missed status
    evbs::edds::dds::OfferedDeadlineMissedStatus deadline_missed_status_;

    //! The liveliness lost status
    evbs::edds::dds::LivelinessLostStatus liveliness_lost_status_;

    //! The offered incompatible qos status
    evbs::edds::dds::OfferedIncompatibleQosStatus offered_incompatible_qos_status_;

    //! A timed callback to remove expired samples for lifespan QoS
    vbs::common::TimedEvent* lifespan_timer_ = nullptr;

    //! The lifespan duration, in microseconds
    std::chrono::duration<double_t, std::ratio<1, 1000000>> lifespan_duration_us_ {0.0};

    uint32_t fixed_payload_size_ = 0u;

    std::shared_ptr<IPayloadPool> payload_pool_;

    std::unique_ptr<LoanCollection> loans_;

    evbs::ertps::rtps::GUID_t guid_;

    std::unique_ptr<ReaderFilterCollection> reader_filters_;
    /*管理多线程并发对同一个writer写数据 */
    std::timed_mutex write_mutex_;

    ReturnCode_t check_instance_preconditions(void* data, const InstanceHandle_t& handle,
                                              InstanceHandle_t& instance_handle);

    InstanceHandle_t do_register_instance(void* key, const InstanceHandle_t instance_handle,
                                          vbs::common::WriteParams& wparams);

    /**
     * Removes the cache change with the minimum sequence number
     * @return True if correct.
     */
    bool remove_min_seq_change();

    void update_publication_matched_status(const evbs::ertps::rtps::MatchingInfo& status);

    /**
     * @brief A method called when an instance misses the deadline
     */
    bool deadline_missed();

    /**
     * @brief A method to reschedule the deadline timer
     */
    bool deadline_timer_reschedule();

    /**
     * @brief A method to remove expired samples, invoked when the lifespan timer expires
     */
    bool lifespan_expired();

    inline ReturnCode_t check_new_change_preconditions(vbs::common::ChangeKind_t change_kind, void* data);

    ReturnCode_t perform_create_new_change(vbs::common::ChangeKind_t change_kind, void* data,
                                           vbs::common::WriteParams& wparams, const InstanceHandle_t& handle,
                                           const uint32_t len);

    static std::shared_ptr<evbs::ertps::TopicAttributes> get_topic_attributes(
        const vbsutil::xmlparser::DataWriterQos& qos, const vbs::TopicImpl& topic, const TypeSupport& type);

    static void set_qos(vbsutil::xmlparser::DataWriterQos& to, const vbsutil::xmlparser::DataWriterQos& from,
                        bool is_default);

    /**
     * Extends the check_qos() call, including the check for
     * resource limits policy.
     * @param qos Pointer to the qos to be checked.
     * @param type Pointer to the associated TypeSupport object.
     * @return True if correct.
     */
    static ReturnCode_t check_qos_including_resource_limits(const vbsutil::xmlparser::DataWriterQos& qos,
                                                            const TypeSupport& type);

    /**
     * Checks the consistency of the qos configuration.
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static ReturnCode_t check_qos(const vbsutil::xmlparser::DataWriterQos& qos);

    /**
     * Checks resource limits policy: Instance allocation consistency
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static ReturnCode_t check_allocation_consistency(const vbsutil::xmlparser::DataWriterQos& qos);

    static bool can_qos_be_updated(const vbsutil::xmlparser::DataWriterQos& to,
                                   const vbsutil::xmlparser::DataWriterQos& from);

    void publisher_qos_updated();

    evbs::ertps::OfferedIncompatibleQosStatus& update_offered_incompatible_qos(PolicyMask incompatible_policies);

    /*!
     * @brief Updates liveliness lost status.
     *
     * @param[in] liveliness_lost_status Liveliness lost status coming from RTPS layer.
     * @return Current liveliness lost status.
     */
    evbs::edds::dds::LivelinessLostStatus& update_liveliness_lost_status(
        const evbs::edds::dds::LivelinessLostStatus& liveliness_lost_status);

    /**
     * Returns the most appropriate listener to handle the callback for the given status,
     * or nullptr if there is no appropriate listener.
     */
    DataWriterListenerInner* get_listener_for(const evbs::edds::dds::StatusMask& status);

    void set_fragment_size_on_change(vbs::common::WriteParams& wparams, vbs::common::CacheChange_t* ch,
                                     const uint32_t& high_mark_for_frag);

    std::shared_ptr<IChangePool> get_change_pool() const;

    std::shared_ptr<IPayloadPool> get_payload_pool();

    bool release_payload_pool();

    template <typename SizeFunctor>
    bool get_free_payload_from_pool(const SizeFunctor& size_getter, PayloadInfo_t& payload) {
        CacheChange_t change;
        if (!payload_pool_) {
            return false;
        }

        uint32_t size = fixed_payload_size_ ? fixed_payload_size_ : size_getter();
        if (!payload_pool_->get_payload(size, change)) {
            return false;
        }

        payload.move_from_change(change);
        return true;
    }

    inline void return_payload_to_pool(PayloadInfo_t& payload) {
        CacheChange_t change;
        payload.move_into_change(change, true);
        (void)payload_pool_->release_payload(change);
    }

    inline bool add_loan(void* data, PayloadInfo_t& payload);

    inline bool check_and_remove_loan(void* data, PayloadInfo_t& payload);

    void remove_reader_filter(const evbs::ertps::rtps::GUID_t& reader_guid);

    void add_reader_filter(const evbs::ertps::rtps::GUID_t& reader_guid,
                           const evbs::ertps::rtps::ContentFilterProperty& filter_property);

    bool is_relevant(const evbs::ertps::rtps::CacheChange_t& change,
                     const evbs::ertps::rtps::GUID_t& reader_guid) const override;

    bool check_filter_signature(evbs::ertps::rtps::CacheChange_t& change,
                                const vbsutil::xmlparser::ContentFilterProperty& filter_property,
                                const evbs::ertps::rtps::GUID_t& reader_guid) override;

    WriterAttributes get_writer_attributes() const;

 private:
    // make sure that NexusContext is released after all entities release.
    std::shared_ptr<NexusContext> context_ = NexusContext::get_context_shared();
    DataWriter* user_datawriter_ = nullptr;
    bool dds_is_initialized_ = false;
    bool batch_free_ = false;
    std::mutex on_publication_matched_mutex_;
    vbs::DataWriterQosImpl vbs_qos_;
    int32_t dds_match_ = 0;
    DomainParticipantImpl* writer_participant_ = nullptr;
    vbs::TopicImpl* writer_topic_ = nullptr;
    vbs::TopicImpl* dds_topic_ = nullptr;
    WriteParams* wparams_ = nullptr;
    int32_t match_count_ = 0;

    VBSGuid vbs_guid_;
    uint16_t writer_guid_ = 0;
    VBSWriteParams dsf_wparams_;

 private:
    void set_wparams_id(uint64_t id);

 public:
    DataWriterImpl(DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                   const std::string& profile_name, vbs::DataWriterListener* const listener = nullptr,
                   const vbs::StatusMask& mask = vbs::StatusMask::all());
    DataWriterImpl(DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                   const vbs::DataWriterQosImpl& local_data_writer_qos,
                   vbs::DataWriterListener* const listener = nullptr,
                   const vbs::StatusMask& mask = vbs::StatusMask::all());

    bool datawriter_init(vbs::TopicImpl* topic, const vbsutil::xmlparser::DataWriterQos& qos, bool auto_enable = true);
    bool datawriter_init_with_profile(vbs::TopicImpl* topic, const vbsutil::xmlparser::DataWriterQos& qos_input,
                                      bool auto_enable);

    bool write(const vbs::VBSDynamicData* data, uint32_t len);

    bool write(void* const data, const uint32_t& len);

    bool write_buffer(void* const data, const uint32_t& len);
    const vbs::DataWriterQosImpl& get_vbs_qos() const;
    bool serialize(void* const data, void* const payload_buffer, uint32_t* const payload_len);

    bool init_verify();
    const DataWriterQosImpl& get_qos() const;
    bool GetQos(DataWriterQosImpl& qos);

    bool SetQos(const DataWriterQosImpl& qos);
    // void user_enable();

    uint16_t get_guid();
    DomainParticipantImpl* get_paritcipantimpl();

    void set_user_listener(vbs::DataWriterListener* listener);
    void set_user_listener(vbs::DataWriterListener* listener, const vbs::StatusMask& mask);

    void on_publication_matched(DataWriterImpl* writer, const evbs::edds::dds::PublicationMatchedStatus& info);
    void on_offered_deadline_missed(DataWriterImpl* writer, const evbs::edds::dds::OfferedDeadlineMissedStatus& status);
    void on_liveliness_lost(DataWriterImpl* writer, const evbs::edds::dds::LivelinessLostStatus& status);
    void on_endpoint_crash(DataWriterImpl* writer, const evbs::ertps::rtps::GUID_t& reader_guid);
};

}  // namespace vbs
#endif  // INCLUDE_IMPLEMENTATION_PUB_DATAWRITERIMPL_HPP_
