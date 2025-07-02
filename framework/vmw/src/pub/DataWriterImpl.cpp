// Copyright 2019, 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: remove reactor
// feature: support e2e deadline and liveliness 1. config deadline sendPeriod 2. enable deadline event
// feature: change name NewusContext to NexusContext
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: Reduce level of DomainParticipantListener
// feature: Log the first packet sent for each topic
// feature: implement ContentFilteredTopic
// feature: use environment variable to control system discovery.
// feature: Enable trace and log control with udp
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: VBS framework adapt to system test tool
// feature: separating interface and implementation about DataWriter
// feature: clean do_coverity
// feature: VBSDynamicData interface constness support
// feature: System discovery supports version discovery and detection
// feature: activate global discovery in vbs
// feature: Upload trace data
// feature: record-replay dsf dynamic discovery with xml
// ------------------------------------------------------------------

#include <functional>
#include <iostream>

#include "ertps/config.h"
#include "edds/rtps/RTPSDomain.h"
#include "edds/rtps/builtin/liveliness/WLP.h"
#include "edds/rtps/resources/ResourceEvent.h"
#include "deps/resources/TimedEvent.h"
#include "dispatcher/DispatcherWriter.hpp"
#include "edds/rtps/writer/StatefulWriter.h"
#include "ertps/attributes/TopicAttributes.h"
#include "ertps/utils/TimeConversion.h"
#include "history/CacheChangePool.h"
#include "dispatcher/DispatcherParticipant.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "elog/Log.hpp"
#include "deps/common/TypeSupport.hpp"
#include "topic/TopicImpl.hpp"
#include "pub/DataWriterImpl.hpp"
#include "pub/Publisher.hpp"
#include "pub/PublisherListener.hpp"
#include "topic/TopicProxy.hpp"
#include "core/condition/StatusConditionImpl.hpp"
#include "history/TopicPayloadPoolRegistry.hpp"
#include "rtps/RTPSDomainImpl.hpp"
#include "history/IChangePool.h"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/DataWriterListener.hpp"
#if !defined(_WIN32)
#include "common/SharingPayloadPoolFactory.h"
#endif

using namespace evbs::ertps;
using namespace evbs::ertps::rtps;
using namespace std::chrono;
using namespace vbs::dispatcher;
using namespace vbs::common;

namespace vbs {

static ChangeKind_t unregister_change_kind(const bool dispose, const vbsutil::xmlparser::DataWriterQos& qos) {
    UNUSED_PARAMETER(qos);
    if (dispose) {
        return NOT_ALIVE_DISPOSED;
    }

    return NOT_ALIVE_UNREGISTERED;
}

class DataWriterImpl::LoanCollection {
 public:
    explicit LoanCollection(const PoolConfig& config) : loans_(get_collection_limits(config)) {}

    bool add_loan(void* const data, PayloadInfo_t& payload) {
        static_cast<void>(data);
        assert(data == payload.payload.data + SerializedPayload_t::representation_header_size);
        return loans_.push_back(payload);
    }

    bool check_and_remove_loan(void* const data, PayloadInfo_t& payload) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        octet* payload_data = static_cast<octet*>(data) - SerializedPayload_t::representation_header_size;
        for (auto it = loans_.begin(); it != loans_.end(); ++it) {
            if (it->payload.data == payload_data) {
                payload = *it;
                (void)loans_.erase(it);
                return true;
            }
        }
        return false;
    }

    bool is_empty() const { return loans_.empty(); }

 private:
    static ResourceLimitedContainerConfig get_collection_limits(const PoolConfig& config) {
        return {config.initial_size, config.maximum_size, (config.initial_size == config.maximum_size) ? 0U : 1U};
    }

    vbsutil::xmlparser::ResourceLimitedVector<PayloadInfo_t> loans_;
};

DataWriterImpl::DataWriterImpl(Publisher* p, TypeSupport type, vbs::TopicImpl* topic,
                               const vbsutil::xmlparser::DataWriterQos& qos, DataWriterListenerInner* listen,
                               const evbs::edds::dds::StatusMask& mask)
    : DomainEntity(mask),
      publisher_(p),
      type_(std::move(type)),
      qos_((&qos == &vbsutil::xmlparser::getDefaultDataWriteQos()) ? publisher_->get_default_datawriter_qos() : qos),
      history_(new DataWriterHistory((*get_topic_attributes(qos_, *topic, type_)), type_->m_typeSize,
                                     qos_.endpoint().history_memory_policy,
                                     [this](const InstanceHandle_t& handle) -> void {
                                         if (nullptr != inner_listener_) {
                                             inner_listener_->on_unacknowledged_sample_removed(this, handle);
                                         }
                                     },
                                     qos.batch_qos())),
#pragma warning(disable : 4355)
      inner_listener_(listen),
      listener_wrapper_(this),
      deadline_duration_us_(qos_.deadline().period.to_ns() * 1e-3),
      lifespan_duration_us_(qos_.lifespan().duration.to_ns() * 1e-3),
      writer_topic_(nullptr) {
    writer_topic_ = topic;
    writer_topic_->get_impl()->reference();
    EndpointAttributes endpoint_attributes;
    endpoint_attributes.endpointKind = WRITER;
    endpoint_attributes.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    endpoint_attributes.setEntityID(qos_.endpoint().entity_id);
    endpoint_attributes.setUserDefinedID(qos_.endpoint().user_defined_id);

    (void)DomainParticipantImpl::preprocess_endpoint_attributes<WRITER, 0x03, 0x02>(
        EntityId_t::unknown(), publisher_->get_participant()->id_counter(), endpoint_attributes, guid_.entityId);

    guid_.guidPrefix = publisher_->get_participant()->guid().guidPrefix;
    dds_is_initialized_ = true;
}

DispatcherWriter* DataWriterImpl::create_dispatcher_writer(WriterAttributes& watt, TopicAttributes& topic_att,
                                                           WriterQos& wqos) {
    auto change_pool = get_change_pool();
    if (!change_pool) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Problem creating change pool for associated Writer");
        return nullptr;
    }

    auto pool = get_payload_pool();
    if (!pool) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Problem creating payload pool for associated Writer");
        return nullptr;
    }

    DispatcherWriter* writer = publisher_->dispatcher_participant()->create_writer(
        guid_, watt, topic_att, wqos, pool, change_pool, history_, static_cast<WriterListener*>(&listener_wrapper_));

    if (writer == nullptr) {
        logDebug(DATA_WRITER, "Trying with a non-datasharing pool");
        (void)release_payload_pool();
        pool = get_payload_pool();
        if (!pool) {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Problem creating payload pool for associated Writer");
            return nullptr;
        }

        writer = publisher_->dispatcher_participant()->create_writer(guid_, watt, topic_att, wqos, pool, change_pool,
                                                                     history_,
                                                                     static_cast<WriterListener*>(&listener_wrapper_));
        if (writer == nullptr) {
            (void)release_payload_pool();
            return nullptr;
        }
    }
    history_->mp_writer = writer;
    history_->guid = guid_;
    history_->topicName = get_topic()->get_name();
    history_->notify_writer = std::bind(&DispatcherWriter::write, history_->mp_writer, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3);
    history_->change_removed_by_history =
        std::bind(&DispatcherWriter::change_removed_by_history, writer, std::placeholders::_1);
    history_->payload_pool_ = std::move(pool);
    history_->change_pool_ = std::move(change_pool);
    history_->fixed_payload_size_ = fixed_payload_size_;
    return writer;
}

evbs::ReturnCode_t DataWriterImpl::enable() {
    if (dds_is_initialized_ == false) {
        return evbs::ReturnCode_t::RETCODE_ERROR;
    }
    if (enable_) {
        return evbs::ReturnCode_t::RETCODE_OK;
    }

    if (false == publisher_->is_enabled()) {
        elogError(DATA_WRITER, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Publisher not enabled. writer guid " << guid());
        return evbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    assert(writer_ == nullptr);

    WriterAttributes w_att = get_writer_attributes();
    // REGISTER THE WRITER
    WriterQos wqos = qos_.get_writerqos(get_publisher()->get_qos());
    reader_filters_.reset(new ReaderFilterCollection(get_type().get_type_name()));

    const std::shared_ptr<evbs::ertps::TopicAttributes> topic_attr_ptr =
        get_topic_attributes(qos_, *(writer_topic_), type_);

    auto writer = create_dispatcher_writer(w_att, *topic_attr_ptr, wqos);
    if (writer == nullptr) {
        elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Problem creating associated Writer");
        return evbs::ReturnCode_t::RETCODE_ERROR;
    }
    //todo dispatcher
#if 0
    writer->is_send_multi = qos_.send_multi();
    // coverity[misra_cpp_2008_rule_14_8_2_violation:SUPPRESS]
    logInfo(DATA_WRITER, "Writer enabled in topic " << get_topic()->get_name() <<
        " send_multi:" << writer->is_send_multi << " guid:" << writer->getGuid());
#endif
    writer_ = writer;
    writer_->reader_data_filter(this);

    // In case it has been loaded from the persistence DB, rebuild instances on history
    history_->rebuild_instances();

    deadline_timer_ = new TimedEvent(
        publisher_->get_participant()->get_resource_event(), [&]() -> bool { return deadline_missed(); },
        qos_.deadline().period.to_ns() * 1e-6);

    lifespan_timer_ = new TimedEvent(
        publisher_->get_participant()->get_resource_event(), [&]() -> bool { return lifespan_expired(); },
        qos_.lifespan().duration.to_ns() * 1e-6);

    // In case it has been loaded from the persistence DB, expire old samples.
    if (qos_.lifespan().duration != c_TimeInfinite) {
        if (lifespan_expired()) {
            lifespan_timer_->restart_timer();
        }
    }
#ifdef BATCH_SEND_ENABLE
    bool create_batch_flush_timmer = (qos_.batch_qos().enable && (qos_.batch_qos().max_flush_delay != c_TimeInfinite) &&
                                      (qos_.batch_qos().max_flush_delay != c_TimeZero));
    if (create_batch_flush_timmer) {
        auto flush_timer = new TimedEvent(
            publisher_->get_participant()->get_resource_event(), [&]() -> bool { return flush(); },
            qos_.batch_qos().max_flush_delay.to_ns() * 1e-6);
        history_->batch_flush_timer(flush_timer);
    }
#endif

    writer_->registerWriter();
    enable_ = true;

    return evbs::ReturnCode_t::RETCODE_OK;
}

void DataWriterImpl::disable() {
    (void)set_listener(nullptr);
    if (writer_ != nullptr) {
        (void)writer_->set_listener(nullptr);
    }
}

ReturnCode_t DataWriterImpl::check_delete_preconditions() {
    if (loans_ && (!loans_->is_empty())) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return ReturnCode_t::RETCODE_OK;
}

DataWriterImpl::~DataWriterImpl() {
    delete lifespan_timer_;
    delete deadline_timer_;

    if (writer_ != nullptr) {

        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
#if 0
        logInfo(DATA_WRITER, "Writer " << guid() << " in topic: " << type_->getName() << " destruct" <<
            " send:" << writer_->get_send_count() <<
            " throughput:" << writer_->get_throughput() <<
            " avg payload len:" << writer_->get_avg_payload_length());
#endif
        writer_->set_listener(nullptr);
        publisher_->dispatcher_participant()->delete_writer(writer_);
        writer_ = nullptr;
        (void)release_payload_pool();
    }

    if (publisher_ != nullptr && batch_free_ != true) {
        (void)publisher_->del_datawriter(this);
    }

    if (writer_participant_ != nullptr) {
        (void)writer_participant_->delete_publisher(publisher_);
        writer_participant_ = nullptr;
    }

    if (wparams_ != nullptr) {
        delete wparams_;
        wparams_ = nullptr;
    }

    if (history_ != nullptr) {
        delete history_;
        history_ = nullptr;
    }

    if (writer_topic_) {
        writer_topic_->get_impl()->dereference();
    }

    publisher_ = nullptr;
    writer_ = nullptr;
    writer_topic_ = nullptr;
    user_listener_ = nullptr;
    deadline_timer_ = nullptr;
    lifespan_timer_ = nullptr;
}
// coverity[misra_cpp_2008_rule_7_1_2_violation:SUPPRESS]
bool DataWriterImpl::sample_init(void*& sample, uint32_t real_size, LoanInitializationKind initialization) {
    switch (initialization) {
        case LoanInitializationKind::NO_LOAN_INITIALIZATION:
            break;

        case LoanInitializationKind::ZERO_LOAN_INITIALIZATION:
            if (SerializedPayload_t::representation_header_size < real_size) {
                // coverity[misra_cpp_2008_rule_5_0_6_violation:SUPPRESS]
                real_size -= SerializedPayload_t::representation_header_size;
                (void)memset(sample, 0, static_cast<uint64_t>(real_size));
            }
            break;

        case LoanInitializationKind::CONSTRUCTED_LOAN_INITIALIZATION:
            if (!type_->construct_sample(sample)) {
                return false;
            }
            break;

        default:
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            logWarning(DATA_WRITER, "Using wrong LoanInitializationKind value ("
                                        << static_cast<int>(initialization)
                                        << "). Using default NO_LOAN_INITIALIZATION");
            break;
    }
    return true;
}

ReturnCode_t DataWriterImpl::loan_sample(void*& sample, LoanInitializationKind initialization) {
    return loan_sample(sample, 0, initialization);
}

ReturnCode_t DataWriterImpl::loan_sample(void*& sample, uint32_t size, LoanInitializationKind initialization) {
    if (size == 0) {  // type should be checked only if size is 0
        if (type_->is_big()) {
            size = type_->getBigBufferSize(sample);
        } else if ((!type_->is_plain()) || (SerializedPayload_t::representation_header_size > type_->m_typeSize)) {
            return ReturnCode_t::RETCODE_ILLEGAL_OPERATION;
        }  // Type should be plain and have space for the representation header
    }

    // Writer should be enabled
    if (nullptr == writer_) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    const std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

    // Get one payload from the pool
    PayloadInfo_t payload;
    uint32_t real_size = size ? size + 4 : type_->m_typeSize;
    if (!get_free_payload_from_pool([real_size]() { return real_size; }, payload)) {
        return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
    }

    // Leave payload state as if serialization has already been performed
    payload.payload.length = real_size;
    payload.payload.pos = real_size;
    payload.payload.data[1] = DEFAULT_ENCAPSULATION;
    payload.payload.encapsulation = DEFAULT_ENCAPSULATION;

    // Sample starts after representation header
    void* sample_loan = payload.payload.data + SerializedPayload_t::representation_header_size;
    if (type_->is_big()) {
        type_->setBigBuffer(sample, sample_loan);
    } else {
        sample = sample_loan;
    }

    // Add to loans collection
    if (!add_loan(sample_loan, payload)) {
        if (type_->is_big()) {
            type_->setBigBuffer(sample, nullptr);
        } else {
            sample = nullptr;
        }
        return_payload_to_pool(payload);
        return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
    }

    if (!sample_init(sample_loan, real_size, initialization)) {
        check_and_remove_loan(sample_loan, payload);
        return_payload_to_pool(payload);
        if (type_->is_big()) {
            type_->setBigBuffer(sample, nullptr);
        } else {
            sample = nullptr;
        }
        return ReturnCode_t::RETCODE_UNSUPPORTED;
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataWriterImpl::discard_loan(void*& data) {
    // Writer should be enabled
    if (nullptr == writer_) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }
    void* sample = data;
    if (type_->is_big()) {
        sample = type_->getBigBuffer(data);
        type_->setBigBuffer(data, nullptr);
    }
    const std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

    // Remove sample from loans collection
    PayloadInfo_t payload;
    if ((nullptr == sample) || (!check_and_remove_loan(sample, payload))) {
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    // Return payload to pool
    return_payload_to_pool(payload);
    sample = nullptr;

    return ReturnCode_t::RETCODE_OK;
}

bool DataWriterImpl::flush() {
#ifdef BATCH_SEND_ENABLE
    if (!qos_.batch_qos().enable) {
        return false;
    }

#if HAVE_STRICT_REALTIME
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);
    if (!lock.try_lock_until(max_blocking_time)) {
        return false;
    }
#else
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());
#endif  // if HAVE_STRICT_REALTIME
    history_->batch_flush(steady_clock::now() +
                          microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time)));
#endif
    return true;
}

bool DataWriterImpl::write(void* data) {
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    WriteParams wparams;
    return ReturnCode_t::RETCODE_OK == create_new_change_with_params(ALIVE, data, wparams, 0);
}

bool DataWriterImpl::write(void* data, vbs::common::WriteParams& params) {
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    return ReturnCode_t::RETCODE_OK == create_new_change_with_params(ALIVE, data, params, 0);
}

bool DataWriterImpl::write_buffer(void* data, vbs::common::WriteParams& params, const uint32_t len) {
    if (len <= SerializedPayload_t::representation_header_size) {
        elogErrorT(DATA_WRITER, RetCode_t::RETCODE_BAD_PARAMETER, "Write buffer len is invalid. " << len);
        return false;
    }
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    return ReturnCode_t::RETCODE_OK == create_new_change_with_params(ALIVE, data, params, len);
}

ReturnCode_t DataWriterImpl::write(void* data, const InstanceHandle_t& handle) {
    InstanceHandle_t instance_handle;
    ReturnCode_t ret = check_write_preconditions(data, handle, instance_handle);
    if (ReturnCode_t::RETCODE_OK == ret) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        WriteParams wparams;
        ret = create_new_change_with_params(ALIVE, data, wparams, instance_handle);
    }
    return ret;
}

ReturnCode_t DataWriterImpl::write_w_timestamp(void* data, const InstanceHandle_t& handle,
                                               const evbs::ertps::Time_t& timestamp) {
    InstanceHandle_t instance_handle;
    ReturnCode_t ret = ReturnCode_t::RETCODE_OK;
    if (timestamp.is_infinite() || (timestamp.seconds < 0)) {
        ret = ReturnCode_t::RETCODE_BAD_PARAMETER;
        return ret;
    }

    ret = check_write_preconditions(data, handle, instance_handle);

    if (ReturnCode_t::RETCODE_OK == ret) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        WriteParams wparams;
        (void)wparams.source_timestamp(timestamp);
        ret = create_new_change_with_params(ALIVE, data, wparams, instance_handle);
    }

    return ret;
}

ReturnCode_t DataWriterImpl::check_write_preconditions(void* data, const InstanceHandle_t& handle,
                                                       InstanceHandle_t& instance_handle) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    if (type_.get()->m_isGetKeyDefined) {
        bool is_key_protected = false;
#if HAVE_SECURITY
        is_key_protected = writer_->getAttributes().endpoint.security_attributes().is_key_protected;
#endif  // if HAVE_SECURITY
        (void)type_.get()->getKey(data, &instance_handle, is_key_protected);
    }

    // Check if the Handle is different from the special value HANDLE_NIL and
    // does not correspond with the instance referred by the data
    if (handle.isDefined() && (handle != instance_handle)) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataWriterImpl::check_instance_preconditions(void* data, const InstanceHandle_t& handle,
                                                          InstanceHandle_t& instance_handle) {
    if (nullptr == writer_) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    if (nullptr == data) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_BAD_PARAMETER, "Data pointer not valid");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (!type_->m_isGetKeyDefined) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_PRECONDITION_NOT_MET, "Topic is NO_KEY, operation not permitted");
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    instance_handle = handle;

#if defined(NDEBUG)
    if (!instance_handle.isDefined())
#endif  // if !defined(NDEBUG)
    {
        bool is_key_protected = false;
#if HAVE_SECURITY
//todo
//is_key_protected = writer_->getAttributes().security_attributes().is_key_protected;
#endif  // if HAVE_SECURITY
        (void)type_->getKey(data, &instance_handle, is_key_protected);
    }

#if !defined(NDEBUG)
    if (handle.isDefined() && (instance_handle != handle)) {
        elogError(DATA_WRITER, RetCode_t::RETCODE_PRECONDITION_NOT_MET, "handle differs from data's key.");
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
#endif  // if !defined(NDEBUG)

    return ReturnCode_t::RETCODE_OK;
}

InstanceHandle_t DataWriterImpl::register_instance(void* key) {
    /// Preconditions
    InstanceHandle_t instance_handle;
    if (ReturnCode_t::RETCODE_OK != check_instance_preconditions(key, HANDLE_NIL, instance_handle)) {
        return HANDLE_NIL;
    }

    WriteParams wparams;
    return do_register_instance(key, instance_handle, wparams);
}

InstanceHandle_t DataWriterImpl::register_instance_w_timestamp(void* key, const evbs::ertps::Time_t& timestamp) {
    /// Preconditions
    InstanceHandle_t instance_handle;
    if (timestamp.is_infinite() || (timestamp.seconds < 0) ||
        (ReturnCode_t::RETCODE_OK != check_instance_preconditions(key, HANDLE_NIL, instance_handle))) {
        return HANDLE_NIL;
    }

    WriteParams wparams;
    (void)wparams.source_timestamp(timestamp);
    return do_register_instance(key, instance_handle, wparams);
}

InstanceHandle_t DataWriterImpl::do_register_instance(void* key, const InstanceHandle_t instance_handle,
                                                      WriteParams& wparams) {
    // TODO(MiguelCompany): wparams should be used when propagating the register_instance operation
    // to the DataReader. See redmine issue #14494
    static_cast<void>(wparams);
#if 0
    // Block lowlevel writer
    auto max_blocking_time =
        std::chrono::steady_clock::now() +
        std::chrono::microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time));

#if HAVE_STRICT_REALTIME
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);
    if (lock.try_lock_until(max_blocking_time))
#else
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());
#endif  // if HAVE_STRICT_REALTIME
#endif
    {
        SerializedPayload_t* payload = nullptr;
        if (history_->register_instance(instance_handle, payload)) {
            // Keep serialization of sample inside the instance
            assert(nullptr != payload);
            if ((0U == payload->length) || (nullptr == payload->data)) {
                uint32_t size = fixed_payload_size_ ? fixed_payload_size_ : type_->getSerializedSizeProvider(key)();
                payload->reserve(size);
                if (!type_->serialize(key, payload)) {
                    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
                    logWarning(DATA_WRITER, "Key data serialization failed");

                    // Serialization of the sample failed. Remove the instance to keep original
                    // state. Note that we will only end-up here if the instance has just been
                    // created, so it will be empty and removing its changes will remove the
                    // instance completely.
                    (void)history_->remove_instance_changes(instance_handle, SequenceNumber_t());
                }
            }
            return instance_handle;
        }
    }

    return HANDLE_NIL;
}

ReturnCode_t DataWriterImpl::unregister_instance(void* instance, const InstanceHandle_t& handle, bool dispose) {
    // Preconditions
    InstanceHandle_t ih;
    ReturnCode_t returned_value = check_instance_preconditions(instance, handle, ih);
    if ((ReturnCode_t::RETCODE_OK == returned_value) && (!history_->is_key_registered(ih))) {
        returned_value = ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    // Operation
    if (ReturnCode_t::RETCODE_OK == returned_value) {
        WriteParams wparams;
        ChangeKind_t change_kind = unregister_change_kind(dispose, qos_);
        returned_value = create_new_change_with_params(change_kind, instance, wparams, ih);
    }

    return returned_value;
}

ReturnCode_t DataWriterImpl::dispose(void* data, const InstanceHandle_t& handle) {

    return unregister_instance(data, handle, true);
}

ReturnCode_t DataWriterImpl::dispose_w_timestamp(void* instance, const InstanceHandle_t& handle,
                                                 const evbs::ertps::Time_t& timestamp) {

    return unregister_instance_w_timestamp(instance, handle, timestamp, true);
}

ReturnCode_t DataWriterImpl::unregister_instance_w_timestamp(void* instance, const InstanceHandle_t& handle,
                                                             const evbs::ertps::Time_t& timestamp, bool dispose) {
    // Preconditions
    InstanceHandle_t instance_handle;
    ReturnCode_t ret = ReturnCode_t::RETCODE_OK;
    if (timestamp.is_infinite() || (timestamp.seconds < 0)) {
        ret = ReturnCode_t::RETCODE_BAD_PARAMETER;
    }
    if (ReturnCode_t::RETCODE_OK == ret) {
        ret = check_instance_preconditions(instance, handle, instance_handle);
    }
    if ((ReturnCode_t::RETCODE_OK == ret) && (!history_->is_key_registered(instance_handle))) {
        ret = ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    // Operation
    if (ReturnCode_t::RETCODE_OK == ret) {
        WriteParams wparams;
        (void)wparams.source_timestamp(timestamp);
        ChangeKind_t change_kind = unregister_change_kind(dispose, qos_);
        ret = create_new_change_with_params(change_kind, instance, wparams, instance_handle);
    }

    return ret;
}

ReturnCode_t DataWriterImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle) {
    /// Preconditions
    if ((key_holder == nullptr) || (!handle.isDefined())) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_BAD_PARAMETER, "Key holder pointer not valid");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if (!type_->m_isGetKeyDefined) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_ILLEGAL_OPERATION, "Topic is NO_KEY, operation not permitted");
        return ReturnCode_t::RETCODE_ILLEGAL_OPERATION;
    }

    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    // Block lowlevel writer
#if HAVE_STRICT_REALTIME
    auto max_blocking_time =
        std::chrono::steady_clock::now() +
        std::chrono::microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time));
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);
    if (!lock.try_lock_until(max_blocking_time)) {
        return ReturnCode_t::RETCODE_TIMEOUT;
    }
#else
    std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());
#endif  // if HAVE_STRICT_REALTIME

    SerializedPayload_t* payload = history_->get_key_value(handle);
    if (nullptr == payload) {
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    (void)type_->deserialize(payload, key_holder);
    return ReturnCode_t::RETCODE_OK;
}

inline ReturnCode_t DataWriterImpl::check_new_change_preconditions(ChangeKind_t change_kind, void* data) {
    // Preconditions
    if (writer_ == nullptr) {
        elogError(DATA_WRITER, RetCode_t::RETCODE_NOT_ENABLED, "Writer not enabled, topic " << get_topic()->get_name());
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }
    if (data == nullptr) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Data pointer not valid, topic " << get_topic()->get_name());
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    if ((change_kind == NOT_ALIVE_UNREGISTERED) || (change_kind == NOT_ALIVE_DISPOSED) ||
        (change_kind == NOT_ALIVE_DISPOSED_UNREGISTERED)) {
        if (!type_->m_isGetKeyDefined) {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            elogError(DATA_WRITER, RetCode_t::RETCODE_ILLEGAL_OPERATION,
                      "Topic is NO_KEY, operation not permitted, topic " << get_topic()->get_name());
            return ReturnCode_t::RETCODE_ILLEGAL_OPERATION;
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

void DataWriterImpl::add_topic_id_to_inline_qos(SerializedPayload_t& payload, uint32_t topic_id) {
    // 2 + 2: PID + Length
    uint32_t cdr_size = 2 + 2 + sizeof(topic_id);
    payload.reserve(payload.length + cdr_size);
    CDRMessage_t msg(payload);

    // PID + PLENGTH
    if (msg.buffer != nullptr) {
        CDRMessage::addUInt16(&msg, vbsutil::xmlparser::PID_TOPIC_ID);
        CDRMessage::addUInt16(&msg, static_cast<uint16_t>(sizeof(topic_id)));

        // topic id
        CDRMessage::addUInt32(&msg, topic_id);

        payload.length = msg.length;
        payload.pos = msg.pos;
    }
}

ReturnCode_t DataWriterImpl::perform_create_new_change(ChangeKind_t change_kind, void* data, WriteParams& wparams,
                                                       const InstanceHandle_t& handle, const uint32_t len) {
    //std::cout << "perform_create_new_change  " << data << std::endl;                                                    
    // Block lowlevel writer
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    auto max_blocking_time = std::chrono::time_point<std::chrono::steady_clock> {};
    if (qos_.reliability().kind == vbsutil::xmlparser::RELIABLE_RELIABILITY_QOS) {
        max_blocking_time = steady_clock::now() +
                            microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time));
    }
#if HAVE_STRICT_REALTIME
    std::unique_lock<std::timed_mutex> write_lock(write_mutex_, std::defer_lock);
    if (!write_lock.try_lock_until(max_blocking_time)) {
        elogErrorKeyT(
            DATA_WRITER, RetCode_t::RETCODE_TIMEOUT, get_topic()->get_name(),
            "Timeout (" << qos_.reliability().max_blocking_time << ") to get lock , topic " << get_topic()->get_name());
        return ReturnCode_t::RETCODE_TIMEOUT;
    }
#else
    std::unique_lock<std::timed_mutex> write_lock(write_mutex_);
#endif  // if HAVE_STRICT_REALTIME

    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());
    PayloadInfo_t payload;
    PayloadInfo_t non_serialize_payload;
    bool serialized = false;
    size_t data_len = 0;
    bool was_loaned = check_and_remove_loan(data, payload);
    if (!was_loaned) {
        // write buffer serialized data with len
        if (len > 0) {
            if (!get_free_payload_from_pool([len]() { return len; }, payload)) {
                return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
            }
            memcpy(payload.payload.data, data, len);
            payload.payload.length = len;
        } else if (type_->is_plain() && !writer_->has_non_plain_reader() && reader_filters_->empty()) {
            data_len = type_->getNonSerializedSize();
            if (!get_free_payload_from_pool([data_len]() { return data_len; }, payload)) {
                return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
            }
            payload.payload.data[2] = 1;
        } else {
            if (!get_free_payload_from_pool(type_->getSerializedSizeProvider(data), payload)) {
                return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
            }

            if ((ALIVE == change_kind) && (!type_->serialize(data, &payload.payload))) {
                elogErrorKeyT(DATA_WRITER, RetCode_t::RETCODE_ERROR, type_.get_type_name(),
                              "Topic " << type_.get_type_name() << " data serialize fail.");
                return_payload_to_pool(payload);
                return ReturnCode_t::RETCODE_ERROR;
            }
            payload.payload.data[2] = 0;
            serialized = true;
        }
    } else {
        payload.payload.data[2] = 1;
    }

    if (type_->is_big()) {
        void* bigbuffer = type_->getBigBuffer(data);
        if (bigbuffer) {
            bool loaned = check_and_remove_loan(bigbuffer, non_serialize_payload);
            if (!loaned) {
                elogErrorKeyT(DATA_WRITER, RetCode_t::RETCODE_ERROR, type_.get_type_name(),
                              "Topic " << type_.get_type_name() << " original data " << bigbuffer << " is not loaned.");
                return RetCode_t::RETCODE_ERROR;
            }
            if (non_serialize_payload.payload_owner != payload.payload_owner) {
                elogErrorKeyT(
                    DATA_WRITER, RetCode_t::RETCODE_ERROR, type_.get_type_name(),
                    "Topic " << type_.get_type_name() << " original-payload's ower is not same as serialized-payload.");
                if (was_loaned) {
                    (void)add_loan(data, payload);
                }
                return RetCode_t::RETCODE_ERROR;
            }
        } else {
            logDebug(DATA_WRITER, "Topic " << type_.get_type_name() << " bigbuffer is null.");
        }
    }

    CacheChange_t* ch = history_->new_change_nts(change_kind, handle);
    if (ch != nullptr) {
        payload.move_into_change(*ch, true);
        if (non_serialize_payload.payload_owner) {
            non_serialize_payload.move_into_change(*ch, false);
        }

        if (data_len > 0 && !serialized) {
            if (ch->payload_owner()->get_payload_type() == PAYLOAD_TYPE_NORMAL &&
                qos_.reliability().kind == vbsutil::xmlparser::BEST_EFFORT_RELIABILITY_QOS &&
                qos_.publish_mode().kind == SYNCHRONOUS_PUBLISH_MODE) {
                ch->serializedPayload.origin_data = static_cast<octet*>(data);
            } else {
                memcpy(ch->serializedPayload.data + ch->serializedPayload.representation_header_size,
                       static_cast<octet*>(data), data_len - ch->serializedPayload.representation_header_size);
            }
            ch->serializedPayload.length = data_len;
        }

        bool added = false;

        if (wparams.topic_id() != WriteParams::INVALID_TOPIC_ID) {
            add_topic_id_to_inline_qos(ch->inline_qos, wparams.topic_id());
        }

        auto filter_hook = [serialized, this](CacheChange_t& ch) {
            if (serialized) {
                reader_filters_->update_filter_info(ch);
            }
        };
        bool add_history = true;
        if (qos_.durability().kind == VOLATILE_DURABILITY_QOS && !type_->m_isGetKeyDefined) {
            if ((writer_->getMatchedReadersSize() == 0) ||
                (writer_->getMatchedReadersSize() > 0 &&
                 qos_.reliability().kind == vbsutil::xmlparser::BEST_EFFORT_RELIABILITY_QOS &&
                 qos_.publish_mode().kind == SYNCHRONOUS_PUBLISH_MODE)) {
                add_history = false;
            }
        }
        added =
            history_->add_pub_change_with_commit_hook(ch, wparams, filter_hook, lock, max_blocking_time, add_history);
        if ((add_history && !added) || !add_history) {
            if (was_loaned) {
                payload.move_from_change(*ch);
                (void)add_loan(data, payload);
            }
            (void)history_->release_change_nts(ch);
        }
        if (add_history && !added) {
            return ReturnCode_t::RETCODE_TIMEOUT;
        }

        if (qos_.deadline().period != c_TimeInfinite) {
            if (!history_->set_next_deadline_nts(
                    handle, steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_))) {
                // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
                elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Could not set the next deadline in the history");
            } else {
                if (timer_owner_ == handle || timer_owner_ == InstanceHandle_t()) {
                    if (deadline_timer_reschedule()) {
                        deadline_timer_->cancel_timer();
                        deadline_timer_->restart_timer();
                    }
                }
            }
        }
        if (qos_.lifespan().duration != c_TimeInfinite) {
            lifespan_duration_us_ =
                duration<double, std::ratio<1, 1000000>>(static_cast<double>(qos_.lifespan().duration.to_ns()) * 1e-3);
            (void)lifespan_timer_->update_interval_millisec(static_cast<double>(qos_.lifespan().duration.to_ns()) *
                                                            1e-6);
            lifespan_timer_->restart_timer();
        }
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
}

ReturnCode_t DataWriterImpl::create_new_change_with_params(ChangeKind_t changeKind, void* data, WriteParams& wparams,
                                                           const uint32_t len) {
    ReturnCode_t ret_code = check_new_change_preconditions(changeKind, data);
    if (!ret_code) {
        return ret_code;
    }

    InstanceHandle_t handle;
    // if len != 0, data is serialized buffer
    if (type_->m_isGetKeyDefined && len == 0) {
        bool is_key_protected = false;
#if HAVE_SECURITY
        is_key_protected = writer_->getAttributes().endpoint.security_attributes().is_key_protected;
#endif  // if HAVE_SECURITY
        std::cout << "create_new_change_with_params  is_key_protected: " << is_key_protected << std::endl;
        (void)type_->getKey(data, &handle, is_key_protected);
    }
    //
    return perform_create_new_change(changeKind, data, wparams, handle, len);
}

ReturnCode_t DataWriterImpl::create_new_change_with_params(ChangeKind_t changeKind, void* data, WriteParams& wparams,
                                                           const InstanceHandle_t& handle) {
    ReturnCode_t ret_code = check_new_change_preconditions(changeKind, data);
    if (!ret_code) {
        return ret_code;
    }

    return perform_create_new_change(changeKind, data, wparams, handle, 0);
}

bool DataWriterImpl::remove_min_seq_change() {

    return history_->removeMinChange();
}

ReturnCode_t DataWriterImpl::clear_history(size_t* removed) {
    return (history_->removeAllChange(removed) ? ReturnCode_t::RETCODE_OK : ReturnCode_t::RETCODE_ERROR);
}

const GUID_t& DataWriterImpl::guid() const {
    return guid_;
}

InstanceHandle_t DataWriterImpl::get_instance_handle() const {
    return guid();
}

void DataWriterImpl::publisher_qos_updated() {
    if (writer_ != nullptr) {
        // NOTIFY THE BUILTIN PROTOCOLS THAT THE WRITER HAS CHANGED
        WriterQos wqos = qos_.get_writerqos(get_publisher()->get_qos());
        const std::shared_ptr<evbs::ertps::TopicAttributes> attr = get_topic_attributes(qos_, *(writer_topic_), type_);
        writer_->updateWriter(*attr, wqos);
    }
}

ReturnCode_t DataWriterImpl::set_qos(const vbsutil::xmlparser::DataWriterQos& qos) {
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    bool enabled = writer_ != nullptr;
    const vbsutil::xmlparser::DataWriterQos& qos_to_set =
        (&qos == &vbsutil::xmlparser::getDefaultDataWriteQos()) ? publisher_->get_default_datawriter_qos() : qos;

    // Default qos is always considered consistent
    if (&qos != &vbsutil::xmlparser::getDefaultDataWriteQos()) {
        ReturnCode_t ret_val = check_qos_including_resource_limits(qos_to_set, type_);
        if (!ret_val) {
            return ret_val;
        }
    }

    if (enabled && (!can_qos_be_updated(qos_, qos_to_set))) {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }
    set_qos(qos_, qos_to_set, !enabled);

    if (enabled) {
        // Notify the participant that a Writer has changed its QOS
        std::shared_ptr<evbs::ertps::TopicAttributes> topic_att = get_topic_attributes(qos_, *(writer_topic_), type_);
        WriterQos wqos = qos_.get_writerqos(get_publisher()->get_qos());
        writer_->updateWriter(*topic_att, wqos);

        // Deadline
        if (qos_.deadline().period != c_TimeInfinite) {
            deadline_duration_us_ = duration<double_t, std::ratio<1, 1000000>>(qos_.deadline().period.to_ns() * 1e-3);
            (void)deadline_timer_->update_interval_millisec(qos_.deadline().period.to_ns() * 1e-6);
        } else {
            deadline_timer_->cancel_timer();
        }

        // Lifespan
        if (qos_.lifespan().duration != c_TimeInfinite) {
            lifespan_duration_us_ = duration<double_t, std::ratio<1, 1000000>>(qos_.lifespan().duration.to_ns() * 1e-3);
            (void)lifespan_timer_->update_interval_millisec(qos_.lifespan().duration.to_ns() * 1e-6);
        } else {
            lifespan_timer_->cancel_timer();
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

const vbsutil::xmlparser::DataWriterQos& DataWriterImpl::get_dds_qos() const {
    return qos_;
}

ReturnCode_t DataWriterImpl::get_dds_qos(vbsutil::xmlparser::DataWriterQos& qos) const {
    qos = qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataWriterImpl::set_listener(DataWriterListenerInner* listener) {
    return set_listener(listener, evbs::edds::dds::StatusMask::all());
}

ReturnCode_t DataWriterImpl::set_listener(DataWriterListenerInner* const listener,
                                          const evbs::edds::dds::StatusMask& mask) {
    inner_listener_ = listener;
    status_mask_ = mask;
    return ReturnCode_t::RETCODE_OK;
}

const DataWriterListenerInner* DataWriterImpl::get_listener() const {
    return inner_listener_;
}

vbs::TopicImpl* DataWriterImpl::get_topic() const {
    return writer_topic_;
}

Publisher* DataWriterImpl::get_publisher() {
    return publisher_;
}

const Publisher* DataWriterImpl::get_publisher() const {
    return publisher_;
}

void DataWriterImpl::InnerDataWriterListener::onWriterMatched(const evbs::ertps::rtps::MatchingInfo& info) {
    data_writer_->update_publication_matched_status(info);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::publication_matched();

    DataWriterListenerInner* listener = data_writer_->get_listener_for(notify_status);
    if (listener != nullptr || data_writer_->user_listener_ != nullptr) {
        evbs::edds::dds::PublicationMatchedStatus callback_status;
        ReturnCode_t ret = data_writer_->get_publication_matched_status(callback_status);
        if (ReturnCode_t::RETCODE_OK == ret) {
            callback_status.content_filter_property.content_filtered_topic_name =
                info.remoteContentFilterProperty.content_filtered_topic_name;
            callback_status.content_filter_property.expression_parameters =
                info.remoteContentFilterProperty.expression_parameters;
            callback_status.content_filter_property.filter_class_name =
                info.remoteContentFilterProperty.filter_class_name;
            callback_status.content_filter_property.filter_expression =
                info.remoteContentFilterProperty.filter_expression;
            callback_status.content_filter_property.related_topic_name =
                info.remoteContentFilterProperty.related_topic_name;

            if (data_writer_->user_listener_ != nullptr) {
                data_writer_->on_publication_matched(data_writer_, callback_status);
            }
            if (listener != nullptr) {
                listener->on_publication_matched(data_writer_, callback_status);
            }
        }
    }

    data_writer_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataWriterImpl::InnerDataWriterListener::on_offered_incompatible_qos(evbs::edds::dds::PolicyMask qos) {
    (void)data_writer_->update_offered_incompatible_qos(qos);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::offered_incompatible_qos();
    DataWriterListenerInner* listener = data_writer_->get_listener_for(notify_status);
    if (listener != nullptr) {
        OfferedIncompatibleQosStatus callback_status;
        if (data_writer_->get_offered_incompatible_qos_status(callback_status) == ReturnCode_t::RETCODE_OK) {
            listener->on_offered_incompatible_qos(data_writer_, callback_status);
        }
    }
    data_writer_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataWriterImpl::InnerDataWriterListener::onWriterChangeReceivedByAll(
    vbs::dispatcher::SubstrateWriter* const /*writer*/, CacheChange_t* ch) {
    if (data_writer_->qos_.reliability().kind == vbsutil::xmlparser::BEST_EFFORT_RELIABILITY_QOS) {
        // coverity[misra_cpp_2008_rule_0_1_2_violation:SUPPRESS]
        if (ch->batch_send && (ch->batch_changes.size() > 0U)) {
            // coverity[misra_cpp_2008_rule_5_0_6_violation:SUPPRESS]
            // coverity[misra_cpp_2008_rule_0_1_2_violation:SUPPRESS]
            // coverity[misra_cpp_2008_rule_5_0_4_violation:SUPPRESS]
            // coverity[misra_cpp_2008_rule_5_0_3_violation:SUPPRESS]
            for (int64_t i = ch->batch_changes.size() - 1U; i >= 0; i--) {
                // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
                CacheChange_t*& ch_remove = ch->batch_changes[i];
                if (data_writer_->type_->m_isGetKeyDefined && ((NOT_ALIVE_UNREGISTERED == ch_remove->kind) ||
                                                               (NOT_ALIVE_DISPOSED_UNREGISTERED == ch_remove->kind))) {
                    (void)data_writer_->history_->remove_instance_changes(ch_remove->instanceHandle,
                                                                          ch_remove->sequenceNumber);
                } else if (data_writer_->qos_.durability().kind == VOLATILE_DURABILITY_QOS) {
                    (void)data_writer_->history_->remove_change_pub(ch_remove);
                }
            }
            return;
        }
    }

    if (data_writer_->type_->m_isGetKeyDefined &&
        ((NOT_ALIVE_UNREGISTERED == ch->kind) || (NOT_ALIVE_DISPOSED_UNREGISTERED == ch->kind))) {

        (void)data_writer_->history_->remove_instance_changes(ch->instanceHandle, ch->sequenceNumber);
    } else if (data_writer_->qos_.durability().kind == VOLATILE_DURABILITY_QOS) {

        (void)data_writer_->history_->remove_change_pub(ch);
    }
}

void DataWriterImpl::InnerDataWriterListener::on_liveliness_lost(const evbs::ertps::LivelinessLostStatus& status) {
    (void)data_writer_->update_liveliness_lost_status(status);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::liveliness_lost();

    DataWriterListenerInner* listener = data_writer_->get_listener_for(notify_status);
    if (listener != nullptr || data_writer_->user_listener_ != nullptr) {
        evbs::edds::dds::LivelinessLostStatus callback_status;
        ReturnCode_t ret = data_writer_->get_liveliness_lost_status(callback_status);
        if (ReturnCode_t::RETCODE_OK == ret) {
            data_writer_->on_liveliness_lost(data_writer_, callback_status);

            if (listener != nullptr) {
                listener->on_liveliness_lost(data_writer_, callback_status);
            }
        }
    }

    data_writer_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataWriterImpl::InnerDataWriterListener::on_reader_discovery(evbs::edds::dds::READER_DISCOVERY_STATUS reason,
                                                                  const evbs::ertps::rtps::GUID_t& reader_guid,
                                                                  vbs::RemoteEndpointInfo& reader_info) {
    switch (reason) {
        case evbs::edds::dds::READER_DISCOVERY_STATUS::REMOVED_READER:
            data_writer_->remove_reader_filter(reader_guid);
            break;

        case evbs::edds::dds::READER_DISCOVERY_STATUS::DISCOVERED_READER:
        case evbs::edds::dds::READER_DISCOVERY_STATUS::CHANGED_QOS_READER:
            data_writer_->add_reader_filter(reader_guid, reader_info.content_filter_property());
            break;
    }
}

void DataWriterImpl::InnerDataWriterListener::on_endpoint_crash(const evbs::ertps::rtps::GUID_t& reader_guid) {
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::endpoint_crash();
    data_writer_->on_endpoint_crash(data_writer_, reader_guid);
    DataWriterListenerInner* listener = data_writer_->get_listener_for(notify_status);
    if (listener != nullptr) {
        listener->on_endpoint_crash(reader_guid);
    }
}

void DataWriterImpl::InnerDataWriterListener::on_non_plain_reader_discovery() {
    if (data_writer_->qos_.durability().kind == VOLATILE_DURABILITY_QOS) {
        return;
    }
    std::unique_lock<RecursiveTimedMutex> lock(data_writer_->history_->getMutex());
    for (History::iterator cit = data_writer_->history_->changesBegin(); cit != data_writer_->history_->changesEnd();
         ++cit) {
        CacheChange_t* change = *cit;
        if (change->serializedPayload.data[2] != 1) {
            continue;
        }
        void* non_serialized_data = static_cast<void*>(change->serializedPayload.data + 4U);
        PayloadInfo_t payload;
        if (!data_writer_->get_free_payload_from_pool(
                data_writer_->type_->getSerializedSizeProvider(non_serialized_data), payload)) {
            logWarning(DATA_WRITER, "No free payload, serialize old change failed.");
            break;
        }
        if (!data_writer_->type_->serialize(non_serialized_data, &payload.payload)) {
            logWarning(DATA_WRITER, "Topic " << data_writer_->type_.get_type_name() << " old data serialize fail.");
            data_writer_->return_payload_to_pool(payload);
        }
        IPayloadPool* pool = change->payload_owner();
        if (pool) {
            (void)pool->release_payload(*change);
        }
        payload.payload.data[2] = 0;
        payload.move_into_change(*change, true);
    }
}

ReturnCode_t DataWriterImpl::wait_for_acknowledgments(const Duration_t& max_wait) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    if (writer_->wait_for_all_acked(max_wait)) {
        return ReturnCode_t::RETCODE_OK;
    }
    return ReturnCode_t::RETCODE_ERROR;
}

ReturnCode_t DataWriterImpl::wait_for_acknowledgments(void* instance, const InstanceHandle_t& handle,
                                                      const Duration_t& max_wait) {
    // Preconditions
    InstanceHandle_t ih;
    ReturnCode_t returned_value = check_instance_preconditions(instance, handle, ih);
    if (ReturnCode_t::RETCODE_OK != returned_value) {
        return returned_value;
    }

    // Block low-level writer
    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    auto max_blocking_time = steady_clock::now() + microseconds(::TimeConv::Time_t2MicroSecondsInt64(max_wait));

#if HAVE_STRICT_REALTIME
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);
    if (!lock.try_lock_until(max_blocking_time)) {
        return ReturnCode_t::RETCODE_TIMEOUT;
    }
#else
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());
#endif  // HAVE_STRICT_REALTIME

    if (!history_->is_key_registered(ih)) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    if (history_->wait_for_acknowledgement_last_change(ih, lock, max_blocking_time)) {
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_TIMEOUT;
}

void DataWriterImpl::update_publication_matched_status(const evbs::ertps::rtps::MatchingInfo& status) {
    auto count_change = status.status == MATCHED_MATCHING ? 1 : -1;
    publication_matched_status_.current_count += count_change;
    publication_matched_status_.current_count_change += count_change;
    if (count_change > 0) {
        publication_matched_status_.total_count += count_change;
        publication_matched_status_.total_count_change += count_change;
    }
    publication_matched_status_.last_subscription_handle = status.remoteEndpointGuid;

    // StatusMask notify_status     = StatusMask::publication_matched();
    // DataWriterListenerInner* listener = get_listener_for(notify_status);

    // if (listener != nullptr) {
    //     listener->on_publication_matched(this, publication_matched_status_);
    //     publication_matched_status_.current_count_change = 0;
    //     publication_matched_status_.total_count_change   = 0;
    // }
    // get_statuscondition().get_impl()->set_status(notify_status, true);
    // logInfo(DATA_WRITER, "Writer matched, TopicName:" << get_topic()->get_name() <<
    //     " guid: " << guid() << " reliabilityKind: " << (int)get_qos().reliability().kind <<
    //     " count_change: " << count_change << " listener:" << (listener != nullptr));
}

ReturnCode_t DataWriterImpl::get_publication_matched_status(evbs::edds::dds::PublicationMatchedStatus& status) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = publication_matched_status_;
        publication_matched_status_.current_count_change = 0;
        publication_matched_status_.total_count_change = 0;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::publication_matched(), false);
    return ReturnCode_t::RETCODE_OK;
}

bool DataWriterImpl::deadline_timer_reschedule() {
    assert(qos_.deadline().period != c_TimeInfinite);

    //std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    steady_clock::time_point next_deadline_us;
    if (!history_->get_next_deadline_nts(timer_owner_, next_deadline_us)) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Could not get the next deadline from the history");
        return false;
    }

    auto interval_ms = duration_cast<milliseconds>(next_deadline_us - steady_clock::now());
    (void)deadline_timer_->update_interval_millisec(static_cast<double_t>(interval_ms.count()));
    return true;
}

bool DataWriterImpl::deadline_missed() {
    assert(qos_.deadline().period != c_TimeInfinite);

    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    deadline_missed_status_.total_count++;
    deadline_missed_status_.total_count_change++;
    deadline_missed_status_.last_instance_handle = timer_owner_;
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::offered_deadline_missed();
    auto listener = get_listener_for(notify_status);

    if (listener != nullptr || user_listener_ != nullptr) {
        if (nullptr != user_listener_) {
            const vbs::OfferedDeadlineMissedStatus sta(deadline_missed_status_);
            user_listener_->on_offered_deadline_missed(user_datawriter_, sta);
        }
        if (nullptr != listener) {
            listener->on_offered_deadline_missed(this, deadline_missed_status_);
        }
        deadline_missed_status_.total_count_change = 0U;
    }

    get_statuscondition().get_impl()->set_status(notify_status, true);

    if (!history_->set_next_deadline_nts(
            timer_owner_, steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_))) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(DATA_WRITER, RetCode_t::RETCODE_ERROR, "Could not set the next deadline in the history");
        return false;
    }
    return deadline_timer_reschedule();
}

ReturnCode_t DataWriterImpl::get_offered_deadline_missed_status(evbs::edds::dds::OfferedDeadlineMissedStatus& status) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = deadline_missed_status_;
        deadline_missed_status_.total_count_change = 0U;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::offered_deadline_missed(), false);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataWriterImpl::get_offered_incompatible_qos_status(
    evbs::edds::dds::OfferedIncompatibleQosStatus& status) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = offered_incompatible_qos_status_;
        offered_incompatible_qos_status_.total_count_change = 0u;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::offered_incompatible_qos(), false);
    return ReturnCode_t::RETCODE_OK;
}

bool DataWriterImpl::lifespan_expired() {
    //std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    CacheChange_t* earliest_change;
    while (history_->get_earliest_change(&earliest_change)) {
        auto source_timestamp = system_clock::time_point() + nanoseconds(earliest_change->sourceTimestamp.to_ns());
        auto now = system_clock::now();

        // Check that the earliest change has expired (the change which started the timer could have
        // been removed from the history)
        if ((now - source_timestamp) < lifespan_duration_us_) {
            auto interval = source_timestamp - now + lifespan_duration_us_;
            (void)lifespan_timer_->update_interval_millisec(
                static_cast<double_t>(duration_cast<milliseconds>(interval).count()));
            return true;
        }
        const bool is_acked = history_->change_is_acked_or_fully_delivered(earliest_change);

        if (!is_acked) {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            logWarning(DATA_WRITER, "Lifespan expired in topic: " << writer_topic_->get_name());
        }

        // The earliest change has expired
        (void)history_->remove_change_pub(earliest_change);

        // Set the timer for the next change if there is one
        if (!history_->get_earliest_change(&earliest_change)) {
            return false;
        }

        // Calculate when the next change is due to expire and restart
        source_timestamp = system_clock::time_point() + nanoseconds(earliest_change->sourceTimestamp.to_ns());
        now = system_clock::now();
        auto interval = source_timestamp - now + lifespan_duration_us_;

        if (interval.count() > 0) {
            (void)lifespan_timer_->update_interval_millisec(
                static_cast<double_t>(duration_cast<milliseconds>(interval).count()));
            return true;
        }
    }

    return false;
}

ReturnCode_t DataWriterImpl::get_liveliness_lost_status(evbs::edds::dds::LivelinessLostStatus& status) {
    if (writer_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = liveliness_lost_status_;
        liveliness_lost_status_.total_count_change = 0U;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::liveliness_lost(), false);
    return ReturnCode_t::RETCODE_OK;
}

bool DataWriterImpl::assert_liveliness() {
    if (writer_ == nullptr) {
        return false;
    }

    return writer_->assert_liveliness() == vbs::ReturnCode_t::RETCODE_OK;
}

std::shared_ptr<evbs::ertps::TopicAttributes> DataWriterImpl::get_topic_attributes(
    const vbsutil::xmlparser::DataWriterQos& qos, const vbs::TopicImpl& topic, const TypeSupport& type) {
    std::shared_ptr<evbs::ertps::TopicAttributes> topic_att(new evbs::ertps::TopicAttributes);
    topic_att->qos.history(qos.history());
    topic_att->qos.resource_limits(qos.resource_limits());
    topic_att->topicName = topic.get_name();
    topic_att->topicDataType = topic.get_type_name();
    topic_att->topicKind = type->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    topic_att->m_crc16Idl = type->getIdlCrc16();
    return topic_att;
}

evbs::ertps::OfferedIncompatibleQosStatus& DataWriterImpl::update_offered_incompatible_qos(
    PolicyMask incompatible_policies) {
    ++offered_incompatible_qos_status_.total_count;
    ++offered_incompatible_qos_status_.total_count_change;
    for (uint32_t id = 1U; id < NEXT_QOS_POLICY_ID; ++id) {
        if (incompatible_policies.test(id)) {
            ++offered_incompatible_qos_status_.policies[static_cast<QosPolicyId_t>(id)].count;
            offered_incompatible_qos_status_.last_policy_id = static_cast<QosPolicyId_t>(id);
        }
    }
    return offered_incompatible_qos_status_;
}

inline evbs::ertps::LivelinessLostStatus& DataWriterImpl::update_liveliness_lost_status(
    const evbs::ertps::LivelinessLostStatus& liveliness_lost_status) {
    liveliness_lost_status_.total_count = liveliness_lost_status.total_count;
    liveliness_lost_status_.total_count_change += liveliness_lost_status.total_count_change;
    return liveliness_lost_status_;
}

void DataWriterImpl::set_qos(vbsutil::xmlparser::DataWriterQos& to, const vbsutil::xmlparser::DataWriterQos& from,
                             bool is_default) {
    if (is_default && (!(to.durability() == from.durability()))) {
        to.durability() = from.durability();
        to.durability().hasChanged = true;
    }
    if (!(to.deadline() == from.deadline())) {
        to.deadline() = from.deadline();
        to.deadline().hasChanged = true;
    }
    if (is_default && (!(to.liveliness() == from.liveliness()))) {
        to.liveliness() = from.liveliness();
        to.liveliness().hasChanged = true;
    }
    if (is_default && (!(to.reliability() == from.reliability()))) {
        to.reliability() = from.reliability();
        to.reliability().hasChanged = true;
    }
    if (is_default && (!(to.history() == from.history()))) {
        to.history() = from.history();
        to.history().hasChanged = true;
    }
    if (is_default && (!(to.resource_limits() == from.resource_limits()))) {
        to.resource_limits() = from.resource_limits();
        to.resource_limits().hasChanged = true;
    }
    if (!(to.lifespan() == from.lifespan())) {
        to.lifespan() = from.lifespan();
        to.lifespan().hasChanged = true;
    }
    if (is_default && (!(to.ownership() == from.ownership()))) {
        to.ownership() = from.ownership();
        to.ownership().hasChanged = true;
    }
    if (!(to.ownership_strength() == from.ownership_strength())) {
        to.ownership_strength() = from.ownership_strength();
        to.ownership_strength().hasChanged = true;
    }
    if (is_default && (!(to.publish_mode() == from.publish_mode()))) {
        to.publish_mode() = from.publish_mode();
    }

    if (is_default && (!(to.properties() == from.properties()))) {
        to.properties() = from.properties();
    }
    if (is_default && (!(to.reliable_writer_qos() == from.reliable_writer_qos()))) {
        to.reliable_writer_qos() = from.reliable_writer_qos();
    }
    if (is_default && (!(to.endpoint() == from.endpoint()))) {
        to.endpoint() = from.endpoint();
    }
    if (is_default && (!(to.writer_resource_limits() == from.writer_resource_limits()))) {
        to.writer_resource_limits() = from.writer_resource_limits();
    }
    if (is_default && !(to.batch_qos() == from.batch_qos())) {
        to.batch_qos() = from.batch_qos();
    }
}

ReturnCode_t DataWriterImpl::check_qos_including_resource_limits(const vbsutil::xmlparser::DataWriterQos& qos,
                                                                 const TypeSupport& type) {
    ReturnCode_t check_qos_return = check_qos(qos);
    if ((ReturnCode_t::RETCODE_OK == check_qos_return) && type->m_isGetKeyDefined) {
        check_qos_return = check_allocation_consistency(qos);
    }
    return check_qos_return;
}

ReturnCode_t DataWriterImpl::check_qos(const vbsutil::xmlparser::DataWriterQos& qos) {
    if ((qos.liveliness().kind == AUTOMATIC_LIVELINESS_QOS) ||
        (qos.liveliness().kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)) {
        if ((qos.liveliness().lease_duration < evbs::ertps::c_TimeInfinite) &&
            (qos.liveliness().lease_duration <= qos.liveliness().announcement_period)) {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                      "WRITERQOS: LeaseDuration " << qos.liveliness().lease_duration.to_ns()
                                                  << " <= announcement period "
                                                  << qos.liveliness().announcement_period.to_ns() << ".");
            return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
        }
    }
    if (qos.reliable_writer_qos().heartbeat_per_max_samples > qos.resource_limits().max_samples) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "heartbeat_per_max_samples should not be greater than max_samples.");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }

    if (qos.reliable_writer_qos().heartbeat_per_max_samples < 0) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "heartbeat_per_max_samples should not be less than zero.");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }

    if (qos.batch_qos().enable && (qos.publish_mode().kind == ASYNCHRONOUS_PUBLISH_MODE)) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "ASYNCHRONOUS_PUBLISH_MODE not support batch send");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataWriterImpl::check_allocation_consistency(const vbsutil::xmlparser::DataWriterQos& qos) {
    if ((qos.resource_limits().max_samples > 0) &&
        (qos.resource_limits().max_samples <
         (qos.resource_limits().max_instances * qos.resource_limits().max_samples_per_instance))) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be greater than max_instances * max_samples_per_instance");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    if (((qos.resource_limits().max_instances <= 0) || (qos.resource_limits().max_samples_per_instance <= 0)) &&
        (qos.resource_limits().max_samples > 0)) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be infinite when max_instances or "
                  "max_samples_per_instance are infinite");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    return ReturnCode_t::RETCODE_OK;
}

bool DataWriterImpl::can_qos_be_updated(const vbsutil::xmlparser::DataWriterQos& to,
                                        const vbsutil::xmlparser::DataWriterQos& from) {
    bool updatable = true;
    if (to.durability().kind != from.durability().kind) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Durability kind cannot be changed after the creation of a DataWriter.");
    }

    if (to.liveliness().kind != from.liveliness().kind) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Liveliness Kind cannot be changed after the creation of a DataWriter.");
    }

    if (to.liveliness().lease_duration != from.liveliness().lease_duration) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Liveliness lease duration cannot be changed after the creation of a DataWriter.");
    }

    if (to.liveliness().announcement_period != from.liveliness().announcement_period) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Liveliness announcement cannot be changed after the creation of a DataWriter.");
    }

    if (to.reliability().kind != from.reliability().kind) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Reliability Kind cannot be changed after the creation of a DataWriter.");
    }
    if (to.ownership().kind != from.ownership().kind) {
        updatable = false;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        logDebug(RTPS_QOS_CHECK, "Ownership Kind cannot be changed after the creation of a DataWriter.");
    }

    return updatable;
}

DataWriterListenerInner* DataWriterImpl::get_listener_for(const evbs::edds::dds::StatusMask& status) {
    if ((inner_listener_ != nullptr) && get_status_mask().is_active(status)) {
        return inner_listener_;
    }
    return publisher_->get_listener_for(status);
}

std::shared_ptr<vbs::common::IChangePool> DataWriterImpl::get_change_pool() const {
    PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);

    return std::make_shared<vbs::common::CacheChangePool>(config);
}

std::shared_ptr<vbs::common::IPayloadPool> DataWriterImpl::get_payload_pool() {
    if (!payload_pool_) {
        if (AUTOMATIC_MEMORY_MODE == history_->m_att.memoryPolicy) {
            history_->autoMemoryMode = true;
        }
        // When the user requested PREALLOCATED_WITH_REALLOC, but we know the type cannot
        // grow, we translate the policy into bare PREALLOCATED
        if (((PREALLOCATED_WITH_REALLOC_MEMORY_MODE == history_->m_att.memoryPolicy) ||
             (AUTOMATIC_MEMORY_MODE == history_->m_att.memoryPolicy)) &&
            ((type_->is_bounded() || type_->is_plain()) && !type_->is_big())) {
            history_->m_att.memoryPolicy = PREALLOCATED_MEMORY_MODE;
        }

        PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);
        if (type_->is_big()) {
            config.maximum_size = config.maximum_size * 2;
        }

        // Avoid calling the serialization size functors on PREALLOCATED mode
        fixed_payload_size_ = (config.memory_policy == PREALLOCATED_MEMORY_MODE) ? config.payload_initial_size : 0u;
#if !defined(_WIN32)
        auto transport_type = publisher_->get_participant()->get_qos().transport().transport_type;
        if (transport_type == vbsutil::xmlparser::TRANSPORT_TYPE_SHM) {
            auto domain_id = publisher_->get_participant()->get_domain_id();
            auto watt = get_writer_attributes();
            payload_pool_ =
                li::dsfdds::SharingPayloadFactory::create(domain_id, guid_, watt, writer_topic_->get_name());
        } else {
#endif
            payload_pool_ = TopicPayloadPoolRegistry::get(writer_topic_->get_name(), config);
            if (!std::static_pointer_cast<ITopicPayloadPool>(payload_pool_)->reserve_history(config, false)) {
                payload_pool_.reset();
            }
#if !defined(_WIN32)
        }
#endif
        loans_.reset(new LoanCollection(config));
    }

    return payload_pool_;
}

bool DataWriterImpl::release_payload_pool() {
    assert(payload_pool_);

    loans_.reset();

    bool result = true;

    {
        PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);
        auto topic_pool = std::static_pointer_cast<ITopicPayloadPool>(payload_pool_);
        result = topic_pool->release_history(config, false);
    }

    payload_pool_.reset();

    return result;
}

inline bool DataWriterImpl::add_loan(void* data, PayloadInfo_t& payload) {
    return loans_ && loans_->add_loan(data, payload);
}

inline bool DataWriterImpl::check_and_remove_loan(void* data, PayloadInfo_t& payload) {
    return loans_ && loans_->check_and_remove_loan(data, payload);
}

void DataWriterImpl::remove_reader_filter(const evbs::ertps::rtps::GUID_t& reader_guid) {
    (void)reader_guid;
    if (reader_filters_) {
        assert(writer_);
        reader_filters_->remove_reader_filter(reader_guid);
        logDebug(DATA_WRITER,
                 "Filter reader removed, topic " << writer_topic_->get_name() << " reader " << reader_guid);
    }
}

void DataWriterImpl::add_reader_filter(const evbs::ertps::rtps::GUID_t& reader_guid,
                                       const evbs::ertps::rtps::ContentFilterProperty& filter_property) {
    if (reader_filters_) {
        reader_filters_->add_reader_filter(reader_guid, filter_property);
        std::string params;
        for (auto& param : filter_property.expression_parameters) {
            params += param.to_string() + ",";
        }
        if (!filter_property.filter_expression.empty()) {
            logInfo(DATA_WRITER, "Filter reader added, topic "
                                     << writer_topic_->get_name() << " reader " << reader_guid << " filter_expression:"
                                     << filter_property.filter_expression << " params:" << params);
        }
    }
}

bool DataWriterImpl::is_relevant(const evbs::ertps::rtps::CacheChange_t& change,
                                 const evbs::ertps::rtps::GUID_t& reader_guid) const {
    return change.is_relevant_for(reader_guid);
}

bool DataWriterImpl::check_filter_signature(evbs::ertps::rtps::CacheChange_t& change,
                                            const vbsutil::xmlparser::ContentFilterProperty& filter_property,
                                            const evbs::ertps::rtps::GUID_t& reader_guid) {
    if (reader_filters_ == nullptr || filter_property.filter_expression.empty()) {
        return true;
    }

    if (!reader_filters_->has_reader_filter(reader_guid)) {
        reader_filters_->add_reader_filter(reader_guid, filter_property);
    }

    // 后上线且guid发生变化时，根据signature过滤
    bool found = true;
    change.inline_qos.pos = 0;
    std::array<uint8_t, 16> filter_signature = {0};
    ContentFilterUtils::compute_signature(filter_property, filter_signature);
    ContentFilterUtils::check_filter_signature(change, filter_signature, found);
    if (!found) {
        change.filtered_out_readers.emplace_back(reader_guid);
        return found;
    }

    // 后上线且过滤规则也发生变化时，只能重新反序列化过滤
    reader_filters_->update_filter_info(change);

    found = is_relevant(change, reader_guid);
    return found;
}

WriterAttributes DataWriterImpl::get_writer_attributes() const {
    WriterAttributes w_att;
    w_att.endpoint.durabilityKind = qos_.durability().durabilityKind();
    w_att.endpoint.endpointKind = WRITER;
    w_att.endpoint.reliabilityKind =
        (qos_.reliability().kind == vbsutil::xmlparser::RELIABLE_RELIABILITY_QOS) ? RELIABLE : BEST_EFFORT;
    w_att.endpoint.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    w_att.endpoint.multicastLocatorList = qos_.endpoint().multicast_locator_list;
    w_att.endpoint.unicastLocatorList = qos_.endpoint().unicast_locator_list;
    w_att.endpoint.remoteLocatorList = qos_.endpoint().remote_locator_list;
    w_att.endpoint.external_unicast_locators = qos_.endpoint().external_unicast_locators;
    w_att.endpoint.ignore_non_matching_locators = qos_.endpoint().ignore_non_matching_locators;
    w_att.mode = (qos_.publish_mode().kind == SYNCHRONOUS_PUBLISH_MODE) ? SYNCHRONOUS_WRITER : ASYNCHRONOUS_WRITER;
    w_att.flow_controller_name = qos_.publish_mode().flow_controller_name;
    w_att.endpoint.properties = qos_.properties();
    w_att.endpoint.ownershipKind = qos_.ownership().kind;
    w_att.endpoint.setEntityID(qos_.endpoint().entity_id);
    w_att.endpoint.setUserDefinedID(qos_.endpoint().user_defined_id);
    w_att.times = qos_.reliable_writer_qos().times;
    w_att.liveliness_kind = qos_.liveliness().kind;
    w_att.liveliness_lease_duration = qos_.liveliness().lease_duration;
    w_att.liveliness_announcement_period = qos_.liveliness().announcement_period;
    w_att.matched_readers_allocation = qos_.writer_resource_limits().matched_subscriber_allocation;
    w_att.disable_heartbeat_piggyback = qos_.reliable_writer_qos().disable_heartbeat_piggyback;
    w_att.heartbeat_per_max_samples = qos_.reliable_writer_qos().heartbeat_per_max_samples;
    w_att.high_watermark = qos_.reliable_writer_qos().high_watermark;
    w_att.low_watermark = qos_.reliable_writer_qos().low_watermark;
    w_att.enable_e2e_protection = qos_.e2e_protection().enabled;
    w_att.e2e_p04_min_data_length = qos_.e2e_protection().MinDataLength;
    w_att.e2e_p04_max_data_length = qos_.e2e_protection().MaxDataLength;
    w_att.e2e_p04_max_delta_counter = qos_.e2e_protection().MaxDeltaCounter;
    w_att.trace_point_status = type_->getSoaLogFilter();
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(writer_topic_->get_name().c_str());
    w_att.endpoint.properties.properties().push_back(std::move(property));

    const std::string* endpoint_partitions = PropertyPolicyHelper::find_property(qos_.properties(), "partitions");

    if (endpoint_partitions != nullptr) {
        property.name("partitions");
        property.value(*endpoint_partitions);
        w_att.endpoint.properties.properties().push_back(std::move(property));
    } else if (publisher_->get_qos().partition().names().size() > 0U) {
        property.name("partitions");
        std::string partitions;
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        bool is_first_partition = true;
        for (auto& partition : publisher_->get_qos().partition().names()) {
            partitions += (is_first_partition ? "" : ";") + partition;
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            is_first_partition = false;
        }
        property.value(std::move(partitions));
        w_att.endpoint.properties.properties().push_back(std::move(property));
    }

    if (qos_.reliable_writer_qos().disable_positive_acks.enabled &&
        (qos_.reliable_writer_qos().disable_positive_acks.duration != c_TimeInfinite)) {
        w_att.disable_positive_acks = true;
        w_att.keep_duration = qos_.reliable_writer_qos().disable_positive_acks.duration;
    }
    return w_att;
}

void DataWriterImpl::get_statistic_send_info(builtin::StatisticSendInfo* info) {
    if (writer_ && writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS)) {
        info->send_count =
            dynamic_cast<RTPSWriter*>(writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_send_count();
        info->send_throughput =
            dynamic_cast<RTPSWriter*>(writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_throughput();
        info->send_payload_length_avg =
            dynamic_cast<RTPSWriter*>(writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_avg_payload_length();
        info->last_pkt_timestamp =
            dynamic_cast<RTPSWriter*>(writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_last_pkt_timestamp();
        info->first_pkt_timestamp =
            dynamic_cast<RTPSWriter*>(writer_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_first_pkt_timestamp();
    }
}

DomainParticipantImpl* DataWriterImpl::get_paritcipantimpl() {
    return writer_participant_;
}

DataWriterImpl::DataWriterImpl(DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                               const std::string& profile_name, DataWriterListener* const listener,
                               const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())),
      user_listener_(listener),
      listener_wrapper_(this),
      dds_match_(0),
      writer_participant_(local_participant),
      writer_topic_(local_topic),
      wparams_(new WriteParams),
      match_count_(0) {
    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        return;
    }

    writer_topic_->get_impl()->reference();
    type_ = *(local_topic->get_type());
    writer_guid_ = vbs_guid_.generate();
    dsf_wparams_.guid(writer_guid_);

    if ((writer_participant_ != nullptr) && (writer_topic_ != nullptr)) {
        const PublisherQos dds_publisher_qos;
        publisher_ = writer_participant_->create_publisher(dds_publisher_qos, nullptr);
        bool result = false;
        auto qos = vbs::QosManager::Instance()->get_writer_xml_qos(profile_name);
        if (qos != nullptr) {
            if (qos->e2e_protection().enabled) {
                if (type_->getSendPeriod() != 0) {
                    qos->deadline().period.from_ns(type_->getSendPeriod() * 1e+6 * qos->deadline().counter);
                    qos->liveliness().lease_duration = {TIME_T_INFINITE_SECONDS, TIME_T_INFINITE_NANOSECONDS};
                } else if (!qos->liveliness().using_e2e_config) {
                    qos->liveliness().lease_duration.from_ns(500 * 1e+6);
                    qos->liveliness().kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
                }
            }
            result = datawriter_init_with_profile(writer_topic_, *qos, false);
        }

        if (result == true) {
            dds_is_initialized_ = true;
            vbs_qos_.dds_writer_qos(qos_);
        } else {
            writer_participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }
    }
}

bool DataWriterImpl::datawriter_init_with_profile(vbs::TopicImpl* topic,
                                                  const vbsutil::xmlparser::DataWriterQos& qos_input,
                                                  bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    vbsutil::xmlparser::DataWriterQos qos = qos_input;
    if (topic->get_qos().reliability_from_xml()) {
        qos.reliability().kind = topic->get_qos().reliability().kind;
    }
    if (topic->get_qos().durability_from_xml()) {
        qos.durability().kind = topic->get_qos().durability().kind;
    }
    if (topic->get_qos().deadline_from_xml()) {
        qos.deadline().period = topic->get_qos().deadline().period;
    }
    if (topic->get_qos().lifespan_from_xml()) {
        qos.lifespan().duration = topic->get_qos().lifespan().duration;
    }
    if (topic->get_qos().ownership_from_xml()) {
        qos.ownership().kind = topic->get_qos().ownership().kind;
    }
    if (topic->get_qos().history_from_xml()) {
        qos.history() = topic->get_qos().history();
    }
    if (topic->get_qos().resource_from_xml()) {
        qos.resource_limits() = topic->get_qos().resource_limits();
    }

    if ((static_cast<vbs::TopicImpl*>(topic))->get_qos().send_multi_from_xml()) {
        qos.send_multi((static_cast<vbs::TopicImpl*>(topic))->get_qos().send_multi());
    }
    logInfo(PUBLISHER, "Writer creating in topic " << topic->get_name());
    return datawriter_init(topic, qos, auto_enable);
}

bool DataWriterImpl::datawriter_init(vbs::TopicImpl* topic, const vbsutil::xmlparser::DataWriterQos& qos,
                                     bool auto_enable) {
    // Look for the correct type registration
    TypeSupport type_support = writer_participant_->find_type(topic->get_type_name());

    /// Preconditions
    // Check the type was registered.
    if (type_support.empty()) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        elogError(PUBLISHER, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Type: " << topic->get_type_name() << " Not Registered");
        return false;
    }

    if (!DataWriterImpl::check_qos_including_resource_limits(qos, type_support)) {
        return false;
    }

    if (&qos == &vbsutil::xmlparser::getDefaultDataWriteQos()) {
        qos_ = publisher_->get_default_datawriter_qos();
    } else {
        qos_ = qos;
    }

    history_ = new DataWriterHistory((*get_topic_attributes(qos_, *topic, type_)), type_->m_typeSize,
                                     qos_.endpoint().history_memory_policy,
                                     [this](const InstanceHandle_t& handle) -> void {
                                         if (nullptr != inner_listener_) {
                                             inner_listener_->on_unacknowledged_sample_removed(this, handle);
                                         }
                                     },
                                     qos.batch_qos());
    deadline_duration_us_ =
        std::chrono::duration<double_t, std::ratio<1, 1000000>>(qos.deadline().period.to_ns() * 1e-3);
    lifespan_duration_us_ =
        std::chrono::duration<double_t, std::ratio<1, 1000000>>(qos_.lifespan().duration.to_ns() * 1e-3);

    EndpointAttributes endpoint_attributes;
    endpoint_attributes.endpointKind = WRITER;
    endpoint_attributes.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    endpoint_attributes.setEntityID(qos_.endpoint().entity_id);
    endpoint_attributes.setUserDefinedID(qos_.endpoint().user_defined_id);

    (void)DomainParticipantImpl::preprocess_endpoint_attributes<WRITER, 0x03, 0x02>(
        EntityId_t::unknown(), publisher_->get_participant()->id_counter(), endpoint_attributes, guid_.entityId);

    guid_.guidPrefix = publisher_->get_participant()->guid().guidPrefix;

    qos.exportDataWriterQosInfo();

    publisher_->add_datawriter(topic, this);

    if (is_enabled() && auto_enable) {
        if (ReturnCode_t::RETCODE_OK != enable()) {
            (void)publisher_->delete_datawriter(this);
            return false;
        }
    }

    // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
    // VbsLogI("Writer " << writer->guid() <<
    //          " created in topic: " << topic->get_name() << " mask:" << get_status_mask() << " enable:" << auto_enable <<
    //          " listener:" << (listener != nullptr));

    return true;
}

DataWriterImpl::DataWriterImpl(DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                               const vbs::DataWriterQosImpl& local_data_writer_qos,
                               vbs::DataWriterListener* const listener, const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())),
      user_listener_(listener),
      listener_wrapper_(this),
      dds_match_(0),
      writer_participant_(local_participant),
      writer_topic_(local_topic),
      wparams_(new vbs::WriteParams),
      match_count_(0) {
    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        return;
    }

    writer_topic_->get_impl()->reference();
    type_ = *(local_topic->get_type());
    writer_guid_ = vbs_guid_.generate();
    dsf_wparams_.guid(writer_guid_);

    const PublisherQos dds_publisher_qos = vbsutil::xmlparser::getDefaultPublisherQos();
    publisher_ = writer_participant_->create_publisher(dds_publisher_qos, nullptr);
    bool result = datawriter_init(writer_topic_, local_data_writer_qos.get_dds_writer_qos());
    if (result != false) {
        dds_is_initialized_ = true;
        vbs_qos_.dds_writer_qos(qos_);
    } else {
        writer_participant_->delete_publisher(publisher_);
        publisher_ = nullptr;
    }
}

void DataWriterImpl::set_wparams_id(uint64_t id) {
    int32_t tmp = dsf_wparams_.guid();
    wparams_->related_sample_identity().sequence_number().high =
        (int32_t)((id & 0xFFFFFFFF00000000) >> 32) | (int32_t)((tmp & 0x0000FFFF) << 16);
    wparams_->related_sample_identity().sequence_number().low = (int32_t)((id & 0xFFFFFFFF));
}

bool DataWriterImpl::write(const vbs::VBSDynamicData* data, uint32_t len) {
    (void)len;
    if (data == nullptr) {
        return false;
    }
    bool ret = false;
    if (dds_is_initialized_) {
        uint64_t framework_id = writer_topic_->framework_seq_id_.load();       
        set_wparams_id(framework_id);
        ret = write(data->vbs_dynamic_data_, *wparams_);
#if !defined(_WIN32)
        if (ret) {
            writer_topic_->framework_seq_id_.fetch_add(1);
        }
#endif
        if (!ret) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "DDS send DynamicData failure, ret " << std::to_string(vbs::ReturnCode_t::RETCODE_ERROR));
        } else {
            if (framework_id == 1) {
                VbsLogI("First package of topic: \"" << writer_topic_->topic_name_ << "\" write successfully!");
            }
        }
    }
    if (ret == true && writer_topic_) {
        writer_topic_->update_network_traffic(type_->m_typeSize, 1, vbs::TRAFFIC_TYPE::SEND_TRAFFIC);
    }
    return ret;
}

bool DataWriterImpl::write(void* const data, const uint32_t& len) {
    (void)len;
    if (data == nullptr) {
        return false;
    }
    bool ret = false;
    if (dds_is_initialized_) {
        uint64_t framework_id = writer_topic_->framework_seq_id_.load();
        std::cout << "DataWriterImpl::writeframework_id:" << framework_id << "  point:  " << data << "  len:  " <<  len 
        << " topic_id_: " << wparams_->topic_id() << std::endl;
        ret = write(data, *wparams_);
        if (ret == false) {
            VbsLogW("DDS Write failure. framework id is " << std::to_string(framework_id));
        } else {
            if (framework_id == 1) {
                VbsLogI("First package of topic: \"" << writer_topic_->topic_name_ << "\" write successfully!");
            }
#if !defined(_WIN32)
            writer_topic_->framework_seq_id_.fetch_add(1);
#endif
        }
    }
    if (ret == true && writer_topic_) {
        writer_topic_->update_network_traffic(type_->m_typeSize, 1, vbs::TRAFFIC_TYPE::SEND_TRAFFIC);
    }
    return ret;
}

bool DataWriterImpl::write_buffer(void* const data, const uint32_t& len) {
    if (data == nullptr) {
        return false;
    }
    bool ret = false;
    if (dds_is_initialized_) {
        uint64_t framework_id = writer_topic_->framework_seq_id_.load();
        set_wparams_id(framework_id);
        ret = write_buffer(data, *wparams_, len);
        if (ret == false) {
            VbsLogW("DDS Write failure. framework id is " << std::to_string(framework_id));
        } else {
            if (framework_id == 1) {
                VbsLogI("First package of topic: \"" << writer_topic_->topic_name_ << "\" write successfully!");
            }
#if !defined(_WIN32)
            writer_topic_->framework_seq_id_.fetch_add(1);
#endif
        }
    }
    if (ret == true && writer_topic_) {
        writer_topic_->update_network_traffic(type_->m_typeSize, 1, vbs::TRAFFIC_TYPE::SEND_TRAFFIC);
    }
    return ret;
}

const DataWriterQosImpl& DataWriterImpl::get_qos() const {
    return vbs_qos_;
}

bool DataWriterImpl::GetQos(vbs::DataWriterQosImpl& qos) {
    qos = vbs_qos_;
    return true;
}

bool DataWriterImpl::SetQos(const vbs::DataWriterQosImpl& qos) {
    vbs_qos_ = qos;
    return true;
}

uint16_t DataWriterImpl::get_guid() {
    return writer_guid_;
}

bool DataWriterImpl::serialize(void* const data, void* const payload_buffer, uint32_t* const payload_len) {
    SerializedPayload_t payload;
    payload.data = reinterpret_cast<octet*>(payload_buffer);
    payload.max_size = *payload_len;
    const bool res = type_->serialize(data, &payload);
    *payload_len = payload.length;
    payload.data = nullptr;
    return res;
}

bool DataWriterImpl::init_verify() {
    return dds_is_initialized_;
}

void DataWriterImpl::set_user_listener(vbs::DataWriterListener* listener) {
    set_user_listener(listener, StatusMask::all());
}

void DataWriterImpl::set_user_listener(vbs::DataWriterListener* const listener, const StatusMask& mask) {
    user_listener_ = listener;
    status_mask_ = *(mask.get_dds_status_mask());
}

void DataWriterImpl::on_publication_matched(DataWriterImpl* writer,
                                            const evbs::edds::dds::PublicationMatchedStatus& info) {
    (void)writer;
    //const std::lock_guard<std::mutex> lock(get_parent()->on_publication_matched_mutex_);
    vbs_qos_.dds_writer_qos(get_dds_qos());
    if (info.current_count_change == 1) {
        dds_match_ = dds_match_ + 1;
        match_count_ = match_count_ + 1;
    } else if (info.current_count_change == -1) {
        dds_match_ = dds_match_ - 1;
        match_count_ = match_count_ - 1;
    } else {
        VbsLogW("DDS on_publication_matched current_count_change is not except");
    }
    VbsDDSPublicationMatchedStatus info_w = info;
    info_w.current_count = match_count_;
    const vbs::PublicationMatchedStatus winfo(info_w);
    if (user_listener_ != nullptr) {
        user_listener_->on_publication_matched(user_datawriter_, winfo);
    }
}

void DataWriterImpl::on_offered_deadline_missed(DataWriterImpl* writer,
                                                const evbs::edds::dds::OfferedDeadlineMissedStatus& status) {
    (void)writer;
    if (user_listener_ != nullptr) {
        const vbs::OfferedDeadlineMissedStatus sta(status);
        user_listener_->on_offered_deadline_missed(user_datawriter_, sta);
    }
}

void DataWriterImpl::on_liveliness_lost(DataWriterImpl* writer, const evbs::edds::dds::LivelinessLostStatus& status) {
    (void)writer;
    if (user_listener_ != nullptr) {
        const vbs::LivelinessLostStatus status_now(status);
        user_listener_->on_liveliness_lost(user_datawriter_, status_now);
    }
}

void DataWriterImpl::on_endpoint_crash(DataWriterImpl* writer, const evbs::ertps::rtps::GUID_t& reader_guid) {
    if (user_listener_ != nullptr) {
        const vbs::EndpointCrashStatus status(writer_participant_->get_domain_id(), reader_guid);
        user_listener_->on_endpoint_crash(user_datawriter_, status);
    }
}

}  // namespace vbs
