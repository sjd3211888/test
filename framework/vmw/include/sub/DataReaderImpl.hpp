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
// feature: change name NewusContext to NexusContext
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: Reduce level of DomainParticipantListener
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// feature: use environment variable to control system discovery.
// feature: Enable trace and log control with udp
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: separating interface and implementation about DataReader
// feature: clean do_coverity
// feature: Support global discovery by udp multicast
// feature: VBS framework add performance test
// feature: activate global discovery in vbs
// ------------------------------------------------------------------

#ifndef INCLUDE_SUB_DATAREADERIMPL_HPP_
#define INCLUDE_SUB_DATAREADERIMPL_HPP_

#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#if !defined(_WIN32)
#include <shared_mutex>
#endif
#include <variant>
#include <cstdint>
#include <mutex>

#include "edds/dds/builtin/topic/PublicationBuiltinTopicData.hpp"
#include "core/Entity.hpp"
#include "deps/base/LoanableCollection.hpp"
#include "core/LoanableSequence.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "deps/core/status/DeadlineMissedStatus.hpp"
#include "deps/core/status/IncompatibleQosStatus.hpp"
#include "deps/core/status/SampleRejectedStatus.hpp"
#include "deps/core/status/SampleLostStatus.hpp"
#include "deps/core/status/StatusMask.hpp"
#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "sub/ReadCondition.hpp"
#include "sub/SampleInfoInner.hpp"
#include "deps/common/TypeSupport.hpp"
#include "deps/common/LocatorList.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "edds/rtps/attributes/ReaderAttributes.h"
#include "edds/rtps/common/Guid.h"
#include "history/IPayloadPool.h"
#include "history/IChangePool.h"
#include "history/ReaderListener.h"
#include "ertps/attributes/TopicAttributes.h"
#include "ertps/qos/LivelinessChangedStatus.h"
#include "history/ITopicPayloadPool.h"
#include "dispatcher/DispatcherParticipant.hpp"
#include "dispatcher/DispatcherReader.hpp"
#include "sub/DataReaderListenerInner.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "edds/dds/builtin/lookup/common/LookupTypes.hpp"
#include "sub/DataReaderImpl/DataReaderLoanManager.hpp"
#include "sub/DataReaderImpl/SampleInfoPool.hpp"
#include "sub/DataReaderImpl/SampleLoanManager.hpp"
#include "sub/DataReaderImpl/StateFilter.hpp"
#include "sub/Subscriber.hpp"
#include "sub/DataReaderHistory.hpp"
#include "vbs/status/StatusMask.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "topic/TopicImpl.hpp"
#include "core/Global.hpp"
#include "qos/DataReaderQosImpl.hpp"
#include "core/WriteParams.hpp"
#include "core/Guid.hpp"
#include "context/EnvManager.hpp"
#include "sub/SampleInfoImpl.hpp"

using evbs::ReturnCode_t;

namespace evbs {
namespace ertps {
namespace rtps {
using vbsutil::xmlparser::GUID_t;
}  // namespace rtps
}  // namespace ertps

namespace edds {
namespace dds {
class DataReaderListenerInner;

using SampleInfoSeq = vbs::LoanableSequence<vbs::SampleInfoInner>;
namespace detail {
class ReadConditionImpl;
}  // namespace detail
}  // namespace dds
}  // namespace edds

}  // namespace evbs
namespace vbs {
namespace detail {
struct ReadTakeCommand;
}  // namespace detail
namespace common {
class TimedEvent;
}  // namespace common

class TopicDescription;
class SampleInfoImpl;
class Subscriber;
class DataReader;
class DomainParticipantImpl;
class TopicImpl;
class DataReaderListener;
// class LoanableCollectionImpl;
class LoanableCollectionFlatDataImpl;
struct LivelinessChangedStatus;

// Not yet implemented
class QueryCondition;

// using TypeSupport = vbs::TypeSupport;

/**
 * Class DataReaderImpl, contains the actual implementation of the behaviour of the Subscriber.
 *
 *  @ingroup EDDS_MODULE
 */
class DataReaderImpl : public DomainEntity {
    friend class DataReader;
    friend struct vbs::detail::ReadTakeCommand;
    friend class vbs::detail::ReadConditionImpl;

 protected:
    using ITopicPayloadPool = vbs::common::ITopicPayloadPool;
    using IPayloadPool = vbs::common::IPayloadPool;
    using IChangePool = vbs::common::IChangePool;
    friend class Subscriber;

    /**
     * Create a data reader, assigning its pointer to the associated implementation.
     * Don't use directly, create DataReaderImpl using create_datareader from Subscriber.
     */
    DataReaderImpl(Subscriber* s, const TypeSupport& type, vbs::TopicDescription* topic,
                   const vbsutil::xmlparser::DataReaderQos& qos, DataReaderListenerInner* listener = nullptr,
                   const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

 public:
    /**
     * @brief Destructor.
     */
    virtual ~DataReaderImpl();

    /**
     * @brief This operation enables the DataReaderImpl.
     *
     * @return RETCODE_OK is successfully enabled. RETCODE_PRECONDITION_NOT_MET if the Subscriber
     * creating this DataReaderImpl is not enabled.
     */
    RTPS_DllAPI evbs::ReturnCode_t enable() override;

    /**
     * Method to check if a DataReaderImpl can be deleted
     * @param recursive == true if is used from delete_contained_entities otherwise
     * delete_datareader
     * @return true if can be deleted according to the standard rules
     */

    bool can_be_deleted(bool recursive = true) const;

    /**
     * Method to block the current thread until an unread message is available.
     *
     * @param [in] timeout Max blocking time for this operation.
     *
     * @return true if there is new unread message, false if timeout
     */
    RTPS_DllAPI bool wait_for_unread_message(const evbs::ertps::Duration_t& timeout);

    /** @name Read or take data methods.
     * Methods to read or take data from the History.
     */

    ///@{

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * - In the case where the @ref HistoryQosPolicy::kind is KEEP_LAST_HISTORY_QOS, the call will
     * return at most
     *   @ref HistoryQosPolicy::depth samples per instance.
     *
     * If the operation succeeds and the number of samples returned has been limited (by means of a
     * maximum limit, as listed above, or insufficient @ref SampleInfo resources), the call will
     * complete successfully and provide those samples the reader is able to return. The user may
     * need to make additional calls, or return outstanding loaned buffers in the case of
     * insufficient resources, in order to access remaining samples.
     *
     * In addition to the collection of samples, the read operation also uses a collection of @ref
     * SampleInfo structures (@c sample_infos).
     *
     * The initial (input) properties of the @c data_values and @c sample_infos collections will
     * determine the precise behavior of this operation. For the purposes of this description the
     * collections are modeled as having three properties:
     *
     * - the current length (@c len, see @ref LoanableCollection::length())
     *
     * - the maximum length (@c max_len, see @ref LoanableCollection::maximum())
     *
     * - whether the collection container owns the memory of the elements within
     *   (@c owns, see @ref LoanableCollection::has_ownership())
     *
     * The initial (input) values of the @c len, @c max_len, and @c owns properties for the @c
     * data_values and
     * @c sample_infos collections govern the behavior of the read operation as specified by the
     * following rules:
     *
     * 1. The values of @c len, @c max_len, and @c owns for the two collections must be identical.
     * Otherwise read will fail with RETCODE_PRECONDITION_NOT_MET.
     *
     * 2. On successful output, the values of @c len, @c max_len, and @c owns will be the same for
     * both collections.
     *
     * 3. If the input <tt> max_len == 0 </tt>, then the @c data_values and @c sample_infos
     * collections will be filled with elements that are 'loaned' by the DataReaderImpl. On output, @c
     * owns will be @c false, @c len will be set to the number of values returned, and @c max_len
     * will be set to a value verifying <tt> max_len >= len </tt>. The use of this variant allows
     * for zero-copy access to the data and the application will need to return the loan to the
     * DataReaderImpl using the @ref return_loan operation.
     *
     * 4. If the input <tt> max_len > 0 </tt> and the input <tt> owns == false </tt>, then the read
     * operation will fail with RETCODE_PRECONDITION_NOT_MET. This avoids the potential
     * hard-to-detect memory leaks caused by an application forgetting to return the loan.
     *
     * 5. If input <tt> max_len > 0 </tt> and the input <tt> owns == true </tt>, then the read
     * operation will copy the Data values and SampleInfo values into the elements already inside
     * the collections. On output, @c owns will be @c true, @c len will be set to the number of
     * values copied, and @c max_len will remain unchanged. The use of this variant forces a copy
     * but the application can control where the copy is placed and the application will not need to
     * return the loan. The number of samples copied depends on the values of
     *    @c max_len and @c max_samples:
     *
     *    - If <tt> max_samples == LENGTH_UNLIMITED </tt>, then at most @c max_len values will be
     * copied. The use of this variant lets the application limit the number of samples returned to
     * what the sequence can accommodate.
     *
     *    - If <tt> max_samples <= max_len </tt>, then at most @c max_samples values will be copied.
     * The use of this variant lets the application limit the number of samples returned to fewer
     * that what the sequence can accommodate.
     *
     *    - If <tt> max_samples > max_len </tt>, then the read operation will fail with
     * RETCODE_PRECONDITION_NOT_MET. This avoids the potential confusion where the application
     * expects to be able to access up to
     *      @c max_samples, but that number can never be returned, even if they are available in the
     * DataReaderImpl, because the output sequence cannot accommodate them.
     *
     * As described above, upon return the @c data_values and @c sample_infos collections may
     * contain elements 'loaned' from the DataReaderImpl. If this is the case, the application will need
     * to use the @ref return_loan operation to return the loan once it is no longer using the Data
     * in the collection. Upon return from
     * @ref return_loan, the collection will have <tt> max_len == 0 </tt> and <tt> owns == false
     * </tt>.
     *
     * The application can determine whether it is necessary to return the loan or not based on the
     * state of the collections when the read operation was called, or by accessing the @c owns
     * property. However, in many cases it may be simpler to always call @ref return_loan, as this
     * operation is harmless (i.e., leaves all elements unchanged) if the collection does not have a
     * loan.
     *
     * On output, the collection of Data values and the collection of SampleInfo structures are of
     * the same length and are in a one-to-one correspondence. Each SampleInfo provides information,
     * such as the @c source_timestamp, the @c sample_state, @c view_state, and @c instance_state,
     * etc., about the corresponding sample.
     *
     * Some elements in the returned collection may not have valid data. If the @c instance_state in
     * the SampleInfo is
     * @ref vbs::NOT_ALIVE_DISPOSED_INSTANCE_STATE or @ref vbs::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, then the
     * last sample for that instance in the collection, that is, the one whose SampleInfo has <tt>
     * sample_rank == 0 </tt> does not contain valid data. Samples that contain no data do not count
     * towards the limits imposed by the
     * @ref ResourceLimitsQosPolicy.
     *
     * The act of reading a sample changes its @c sample_state to @ref vbs::READ_SAMPLE_STATE. If the
     * sample belongs to the most recent generation of the instance, it will also set the @c
     * view_state of the instance to be
     * @ref vbs::NOT_NEW_VIEW_STATE. It will not affect the @c instance_state of the instance.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @em Important: If the samples "returned" by this method are loaned from the middleware (see
     * @ref take for more information on memory loaning), it is important that their contents not be
     * changed. Because the memory in which the data is stored belongs to the middleware, any
     * modifications made to the data will be seen the next time the same samples are read or taken;
     * the samples will no longer reflect the state that was received from the network.
     *
     * @param [in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param [in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param [in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                 @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described above.
     * @param [in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param [in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param [in]     instance_states Only data samples with @c instance_state matching one of
     * these will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read(evbs::edds::dds::LoanableCollection& data_values,
                                  evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples = LENGTH_UNLIMITED,
                                  vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                  vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                  vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    /**
     * NOT YET IMPLEMENTED
     *
     * This operation accesses via ‘read’ the samples that match the criteria specified in the
     * ReadCondition. This operation is especially useful in combination with QueryCondition to
     * filter data samples based on the content.
     *
     * The specified ReadCondition must be attached to the DataReaderImpl; otherwise the operation will
     * fail and return RETCODE_PRECONDITION_NOT_MET.
     *
     * In case the ReadCondition is a ‘plain’ ReadCondition and not the specialized QueryCondition,
     * the operation is equivalent to calling read and passing as @c sample_states, @c view_states
     * and @c instance_states the value of the corresponding attributes in @c a_condition. Using
     * this operation the application can avoid repeating the same parameters specified when
     * creating the ReadCondition.
     *
     * The samples are accessed with the same semantics as the read operation. If the DataReaderImpl has
     * no samples that meet the constraints, the return value will be RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned.
     * @param[in]     a_condition     A ReadCondition that returned @c data_values must pass
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                              vbs::ReadCondition* a_condition);

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl. The behavior is
     * identical to
     * @ref read, except that all samples returned belong to the single specified instance whose
     * handle is
     * @c a_handle.
     *
     * Upon successful completion, the data collection will contain samples all belonging to the
     * same instance. The corresponding @ref SampleInfo verifies @ref SampleInfo::instance_handle ==
     * @c a_handle.
     *
     * This operation is semantically equivalent to the @ref read operation, except in building the
     * collection. The DataReaderImpl will check that the sample belongs to the specified instance and
     * otherwise it will not place the sample in the returned collection.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param [in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param [in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param [in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                 @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param [in]     a_handle        The specified instance to return samples for. The method will
     * fail with RETCODE_BAD_PARAMETER if the handle does not correspond to an existing data-object
     * known to the DataReaderImpl.
     * @param [in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param [in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param [in]     instance_states Only data samples with @c instance_state matching one of
     * these will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read_instance(evbs::edds::dds::LoanableCollection& data_values,
                                           evbs::edds::dds::SampleInfoSeq& sample_infos,
                                           int32_t max_samples = LENGTH_UNLIMITED,
                                           const evbs::edds::dds::InstanceHandle_t& a_handle = HANDLE_NIL,
                                           vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                           vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                           vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl where all the samples
     * belong to a single instance. The behavior is similar to @ref read_instance, except that the
     * actual instance is not directly specified. Rather, the samples will all belong to the 'next'
     * instance with @c instance_handle 'greater' than the specified 'previous_handle' that has
     * available samples.
     *
     * This operation implies the existence of a total order 'greater-than' relationship between the
     * instance handles. The specifics of this relationship are not all important and are
     * implementation specific. The important thing is that, according to the middleware, all
     * instances are ordered relative to each other. This ordering is between the instance handles,
     * and should not depend on the state of the instance (e.g. whether it has data or not) and must
     * be defined even for instance handles that do not correspond to instances currently managed by
     * the DataReaderImpl. For the purposes of the ordering, it should be 'as if' each instance handle
     * was represented as an integer.
     *
     * The behavior of this operation is 'as if' the DataReaderImpl invoked @ref read_instance, passing
     * the smallest
     * @c instance_handle among all the ones that: (a) are greater than @c previous_handle, and (b)
     * have available samples (i.e. samples that meet the constraints imposed by the specified
     * states).
     *
     * The special value @ref HANDLE_NIL is guaranteed to be 'less than' any valid @c
     * instance_handle. So the use of the parameter value @c previous_handle == @ref HANDLE_NIL will
     * return the samples for the instance which has the smallest @c instance_handle among all the
     * instances that contain available samples.
     *
     * This operation is intended to be used in an application-driven iteration, where the
     * application starts by passing @c previous_handle == @ref HANDLE_NIL, examines the samples
     * returned, and then uses the
     * @c instance_handle returned in the @ref SampleInfo as the value of the @c previous_handle
     * argument to the next call to @ref read_next_instance. The iteration continues until @ref
     * read_next_instance fails with RETCODE_NO_DATA.
     *
     * Note that it is possible to call the @ref read_next_instance operation with a @c
     * previous_handle that does not correspond to an instance currently managed by the DataReaderImpl.
     * This is because as stated earlier the 'greater-than' relationship is defined even for handles
     * not managed by the DataReaderImpl. One practical situation where this may occur is when an
     * application is iterating through all the instances, takes all the samples of a
     * @ref vbs::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE instance, returns the loan (at which point the
     * instance information may be removed, and thus the handle becomes invalid), and tries to read
     * the next instance.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param [in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param [in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param [in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                 @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param [in]     previous_handle The 'next smallest' instance with a value greater than this
     * value that has available samples will be returned.
     * @param [in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param [in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param [in]     instance_states Only data samples with @c instance_state matching one of
     * these will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t
    read_next_instance(evbs::edds::dds::LoanableCollection& data_values, evbs::edds::dds::SampleInfoSeq& sample_infos,
                       int32_t max_samples = LENGTH_UNLIMITED,
                       const evbs::edds::dds::InstanceHandle_t& previous_handle = vbs::HANDLE_NIL,
                       vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                       vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                       vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);
    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl. The behavior is
     * identical to
     * @ref read, except that all samples returned belong to the single specified instance whose
     * handle is
     * @c a_handle.
     *
     * Upon successful completion, the data collection will contain samples all belonging to the
     * same instance. The corresponding @ref SampleInfo verifies @ref SampleInfo::instance_handle ==
     * @c a_handle.
     *
     * This operation is semantically equivalent to the @ref read operation, except in building the
     * collection. The DataReaderImpl will check that the sample belongs to the specified instance and
     * otherwise it will not place the sample in the returned collection.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param [in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param [in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param [in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                 @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param [in]     s_handle        The instances to return samples for. The method will
     * fail with RETCODE_BAD_PARAMETER if the handle does not correspond to an existing data-object
     * known to the DataReaderImpl.
     * @param [in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param [in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param [in]     instance_states Only data samples with @c instance_state matching one of
     * these will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read_instances(evbs::edds::dds::LoanableCollection& data_values,
                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                            int32_t max_samples = LENGTH_UNLIMITED,
                                            const std::vector<InstanceHandle_t>& s_handle = {vbs::HANDLE_NIL},
                                            vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                            vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                            vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    /**
     * NOT YET IMPLEMENTED
     *
     * This operation accesses a collection of Data values from the DataReaderImpl. The behavior is
     * identical to
     * @ref read_next_instance except that all samples returned satisfy the specified condition. In
     * other words, on success all returned samples belong to the same instance, and the instance is
     * the instance with ‘smallest’ @c instance_handle among the ones that verify (a) @c
     * instance_handle >= @c previous_handle and (b) have samples for which the specified
     * ReadCondition evaluates to TRUE.
     *
     * Similar to the operation @ref read_next_instance it is possible to call
     * @ref read_next_instance_w_condition with a @c previous_handle that does not correspond to an
     * instance currently managed by the DataReaderImpl.
     *
     * The behavior of the @ref read_next_instance_w_condition operation follows the same rules than
     * the read operation regarding the pre-conditions and post-conditions for the @c data_values
     * and @c sample_infos collections. Similar to read, the @ref read_next_instance_w_condition
     * operation may ‘loan’ elements to the output collections which must then be returned by means
     * of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the return value will be
     * RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param[in]     previous_handle The 'next smallest' instance with a value greater than this
     * value that has available samples will be returned.
     * @param[in]     a_condition     A ReadCondition that returned @c data_values must pass
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read_next_instance_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                            int32_t max_samples,
                                                            const InstanceHandle_t& previous_handle,
                                                            vbs::ReadCondition* a_condition);

    /**
     * @brief This operation copies the next, non-previously accessed Data value from the
     * DataReaderImpl; the operation also copies the corresponding SampleInfo. The implied order among
     * the samples stored in the DataReaderImpl is the same as for the read operation.
     *
     * The read_next_sample operation is semantically equivalent to the read operation where the
     * input Data sequence has <tt> max_length = 1 </tt>, the <tt> sample_states =
     * vbs::NOT_READ_SAMPLE_STATE </tt>, the <tt> view_states = vbs::ANY_VIEW_STATE </tt>, and the <tt>
     * instance_states = vbs::ANY_INSTANCE_STATE </tt>.
     *
     * The read_next_sample operation provides a simplified API to ‘read’ samples avoiding the need
     * for the application to manage sequences and specify states.
     *
     * If there is no unread data in the DataReaderImpl, the operation will return RETCODE_NO_DATA and
     * nothing is copied
     *
     * @param [out] data Data pointer to store the sample
     * @param [out] info SampleInfo pointer to store the sample information
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t read_next_sample(void* data, vbs::SampleInfoInner* info);

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data-samples from the DataReaderImpl and a corresponding
     * collection of SampleInfo structures, and 'removes' them from the DataReaderImpl. The operation
     * will return either a 'list' of samples or else a single sample. This is controlled by the
     * PresentationQosPolicy using the same logic as for the @ref read operation.
     *
     * The act of taking a sample removes it from the DataReaderImpl so it cannot be 'read' or 'taken'
     * again. If the sample belongs to the most recent generation of the instance, it will also set
     * the @c view_state of the instance to NOT_NEW. It will not affect the @c instance_state of the
     * instance.
     *
     * The behavior of the take operation follows the same rules than the @ref read operation
     * regarding the pre-conditions and post-conditions for the @c data_values and @c sample_infos
     * collections. Similar to
     * @ref read, the take operation may 'loan' elements to the output collections which must then
     * be returned by means of @ref return_loan. The only difference with @ref read is that, as
     * stated, the samples returned by take will no longer be accessible to successive calls to read
     * or take.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param [in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param [in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param [in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                 @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param [in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param [in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param [in]     instance_states Only data samples with @c instance_state matching one of
     * these will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take(evbs::edds::dds::LoanableCollection& data_values,
                                  evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples = LENGTH_UNLIMITED,
                                  vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                  vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                  vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    RTPS_DllAPI ReturnCode_t take_buffer(evbs::edds::dds::LoanableCollection& data_values,
                                         evbs::edds::dds::SampleInfoSeq& sample_infos,
                                         int32_t max_samples = LENGTH_UNLIMITED,
                                         vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                         vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                         vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    /**
     * NOT YET IMPLEMENTED
     *
     * This operation is analogous to @ref read_w_condition except it accesses samples via the
     * ‘take’ operation.
     *
     * The specified ReadCondition must be attached to the DataReaderImpl; otherwise the operation will
     * fail and return RETCODE_PRECONDITION_NOT_MET.
     *
     * The samples are accessed with the same semantics as the @ref take operation.
     *
     * This operation is especially useful in combination with QueryCondition to filter data samples
     * based on the content.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the return value will be
     * RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are.
     * @param[in]     a_condition     A ReadCondition that returned @c data_values must pass
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                              vbs::ReadCondition* a_condition);

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl and 'removes' them
     * from the DataReaderImpl.
     *
     * This operation has the same behavior as @ref read_instance, except that the samples are
     * 'taken' from the DataReaderImpl such that they are no longer accessible via subsequent 'read' or
     * 'take' operations.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param[in]     a_handle        The specified instance to return samples for. The method will
     * fail with RETCODE_BAD_PARAMETER if the handle does not correspond to an existing data-object
     * known to the DataReaderImpl.
     * @param[in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param[in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param[in]     instance_states Only data samples with @c instance_state matching one of these
     * will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_instance(evbs::edds::dds::LoanableCollection& data_values,
                                           evbs::edds::dds::SampleInfoSeq& sample_infos,
                                           int32_t max_samples = LENGTH_UNLIMITED,
                                           const InstanceHandle_t& a_handle = vbs::HANDLE_NIL,
                                           vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                           vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                           vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);

    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl and 'removes' them
     * from the DataReaderImpl.
     *
     * This operation has the same behavior as @ref read_next_instance, except that the samples are
     * 'taken' from the DataReaderImpl such that they are no longer accessible via subsequent 'read' or
     * 'take' operations.
     *
     * Similar to the operation @ref read_next_instance, it is possible to call this operation with
     * a
     * @c previous_handle that does not correspond to an instance currently managed by the
     * DataReaderImpl.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param[in]     previous_handle The 'next smallest' instance with a value greater than this
     * value that has available samples will be returned.
     * @param[in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param[in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param[in]     instance_states Only data samples with @c instance_state matching one of these
     * will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_next_instance(evbs::edds::dds::LoanableCollection& data_values,
                                                evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                int32_t max_samples = LENGTH_UNLIMITED,
                                                const InstanceHandle_t& previous_handle = vbs::HANDLE_NIL,
                                                vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                                vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                                vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);
    /**
     * Access a collection of data samples from the DataReaderImpl.
     *
     * This operation accesses a collection of data values from the DataReaderImpl and 'removes' them
     * from the DataReaderImpl.
     *
     * This operation has the same behavior as @ref read_instance, except that the samples are
     * 'taken' from the DataReaderImpl such that they are no longer accessible via subsequent 'read' or
     * 'take' operations.
     *
     * The behavior of this operation follows the same rules as the @ref read operation regarding
     * the pre-conditions and post-conditions for the @c data_values and @c sample_infos. Similar to
     * @ref read, this operation may 'loan' elements to the output collections, which must then be
     * returned by means of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the operations fails with
     * RETCODE_NO_DATA.
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param[in]     s_handle        The instances to return samples for. The method will
     * fail with RETCODE_BAD_PARAMETER if the handle does not correspond to an existing data-object
     * known to the DataReaderImpl.
     * @param[in]     sample_states   Only data samples with @c sample_state matching one of these
     * will be returned.
     * @param[in]     view_states     Only data samples with @c view_state matching one of these
     * will be returned.
     * @param[in]     instance_states Only data samples with @c instance_state matching one of these
     * will be returned.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_instances(evbs::edds::dds::LoanableCollection& data_values,
                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                            int32_t max_samples = LENGTH_UNLIMITED,
                                            const std::vector<InstanceHandle_t>& s_handle = {vbs::HANDLE_NIL},
                                            vbs::SampleStateMask sample_states = vbs::ANY_SAMPLE_STATE,
                                            vbs::ViewStateMask view_states = vbs::ANY_VIEW_STATE,
                                            vbs::InstanceStateMask instance_states = vbs::ANY_INSTANCE_STATE);
    /**
     * NOT YET IMPLEMENTED
     *
     * This operation accesses a collection of Data values from the DataReaderImpl. The behavior is
     * identical to
     * @ref read_next_instance except that all samples returned satisfy the specified condition. In
     * other words, on success all returned samples belong to the same instance, and the instance is
     * the instance with ‘smallest’
     * @c instance_handle among the ones that verify (a) @c instance_handle >= @c previous_handle
     * and (b) have samples for which the specified ReadCondition evaluates to TRUE.
     *
     * Similar to the operation @ref read_next_instance it is possible to call @ref
     * read_next_instance_w_condition with a @c previous_handle that does not correspond to an
     * instance currently managed by the DataReaderImpl.
     *
     * The behavior of the @ref read_next_instance_w_condition operation follows the same rules than
     * the read operation regarding the pre-conditions and post-conditions for the @c data_values
     * and @c sample_infos collections. Similar to read, the @ref read_next_instance_w_condition
     * operation may ‘loan’ elements to the output collections which must then be returned by means
     * of @ref return_loan.
     *
     * If the DataReaderImpl has no samples that meet the constraints, the return value will be
     * RETCODE_NO_DATA
     *
     * @param[in,out] data_values     A LoanableCollection object where the received data samples
     * will be returned.
     * @param[in,out] sample_infos    A SampleInfoSeq object where the received sample info will be
     * returned.
     * @param[in]     max_samples     The maximum number of samples to be returned. If the special
     * value
     *                                @ref LENGTH_UNLIMITED is provided, as many samples will be
     * returned as are available, up to the limits described in the documentation for @ref read().
     * @param[in]     previous_handle The 'next smallest' instance with a value greater than this
     * value that has available samples will be returned.
     * @param[in]     a_condition     A ReadCondition that returned @c data_values must pass
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_next_instance_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                            int32_t max_samples,
                                                            const InstanceHandle_t& previous_handle,
                                                            vbs::ReadCondition* a_condition);

    /**
     * @brief This operation copies the next, non-previously accessed Data value from the DataReaderImpl
     * and ‘removes’ it from the DataReaderImpl so it is no longer accessible. The operation also copies
     * the corresponding SampleInfo.
     *
     * This operation is analogous to @ref read_next_sample except for the fact that the sample is
     * ‘removed’ from the DataReaderImpl.
     *
     * This operation is semantically equivalent to the @ref take operation where the input sequence
     * has <tt> max_length = 1 </tt>, the <tt> sample_states = vbs::NOT_READ_SAMPLE_STATE </tt>, the <tt>
     * view_states = vbs::ANY_VIEW_STATE </tt>, and the <tt> instance_states = vbs::ANY_INSTANCE_STATE </tt>.
     *
     * This operation provides a simplified API to ’take’ samples avoiding the need for the
     * application to manage sequences and specify states.
     *
     * If there is no unread data in the DataReaderImpl, the operation will return RETCODE_NO_DATA and
     * nothing is copied.
     *
     * @param [out] data Data pointer to store the sample
     * @param [out] info SampleInfo pointer to store the sample information
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t take_next_sample(void* data, vbs::SampleInfoInner* info);

    ///@}

    /**
     * This operation indicates to the DataReaderImpl that the application is done accessing the
     * collection of
     * @c data_values and @c sample_infos obtained by some earlier invocation of @ref read or @ref
     * take on the DataReaderImpl.
     *
     * The @c data_values and @c sample_infos must belong to a single related ‘pair’; that is, they
     * should correspond to a pair returned from a single call to read or take. The @c data_values
     * and @c sample_infos must also have been obtained from the same DataReaderImpl to which they are
     * returned. If either of these conditions is not met, the operation will fail and return
     * RETCODE_PRECONDITION_NOT_MET.
     *
     * This operation allows implementations of the @ref read and @ref take operations to "loan"
     * buffers from the DataReaderImpl to the application and in this manner provide "zero-copy" access
     * to the data. During the loan, the DataReaderImpl will guarantee that the data and
     * sample-information are not modified.
     *
     * It is not necessary for an application to return the loans immediately after the read or take
     * calls. However, as these buffers correspond to internal resources inside the DataReaderImpl, the
     * application should not retain them indefinitely.
     *
     * The use of the @ref return_loan operation is only necessary if the read or take calls
     * "loaned" buffers to the application. This only occurs if the @c data_values and @c
     * sample_infos collections had <tt> max_len == 0 </tt> at the time read or take was called. The
     * application may also examine the @c owns property of the collection to determine if there is
     * an outstanding loan. However, calling @ref return_loan on a collection that does not have a
     * loan is safe and has no side effects.
     *
     * If the collections had a loan, upon return from return_loan the collections will have <tt>
     * max_len == 0 </tt>.
     *
     * @param [in,out] data_values   A LoanableCollection object where the received data samples
     * were obtained from an earlier invocation of read or take on this DataReaderImpl.
     * @param [in,out] sample_infos  A SampleInfoSeq object where the received sample infos were
     * obtained from an earlier invocation of read or take on this DataReaderImpl.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t return_loan(evbs::edds::dds::LoanableCollection& data_values,
                                         evbs::edds::dds::SampleInfoSeq& sample_infos);

    /**
     * Takes as a parameter an instance and returns a handle that can be used in subsequent
     * operations that accept an instance handle as an argument. The instance parameter is only used
     * for the purpose of examining the fields that define the key.
     *
     * @param [in] instance Data pointer to the sample
     *
     * @return handle of the given @c instance.
     * @return HANDLE_NIL if @c instance is nullptr.
     * @return HANDLE_NIL if there is no instance on the DataReaderImpl's history with the same key as
     * @c instance.
     */
    RTPS_DllAPI InstanceHandle_t lookup_instance(const void* instance) const;

    /**
     * @brief Returns information about the first untaken sample.
     *
     * @param [out] info Pointer to a SampleInfo_t structure to store first untaken sample
     * information.
     *
     * @return RETCODE_OK if sample info was returned. RETCODE_NO_DATA if there is no sample to
     * take.
     */
    RTPS_DllAPI ReturnCode_t get_first_untaken_info(vbs::SampleInfoInner* info);

    /**
     * Get the number of samples pending to be read.
     * The number includes samples that may not yet be available to be read or taken by the user,
     * due to samples being received out of order.
     *
     * @return the number of samples on the reader history that have never been read.
     */
    RTPS_DllAPI uint64_t get_unread_count();

    /**
     * Get the number of samples pending to be read.
     *
     * @param mark_as_read  Whether the unread samples should be marked as read or not.
     *
     * @return the number of samples on the reader history that have never been read.
     */
    RTPS_DllAPI uint64_t get_unread_count(bool mark_as_read);

    /**
     * Get associated GUID.
     *
     * @return Associated GUID
     */
    RTPS_DllAPI const evbs::ertps::rtps::GUID_t& guid();

    /**
     * Get associated GUID.
     *
     * @return Associated GUID
     */
    RTPS_DllAPI const evbs::ertps::rtps::GUID_t& guid() const;

    /**
     * @brief Getter for the associated InstanceHandle.
     *
     * @return Copy of the InstanceHandle
     */
    RTPS_DllAPI InstanceHandle_t get_instance_handle() const;

    /**
     * Getter for the data type.
     *
     * @return TypeSupport associated to the DataReaderImpl.
     */
    RTPS_DllAPI TypeSupport type();

    /**
     * Get TopicDescription.
     *
     * @return TopicDescription pointer.
     */
    RTPS_DllAPI const vbs::TopicDescription* get_topicdescription() const;

    /**
     * @brief Get the requested deadline missed status.
     *
     * @return The deadline missed status.
     */
    RTPS_DllAPI ReturnCode_t
    get_requested_deadline_missed_status(evbs::edds::dds::RequestedDeadlineMissedStatus& status);

    /**
     * @brief Get the requested incompatible qos status.
     *
     * @param [out] status Requested incompatible qos status.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t
    get_requested_incompatible_qos_status(evbs::edds::dds::RequestedIncompatibleQosStatus& status);

    /**
     * @brief Setter for the DataReaderQos.
     *
     * @param [in] qos new value for the DataReaderQos.
     *
     * @return RETCODE_IMMUTABLE_POLICY if any of the Qos cannot be changed,
     * RETCODE_INCONSISTENT_POLICY if the Qos is not self consistent and RETCODE_OK if the qos is
     * changed correctly.
     */
    RTPS_DllAPI ReturnCode_t set_qos(const vbsutil::xmlparser::DataReaderQos& qos);

    /**
     * @brief Getter for the DataReaderQos.
     *
     * @return Pointer to the DataReaderQos.
     */
    RTPS_DllAPI const vbsutil::xmlparser::DataReaderQos& get_dds_qos() const;

    /**
     * @brief Getter for the DataReaderQos.
     *
     * @param [in] qos DataReaderQos where the qos is returned.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_qos(vbsutil::xmlparser::DataReaderQos& qos) const;

    /**
     * Modifies the DataReaderListenerInner, sets the mask to StatusMask::all().
     *
     * @param [in] listener new value for the DataReaderListenerInner.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DataReaderListenerInner* listener);

    /**
     * Modifies the DataReaderListenerInner.
     *
     * @param [in] listener new value for the DataReaderListenerInner.
     * @param [in] mask StatusMask that holds statuses the listener responds to (default: all).
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t set_listener(DataReaderListenerInner* listener, const evbs::edds::dds::StatusMask& mask);
    /**
     * @brief Getter for the DataReaderListenerInner
     *
     * @return Pointer to the DataReaderListenerInner
     */
    RTPS_DllAPI const DataReaderListenerInner* get_listener() const;

    /* TODO
       RTPS_DllAPI bool get_key_value(
            void* data,
            const InstanceHandle_t& handle);
     */

    /**
     * @brief Get the liveliness changed status.
     *
     * @param [out] status LivelinessChangedStatus object where the status is returned.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_liveliness_changed_status(evbs::edds::dds::LivelinessChangedStatus& status);

    /**
     * @brief Get the SAMPLE_LOST communication status
     *
     * @param[out] status SampleLostStatus object where the status is returned.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_sample_lost_status(evbs::edds::dds::SampleLostStatus& status);

    /**
     * @brief Get the SAMPLE_REJECTED communication status
     *
     * @param[out] status SampleRejectedStatus object where the status is returned.
     *
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_sample_rejected_status(evbs::edds::dds::SampleRejectedStatus& status);

    /**
     * @brief Returns the subscription matched status
     *
     * @param[out] status subscription matched status struct
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t get_subscription_matched_status(evbs::edds::dds::SubscriptionMatchedStatus& status);

    /**
     * @brief This operation creates a ReadCondition. The returned ReadCondition will be attached
     * and belong to the DataReaderImpl.
     *
     * @param [in] sample_states   Only data samples with @c sample_state matching one of these will
     * trigger the created condition.
     * @param [in] view_states     Only data samples with @c view_state matching one of these will
     * trigger the created condition.
     * @param [in] instance_states Only data samples with @c instance_state matching one of these
     * will trigger the created condition.
     *
     * @return pointer to the created ReadCondition, nullptr in case of error.
     */
    RTPS_DllAPI ReadCondition* create_readcondition(vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                                    vbs::InstanceStateMask instance_states);

    /**
     * @brief This operation deletes a ReadCondition attached to the DataReaderImpl.
     *
     * @param a_condition pointer to a ReadCondition belonging to the DataReaderImpl
     * @return RETCODE_OK
     */
    RTPS_DllAPI ReturnCode_t delete_readcondition(vbs::ReadCondition* a_condition);

    /**
     * @brief Getter for the Subscriber
     * @return Subscriber pointer
     */
    RTPS_DllAPI const Subscriber* get_subscriber() const;

    /**
     * This operation deletes all the entities that were created by means of the “create” operations
     * on the DataReaderImpl. That is, it deletes all contained ReadCondition and QueryCondition
     * objects.
     *
     * The operation will return PRECONDITION_NOT_MET if the any of the contained entities is in a
     * state where it cannot be deleted.
     *
     * @return Any of the standard return codes.
     */
    RTPS_DllAPI ReturnCode_t delete_contained_entities();

    /**
     * Checks whether a loaned sample is still valid or is corrupted.
     * Calling this method on a sample which has not been loaned, or one for which the loan has been
     * returned yields undefined behavior.
     *
     * @param data Pointer to the sample data to check
     * @param info Pointer to the SampleInfo related to \c data
     *
     * @return true if the sample is valid
     */
    RTPS_DllAPI bool is_sample_valid(const void* data, const vbs::SampleInfoInner* info) const;

    /**
     * Get the list of locators on which this DataReaderImpl is listening.
     *
     * @param [out] locators  LocatorList where the list of locators will be stored.
     *
     * @return NOT_ENABLED if the reader has not been enabled.
     * @return OK if a list of locators is returned.
     */
    RTPS_DllAPI ReturnCode_t get_listening_locators(vbsutil::xmlparser::LocatorList& locators) const;

    void get_statistic_recv_info(evbs::edds::dds::builtin::StatisticRecvInfo* info);

    void filter_has_been_updated();

 private:
    /** @name Read or take data methods.
     * Methods to read or take data from the History.
     */

    ReturnCode_t read_or_take(evbs::edds::dds::LoanableCollection& data_values,
                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                              const InstanceHandle_t& handle, vbs::SampleStateMask sample_states,
                              vbs::ViewStateMask view_states, vbs::InstanceStateMask instance_states,
                              bool exact_instance, bool single_instance, bool should_take, bool raw_buffer);

    ReturnCode_t read_or_take_next_sample(void* data, vbs::SampleInfoInner* info, bool should_take);

    ///@{

#define READ(data_values, sample_infos, max_samples, sample_states, view_states, instance_states)                      \
    read_or_take(data_values, sample_infos, max_samples, vbs::HANDLE_NIL, sample_states, view_states, instance_states, \
                 false, false, false, false)

#define READ_INSTANCE(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states)  \
    read_or_take(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states, true, \
                 true, false, false);

#define READ_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states,        \
                           instance_states)                                                                            \
    read_or_take(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states, \
                 false, true, false, false);

#define TAKE(data_values, sample_infos, max_samples, sample_states, view_states, instance_states)                      \
    read_or_take(data_values, sample_infos, max_samples, vbs::HANDLE_NIL, sample_states, view_states, instance_states, \
                 false, false, true, false);

#define TAKE_BUFFER(data_values, sample_infos, max_samples, sample_states, view_states, instance_states)               \
    read_or_take(data_values, sample_infos, max_samples, vbs::HANDLE_NIL, sample_states, view_states, instance_states, \
                 false, false, true, true);

#define TAKE_INSTANCE(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states)  \
    read_or_take(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states, true, \
                 true, true, false);

#define TAKE_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states,        \
                           instance_states)                                                                            \
    read_or_take(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states, \
                 false, true, true, false);

#define READ_NEXT_SAMPLE(data, info) read_or_take_next_sample(data, info, false)
#define TAKE_NEXT_SAMPLE(data, info) read_or_take_next_sample(data, info, true)

    void disable();

    /* Extends the check_qos() call, including the check for
     * resource limits policy.
     * @param qos Pointer to the qos to be checked.
     * @param type Pointer to the associated TypeSupport object.
     * @return True if correct.
     */
    static ReturnCode_t check_qos_including_resource_limits(const vbsutil::xmlparser::DataReaderQos& qos,
                                                            const TypeSupport& type);

    /* Check whether values in the DataReaderQos are compatible among them or not
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static ReturnCode_t check_qos(const vbsutil::xmlparser::DataReaderQos& qos);

    /* Checks resource limits policy: Instance allocation consistency
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static ReturnCode_t check_allocation_consistency(const vbsutil::xmlparser::DataReaderQos& qos);

    /* Check whether the DataReaderQos can be updated with the values provided. This method DOES NOT
     * update anything.
     * @param to Reference to the qos instance to be changed.
     * @param from Reference to the qos instance with the new values.
     * @return True if they can be updated.
     */
    static bool can_qos_be_updated(const vbsutil::xmlparser::DataReaderQos& to,
                                   const vbsutil::xmlparser::DataReaderQos& from);

    /* Update a DataReaderQos with new values
     * @param to Reference to the qos instance to be changed.
     * @param from Reference to the qos instance with the new values.
     * @param first_time Boolean indicating whether is the first time (If not some parameters cannot
     * be set).
     */
    static void set_qos(vbsutil::xmlparser::DataReaderQos& to, const vbsutil::xmlparser::DataReaderQos& from,
                        bool first_time);

    const vbs::detail::StateFilter& get_last_mask_state() const;

    void try_notify_read_conditions() noexcept;

    std::recursive_mutex& get_conditions_mutex() const noexcept;

 protected:
    //! Subscriber
    Subscriber* subscriber_ = nullptr;

    //! Pointer to associated DispatcherReader
    vbs::dispatcher::DispatcherReader* reader_ = nullptr;

    //! Pointer to the TopicDataType object.
    TypeSupport type_;

    vbs::TopicDescription* topic_ = nullptr;

    vbsutil::xmlparser::DataReaderQos qos_;

    //! History
    vbs::detail::DataReaderHistory* history_ = nullptr;

    //! Listener
    DataReaderListenerInner* listener_ = nullptr;
    mutable std::mutex listener_mutex_;

    evbs::ertps::rtps::GUID_t guid_;

    class InnerDataReaderListener : public vbs::common::ReaderListener {
     public:
        InnerDataReaderListener(DataReaderImpl* s) : data_reader_(s) {}

        ~InnerDataReaderListener() override {}

        void onReaderMatched(const vbs::common::MatchingInfo& info) override;

        void on_data_available(const evbs::ertps::rtps::GUID_t& writer_guid,
                               const vbs::common::SequenceNumber_t& first_sequence,
                               const vbs::common::SequenceNumber_t& last_sequence,
                               bool& should_notify_individual_changes) override;

        void on_liveliness_changed(const evbs::ertps::LivelinessChangedStatus& status) override;

        void on_requested_incompatible_qos(evbs::edds::dds::PolicyMask qos) override;
        void on_sample_lost(const int32_t sample_lost_since_last_update,
                            const evbs::edds::dds::SampleLostStatusKind statusKind) override;

        void on_sample_rejected(evbs::edds::dds::SampleRejectedStatusKind reason,
                                const vbs::common::CacheChange_t* const change) override;

        void on_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) override;
        void on_endpoint_crash(const evbs::ertps::rtps::GUID_t& writer_guid) override;
        DataReaderImpl* data_reader_;
    } inner_listener_;

    //! A timer used to check for deadlines
    vbs::common::TimedEvent* deadline_timer_ = nullptr;

    //! Deadline duration in microseconds
    std::chrono::duration<double_t, std::ratio<1, 1000000>> deadline_duration_us_ {0.0};

    //! The current timer owner, i.e. the instance which started the deadline timer
    evbs::ertps::rtps::InstanceHandle_t timer_owner_;

    //! Subscription matched status
    evbs::edds::dds::SubscriptionMatchedStatus subscription_matched_status_;

    //! Liveliness changed status
    evbs::edds::dds::LivelinessChangedStatus liveliness_changed_status_;

    //! Requested deadline missed status
    evbs::ertps::RequestedDeadlineMissedStatus deadline_missed_status_;

    //! Requested incompatible QoS status
    evbs::edds::dds::RequestedIncompatibleQosStatus requested_incompatible_qos_status_;

    //! Sample lost status
    evbs::edds::dds::SampleLostStatus sample_lost_status_;
    //! Sample rejected status
    evbs::edds::dds::SampleRejectedStatus sample_rejected_status_;

    //! A timed callback to remove expired samples
    vbs::common::TimedEvent* lifespan_timer_ = nullptr;

    //! The lifespan duration
    std::chrono::duration<double_t, std::ratio<1, 1000000>> lifespan_duration_us_ {0.0};

    //std::shared_ptr<ITopicPayloadPool> payload_pool_;
    std::shared_ptr<vbs::detail::SampleLoanManager> sample_pool_;

    vbs::detail::SampleInfoPool* sample_info_pool_ = nullptr;
    vbs::detail::DataReaderLoanManager* loan_manager_ = nullptr;

    /**
     * Mutex to protect ReadCondition collection
     * is required because the RTPSReader mutex is only available when the object is enabled
     * @note use get_conditions_mutex() instead of directly referencing it
     * @note lock get_conditions_mutex() after lock reader_->getMutex() to avoid ABBAs because
     *       try_notify_read_conditions() will be called from the callbacks with the reader
     *       mutex locked
     */
    mutable std::recursive_mutex conditions_mutex_;

    // Order for the ReadCondition collection
    struct ReadConditionOrder {
        using is_transparent = void;

        bool operator()(const vbs::detail::ReadConditionImpl* lhs, const vbs::detail::ReadConditionImpl* rhs) const;
        bool operator()(const vbs::detail::ReadConditionImpl* lhs, const vbs::detail::StateFilter& rhs) const;
        bool operator()(const vbs::detail::StateFilter& lhs, const vbs::detail::ReadConditionImpl* rhs) const;

        template <class S, class V, class I>
        static inline bool less(S&& s1, V&& v1, I&& i1, S&& s2, V&& v2, I&& i2) {
            return (s1 < s2) || ((s1 == s2) && ((v1 < v2) || ((v1 == v2) && (i1 < i2))));
        }
    };

    // ReadConditions collection
    std::set<vbs::detail::ReadConditionImpl*, ReadConditionOrder> read_conditions_;

    // State of the History mask last time it was queried
    // protected with the RTPSReader mutex
    vbs::detail::StateFilter last_mask_state_ {};

    ReturnCode_t check_collection_preconditions_and_calc_max_samples(evbs::edds::dds::LoanableCollection& data_values,
                                                                     evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                                     int32_t& max_samples);

    ReturnCode_t prepare_loan(evbs::edds::dds::LoanableCollection& data_values,
                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t& max_samples);

    void set_read_communication_status(bool trigger_value);

    void update_subscription_matched_status(const evbs::ertps::rtps::MatchingInfo& status);

    bool on_data_available(const evbs::ertps::rtps::GUID_t& writer_guid,
                           const vbs::common::SequenceNumber_t& first_sequence,
                           const vbs::common::SequenceNumber_t& last_sequence);

    /**
     * @brief A method called when a new cache change is added
     * @param change The cache change that has been added
     * @return True if the change was added (due to some QoS it could have been 'rejected')
     */
    bool on_new_cache_change_added(const vbs::common::CacheChange_t* const change);

    /**
     * @brief Method called when an instance misses the deadline
     */
    bool deadline_missed();

    /**
     * @brief A method to reschedule the deadline timer
     */
    bool deadline_timer_reschedule();

    /**
     * @brief A method called when the lifespan timer expires
     */
    bool lifespan_expired();

    std::shared_ptr<evbs::ertps::TopicAttributes> topic_attributes() const;

    void subscriber_qos_updated();

    evbs::edds::dds::RequestedIncompatibleQosStatus& update_requested_incompatible_qos(
        PolicyMask incompatible_policies);

    evbs::edds::dds::LivelinessChangedStatus& update_liveliness_status(
        const evbs::ertps::LivelinessChangedStatus& status);

    const evbs::edds::dds::SampleLostStatus& update_sample_lost_status(
        int32_t sample_lost_since_last_update, evbs::edds::dds::SampleLostStatusKind statusKind);

    /*!
     * @brief Update SampleRejectedStatus with information about a new rejected sample.
     *
     * @param[in] Reason why the new sample was rejected.
     * @param[in] New sample which was rejected.
     */
    const evbs::edds::dds::SampleRejectedStatus& update_sample_rejected_status(
        evbs::edds::dds::SampleRejectedStatusKind reason, const vbs::common::CacheChange_t* const change_in);

    /**
     * Returns the most appropriate listener to handle the callback for the given status,
     * or nullptr if there is no appropriate listener.
     */
    DataReaderListenerInner* get_listener_for(const evbs::edds::dds::StatusMask& status);

    //std::shared_ptr<IPayloadPool> get_payload_pool();

    //void release_payload_pool();

    void stop();

    std::shared_ptr<vbs::common::IChangePool> get_change_pool() const;
    std::shared_ptr<vbs::common::IChangePool> change_pool_;

 private:
    void update_reader_qos();

    void deadline_event_update(const vbs::common::CacheChange_t* const change);

    bool lifespan_event_update(const vbs::common::CacheChange_t* const change);

 private:
    // make sure that NexusContext is released after all entities release.
    std::shared_ptr<NexusContext> context_ = NexusContext::get_context_shared();

    bool batch_free_ = false;
    bool dds_is_initialized_ = false;
    DataReader* user_datareader_ = nullptr;
    TopicImpl* vbs_topic_ = nullptr;
    vbs::DataReaderListener* reader_listener_ = nullptr;
#if !defined(_WIN32)
    std::shared_mutex reader_mutex_;
#else
    std::mutex reader_mutex_;
#endif
    std::mutex reader_take_mutex_;
    std::mutex data_mutex_;
    std::mutex match_mutex_;
    vbs::DataReaderQosImpl vbs_qos_;
    vbs::DomainParticipantImpl* reader_participant_ = nullptr;
    int32_t match_count_ = 0;

    vbs::VBSGuid vbs_guid_;
    uint16_t reader_guid_ = 0;

 public:
    /**
     * DataReaderImpl static constructor.
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    // coverity[misra_cpp_2008_rule_3_2_4_violation:SUPPRESS]
    // coverity[misra_cpp_2008_rule_3_2_2_violation:SUPPRESS]
    // coverity[cert_dcl60_cpp_violation:SUPPRESS]
    DataReaderImpl(DomainParticipantImpl* const local_participant, TopicImpl* const local_topic,
                   const std::string& profile_name, vbs::DataReaderListener* const listener,
                   const vbs::StatusMask& mask = vbs::StatusMask::all());

    /**
     * DataReaderImpl dynamic constructor.
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param local_data_reader_qos datareader qos.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    // coverity[misra_cpp_2008_rule_3_2_4_violation:SUPPRESS]
    // coverity[misra_cpp_2008_rule_3_2_2_violation:SUPPRESS]
    // coverity[cert_dcl60_cpp_violation:SUPPRESS]
    DataReaderImpl(vbs::DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                   const vbs::DataReaderQosImpl& local_data_reader_qos,
                   vbs::DataReaderListener* const listener = nullptr,
                   const vbs::StatusMask& mask = vbs::StatusMask::all());
    /**
     * DataReaderImpl static constructor.
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    // coverity[misra_cpp_2008_rule_3_2_4_violation:SUPPRESS]
    // coverity[misra_cpp_2008_rule_3_2_2_violation:SUPPRESS]
    // coverity[cert_dcl60_cpp_violation:SUPPRESS]

    DataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                   vbs::ContentFilteredTopicImpl* const local_topic, const std::string& profile_name,
                   vbs::DataReaderListener* const listener, const vbs::StatusMask& mask = vbs::StatusMask::all());
    /**
     * DataReaderImpl dynamic constructor.
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param local_data_reader_qos datareader qos.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    // coverity[misra_cpp_2008_rule_3_2_4_violation:SUPPRESS]
    // coverity[misra_cpp_2008_rule_3_2_2_violation:SUPPRESS]
    // coverity[cert_dcl60_cpp_violation:SUPPRESS]
    DataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                   vbs::ContentFilteredTopicImpl* const local_topic,
                   const vbs::DataReaderQosImpl& local_data_reader_qos,
                   vbs::DataReaderListener* const listener = nullptr,
                   const vbs::StatusMask& mask = vbs::StatusMask::all());

    void initDataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                            std::variant<vbs::TopicImpl*, vbs::ContentFilteredTopicImpl*> const local_topic,
                            std::variant<const vbs::DataReaderQosImpl*, const std::string*> qos_info,
                            vbs::DataReaderListener* const listener, const vbs::StatusMask& mask);

    bool datareader_init(vbs::TopicDescription* topic, const vbsutil::xmlparser::DataReaderQos& reader_qos,
                         bool auto_enable);

    bool datareader_init_with_profile(vbs::TopicDescription* topic_desc,
                                      const vbsutil::xmlparser::DataReaderQos& qos_input, bool auto_enable);

    vbs::ReturnCode_t take_next_sample(void* data, vbs::SampleInfoImpl* sample_info = nullptr);

    vbs::ReturnCode_t take_next_sample(vbs::VBSDynamicData* dynamic_data, vbs::SampleInfoImpl* sample_info = nullptr);

    void set_user_listener(vbs::DataReaderListener* const listener,
                           const vbs::StatusMask& mask = vbs::StatusMask::all());

    vbs::ReturnCode_t take_flatdata(std::shared_ptr<vbs::LoanableCollectionFlatDataImpl>& datas,
                                    std::shared_ptr<vbs::SampleInfoImpl>& infos);

    vbs::ReturnCode_t take_buffer(std::shared_ptr<vbs::LoanableCollectionFlatDataImpl>& datas,
                                  std::shared_ptr<vbs::SampleInfoImpl>& infos);

    bool deserialize(void* payload_buffer, const uint32_t payload_len, void* data);

    /**
     * @brief Getter for the DataReaderQos.
     *
     * @return Pointer to the DataReaderQos.
     */
    const DataReaderQosImpl& get_qos() const;

    /**
     * @brief Check if initialization was successful.
     *
     * This function verifies whether the previous initialization process
     * completed successfully. It should be called after the initialization
     * routine to ensure that the system or object is ready for use.
     *
     * @return true if the initialization was successful, false otherwise.
     */
    bool init_verify();

    /**
     * @brief get guid
     *
     * @return guid
     */
    uint16_t get_guid();

    bool GetQos(DataReaderQosImpl& qos);
    bool SetQos(const DataReaderQosImpl& qos);

 protected:
    const TypeSupport& get_type() { return type_; }

    void user_data_available(vbs::DataReaderImpl* reader);
    void user_subscription_matched(vbs::DataReaderImpl* reader, const evbs::edds::dds::SubscriptionMatchedStatus& info);
    void user_requested_deadline_missed(vbs::DataReaderImpl* reader,
                                        const evbs::edds::dds::RequestedDeadlineMissedStatus& status);
    void user_liveliness_changed(vbs::DataReaderImpl* reader, const evbs::edds::dds::LivelinessChangedStatus& status);
    void user_sample_lost(vbs::DataReaderImpl* reader, const evbs::edds::dds::SampleLostStatus& status);
    void user_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status);
    void user_endpoint_crash(const evbs::ertps::rtps::GUID_t& reader_guid);
    void user_requested_incompatible_qos(vbs::DataReaderImpl* reader, const RequestedIncompatibleQosStatus& status);
    void user_sample_rejected(vbs::DataReaderImpl* reader, const evbs::ertps::SampleRejectedStatus& status);
};

} /* namespace vbs */
#endif  // INCLUDE_SUB_DATAREADERIMPL_HPP_
