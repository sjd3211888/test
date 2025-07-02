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
// feature: remove reactor
// feature: support e2e deadline and liveliness 1. config deadline sendPeriod 2. enable deadline event
// feature: change name NewusContext to NexusContext
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: Reduce level of DataReaderListenerInner
// feature: implement ContentFilteredTopic
// feature: use environment variable to control system discovery.
// feature: Enable trace and log control with udp
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: VBS framework adapt to system test tool
// feature: separating interface and implementation about DataReader
// feature: clean do_coverity
// feature: System discovery supports version discovery and detection
// feature: activate global discovery in vbs
// feature: Upload trace data
// feature: record-replay dsf dynamic discovery with xml
// ------------------------------------------------------------------

#include <memory>
#include <stdexcept>
#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#include "edds/rtps/RTPSDomain.h"
#include "dispatcher/DispatcherReader.hpp"
#include "deps/resources/ResourceEvent.h"
#include "deps/resources/TimedEvent.h"
#include "ertps/utils/TimeConversion.h"
#include "ertps/subscriber/SampleInfo.h"
#include "dispatcher/DispatcherParticipant.hpp"
#include "ertps/config.h"
#include "core/StackAllocatedSequence.hpp"
#include "elog/Log.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/SampleInfoInner.hpp"
#include "sub/Subscriber.hpp"
#include "sub/SubscriberListener.hpp"
#include "topic/TopicImpl.hpp"
#include "deps/common/TypeSupport.hpp"
#include "topic/ContentFilteredTopicCore.hpp"
#include "core/condition/StatusConditionImpl.hpp"
#include "sub/DataReaderImpl/ReadTakeCommand.hpp"
#include "sub/DataReaderImpl/StateFilter.hpp"
#include "sub/ReadConditionImpl.hpp"
#include "history/TopicPayloadPoolRegistry.hpp"
#include "history/CacheChangePool.h"
#include "domain/DomainParticipantImpl.hpp"
#include "context/NexusContext.hpp"
#include "context/QosManager.hpp"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/status/E2EExceptionStatus.hpp"
#include "topic/ContentFilteredTopicImpl.hpp"
#include "core/LoanableCollectionImpl.hpp"

using evbs::ertps::c_TimeInfinite;
using namespace vbs::common;
using namespace evbs::ertps;
using namespace evbs::ertps::rtps;
using namespace std::chrono;

using evbs::ReturnCode_t;

namespace vbs {

static bool collections_have_same_properties(const evbs::edds::dds::LoanableCollection& data_values,
                                             const vbs::LoanableSequence<vbs::SampleInfoInner>& sample_infos) {
    return ((data_values.has_ownership() == sample_infos.has_ownership()) &&
            (data_values.maximum() == sample_infos.maximum()) && (data_values.length() == sample_infos.length()));
}

static bool qos_has_unique_network_request(const vbsutil::xmlparser::DataReaderQos& qos) {
    return nullptr != PropertyPolicyHelper::find_property(qos.properties(), "edds.unique_network_flows");
}

static bool qos_has_specific_locators(const vbsutil::xmlparser::DataReaderQos& qos) {
    const vbsutil::xmlparser::RTPSEndpointQos& endpoint = qos.endpoint();
    return (!endpoint.unicast_locator_list.empty()) || (!endpoint.multicast_locator_list.empty()) ||
           (!endpoint.remote_locator_list.empty());
}

DataReaderImpl::DataReaderImpl(Subscriber* s, const TypeSupport& type, vbs::TopicDescription* topic,
                               const vbsutil::xmlparser::DataReaderQos& qos, DataReaderListenerInner* listener,
                               const evbs::edds::dds::StatusMask& mask)
    : DomainEntity(mask),
      subscriber_(s),
      type_(type),
      topic_(topic),
      qos_(&qos == &vbsutil::xmlparser::getDefaultDataReaderQos() ? subscriber_->get_default_datareader_qos() : qos),
#pragma warning(disable : 4355)
      history_(new vbs::detail::DataReaderHistory(type, *topic, qos_)),
      listener_(listener),
      inner_listener_(this),
      deadline_duration_us_(qos_.deadline().period.to_ns() * 1e-3),
      lifespan_duration_us_(qos_.lifespan().duration.to_ns() * 1e-3),
      sample_info_pool_(new vbs::detail::SampleInfoPool(qos)),
      loan_manager_(new vbs::detail::DataReaderLoanManager(qos)) {
    topic_->get_impl()->reference();
    EndpointAttributes endpoint_attributes;
    endpoint_attributes.endpointKind = READER;
    endpoint_attributes.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    endpoint_attributes.setEntityID(qos_.endpoint().entity_id);
    endpoint_attributes.setUserDefinedID(qos_.endpoint().user_defined_id);
    (void)vbs::DomainParticipantImpl::preprocess_endpoint_attributes<READER, 0x04, 0x07>(
        EntityId_t::unknown(), subscriber_->get_participant()->id_counter(), endpoint_attributes, guid_.entityId);
    guid_.guidPrefix = subscriber_->get_participant()->guid().guidPrefix;
    dds_is_initialized_ = true;
}

std::shared_ptr<vbs::common::IChangePool> DataReaderImpl::get_change_pool() const {
    PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);

    return std::make_shared<vbs::common::CacheChangePool>(config);
}

evbs::ReturnCode_t DataReaderImpl::enable() {
    if (dds_is_initialized_ == false) {
        return evbs::ReturnCode_t::RETCODE_ERROR;
    }

    if (enable_) {
        return evbs::ReturnCode_t::RETCODE_OK;
    }

    if (false == get_subscriber()->is_enabled()) {
        return evbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    evbs::ertps::rtps::ReaderAttributes att;

    att.endpoint.durabilityKind = qos_.durability().durabilityKind();
    att.endpoint.endpointKind = READER;
    att.endpoint.reliabilityKind = qos_.reliability().kind == vbsutil::xmlparser::RELIABLE_RELIABILITY_QOS
                                       ? vbsutil::xmlparser::RELIABLE
                                       : vbsutil::xmlparser::BEST_EFFORT;
    att.endpoint.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    att.endpoint.multicastLocatorList = qos_.endpoint().multicast_locator_list;
    att.endpoint.unicastLocatorList = qos_.endpoint().unicast_locator_list;
    att.endpoint.remoteLocatorList = qos_.endpoint().remote_locator_list;
    att.endpoint.external_unicast_locators = qos_.endpoint().external_unicast_locators;
    att.endpoint.ignore_non_matching_locators = qos_.endpoint().ignore_non_matching_locators;
    att.endpoint.properties = qos_.properties();
    att.endpoint.ownershipKind = qos_.ownership().kind;
    att.endpoint.setEntityID(qos_.endpoint().entity_id);
    att.endpoint.setUserDefinedID(qos_.endpoint().user_defined_id);
    att.times = qos_.reliable_reader_qos().times;
    att.liveliness_lease_duration = qos_.liveliness().lease_duration;
    att.liveliness_kind_ = qos_.liveliness().kind;
    att.matched_writers_allocation = qos_.reader_resource_limits().matched_publisher_allocation;
    att.disable_positive_acks = qos_.reliable_reader_qos().disable_positive_ACKs.enabled;
    att.enable_e2e_protection = qos_.e2e_protection().enabled;
    att.e2e_p04_min_data_length = qos_.e2e_protection().MinDataLength;
    att.e2e_p04_max_data_length = qos_.e2e_protection().MaxDataLength;
    att.e2e_p04_max_delta_counter = qos_.e2e_protection().MaxDeltaCounter;
    att.dest_order_reception = (qos_.destination_order().kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
    att.trace_point_status = type_->getSoaLogFilter();
    att.endpoint.type_is_plain = type_->is_plain();
    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(topic_->get_impl()->get_rtps_topic_name().c_str());
    att.endpoint.properties.properties().push_back(std::move(property));

    std::string* endpoint_partitions = PropertyPolicyHelper::find_property(qos_.properties(), "partitions");

    if (endpoint_partitions) {
        property.name("partitions");
        property.value(*endpoint_partitions);
        att.endpoint.properties.properties().push_back(std::move(property));
    } else if (subscriber_->get_qos().partition().names().size() > 0U) {
        property.name("partitions");
        std::string partitions;
        bool is_first_partition = true;
        for (auto& partition : subscriber_->get_qos().partition().names()) {
            partitions += (is_first_partition ? "" : ";") + partition;
            is_first_partition = false;
        }
        property.value(std::move(partitions));
        att.endpoint.properties.properties().push_back(std::move(property));
    }

    history_->topicName = get_topicdescription()->get_name();
    history_->guid = guid();

    if (AUTOMATIC_MEMORY_MODE == history_->m_att.memoryPolicy) {
        history_->autoMemoryMode = true;
    }
    if (((PREALLOCATED_WITH_REALLOC_MEMORY_MODE == history_->m_att.memoryPolicy) ||
         (AUTOMATIC_MEMORY_MODE == history_->m_att.memoryPolicy)) &&
        ((type_->is_bounded() || type_->is_plain()) && !type_->is_big())) {
        history_->m_att.memoryPolicy = PREALLOCATED_MEMORY_MODE;
    }
    PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);
    sample_pool_ = std::make_shared<detail::SampleLoanManager>(config, type_);
    change_pool_ = get_change_pool();
    if (change_pool_ == nullptr) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Problem creating change pool");
        return evbs::ReturnCode_t::RETCODE_ERROR;
    }

    ReaderQos rqos = qos_.get_readerqos(subscriber_->get_qos());
    std::shared_ptr<evbs::ertps::TopicAttributes> topic_attr_ptr = topic_attributes();

    vbs::dispatcher::DispatcherReader* reader = subscriber_->dispatcher_participant()->create_reader(
        guid_, att, *topic_attr_ptr, rqos, change_pool_, history_, static_cast<ReaderListener*>(&inner_listener_));

    if (reader == nullptr) {
        //release_payload_pool();
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Problem creating associated Reader");
        return evbs::ReturnCode_t::RETCODE_ERROR;
    }

    history_->mp_reader = reader;

//todo dispatcher
#if 0
    reader->setSendMulti(qos_.send_multi());
    logInfo(DATA_READER, "Reader enabled in topic " << get_topicdescription()->get_name() <<
        " send_multi:" << reader->getSendMulti() << " guid:" << reader->getGuid());
#endif

    auto content_topic = dynamic_cast<vbs::ContentFilteredTopicCore*>(topic_->get_impl());
    if (nullptr != content_topic) {
        static_cast<evbs::ertps::rtps::RTPSReader*>(reader->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))
            ->reader_data_filter(content_topic);
        content_topic->add_reader(this);
    }
    reader_ = reader;

    deadline_timer_ = new TimedEvent(
        subscriber_->get_participant()->get_resource_event(), [&]() -> bool { return deadline_missed(); },
        qos_.deadline().period.to_ns() * 1e-6);

    lifespan_timer_ = new TimedEvent(
        subscriber_->get_participant()->get_resource_event(), [&]() -> bool { return lifespan_expired(); },
        qos_.lifespan().duration.to_ns() * 1e-6);

    // Register the reader

    if (endpoint_partitions) {
        std::istringstream partition_string(*endpoint_partitions);
        std::string partition_name;
        rqos.m_partition.clear();

        while (std::getline(partition_string, partition_name, ';')) {
            rqos.m_partition.push_back(partition_name.c_str());
        }
    }

    vbsutil::xmlparser::ContentFilterProperty* filter_property = nullptr;
    if (nullptr != content_topic && !content_topic->filter_property.filter_expression.empty()) {
        filter_property = &content_topic->filter_property;
    }

    if (!reader_->registerReader(filter_property)) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Could not register reader on discovery protocols");

        (void)reader_->setListener(nullptr);
        stop();

        return evbs::ReturnCode_t::RETCODE_ERROR;
    }

    enable_ = true;
    return evbs::ReturnCode_t::RETCODE_OK;
}

void DataReaderImpl::disable() {
    (void)set_listener(nullptr);
    if (reader_ != nullptr) {
        (void)reader_->setListener(nullptr);
    }
}

ReturnCode_t DataReaderImpl::read(evbs::edds::dds::LoanableCollection& data_values,
                                  evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                  vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                  vbs::InstanceStateMask instance_states) {
    return READ(data_values, sample_infos, max_samples, sample_states, view_states, instance_states);
}

ReturnCode_t DataReaderImpl::read_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                              ReadCondition* a_condition) {
    if (nullptr == a_condition) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return READ(data_values, sample_infos, max_samples, a_condition->get_sample_state_mask(),
                a_condition->get_view_state_mask(), a_condition->get_instance_state_mask());
}

ReturnCode_t DataReaderImpl::read_instance(evbs::edds::dds::LoanableCollection& data_values,
                                           evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                           const InstanceHandle_t& a_handle, vbs::SampleStateMask sample_states,
                                           vbs::ViewStateMask view_states, vbs::InstanceStateMask instance_states) {
    return READ_INSTANCE(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states);
}

ReturnCode_t DataReaderImpl::read_next_instance(evbs::edds::dds::LoanableCollection& data_values,
                                                evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                                const InstanceHandle_t& previous_handle,
                                                vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                                vbs::InstanceStateMask instance_states) {
    return READ_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states,
                              instance_states);
}

ReturnCode_t DataReaderImpl::read_next_instance_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                            int32_t max_samples,
                                                            const InstanceHandle_t& previous_handle,
                                                            ReadCondition* a_condition) {
    if (nullptr == a_condition) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return READ_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle,
                              a_condition->get_sample_state_mask(), a_condition->get_view_state_mask(),
                              a_condition->get_instance_state_mask());
}

ReturnCode_t DataReaderImpl::take(evbs::edds::dds::LoanableCollection& data_values,
                                  evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                  vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                  vbs::InstanceStateMask instance_states) {
    return TAKE(data_values, sample_infos, max_samples, sample_states, view_states, instance_states);
}

ReturnCode_t DataReaderImpl::take_buffer(evbs::edds::dds::LoanableCollection& data_values,
                                         evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                         vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                         vbs::InstanceStateMask instance_states) {
    return TAKE_BUFFER(data_values, sample_infos, max_samples, sample_states, view_states, instance_states);
}

ReturnCode_t DataReaderImpl::take_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                              evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                              ReadCondition* a_condition) {
    if (nullptr == a_condition) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return TAKE(data_values, sample_infos, max_samples, a_condition->get_sample_state_mask(),
                a_condition->get_view_state_mask(), a_condition->get_instance_state_mask());
}

ReturnCode_t DataReaderImpl::take_instance(evbs::edds::dds::LoanableCollection& data_values,
                                           evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                           const InstanceHandle_t& a_handle, vbs::SampleStateMask sample_states,
                                           vbs::ViewStateMask view_states, vbs::InstanceStateMask instance_states) {
    return TAKE_INSTANCE(data_values, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states);
}

ReturnCode_t DataReaderImpl::take_next_instance(evbs::edds::dds::LoanableCollection& data_values,
                                                evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                                const InstanceHandle_t& previous_handle,
                                                vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                                vbs::InstanceStateMask instance_states) {
    return TAKE_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle, sample_states, view_states,
                              instance_states);
}

ReturnCode_t DataReaderImpl::take_next_instance_w_condition(evbs::edds::dds::LoanableCollection& data_values,
                                                            evbs::edds::dds::SampleInfoSeq& sample_infos,
                                                            int32_t max_samples,
                                                            const InstanceHandle_t& previous_handle,
                                                            ReadCondition* a_condition) {
    if (nullptr == a_condition) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return TAKE_NEXT_INSTANCE(data_values, sample_infos, max_samples, previous_handle,
                              a_condition->get_sample_state_mask(), a_condition->get_view_state_mask(),
                              a_condition->get_instance_state_mask());
}

ReturnCode_t DataReaderImpl::read_next_sample(void* data, vbs::SampleInfoInner* info) {
    return READ_NEXT_SAMPLE(data, info);
}

ReturnCode_t DataReaderImpl::take_next_sample(void* data, vbs::SampleInfoInner* info) {
    return TAKE_NEXT_SAMPLE(data, info);
}

void DataReaderImpl::stop() {
    delete lifespan_timer_;
    delete deadline_timer_;

    if (reader_ != nullptr) {
        logDebug(DATA_READER, "Removing " << guid().entityId << " in topic: " << topic_->get_name());
        reader_->disable();
        if (history_ != nullptr) {
            std::lock_guard<RecursiveTimedMutex> guard(history_->getMutex());
            history_->mp_reader = nullptr;
            for (auto it = history_->changesBegin(); it != history_->changesEnd(); ++it) {
                history_->releaseCache(*it);
            }
        }
        subscriber_->dispatcher_participant()->delete_reader(reader_);
        reader_ = nullptr;
    }

    if (subscriber_ != nullptr && batch_free_ != true) {
        (void)subscriber_->delete_datareader(this, false);
    }

    if (reader_participant_ != nullptr) {
        (void)reader_participant_->delete_subscriber(subscriber_);
        reader_participant_ = nullptr;
    }

    if (history_ != nullptr) {
        delete history_;
        history_ = nullptr;
    }
}

DataReaderImpl::~DataReaderImpl() {
    // assert there are no pending conditions
    assert(read_conditions_.empty());

    // Disable the datareader to prevent receiving data in the middle of deleting it
    // disable();
    // is_deleting = true;
    (void)set_listener(nullptr);
    if (reader_ != nullptr) {
        (void)reader_->setListener(nullptr);
    }

    stop();
    if (topic_) {
        topic_->get_impl()->dereference();
    }

    subscriber_ = nullptr;
    reader_ = nullptr;
    topic_ = nullptr;
    listener_ = nullptr;
    deadline_timer_ = nullptr;
    lifespan_timer_ = nullptr;
    if (sample_info_pool_ && sample_info_pool_->num_allocated() > 0) {
        elogError(RTPS_READER, RetCode_t::RETCODE_ILLEGAL_OPERATION,
                  "Reader " << guid() << " deconstruct but sample info pool is used.");
    }
    if (sample_info_pool_ != nullptr) {
        delete sample_info_pool_;
        sample_info_pool_ = nullptr;
    }
    if (loan_manager_ != nullptr) {
        delete loan_manager_;
        loan_manager_ = nullptr;
    }
}

bool DataReaderImpl::can_be_deleted(bool recursive) const {
    if (reader_ != nullptr) {
        std::lock_guard<RecursiveTimedMutex> _(history_->getMutex());

        // According with the standard
        // delete_datareader() should fail with outstanding ReadConditions
        // delete_contained_entities() should not
        if (!recursive) {
            std::lock_guard<std::recursive_mutex> __(get_conditions_mutex());

            if (!read_conditions_.empty()) {
                logWarning(DATA_READER, "DataReaderImpl " << guid() << " has ReadConditions not yet deleted");
                return false;
            }
        }

        return !loan_manager_->has_outstanding_loans();
    }

    return true;
}

bool DataReaderImpl::wait_for_unread_message(const Duration_t& timeout) {
    return reader_ ? reader_->wait_for_unread_cache(timeout) : false;
}

inline void DataReaderImpl::set_read_communication_status(bool trigger_value) {
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::data_on_readers();
    subscriber_->get_statuscondition().get_impl()->set_status(notify_status, trigger_value);

    notify_status = evbs::edds::dds::StatusMask::data_available();
    get_statuscondition().get_impl()->set_status(notify_status, trigger_value);
}

ReturnCode_t DataReaderImpl::check_collection_preconditions_and_calc_max_samples(
    evbs::edds::dds::LoanableCollection& data_values, evbs::edds::dds::SampleInfoSeq& sample_infos,
    int32_t& max_samples) {
    // Properties should be the same on both collections
    if (!collections_have_same_properties(data_values, sample_infos)) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    // Check if a loan is required
    if (0 < data_values.maximum()) {
        // Loan not required, input collections should not be already loaned
        if (false == data_values.has_ownership()) {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }

        int32_t collection_max = data_values.maximum();

        // We consider all negative value to be LENGTH_UNLIMITED
        if (0 > max_samples) {
            // When max_samples is LENGTH_UNLIMITED, the collection imposes the maximum number of
            // samples
            max_samples = collection_max;
        } else {
            if (max_samples > collection_max) {
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
            }
        }
    }

    // All preconditions have been checked. Now apply resource limits on max_samples.
    if ((0 > max_samples) || (max_samples > qos_.reader_resource_limits().max_samples_per_read)) {
        max_samples = qos_.reader_resource_limits().max_samples_per_read;
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::prepare_loan(evbs::edds::dds::LoanableCollection& data_values,
                                          evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t& max_samples) {
    if (0 < data_values.maximum()) {
        // A loan was not requested
        return ReturnCode_t::RETCODE_OK;
    }

    if (max_samples > 0) {
        // Check if there are enough sample_infos
        size_t num_infos = sample_info_pool_->num_allocated();
        if (num_infos == qos_.reader_resource_limits().sample_infos_allocation.maximum) {
            elogErrorKeyT(RTPS_READER, RetCode_t::RETCODE_OUT_OF_RESOURCES, topic_->get_name(),
                          "Prepare loan faild. sample_info_pool_ allocated_num reached max "
                              << qos_.reader_resource_limits().sample_infos_allocation.maximum);
            return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
        }

        // Limit max_samples to available sample_infos
        num_infos += static_cast<size_t>(max_samples);
        if (num_infos > qos_.reader_resource_limits().sample_infos_allocation.maximum) {
            size_t exceed = num_infos - qos_.reader_resource_limits().sample_infos_allocation.maximum;
            max_samples -= static_cast<int32_t>(exceed);
        }
    }

    if (max_samples > 0) {
        // Check if there are enough samples
        int32_t num_samples = sample_pool_->num_allocated();
        int32_t max_resource_samples = qos_.resource_limits().max_samples;
        if (max_resource_samples <= 0) {
            max_resource_samples = std::numeric_limits<int32_t>::max();
        }
        if (num_samples == max_resource_samples) {
            elogErrorKeyT(RTPS_READER, RetCode_t::RETCODE_OUT_OF_RESOURCES, topic_->get_name(),
                          "Prepare loan faild. sample_pool_ allocated_num reached max " << max_resource_samples);
            return ReturnCode_t::RETCODE_OUT_OF_RESOURCES;
        }

        // Limit max_samples to available samples
        num_samples += max_samples;
        if (num_samples > max_resource_samples) {
            int32_t exceed = num_samples - max_resource_samples;
            max_samples -= exceed;
        }
    }

    // Check if there are enough loans
    return ReturnCode_t(loan_manager_->get_loan(data_values, sample_infos)());
}

ReturnCode_t DataReaderImpl::read_or_take(evbs::edds::dds::LoanableCollection& data_values,
                                          evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                          const InstanceHandle_t& handle, vbs::SampleStateMask sample_states,
                                          vbs::ViewStateMask view_states, vbs::InstanceStateMask instance_states,
                                          bool exact_instance, bool single_instance, bool should_take,
                                          bool raw_buffer) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    ReturnCode_t code = check_collection_preconditions_and_calc_max_samples(data_values, sample_infos, max_samples);
    if (!code) {
        return code;
    }
#if 0
    auto max_blocking_time =
        std::chrono::steady_clock::now() +
#if HAVE_STRICT_REALTIME
        std::chrono::microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time));
#else
        std::chrono::hours(24);
#endif  // if HAVE_STRICT_REALTIME

    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);

    if ((!lock.try_lock_until(max_blocking_time))) {
        return ReturnCode_t::RETCODE_TIMEOUT;
    }
#endif
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());
    set_read_communication_status(false);

    detail::StateFilter states = {sample_states, view_states, instance_states};
    auto it = history_->lookup_available_instance(handle, exact_instance);
    if (!it.first) {
        if (exact_instance && (!history_->is_instance_present(handle))) {
            return ReturnCode_t::RETCODE_BAD_PARAMETER;
        } else {
            return ReturnCode_t::RETCODE_NO_DATA;
        }
    }
    code = ReturnCode_t(prepare_loan(data_values, sample_infos, max_samples)());
    if (!code) {
        elogErrorKeyT(DATA_READER, RetCode_t::RETCODE_OUT_OF_RESOURCES, topic_->get_name(),
                      "Topic " << topic_->get_name() << " out of resource.");
        return code;
    }
    vbs::detail::ReadTakeCommand cmd(*this, data_values, sample_infos, max_samples, states, it.second, single_instance);
    while (!cmd.is_finished()) {
        if (!type_->m_isGetKeyDefined) {
            (void)cmd.add_changes(should_take, raw_buffer);
        } else {
            (void)cmd.add_instance(should_take, raw_buffer);
        }
    }

//todo dispatcher
#if 0
    if (should_take) {
        reader_->on_take_sample(data_values.length());
    }
#endif
    try_notify_read_conditions();
    return ReturnCode_t(cmd.return_value()());
}

ReturnCode_t DataReaderImpl::read_instances(evbs::edds::dds::LoanableCollection& data_values,
                                            evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                            const std::vector<InstanceHandle_t>& s_handle,
                                            vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                            vbs::InstanceStateMask instance_states) {
    ReturnCode_t ret_val = ReturnCode_t::RETCODE_ERROR;
    ReturnCode_t ret_tmp = ReturnCode_t::RETCODE_ERROR;
    for (const auto& element : s_handle) {
        ret_tmp = read_or_take(data_values, sample_infos, max_samples, element, sample_states, view_states,
                               instance_states, true, true, false, false);

        if (ret_tmp == ReturnCode_t::RETCODE_OK) {
            ret_val = ret_tmp;
        } else if (ret_val != ReturnCode_t::RETCODE_OK && ret_tmp != ReturnCode_t::RETCODE_OK) {
            ret_val = ret_tmp;
        }
    }
    return ret_val;
}

ReturnCode_t DataReaderImpl::take_instances(evbs::edds::dds::LoanableCollection& data_values,
                                            evbs::edds::dds::SampleInfoSeq& sample_infos, int32_t max_samples,
                                            const std::vector<InstanceHandle_t>& s_handle,
                                            vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                            vbs::InstanceStateMask instance_states) {
    ReturnCode_t ret_val = ReturnCode_t::RETCODE_ERROR;
    ReturnCode_t ret_tmp = ReturnCode_t::RETCODE_ERROR;
    for (const auto& element : s_handle) {
        ret_tmp = read_or_take(data_values, sample_infos, max_samples, element, sample_states, view_states,
                               instance_states, true, true, true, false);

        if (ret_tmp == ReturnCode_t::RETCODE_OK) {
            ret_val = ret_tmp;
        } else if (ret_val != ReturnCode_t::RETCODE_OK && ret_tmp != ReturnCode_t::RETCODE_OK) {
            ret_val = ret_tmp;
        }
    }
    return ret_val;
}

ReturnCode_t DataReaderImpl::return_loan(evbs::edds::dds::LoanableCollection& data_values,
                                         evbs::edds::dds::SampleInfoSeq& sample_infos) {
    static_cast<void>(data_values);
    static_cast<void>(sample_infos);

    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    // Properties should be the same on both collections
    if (!collections_have_same_properties(data_values, sample_infos)) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    // They should have a loan
    if (data_values.has_ownership() == true) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

    // Check if they were loaned by this reader
    ReturnCode_t code = ReturnCode_t(loan_manager_->return_loan(data_values, sample_infos)());
    if (!code) {
        return code;
    }

    // Return samples and infos
    evbs::edds::dds::LoanableCollection::size_type n = sample_infos.length();
    while (n > 0) {
        --n;
        if (sample_infos[n].valid_data) {
            sample_pool_->return_loan(data_values.buffer()[n]);
        }

        sample_info_pool_->return_item(&sample_infos[n]);
    }

    (void)data_values.unloan();
    (void)sample_infos.unloan();

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_or_take_next_sample(void* data, vbs::SampleInfoInner* info, bool should_take) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    if (history_->getHistorySize() == 0U) {
        return ReturnCode_t::RETCODE_NO_DATA;
    }

    auto max_blocking_time =
        std::chrono::steady_clock::now() +
#if HAVE_STRICT_REALTIME
        std::chrono::microseconds(::TimeConv::Time_t2MicroSecondsInt64(qos_.reliability().max_blocking_time));
#else
        std::chrono::hours(24);
#endif  // if HAVE_STRICT_REALTIME

    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex(), std::defer_lock);

    // if (is_deleting) {
    //     return ReturnCode_t::RETCODE_NOT_ENABLED;
    // }

    if (!lock.try_lock_until(max_blocking_time)) {
        return ReturnCode_t::RETCODE_TIMEOUT;
    }

    set_read_communication_status(false);

    auto it = history_->lookup_available_instance(vbs::HANDLE_NIL, false);
    if (!it.first) {
        return ReturnCode_t::RETCODE_NO_DATA;
    }
    StackAllocatedSequence<void*, 1> data_values;
    const_cast<void**>(data_values.buffer())[0] = data;

    StackAllocatedSequence<vbs::SampleInfoInner, 1> sample_infos;
    vbs::detail::StateFilter states {vbs::NOT_READ_SAMPLE_STATE, vbs::ANY_VIEW_STATE, vbs::ANY_INSTANCE_STATE};

    vbs::detail::ReadTakeCommand cmd(*this, data_values, sample_infos, 1, states, it.second, false);
    while (!cmd.is_finished()) {
        if (!type_->m_isGetKeyDefined) {
            (void)cmd.add_changes(should_take, false);
        } else {
            (void)cmd.add_instance(should_take, false);
        }
    }

    //todo dispatcher
#if 0
    if (should_take) {
        reader_->on_take_sample(data_values.length());
    }
#endif
    ReturnCode_t code = ReturnCode_t(cmd.return_value()());
    if (ReturnCode_t::RETCODE_OK == code) {
        *info = sample_infos[0];
    }

    try_notify_read_conditions();
    return code;
}

ReturnCode_t DataReaderImpl::get_first_untaken_info(vbs::SampleInfoInner* /*info*/) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }
#if 0
    if (history_->get_first_untaken_info(*info)) {
        return ReturnCode_t::RETCODE_OK;
    }
#endif
    return ReturnCode_t::RETCODE_NO_DATA;
}

uint64_t DataReaderImpl::get_unread_count() {
    return get_unread_count(false);
}

uint64_t DataReaderImpl::get_unread_count(bool mark_as_read) {
    uint64_t ret_val = reader_ ? history_->get_unread_count(mark_as_read) : 0U;
    if (mark_as_read) {
        try_notify_read_conditions();
    }
    return ret_val;
}

const GUID_t& DataReaderImpl::guid() {
    return guid_;
}

const GUID_t& DataReaderImpl::guid() const {
    return guid_;
}

InstanceHandle_t DataReaderImpl::get_instance_handle() const {
    return guid();
}

void DataReaderImpl::subscriber_qos_updated() {
    update_reader_qos();
}

void DataReaderImpl::update_reader_qos() {
    if (reader_) {
        vbsutil::xmlparser::ContentFilterProperty* filter_property = nullptr;
        auto content_topic = dynamic_cast<vbs::ContentFilteredTopicCore*>(topic_->get_impl());
        if (nullptr != content_topic && !content_topic->filter_property.filter_expression.empty()) {
            filter_property = &content_topic->filter_property;
        }
        vbsutil::xmlparser::ReaderQos rqos = qos_.get_readerqos(get_subscriber()->get_qos());
        std::shared_ptr<evbs::ertps::TopicAttributes> topic_attr_ptr = topic_attributes();
        reader_->updateReader(*topic_attr_ptr, rqos, filter_property);
    }
}

ReturnCode_t DataReaderImpl::set_qos(const vbsutil::xmlparser::DataReaderQos& qos) {
    bool enabled = reader_ != nullptr;
    const vbsutil::xmlparser::DataReaderQos& qos_to_set =
        (&qos == &vbsutil::xmlparser::getDefaultDataReaderQos()) ? subscriber_->get_default_datareader_qos() : qos;

    // Default qos is always considered consistent
    if (&qos != &vbsutil::xmlparser::getDefaultDataReaderQos()) {
        ReturnCode_t check_result = check_qos_including_resource_limits(qos_to_set, type_);
        if ((!check_result)) {
            return check_result;
        }
    }

    if (enabled && (!can_qos_be_updated(qos_, qos_to_set))) {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }

    set_qos(qos_, qos_to_set, !enabled);

    if (enabled) {
        // NOTIFY THE BUILTIN PROTOCOLS THAT THE READER HAS CHANGED
        update_reader_qos();

        // Deadline
        if (qos_.deadline().period != c_TimeInfinite) {
            deadline_duration_us_ = duration<double_t, std::ratio<1, 1000000>>(qos_.deadline().period.to_ns() * 1e-3);
            (void)deadline_timer_->update_interval_millisec(qos_.deadline().period.to_ns() * 1e-6);
        } else {
            deadline_timer_->cancel_timer();
        }

        // Lifespan
        if (qos_.lifespan().duration != c_TimeInfinite) {
            lifespan_duration_us_ =
                std::chrono::duration<double_t, std::ratio<1, 1000000>>(qos_.lifespan().duration.to_ns() * 1e-3);
            (void)lifespan_timer_->update_interval_millisec(qos_.lifespan().duration.to_ns() * 1e-6);
        } else {
            lifespan_timer_->cancel_timer();
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

const vbsutil::xmlparser::DataReaderQos& DataReaderImpl::get_dds_qos() const {
    return qos_;
}

ReturnCode_t DataReaderImpl::get_qos(vbsutil::xmlparser::DataReaderQos& qos) const {
    qos = qos_;
    return ReturnCode_t::RETCODE_OK;
}

void DataReaderImpl::InnerDataReaderListener::on_data_available(const GUID_t& writer_guid,
                                                                const vbs::common::SequenceNumber_t& first_sequence,
                                                                const vbs::common::SequenceNumber_t& last_sequence,
                                                                bool& should_notify_individual_changes) {
    should_notify_individual_changes = false;
    if (data_reader_->on_data_available(writer_guid, first_sequence, last_sequence)) {
        data_reader_->user_data_available(data_reader_);
        // prefer to use reader lisener
        DataReaderListenerInner* listener =
            data_reader_->get_listener_for(evbs::edds::dds::StatusMask::data_available());
        if (listener != nullptr) {
            if (data_reader_->reader_ != nullptr) {
                MessageLog(data_reader_->reader_->getTopicName(), writer_guid, MSG_TRACE_TYPE_UPLOAD,
                           first_sequence.to64long(), vbsutil::xmlparser::c_RTPSTimeZero, 0);
            }
            listener->on_data_available(data_reader_);
            if (data_reader_->reader_ != nullptr) {
                MessageLog(data_reader_->reader_->getTopicName(), writer_guid, MSG_TRACE_TYPE_UPLOAD_DONE,
                           first_sequence.to64long(), vbsutil::xmlparser::c_RTPSTimeZero, 0);
            }
        } else {
            SubscriberListener* subscriber_listener =
                data_reader_->subscriber_->get_listener_for(evbs::edds::dds::StatusMask::data_on_readers());
            if (subscriber_listener != nullptr) {
                subscriber_listener->on_data_on_readers(data_reader_->subscriber_);
            }
        }
        data_reader_->set_read_communication_status(true);
    }
}

void DataReaderImpl::InnerDataReaderListener::onReaderMatched(const vbs::common::MatchingInfo& info) {
    data_reader_->update_subscription_matched_status(info);

    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::subscription_matched();
    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if (listener != nullptr || (data_reader_->reader_listener_ != nullptr)) {
        evbs::edds::dds::SubscriptionMatchedStatus callback_status;
        if (ReturnCode_t::RETCODE_OK == data_reader_->get_subscription_matched_status(callback_status)) {
            data_reader_->user_subscription_matched(data_reader_, callback_status);
            if (listener != nullptr) {
                listener->on_subscription_matched(data_reader_, callback_status);
            }
        }
    }

    data_reader_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataReaderImpl::InnerDataReaderListener::on_liveliness_changed(
    const evbs::edds::dds::LivelinessChangedStatus& status) {
    (void)data_reader_->update_liveliness_status(status);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::liveliness_changed();
    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if ((listener != nullptr) || (data_reader_->reader_listener_ != nullptr)) {
        evbs::edds::dds::LivelinessChangedStatus callback_status;
        if (data_reader_->get_liveliness_changed_status(callback_status) == ReturnCode_t::RETCODE_OK) {
            data_reader_->user_liveliness_changed(data_reader_, callback_status);
            if (listener != nullptr) {
                listener->on_liveliness_changed(data_reader_, callback_status);
            }
        }
    }
    data_reader_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataReaderImpl::InnerDataReaderListener::on_requested_incompatible_qos(evbs::edds::dds::PolicyMask qos) {
    (void)data_reader_->update_requested_incompatible_qos(qos);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::requested_incompatible_qos();
    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if ((listener != nullptr) || (data_reader_->reader_listener_ != nullptr)) {
        evbs::edds::dds::RequestedIncompatibleQosStatus callback_status;
        if (data_reader_->get_requested_incompatible_qos_status(callback_status) == ReturnCode_t::RETCODE_OK) {
            data_reader_->user_requested_incompatible_qos(data_reader_, callback_status);
            if (listener != nullptr) {
                listener->on_requested_incompatible_qos(data_reader_, callback_status);
            }
        }
    }
    data_reader_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataReaderImpl::InnerDataReaderListener::on_sample_lost(const int32_t sample_lost_since_last_update,
                                                             const evbs::edds::dds::SampleLostStatusKind statusKind) {
    (void)data_reader_->update_sample_lost_status(sample_lost_since_last_update, statusKind);
    const evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::sample_lost();

    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if ((listener != nullptr) || (data_reader_->reader_listener_ != nullptr)) {
        evbs::edds::dds::SampleLostStatus callback_status;
        if (data_reader_->get_sample_lost_status(callback_status) == ReturnCode_t::RETCODE_OK) {
            data_reader_->user_sample_lost(data_reader_, callback_status);
            if (listener != nullptr) {
                listener->on_sample_lost(data_reader_, callback_status);
            }
        }
    }
    data_reader_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataReaderImpl::InnerDataReaderListener::on_sample_rejected(evbs::edds::dds::SampleRejectedStatusKind reason,
                                                                 const CacheChange_t* const change_in) {
    (void)data_reader_->update_sample_rejected_status(reason, change_in);
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::sample_rejected();
    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if ((listener != nullptr) || (data_reader_->reader_listener_ != nullptr)) {
        evbs::edds::dds::SampleRejectedStatus callback_status;
        if (data_reader_->get_sample_rejected_status(callback_status) == ReturnCode_t::RETCODE_OK) {
            data_reader_->user_sample_rejected(data_reader_, callback_status);
            if (listener != nullptr) {
                listener->on_sample_rejected(data_reader_, callback_status);
            }
        }
    }
    data_reader_->get_statuscondition().get_impl()->set_status(notify_status, true);
}

void DataReaderImpl::InnerDataReaderListener::on_endpoint_crash(const evbs::ertps::rtps::GUID_t& writer_guid) {
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::endpoint_crash();

    data_reader_->user_endpoint_crash(writer_guid);
    DataReaderListenerInner* listener = data_reader_->get_listener_for(notify_status);
    if (listener != nullptr) {
        listener->on_endpoint_crash(writer_guid);
    }
}

bool DataReaderImpl::on_data_available(const GUID_t& writer_guid, const vbs::common::SequenceNumber_t& first_sequence,
                                       const vbs::common::SequenceNumber_t& last_sequence) {
    bool ret_val = true;

    if (qos_.deadline().period != c_TimeInfinite || qos_.lifespan().duration != c_TimeInfinite) {
        ret_val = false;
        for (auto seq = first_sequence; seq <= last_sequence; ++seq) {
            CacheChange_t* change = nullptr;

            if (history_->get_change(seq, writer_guid, &change)) {
                ret_val |= on_new_cache_change_added(change);
            }
        }
    }

    try_notify_read_conditions();

    return ret_val;
}

void DataReaderImpl::deadline_event_update(const CacheChange_t* const change) {
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    if (!history_->set_next_deadline_nts(
            change->instanceHandle,
            steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_))) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Could not set next deadline in the history");
    } else if (timer_owner_ == change->instanceHandle || timer_owner_ == InstanceHandle_t()) {
        if (deadline_timer_reschedule()) {
            deadline_timer_->cancel_timer();
            deadline_timer_->restart_timer();
        }
    }
}

bool DataReaderImpl::lifespan_event_update(const CacheChange_t* const change) {
    auto source_timestamp = system_clock::time_point() + nanoseconds(change->sourceTimestamp.to_ns());
    auto now = system_clock::now();
    CacheChange_t* new_change = const_cast<CacheChange_t*>(change);

    // The new change could have expired if it arrived too late
    // If so, remove it from the history and return false to avoid notifying the listener
    if (now - source_timestamp >= lifespan_duration_us_) {
        (void)history_->remove_change_sub(new_change);
        logWarning(DATA_READER, "Lifespan expired in topic: " << topic_->get_name());
        return false;
    }

    CacheChange_t* earliest_change;
    if (history_->get_earliest_change(&earliest_change)) {
        if (earliest_change == change) {
            // The new change has been added at the beginning of the the history
            // As the history is sorted by timestamp, this means that the new change has the
            // smallest timestamp We have to stop the timer as this will be the next change to
            // expire
            lifespan_timer_->cancel_timer();
        }
    } else {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "A change was added to history that could not be retrieved");
    }

    auto interval = source_timestamp - now + duration_cast<nanoseconds>(lifespan_duration_us_);

    // Update and restart the timer
    // If the timer is already running this will not have any effect
    (void)lifespan_timer_->update_interval_millisec(static_cast<double>(interval.count()) * 1e-6);
    lifespan_timer_->restart_timer();
    return true;
}

bool DataReaderImpl::on_new_cache_change_added(const vbs::common::CacheChange_t* const change) {
#if 0
    vbs::common::CacheChange_t* new_change = const_cast<vbs::common::CacheChange_t*>(change);

    if (!history_->update_instance_nts(new_change)) {
        history_->remove_change_sub(new_change);
        return false;
    }
#endif
    if (qos_.deadline().period != c_TimeInfinite) {
        deadline_event_update(change);
    }

    if (qos_.lifespan().duration == c_TimeInfinite) {
        return true;
    }

    return lifespan_event_update(change);
}

void DataReaderImpl::update_subscription_matched_status(const evbs::ertps::rtps::MatchingInfo& status) {
    auto count_change = status.status == MATCHED_MATCHING ? 1 : -1;
    subscription_matched_status_.current_count += count_change;
    subscription_matched_status_.current_count_change += count_change;
    if (count_change > 0) {
        subscription_matched_status_.total_count += count_change;
        subscription_matched_status_.total_count_change += count_change;
    }
    subscription_matched_status_.last_publication_handle = status.remoteEndpointGuid;

    if (count_change < 0) {
        history_->writer_not_alive(status.remoteEndpointGuid);
        try_notify_read_conditions();
    }
}

ReturnCode_t DataReaderImpl::get_subscription_matched_status(evbs::edds::dds::SubscriptionMatchedStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = subscription_matched_status_;
        subscription_matched_status_.current_count_change = 0;
        subscription_matched_status_.total_count_change = 0;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::subscription_matched(), false);
    return ReturnCode_t::RETCODE_OK;
}

bool DataReaderImpl::deadline_timer_reschedule() {
    assert(qos_.deadline().period != c_TimeInfinite);

    //std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    steady_clock::time_point next_deadline_us;
    if (!history_->get_next_deadline_nts(timer_owner_, next_deadline_us)) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Could not get the next deadline from the history");
        return false;
    }
    auto interval_ms = duration_cast<milliseconds>(next_deadline_us - steady_clock::now());

    (void)deadline_timer_->update_interval_millisec(static_cast<double_t>(interval_ms.count()));
    return true;
}

bool DataReaderImpl::deadline_missed() {
    assert(qos_.deadline().period != c_TimeInfinite);

    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    deadline_missed_status_.total_count++;
    deadline_missed_status_.total_count_change++;
    deadline_missed_status_.last_instance_handle = timer_owner_;
    evbs::edds::dds::StatusMask notify_status = evbs::edds::dds::StatusMask::requested_deadline_missed();

    if (reader_listener_ != nullptr) {
        user_requested_deadline_missed(this, deadline_missed_status_);
    }
    auto listener = get_listener_for(notify_status);
    if (nullptr != listener) {
        listener->on_requested_deadline_missed(this, deadline_missed_status_);
    }
    if (reader_listener_ != nullptr || nullptr != listener) {
        deadline_missed_status_.total_count_change = 0U;
    }
    get_statuscondition().get_impl()->set_status(notify_status, true);

    if (!history_->set_next_deadline_nts(
            timer_owner_, steady_clock::now() + duration_cast<system_clock::duration>(deadline_duration_us_), true)) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "Could not set next deadline in the history");
        return false;
    }
    return deadline_timer_reschedule();
}

ReturnCode_t DataReaderImpl::get_requested_deadline_missed_status(
    evbs::edds::dds::RequestedDeadlineMissedStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = deadline_missed_status_;
        deadline_missed_status_.total_count_change = 0U;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::requested_deadline_missed(), false);
    return ReturnCode_t::RETCODE_OK;
}

bool DataReaderImpl::lifespan_expired() {
    std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

    CacheChange_t* earliest_change;
    while (history_->get_earliest_change(&earliest_change)) {
        auto source_timestamp = system_clock::time_point() + nanoseconds(earliest_change->sourceTimestamp.to_ns());
        auto now = system_clock::now();

        // Check that the earliest change has expired (the change which started the timer could have
        // been removed from the history)
        if (now - source_timestamp < lifespan_duration_us_) {
            auto interval = source_timestamp - now + lifespan_duration_us_;
            (void)lifespan_timer_->update_interval_millisec(
                static_cast<double_t>(duration_cast<milliseconds>(interval).count()));
            return true;
        }

        if (!earliest_change->isRead) {
            logWarning(DATA_READER, "Lifespan expired in topic: " << topic_->get_name());
        }

        // The earliest change has expired
        (void)history_->remove_change_sub(earliest_change);

        try_notify_read_conditions();

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

ReturnCode_t DataReaderImpl::set_listener(DataReaderListenerInner* listener) {
    return set_listener(listener, evbs::edds::dds::StatusMask::all());
}

ReturnCode_t DataReaderImpl::set_listener(DataReaderListenerInner* listener, const evbs::edds::dds::StatusMask& mask) {
    std::lock_guard<std::mutex> _(listener_mutex_);
    listener_ = listener;
    status_mask_ = mask;

    return ReturnCode_t::RETCODE_OK;
}

const DataReaderListenerInner* DataReaderImpl::get_listener() const {
    std::lock_guard<std::mutex> _(listener_mutex_);
    return listener_;
}

ReturnCode_t DataReaderImpl::get_liveliness_changed_status(evbs::edds::dds::LivelinessChangedStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

        status = liveliness_changed_status_;
        liveliness_changed_status_.alive_count_change = 0U;
        liveliness_changed_status_.not_alive_count_change = 0U;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::liveliness_changed(), false);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_requested_incompatible_qos_status(
    evbs::edds::dds::RequestedIncompatibleQosStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::unique_lock<RecursiveTimedMutex> lock(history_->getMutex());

        status = requested_incompatible_qos_status_;
        requested_incompatible_qos_status_.total_count_change = 0u;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::requested_incompatible_qos(), false);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_sample_lost_status(evbs::edds::dds::SampleLostStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

        status = sample_lost_status_;
        sample_lost_status_.total_count_change = 0;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::sample_lost(), false);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_sample_rejected_status(evbs::edds::dds::SampleRejectedStatus& status) {
    if (reader_ == nullptr) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    {
        std::lock_guard<RecursiveTimedMutex> lock(history_->getMutex());

        status = sample_rejected_status_;
        sample_rejected_status_.total_count_change = 0u;
    }

    get_statuscondition().get_impl()->set_status(evbs::edds::dds::StatusMask::sample_rejected(), false);
    return ReturnCode_t::RETCODE_OK;
}

const Subscriber* DataReaderImpl::get_subscriber() const {
    return subscriber_;
}

/* TODO
   bool DataReaderImpl::wait_for_historical_data(
        const Duration_t& max_wait) const
   {
    (void)max_wait;
    // TODO Implement
    return false;
   }
 */

const vbs::TopicDescription* DataReaderImpl::get_topicdescription() const {
    return topic_;
}

TypeSupport DataReaderImpl::type() {
    return type_;
}

evbs::edds::dds::RequestedIncompatibleQosStatus& DataReaderImpl::update_requested_incompatible_qos(
    evbs::edds::dds::PolicyMask incompatible_policies) {
    ++requested_incompatible_qos_status_.total_count;
    ++requested_incompatible_qos_status_.total_count_change;
    for (octet id = 1U; id < NEXT_QOS_POLICY_ID; ++id) {
        if (incompatible_policies.test(id)) {
            ++requested_incompatible_qos_status_.policies[static_cast<QosPolicyId_t>(id)].count;
            requested_incompatible_qos_status_.last_policy_id = static_cast<QosPolicyId_t>(id);
        }
    }
    return requested_incompatible_qos_status_;
}

evbs::edds::dds::LivelinessChangedStatus& DataReaderImpl::update_liveliness_status(
    const evbs::edds::dds::LivelinessChangedStatus& status) {
    if (0 < status.not_alive_count_change) {
        history_->writer_not_alive(vbsutil::xmlparser::iHandle2GUID(status.last_publication_handle));
        try_notify_read_conditions();
    }

    liveliness_changed_status_.alive_count = status.alive_count;
    liveliness_changed_status_.not_alive_count = status.not_alive_count;
    liveliness_changed_status_.alive_count_change += status.alive_count_change;
    liveliness_changed_status_.not_alive_count_change += status.not_alive_count_change;
    liveliness_changed_status_.last_publication_handle = status.last_publication_handle;

    return liveliness_changed_status_;
}

const evbs::edds::dds::SampleLostStatus& DataReaderImpl::update_sample_lost_status(
    int32_t sample_lost_since_last_update, evbs::edds::dds::SampleLostStatusKind statusKind) {
    sample_lost_status_.total_count += sample_lost_since_last_update;
    sample_lost_status_.total_count_change += sample_lost_since_last_update;
    sample_lost_status_.last_status = statusKind;

    return sample_lost_status_;
}

ReturnCode_t DataReaderImpl::check_qos_including_resource_limits(const vbsutil::xmlparser::DataReaderQos& qos,
                                                                 const TypeSupport& type) {
    ReturnCode_t check_qos_return = check_qos(qos);
    if (ReturnCode_t::RETCODE_OK == check_qos_return && type->m_isGetKeyDefined) {
        check_qos_return = check_allocation_consistency(qos);
    }
    return check_qos_return;
}

ReturnCode_t DataReaderImpl::check_qos(const vbsutil::xmlparser::DataReaderQos& qos) {
    if (qos.reader_resource_limits().max_samples_per_read <= 0) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples_per_read should be strictly possitive");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    if (qos_has_unique_network_request(qos) && qos_has_specific_locators(qos)) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "unique_network_request cannot be set along specific locators");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::check_allocation_consistency(const vbsutil::xmlparser::DataReaderQos& qos) {
    if ((qos.resource_limits().max_samples > 0) &&
        (qos.resource_limits().max_samples <
         (qos.resource_limits().max_instances * qos.resource_limits().max_samples_per_instance))) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be greater than max_instances * max_samples_per_instance");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    if (((qos.resource_limits().max_instances <= 0) || (qos.resource_limits().max_samples_per_instance <= 0)) &&
        (qos.resource_limits().max_samples > 0)) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be infinite when max_instances or "
                  "max_samples_per_instance are infinite");
        return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    return ReturnCode_t::RETCODE_OK;
}

bool DataReaderImpl::can_qos_be_updated(const vbsutil::xmlparser::DataReaderQos& to,
                                        const vbsutil::xmlparser::DataReaderQos& from) {
    bool updatable = true;
    if ((!(to.resource_limits() == from.resource_limits()))) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "resource_limits cannot be changed after the creation of a DataReaderImpl.");
    }
    if ((to.history().kind != from.history().kind) || (to.history().depth != from.history().depth)) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "History cannot be changed after the creation of a DataReaderImpl.");
    }

    if (to.durability().kind != from.durability().kind) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "Durability kind cannot be changed after the creation of a DataReaderImpl.");
    }
    if ((to.liveliness().kind != from.liveliness().kind) ||
        (to.liveliness().lease_duration != from.liveliness().lease_duration) ||
        (to.liveliness().announcement_period != from.liveliness().announcement_period)) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "Liveliness cannot be changed after the creation of a DataReaderImpl.");
    }
    if (to.reliability().kind != from.reliability().kind) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "Reliability Kind cannot be changed after the creation of a DataReaderImpl.");
    }
    if (to.ownership().kind != from.ownership().kind) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "Ownership Kind cannot be changed after the creation of a DataReaderImpl.");
    }
    if (!(to.reader_resource_limits() == from.reader_resource_limits())) {
        updatable = false;
        logDebug(DDS_QOS_CHECK, "reader_resource_limits cannot be changed after the creation of a DataReaderImpl.");
    }
    return updatable;
}

void DataReaderImpl::set_qos(vbsutil::xmlparser::DataReaderQos& to, const vbsutil::xmlparser::DataReaderQos& from,
                             bool first_time) {
    if (first_time && (to.durability().kind != from.durability().kind)) {
        to.durability() = from.durability();
        to.durability().hasChanged = true;
    }
    if (to.deadline().period != from.deadline().period) {
        to.deadline() = from.deadline();
        to.deadline().hasChanged = true;
    }
    if (first_time && (!(to.liveliness() == from.liveliness()))) {
        to.liveliness() = from.liveliness();
        to.liveliness().hasChanged = true;
    }
    if (first_time && (!(to.reliability() == from.reliability()))) {
        to.reliability() = from.reliability();
        to.reliability().hasChanged = true;
    }
    if (first_time && (to.ownership().kind != from.ownership().kind)) {
        to.ownership() = from.ownership();
        to.ownership().hasChanged = true;
    }
    if (to.lifespan().duration != from.lifespan().duration) {
        to.lifespan() = from.lifespan();
        to.lifespan().hasChanged = true;
    }
    if (first_time && (!(to.reliable_reader_qos() == from.reliable_reader_qos()))) {
        to.reliable_reader_qos() = from.reliable_reader_qos();
    }
    if (first_time && ((to.history().kind != from.history().kind) || (to.history().depth != from.history().depth))) {
        to.history() = from.history();
        to.history().hasChanged = true;
    }
    if (first_time && (!(to.resource_limits() == from.resource_limits()))) {
        to.resource_limits() = from.resource_limits();
        to.resource_limits().hasChanged = true;
    }
    if (first_time && (!(to.properties() == from.properties()))) {
        to.properties() = from.properties();
    }

    if (first_time && (!(to.endpoint() == from.endpoint()))) {
        to.endpoint() = from.endpoint();
    }

    if (first_time && (!(to.reader_resource_limits() == from.reader_resource_limits()))) {
        to.reader_resource_limits() = from.reader_resource_limits();
    }
}

std::shared_ptr<evbs::ertps::TopicAttributes> DataReaderImpl::topic_attributes() const {
    // evbs::ertps::TopicAttributes topic_att;
    std::shared_ptr<evbs::ertps::TopicAttributes> topic_att(new evbs::ertps::TopicAttributes());
    topic_att->topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    topic_att->topicName = topic_->get_impl()->get_rtps_topic_name();
    topic_att->topicDataType = topic_->get_type_name();
    topic_att->qos.history(qos_.history());
    topic_att->qos.resource_limits(qos_.resource_limits());
    topic_att->m_crc16Idl = type_->getIdlCrc16();

    return topic_att;
}

DataReaderListenerInner* DataReaderImpl::get_listener_for(const evbs::edds::dds::StatusMask& status) {
    {
        std::lock_guard<std::mutex> _(listener_mutex_);

        if ((listener_ != nullptr) && get_status_mask().is_active(status)) {
            return listener_;
        }
    }

    return subscriber_->get_listener_for(status);
}
#if 0
std::shared_ptr<IPayloadPool> DataReaderImpl::get_payload_pool() {
    // When the user requested PREALLOCATED_WITH_REALLOC, but we know the type cannot
    // grow, we translate the policy into bare PREALLOCATED
    if ((PREALLOCATED_WITH_REALLOC_MEMORY_MODE == history_->m_att.memoryPolicy) &&
        (type_->is_bounded() || type_->is_plain())) {
        history_->m_att.memoryPolicy = PREALLOCATED_MEMORY_MODE;
    }

    PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);

    if ((!payload_pool_)) {
        payload_pool_ = TopicPayloadPoolRegistry::get(topic_->get_impl()->get_rtps_topic_name(), config);
        sample_pool_  = std::make_shared<detail::SampleLoanManager>(config, type_);
    }

    (void)payload_pool_->reserve_history(config, true);
    return payload_pool_;
}

void DataReaderImpl::release_payload_pool() {
    assert(payload_pool_);

    PoolConfig config = PoolConfig::from_history_attributes(history_->m_att);
    (void)payload_pool_->release_history(config, true);
    payload_pool_.reset();
}
#endif
bool DataReaderImpl::is_sample_valid(const void* data, const vbs::SampleInfoInner* info) const {
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(info);
    return true;
}

ReturnCode_t DataReaderImpl::get_listening_locators(vbsutil::xmlparser::LocatorList& /*locators*/) const {
    if (nullptr == reader_) {
        return ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    //(void)locators.assign(reader_->getAttributes().unicastLocatorList);
    //locators.push_back(reader_->getAttributes().multicastLocatorList);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::delete_contained_entities() {
    std::lock_guard<std::recursive_mutex> _(get_conditions_mutex());

    // Check pending ReadConditions
    for (vbs::detail::ReadConditionImpl* impl : read_conditions_) {
        // should be alive
        auto keep_alive = impl->shared_from_this();
        assert((bool)keep_alive);
        // free ReadConditions
        impl->detach_all_conditions();
    }

    // release the colection
    read_conditions_.clear();

    return ReturnCode_t::RETCODE_OK;
}

void DataReaderImpl::filter_has_been_updated() {
    update_reader_qos();
}

InstanceHandle_t DataReaderImpl::lookup_instance(const void* instance) const {
    InstanceHandle_t handle = vbs::HANDLE_NIL;

    if (instance && type_->m_isGetKeyDefined) {
        if (type_->getKey(const_cast<void*>(instance), &handle, false)) {
            if (!history_->is_instance_present(handle)) {
                handle = vbs::HANDLE_NIL;
            }
        }
    }
    return handle;
}

const evbs::edds::dds::SampleRejectedStatus& DataReaderImpl::update_sample_rejected_status(
    evbs::edds::dds::SampleRejectedStatusKind reason, const vbs::common::CacheChange_t* const change_in) {
    ++sample_rejected_status_.total_count;
    ++sample_rejected_status_.total_count_change;
    sample_rejected_status_.last_reason = reason;
    sample_rejected_status_.last_instance_handle = change_in->instanceHandle;
    return sample_rejected_status_;
}

bool DataReaderImpl::ReadConditionOrder::operator()(const vbs::detail::ReadConditionImpl* lhs,
                                                    const vbs::detail::ReadConditionImpl* rhs) const {
    return less(lhs->get_sample_state_mask(), lhs->get_view_state_mask(), lhs->get_instance_state_mask(),
                rhs->get_sample_state_mask(), rhs->get_view_state_mask(), rhs->get_instance_state_mask());
}

bool DataReaderImpl::ReadConditionOrder::operator()(const vbs::detail::ReadConditionImpl* lhs,
                                                    const detail::StateFilter& rhs) const {
    return less(lhs->get_sample_state_mask(), lhs->get_view_state_mask(), lhs->get_instance_state_mask(),
                rhs.sample_states, rhs.view_states, rhs.instance_states);
}

bool DataReaderImpl::ReadConditionOrder::operator()(const detail::StateFilter& lhs,
                                                    const vbs::detail::ReadConditionImpl* rhs) const {
    return less(lhs.sample_states, lhs.view_states, lhs.instance_states, rhs->get_sample_state_mask(),
                rhs->get_view_state_mask(), rhs->get_instance_state_mask());
}

std::recursive_mutex& DataReaderImpl::get_conditions_mutex() const noexcept {
    return conditions_mutex_;
}

ReadCondition* DataReaderImpl::create_readcondition(vbs::SampleStateMask sample_states, vbs::ViewStateMask view_states,
                                                    vbs::InstanceStateMask instance_states) {
    // Check the mask set up makes sense
    if ((sample_states == 0U) && (view_states == 0U) && (instance_states == 0U)) {
        return nullptr;
    }
    detail::StateFilter current_mask {};
    if (nullptr != reader_) {
        try {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            current_mask = get_last_mask_state();
        } catch (std::runtime_error& ex) {
            // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
            elogError(DATA_READER, RetCode_t::RETCODE_ERROR,
                      "catch runtime_error in create_readcondition" << ex.what());
        }
    }
    std::lock_guard<std::recursive_mutex> _(get_conditions_mutex());

    // Check if there is an associated ReadConditionImpl object already
    detail::StateFilter key = {sample_states, view_states, instance_states};

#ifdef __cpp_lib_generic_associative_lookup
    // c++14
    auto it = read_conditions_.find(key);
#else
    // TODO: remove this when C++14 is enforced
    ReadConditionOrder sort;
    auto it = lower_bound(read_conditions_.begin(), read_conditions_.end(), key, sort);
    if ((it != read_conditions_.end()) && (sort(*it, key) || sort(key, *it))) {
        it = read_conditions_.end();
    }
#endif  // ifdef __cpp_lib_generic_associative_lookup

    std::shared_ptr<vbs::detail::ReadConditionImpl> impl;

    if (it != read_conditions_.end()) {
        // already there
        try {
            impl = (*it)->shared_from_this();
        } catch (const std::bad_weak_ptr& e) {
            std::cerr << e.what() << '\n';
        }
    } else {
        // create a new one
        impl = std::make_shared<vbs::detail::ReadConditionImpl>(*this, key);
        impl->set_trigger_value(current_mask);
        // Add the implementation object to the collection
        (void)read_conditions_.insert(impl.get());
    }

    // Now create the ReadCondition and associate it with the implementation
    vbs::ReadCondition* cond = new vbs::ReadCondition();
    auto ret_code = impl->attach_condition(cond);

    // attach cannot fail in this scenario
    assert(!!ret_code);
    (void)ret_code;

    return cond;
}

ReturnCode_t DataReaderImpl::delete_readcondition(vbs::ReadCondition* a_condition) {
    if (nullptr == a_condition) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    vbs::detail::ReadConditionImpl* impl = a_condition->get_impl();

    if (nullptr == impl) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    std::lock_guard<std::recursive_mutex> _(get_conditions_mutex());

    // Check if there is an associated ReadConditionImpl object already
    auto it = read_conditions_.find(impl);

    if (it == read_conditions_.end()) {
        // The ReadCondition is unknown to this DataReaderImpl
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    std::weak_ptr<vbs::detail::ReadConditionImpl> wp;
#ifdef __cpp_lib_enable_shared_from_this
    wp = impl->weak_from_this();
#else
    // remove when C++17 is enforced
    try {
        wp = std::weak_ptr<vbs::detail::ReadConditionImpl>(impl->shared_from_this());
    } catch (const std::exception& e) {
        elogError(DATA_READER, RetCode_t::RETCODE_ERROR, "catch exception with weak_ptr");
    }
#endif  // ifdef __cpp_lib_enable_shared_from_this

    // Detach from the implementation object
    auto ret_code = impl->detach_condition(a_condition);

    if (!!ret_code) {
        // delete the condition
        delete a_condition;

        // check if we must remove the implementation object
        if (wp.expired()) {
            (void)read_conditions_.erase(it);
        }
    }

    return ret_code;
}

const detail::StateFilter& DataReaderImpl::get_last_mask_state() const {
    if (nullptr == reader_) {
        throw std::runtime_error("The DataReaderImpl has not yet been enabled.");
    }

    std::lock_guard<RecursiveTimedMutex> _(history_->getMutex());
    return last_mask_state_;
}

void DataReaderImpl::try_notify_read_conditions() noexcept {
    // If disabled ignore always
    if (nullptr == reader_) {
        return;
    }
    std::unique_lock<std::recursive_mutex> _(get_conditions_mutex());
    if (read_conditions_.size() == 0) {
        return;
    }
    // Update and check the mask change requires notification
    detail::StateFilter current_mask {};
    bool notify = false;
    {
        _.unlock();
        auto old_mask = last_mask_state_;
        last_mask_state_ = history_->get_mask_status();
        current_mask = last_mask_state_;

        notify = (last_mask_state_.sample_states & ~old_mask.sample_states) ||
                 (last_mask_state_.view_states & ~old_mask.view_states) ||
                 (last_mask_state_.instance_states & ~old_mask.instance_states);
    }

    // traverse the conditions notifying
    _.lock();
    for (vbs::detail::ReadConditionImpl* impl : read_conditions_) {
        impl->set_trigger_value(current_mask);
        if (notify) {
            impl->notify();
        }
    }
}

void DataReaderImpl::InnerDataReaderListener::on_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) {
    // First check if we can handle with on_data_on_readers
    data_reader_->user_e2e_exception(status);
    SubscriberListener* sub_listener =
        data_reader_->subscriber_->get_listener_for(evbs::edds::dds::StatusMask::e2e_expection());
    if (sub_listener != nullptr) {
        sub_listener->on_e2e_exception(status);
    } else {
        DataReaderListenerInner* reader_listener =
            data_reader_->get_listener_for(evbs::edds::dds::StatusMask::e2e_expection());
        if (reader_listener != nullptr) {
            reader_listener->on_e2e_exception(status);
        }
    }
}

void DataReaderImpl::get_statistic_recv_info(evbs::edds::dds::builtin::StatisticRecvInfo* info) {
    if (reader_ && reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS)) {
        info->avg_throughput =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_avg_throughput();
        info->avg_latancy =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_avg_latancy();
        info->lost_count =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_lost_count();
        info->recv_count =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_recv_count();
        info->untake_count =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_unread_count();
        info->take_count_total =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_take_count();
        info->last_pkt_timestamp =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_last_pkt_timestamp();
        info->first_pkt_timestamp =
            static_cast<RTPSReader*>(reader_->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS))->get_first_pkt_timestamp();
    }
}

DataReaderImpl::DataReaderImpl(DomainParticipantImpl* const local_participant, TopicImpl* const local_topic,
                               const std::string& profile_name, vbs::DataReaderListener* const listener,
                               const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())), inner_listener_(this) {
    initDataReaderImpl(local_participant, local_topic, &profile_name, listener, mask);
}

DataReaderImpl::DataReaderImpl(vbs::DomainParticipantImpl* const local_participant, vbs::TopicImpl* const local_topic,
                               const vbs::DataReaderQosImpl& local_data_reader_qos,
                               vbs::DataReaderListener* const listener, const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())), inner_listener_(this) {
    initDataReaderImpl(local_participant, local_topic, &local_data_reader_qos, listener, mask);
}

DataReaderImpl::DataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                               vbs::ContentFilteredTopicImpl* const local_topic, const std::string& profile_name,
                               vbs::DataReaderListener* const listener, const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())), inner_listener_(this) {
    initDataReaderImpl(local_participant, local_topic, &profile_name, listener, mask);
}

DataReaderImpl::DataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                               vbs::ContentFilteredTopicImpl* const local_topic,
                               const vbs::DataReaderQosImpl& local_data_reader_qos,
                               vbs::DataReaderListener* const listener, const vbs::StatusMask& mask)
    : DomainEntity(*(mask.get_dds_status_mask())), inner_listener_(this) {
    initDataReaderImpl(local_participant, local_topic, &local_data_reader_qos, listener, mask);
}

void DataReaderImpl::initDataReaderImpl(vbs::DomainParticipantImpl* const local_participant,
                                        std::variant<vbs::TopicImpl*, vbs::ContentFilteredTopicImpl*> const local_topic,
                                        std::variant<const vbs::DataReaderQosImpl*, const std::string*> qos_info,
                                        vbs::DataReaderListener* const listener, const vbs::StatusMask& mask) {
    vbs::TopicImpl* the_topic = nullptr;
    if (local_topic.index() == 0) {
        the_topic = std::get<0>(local_topic);
        topic_ = the_topic;
    } else {
        the_topic = std::get<1>(local_topic)->related_topic_->get_instance();
        topic_ = std::get<1>(local_topic)->dds_topic_;
    }
    {
        reader_participant_ = local_participant;
        vbs_topic_ = the_topic;
    }
    if ((local_participant == nullptr) || (vbs_topic_ == nullptr)) {
        return;
    }
    topic_->get_impl()->reference();
    type_ = vbs_topic_->type_;
    set_user_listener(listener, mask);
    reader_guid_ = vbs_guid_.generate();

    bool result = false;
    const vbsutil::xmlparser::SubscriberQos subscriber_qos;
    subscriber_ = local_participant->create_subscriber(subscriber_qos, nullptr);
    if (qos_info.index() == 0) {
        const auto& qos = *std::get<0>(qos_info);
        result = datareader_init(topic_, qos.get_dds_reader_qos(), false);
    } else {
        const auto& profile_name = *std::get<1>(qos_info);
        auto qos = QosManager::Instance()->get_reader_xml_qos(profile_name);
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
            result = datareader_init_with_profile(topic_, *qos, false);
        }
    }

    if (result != false) {
        dds_is_initialized_ = true;
        vbs_qos_.dds_reader_qos(qos_);
    } else {
        reader_participant_->delete_subscriber(subscriber_);
        subscriber_ = nullptr;
    }
}

bool DataReaderImpl::datareader_init(vbs::TopicDescription* topic, const vbsutil::xmlparser::DataReaderQos& reader_qos,
                                     bool auto_enable) {
    // Look for the correct type registration
    TypeSupport type_support = reader_participant_->find_type(topic->get_type_name());

    /// Preconditions
    // Check the type was registered.
    if (type_support.empty()) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Type : " << topic->get_type_name() << " Not Registered");
        return false;
    }

    if (!DataReaderImpl::check_qos_including_resource_limits(reader_qos, type_support)) {
        return false;
    }

    if (&reader_qos == &vbsutil::xmlparser::getDefaultDataReaderQos()) {
        qos_ = subscriber_->get_default_datareader_qos();
    } else {
        qos_ = reader_qos;
    }

    history_ = new vbs::detail::DataReaderHistory(type_, *topic, qos_);

    deadline_duration_us_ =
        std::chrono::duration<double_t, std::ratio<1, 1000000>>(qos_.deadline().period.to_ns() * 1e-3);
    lifespan_duration_us_ =
        std::chrono::duration<double_t, std::ratio<1, 1000000>>(qos_.lifespan().duration.to_ns() * 1e-3);
    sample_info_pool_ = new vbs::detail::SampleInfoPool(qos_);
    loan_manager_ = new vbs::detail::DataReaderLoanManager(qos_);
    {
        EndpointAttributes endpoint_attributes;
        endpoint_attributes.endpointKind = READER;
        endpoint_attributes.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
        endpoint_attributes.setEntityID(qos_.endpoint().entity_id);
        endpoint_attributes.setUserDefinedID(qos_.endpoint().user_defined_id);
        (void)vbs::DomainParticipantImpl::preprocess_endpoint_attributes<READER, 0x04, 0x07>(
            EntityId_t::unknown(), subscriber_->get_participant()->id_counter(), endpoint_attributes, guid_.entityId);
        guid_.guidPrefix = subscriber_->get_participant()->guid().guidPrefix;
    }

    reader_qos.exportDataReaderQosInfo();

    subscriber_->add_datareader(dynamic_cast<vbs::TopicImpl*>(topic), this);

    if (is_enabled() && auto_enable) {
        if (evbs::ReturnCode_t::RETCODE_OK != enable()) {
            (void)subscriber_->delete_datareader(this);
            return false;
        }
    }

    logInfo(SUBSCRIBER, "Reader " << guid() << " created in topic: " << topic->get_name() << " enable:" << auto_enable
                                  << " mask:" << get_status_mask());

    return true;
}

bool DataReaderImpl::datareader_init_with_profile(vbs::TopicDescription* topic_desc,
                                                  const vbsutil::xmlparser::DataReaderQos& qos_input,
                                                  bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    vbsutil::xmlparser::DataReaderQos qos = qos_input;
    TopicImpl* topic = subscriber_->get_topic_from_desc(topic_desc);
    if (topic) {
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
        if (topic->get_qos().send_multi_from_xml()) {
            qos.send_multi(topic->get_qos().send_multi());
        }
    }
    return datareader_init(topic_desc, qos, auto_enable);
}

void DataReaderImpl::set_user_listener(vbs::DataReaderListener* const listener, const vbs::StatusMask& mask) {
    reader_listener_ = listener;
    status_mask_ = *(mask.get_dds_status_mask());
}

void DataReaderImpl::user_data_available(DataReaderImpl* reader) {
    (void)reader;
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif

    if (reader_listener_ != nullptr) {
        reader_listener_->on_data_available(user_datareader_);
    }
}

void DataReaderImpl::user_subscription_matched(DataReaderImpl* reader,
                                               const evbs::edds::dds::SubscriptionMatchedStatus& info) {
    (void)reader;
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif
    //const std::unique_lock<std::mutex> lock2(match_mutex_);
    vbs_qos_.dds_reader_qos(qos_);
    if (info.current_count_change == 1) {
        match_count_ = match_count_ + 1;
    } else if (info.current_count_change == -1) {
        match_count_ = match_count_ - 1;
    } else {
        VbsLogW("DDS on_subscription_matched current_count_change is not except");
    }
    evbs::edds::dds::SubscriptionMatchedStatus info_w = info;
    info_w.current_count = match_count_;
    if (reader_listener_ != nullptr) {
        const vbs::SubscriptionMatchedStatus status_now(info_w);
        reader_listener_->on_subscription_matched(user_datareader_, status_now);
    }
}

void DataReaderImpl::user_requested_deadline_missed(DataReaderImpl* reader,
                                                    const evbs::edds::dds::RequestedDeadlineMissedStatus& status) {
    (void)reader;
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif
    //const std::unique_lock<std::mutex> lock2(listener_mutex_);
    if (reader_listener_ != nullptr) {
        const vbs::RequestedDeadlineMissedStatus status_now(status);
        reader_listener_->on_requested_deadline_missed(user_datareader_, status_now);
    }
}

void DataReaderImpl::user_liveliness_changed(DataReaderImpl* reader,
                                             const evbs::edds::dds::LivelinessChangedStatus& status) {
    (void)reader;
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif
    //const std::unique_lock<std::mutex> lock2(listener_mutex_);
    if (reader_listener_ != nullptr) {
        const vbs::LivelinessChangedStatus status_now(status);
        reader_listener_->on_liveliness_changed(user_datareader_, status_now);
    }
}

void DataReaderImpl::user_sample_lost(DataReaderImpl* reader, const evbs::edds::dds::SampleLostStatus& status) {
    (void)reader;
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif
    //const std::unique_lock<std::mutex> lock2(listener_mutex_);
    if (reader_listener_ != nullptr) {
        const vbs::SampleLostStatus status_now(status);
        reader_listener_->on_sample_lost(user_datareader_, status_now);
    }
}

void DataReaderImpl::user_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) {
#if !defined(_WIN32)
    const std::shared_lock<std::shared_mutex> lock(reader_mutex_);
#else
    const std::lock_guard<std::mutex> lock(reader_mutex_);
#endif
    //const std::unique_lock<std::mutex> lock2(listener_mutex_);
    if (reader_listener_ != nullptr) {
        const vbs::E2EExceptionStatus status_now(status);
        reader_listener_->on_e2e_exception(status_now);
    }
}

void DataReaderImpl::user_endpoint_crash(const evbs::ertps::rtps::GUID_t& reader_guid) {
    if (reader_listener_ != nullptr) {
        const vbs::EndpointCrashStatus status(reader_participant_->get_domain_id(), reader_guid);
        reader_listener_->on_endpoint_crash(user_datareader_, status);
    }
}

void DataReaderImpl::user_requested_incompatible_qos(DataReaderImpl* reader,
                                                     const RequestedIncompatibleQosStatus& status) {
    (void)reader;
    (void)status;
}
void DataReaderImpl::user_sample_rejected(DataReaderImpl* reader, const evbs::ertps::SampleRejectedStatus& status) {
    (void)reader;
    (void)status;
}

ReturnCode_t DataReaderImpl::take_flatdata(std::shared_ptr<vbs::LoanableCollectionFlatDataImpl>& datas,
                                           std::shared_ptr<vbs::SampleInfoImpl>& infos) {
    std::shared_ptr<vbs::LoanableCollectionFlatDataImpl> loanable_collection =
        std::make_shared<vbs::LoanableCollectionFlatDataImpl>();
    std::shared_ptr<vbs::SampleInfoImpl> sample_info = std::make_shared<vbs::SampleInfoImpl>();
    sample_info->dny_dds_infos_ = evbs::edds::dds::SampleInfoSeq();

#if !defined(_WIN32)
    take(loanable_collection->dds_load_seq_, std::get<1>(sample_info->dny_dds_infos_));
#else
    take(loanable_collection->dds_load_seq_, std::get<1>(sample_info->dny_dds_infos_));
#endif

    vbs_topic_->update_network_traffic(
        (type_->m_typeSize) * static_cast<int>(loanable_collection->dds_load_seq_.length()),
        static_cast<int>(loanable_collection->dds_load_seq_.length()), vbs::TRAFFIC_TYPE::RECV_TRAFFIC);
    loanable_collection->dny_dds_infos_ = sample_info;
    loanable_collection->dds_reader_ = this;

    datas = std::move(loanable_collection);
    infos = std::move(sample_info);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_buffer(std::shared_ptr<vbs::LoanableCollectionFlatDataImpl>& datas,
                                         std::shared_ptr<vbs::SampleInfoImpl>& infos) {
    std::shared_ptr<vbs::LoanableCollectionFlatDataImpl> loanable_collection =
        std::make_shared<vbs::LoanableCollectionFlatDataImpl>();
    std::shared_ptr<vbs::SampleInfoImpl> sample_info = std::make_shared<vbs::SampleInfoImpl>();
    sample_info->dny_dds_infos_ = evbs::edds::dds::SampleInfoSeq();

#if !defined(_WIN32)
    take_buffer(loanable_collection->dds_load_seq_, std::get<1>(sample_info->dny_dds_infos_));
#else
    take_buffer(loanable_collection->dds_load_seq_, std::get<1>(sample_info->dny_dds_infos_));
#endif

    vbs_topic_->update_network_traffic(
        (type_->m_typeSize) * static_cast<int>(loanable_collection->dds_load_seq_.length()),
        static_cast<int>(loanable_collection->dds_load_seq_.length()), vbs::TRAFFIC_TYPE::RECV_TRAFFIC);
    loanable_collection->dny_dds_infos_ = sample_info;
    loanable_collection->dds_reader_ = this;

    datas = std::move(loanable_collection);
    infos = std::move(sample_info);
    return ReturnCode_t::RETCODE_OK;
}

const DataReaderQosImpl& DataReaderImpl::get_qos() const {
    return vbs_qos_;
}

bool DataReaderImpl::deserialize(void* payload_buffer, const uint32_t payload_len, void* data) {
    SerializedPayload_t payload;
    payload.data = reinterpret_cast<octet*>(payload_buffer);
    payload.length = payload_len;
    payload.max_size = payload_len;
    const bool res = type_->deserialize(&payload, data);
    payload.data = nullptr;
    return res;
}

ReturnCode_t DataReaderImpl::take_next_sample(vbs::VBSDynamicData* dynamic_data, SampleInfoImpl* sample_info) {
    ReturnCode_t ret = ReturnCode_t::RETCODE_ERROR;
    if (dynamic_data == nullptr) {
        return ret;
    }

    vbs::SampleInfoInner dds_info;
    ret = take_next_sample(static_cast<void*>(dynamic_data->vbs_dynamic_data_), &dds_info)();
    if (sample_info) {
        sample_info->dny_dds_infos_ = dds_info;
    }

    if (ret == ReturnCode_t::RETCODE_OK && vbs_topic_) {
        vbs_topic_->update_network_traffic(type_->m_typeSize, 1, vbs::TRAFFIC_TYPE::RECV_TRAFFIC);
    }

    return ret;
}

ReturnCode_t DataReaderImpl::take_next_sample(void* data, SampleInfoImpl* sample_info) {
    ReturnCode_t ret = ReturnCode_t::RETCODE_ERROR;
    if (data == nullptr) {
        return ret;
    }

    vbs::SampleInfoInner dds_info;
    ret = take_next_sample(data, &dds_info)();
    if (sample_info) {
        sample_info->dny_dds_infos_ = dds_info;
    }

    if (ret == ReturnCode_t::RETCODE_OK && vbs_topic_) {
        vbs_topic_->update_network_traffic(type_->m_typeSize, 1, vbs::TRAFFIC_TYPE::RECV_TRAFFIC);
    }
    return ret;
}

bool DataReaderImpl::init_verify() {
    return dds_is_initialized_;
}

uint16_t DataReaderImpl::get_guid() {
    return reader_guid_;
}

bool DataReaderImpl::GetQos(DataReaderQosImpl& qos) {
    qos = vbs_qos_;
    return true;
}

bool DataReaderImpl::SetQos(const DataReaderQosImpl& qos) {
    vbs_qos_ = qos;
    return true;
}

}  // namespace vbs
