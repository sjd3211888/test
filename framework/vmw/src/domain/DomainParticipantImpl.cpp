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
// feature: VBS support SOA log collection.
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager

#include <chrono>
#include <string>
#include <asio.hpp>

#include "edds/rtps/attributes/PropertyPolicy.h"
#include "edds/rtps/attributes/RTPSParticipantAttributes.h"
#include "edds/rtps/participant/ParticipantDiscoveryInfo.h"
#include "edds/rtps/participant/RTPSParticipant.h"
#include "edds/rtps/reader/ReaderDiscoveryInfo.h"
#include "edds/rtps/RTPSDomain.h"
#include "edds/rtps/writer/WriterDiscoveryInfo.h"
#include "ertps/attributes/PublisherAttributes.h"
#include "ertps/attributes/SubscriberAttributes.h"
#include "ertps/types/DynamicPubSubType.h"
#include "ertps/types/DynamicType.h"
#include "ertps/types/TypeDescriptor.h"
#include "ertps/types/DynamicTypeBuilderFactory.h"
#include "ertps/types/DynamicTypeMember.h"
#include "ertps/types/TypeObjectFactory.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "ecdr/exceptions/BadParamException.h"
#include "edds/rtps/builtin/BuiltinProtocols.h"
#include "utils/QosPolicyUtils.hpp"
#include "edds/dds/builtin/lookup/LookupManager.hpp"
#include "edds/dds/builtin/lookup/LookupRequestListener.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/DomainParticipantFactoryInner.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "elog/Log.hpp"
#include "pub/Publisher.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/Subscriber.hpp"
#include "deps/common/TypeSupport.hpp"
#include "edds/rtps/interfaces/IContentFilter.hpp"
#include "topic/TopicCore.hpp"
#include "topic/TopicProxy.hpp"
#include "topic/TopicProxyFactory.hpp"
#include "topic/ContentFilteredTopicCore.hpp"
#include "utils/QosConverters.hpp"
#include "rtps/RTPSDomainImpl.hpp"
#include "deps/common/SystemInfo.hpp"
#include "dispatcher/DispatcherParticipant.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "pub/DataWriterImpl.hpp"
#include "context/QosManager.hpp"
#include "edds/rtps/common/E2E_common.h"
#include "vbs/DomainParticipant.hpp"

namespace vbs {

using evbs::ertps::ParticipantAttributes;
using evbs::ertps::PublisherAttributes;
using evbs::ertps::SubscriberAttributes;
using evbs::ertps::TopicAttributes;
using evbs::ertps::rtps::ParticipantDiscoveryInfo;

using evbs::ertps::xmlparser::XMLP_ret;
using evbs::ertps::xmlparser::XMLProfileManager;

using evbs::ertps::rtps::EndpointKind_t;
using evbs::ertps::rtps::GUID_t;
using evbs::ertps::rtps::ReaderDiscoveryInfo;
using evbs::ertps::rtps::ReaderProxyData;
using evbs::ertps::rtps::ResourceEvent;
using evbs::ertps::rtps::WriterDiscoveryInfo;
using evbs::ertps::rtps::WriterProxyData;

DomainParticipantImpl::DomainParticipantImpl(DomainId_t did, const vbs::DomainParticipantQosInner& qos,
                                             const evbs::edds::dds::StatusMask& mask,
                                             DomainParticipantListenerInner* listen)
    : Entity(mask),
      domain_id_(did),
      next_instance_id_(0U),
      qos_(qos),
      dispatcher_participant_(nullptr),
      listener_(listen),
      default_pub_qos_(vbsutil::xmlparser::getDefaultPublisherQos()),
      default_sub_qos_(vbsutil::xmlparser::getDefaultSubscriberQos()),
      default_topic_qos_(vbsutil::xmlparser::getDefaultTopicQos()),
      id_counter_(0U),
#pragma warning(disable : 4355)
      rtps_listener_(this) {
    qos_.transport().transport_type = qos.transport().transport_type;

    std::unique_ptr<PublisherAttributes> pub_attr(new PublisherAttributes);
    XMLProfileManager::getDefaultPublisherAttributes(*pub_attr);
    utils::set_qos_from_attributes(default_pub_qos_, *pub_attr);

    std::unique_ptr<SubscriberAttributes> sub_attr(new SubscriberAttributes);
    XMLProfileManager::getDefaultSubscriberAttributes(*sub_attr);
    utils::set_qos_from_attributes(default_sub_qos_, *sub_attr);

    std::unique_ptr<TopicAttributes> top_attr(new TopicAttributes);
    XMLProfileManager::getDefaultTopicAttributes(*top_attr);
    utils::set_qos_from_attributes(default_topic_qos_, *top_attr);

    // Pre calculate participant id and generated guid
    participant_id_ = qos_.wire_protocol().participant_id;
    evbs::ertps::rtps::RTPSDomainImpl::create_participant_guid(participant_id_, guid_);

    std::string* guid_property =
        evbs::ertps::rtps::PropertyPolicyHelper::find_property(qos_.properties(), "vbs.guidprefix");
    if (guid_property != nullptr) {
        uint32_t hash = evbs::ertps::rtps::e2eCommon::calculateCRC32P4(
            0, reinterpret_cast<const uint8_t*>(guid_property->data()), guid_property->size(), true);
        guid_.guidPrefix.value[4] = static_cast<octet>(hash & 0xFF);
        guid_.guidPrefix.value[5] = static_cast<octet>((hash >> 8) & 0xFF);
        guid_.guidPrefix.value[6] = static_cast<octet>((hash >> 16) & 0xFF);
        guid_.guidPrefix.value[7] = static_cast<octet>((hash >> 24) & 0xFF);
    }

    // 中心server负责类型中转，需要同时具有收发功能
    if (qos_.wire_protocol().builtin.discovery_config.discoveryProtocol ==
        evbs::ertps::rtps::DiscoveryProtocol_t::SERVER) {
        qos_.wire_protocol().builtin.lookup_config.use_client = true;
        qos_.wire_protocol().builtin.lookup_config.use_server = true;
    }

    type_lookup_ =
        qos_.wire_protocol().builtin.lookup_config.use_client || qos_.wire_protocol().builtin.lookup_config.use_server;

    /* Fill physical data properties if they are found and empty */
    std::string* property_value =
        evbs::ertps::rtps::PropertyPolicyHelper::find_property(qos_.properties(), parameter_policy_physical_data_host);
    if ((nullptr != property_value) && property_value->empty()) {
        try {
            (void)property_value->assign(asio::ip::host_name() + ":" + std::to_string(utils::default_domain_id()));
        } catch (asio::system_error const& ex) {
            std::cerr << "Caught system error: " << ex.what() << '\n';
            std::cerr << "Error code: " << ex.code() << '\n';
            return;
        }
    }

    property_value =
        evbs::ertps::rtps::PropertyPolicyHelper::find_property(qos_.properties(), parameter_policy_physical_data_user);
    if ((nullptr != property_value) && property_value->empty()) {
        std::string username = "unknown";
        if (ReturnCode_t::RETCODE_OK == vbs::SystemInfo::get_username(username)) {
            (void)property_value->assign(username);
        }
    }

    property_value = evbs::ertps::rtps::PropertyPolicyHelper::find_property(qos_.properties(),
                                                                            parameter_policy_physical_data_process);
    if ((nullptr != property_value) && property_value->empty()) {
        (void)property_value->assign(std::to_string(vbs::SystemInfo::instance().process_id()));
    }
    logInfo(PARTICIPANT, "[VBS_VERSION:" << VBS_VERSION_VALUE << "]");
}

void DomainParticipantImpl::disable() {
    set_listener(nullptr);

    // The function to disable the DomainParticipantImpl is called from
    // DomainParticipantFactoryInner::delete_participant() and DomainParticipantFactoryInner destructor.
    auto dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant != nullptr) {
        (void)dispatcher_participant->set_listener(nullptr);

        {
            const std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto pub_it = publishers_by_handle_.begin(); pub_it != publishers_by_handle_.end(); ++pub_it) {
                pub_it->second->disable();
            }
        }

        {
            const std::lock_guard<std::mutex> lock(mtx_subs_);
            for (auto sub_it = subscribers_by_handle_.begin(); sub_it != subscribers_by_handle_.end(); ++sub_it) {
                sub_it->second->disable();
            }
        }
    }
}

DomainParticipantImpl::~DomainParticipantImpl() {
    if (batch_free_ != true) {
        DomainParticipantFactoryInner::get_instance()->participant_has_been_deleted(this);
    }

    {
        const std::lock_guard<std::mutex> lock(mtx_pubs_);
        for (auto pub_it = publishers_by_handle_.begin(); pub_it != publishers_by_handle_.end(); ++pub_it) {
            pub_it->second->batch_free_ = true;
            delete pub_it->second;
        }
        publishers_by_handle_.clear();
    }

    {
        const std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto sub_it = subscribers_by_handle_.begin(); sub_it != subscribers_by_handle_.end(); ++sub_it) {
            sub_it->second->batch_free_ = true;
            delete sub_it->second;
        }
        subscribers_by_handle_.clear();
    }

    {
        const std::lock_guard<std::mutex> lock(mtx_topics_);
        filtered_topics_.clear();
        for (auto topic_it = topics_.begin(); topic_it != topics_.end(); ++topic_it) {
            delete topic_it->second;
        }
        topics_.clear();
        topics_by_handle_.clear();
    }

    if (dispatcher_participant_ != nullptr) {
        delete dispatcher_participant_;
    }

    {
        const std::lock_guard<std::mutex> lock(mtx_types_);
        types_.clear();
    }
}

ReturnCode_t DomainParticipantImpl::set_listener(DomainParticipantListenerInner* const listener) {
    return set_listener(listener, std::chrono::seconds::max());
}

ReturnCode_t DomainParticipantImpl::set_listener(DomainParticipantListenerInner* const listener,
                                                 const std::chrono::seconds timeout) {
    return set_listener(listener, evbs::edds::dds::StatusMask::all(), timeout);
}

ReturnCode_t DomainParticipantImpl::set_listener(DomainParticipantListenerInner* const listener,
                                                 const evbs::edds::dds::StatusMask& mask) {
    return set_listener(listener, mask, std::chrono::seconds::max());
}

ReturnCode_t DomainParticipantImpl::set_listener(DomainParticipantListenerInner* const listener,
                                                 const evbs::edds::dds::StatusMask& mask,
                                                 const std::chrono::seconds timeout) {
    ReturnCode_t ret_val = ReturnCode_t::RETCODE_OK;
    auto time_out = std::chrono::time_point<std::chrono::system_clock>::max();
    if (timeout < std::chrono::seconds::max()) {
        auto now = std::chrono::system_clock::now();
        time_out = now + timeout;
    }

    std::unique_lock<std::mutex> lock(mtx_gs_);
    if (!cv_gs_.wait_until(lock, time_out, [this, listener, &ret_val] {
            if (rtps_listener_.callback_counter_ <= 0) {
                rtps_listener_.callback_counter_ = (listener == nullptr) ? -1 : 0;
                listener_ = listener;
                ret_val = ReturnCode_t::RETCODE_OK;
            } else {
                ret_val = ReturnCode_t::RETCODE_ERROR;
            }
            return ret_val == ReturnCode_t::RETCODE_OK;
        })) {
        ret_val = ReturnCode_t::RETCODE_ERROR;
    }

    if (ret_val == ReturnCode_t::RETCODE_OK) {
        status_mask_ = mask;
    }

    return ret_val;
}

vbsutil::elog::ReturnCode_t DomainParticipantImpl::enable() {
    if (enable_) {
        return ReturnCode_t::RETCODE_OK;
    }

    enable_ = true;
    // Should not have been previously enabled
    assert(get_dispatcher_participant() == nullptr);

    evbs::ertps::rtps::RTPSParticipantAttributes rtps_attr;
    utils::set_attributes_from_qos(rtps_attr, qos_);
    rtps_attr.participantID = participant_id_;

    // If DEFAULT_VBS_MASTER_URI is specified then try to create default client if
    // that already exists.
    vbs::dispatcher::DispatcherParticipant* part =
        new vbs::dispatcher::DispatcherParticipant(domain_id_, rtps_attr, &rtps_listener_);
    //RTPSParticipant* part = RTPSDomain::createParticipant(domain_id_, false, rtps_attr, &rtps_listener_);

    if (part == nullptr) {
        enable_ = false;
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    if (!part->active()) {
        delete part;
        enable_ = false;
        return vbsutil::elog::ReturnCode_t::RETCODE_ERROR;
    }

    //part->set_parent_domain_participant(this);
    guid_ = part->getGuid();
    {
        const std::lock_guard<std::mutex> _(mtx_gs_);

        dispatcher_participant_ = part;

#if 0
        part->set_check_type_function(
            [this](const std::string& type_name) -> bool { return find_type(type_name).get() != nullptr; });
#endif
    }
    // auto enable
    {
        // enable topics first
        {
            const std::lock_guard<std::mutex> lock(mtx_topics_);

            for (auto& topic : topics_) {
                topic.second->enable_topic();
            }
        }

        // Enable publishers
        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto& pub : publishers_by_handle_) {
                pub.second->dispatcher_participant_ = part;
                (void)pub.second->enable();
            }
        }

        // Enable subscribers
        {
            std::lock_guard<std::mutex> lock(mtx_subs_);

            for (auto& sub : subscribers_by_handle_) {
                sub.second->dispatcher_participant_ = part;
                (void)sub.second->enable();
            }
        }
    }
    part->enable();
    initToolHookGetInfoFunc();
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::set_qos(const vbs::DomainParticipantQosInner& qos) {
    bool enabled = false;
    bool qos_should_be_updated = false;
    evbs::ertps::rtps::RTPSParticipantAttributes patt;
    vbs::dispatcher::DispatcherParticipant* dispatcher_participant = nullptr;

    {
        const std::lock_guard<std::mutex> _(mtx_gs_);

        dispatcher_participant = dispatcher_participant_;
        enabled = (dispatcher_participant != nullptr);
        const vbs::DomainParticipantQosInner& qos_to_set =
            (&qos == &PARTICIPANT_QOS_DEFAULT)
                ? DomainParticipantFactoryInner::get_instance()->get_default_participant_qos()
                : qos;

        if (&qos != &PARTICIPANT_QOS_DEFAULT) {
            ReturnCode_t ret_val = check_qos(qos_to_set);
            if (!ret_val) {
                return ret_val;
            }
        }

        if (enabled && (!can_qos_be_updated(qos_, qos_to_set))) {
            return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
        }

        qos_should_be_updated = set_qos(qos_, qos_to_set, !enabled);
        if (enabled) {
            if (qos_should_be_updated) {
                // Notify the participant that there is a QoS update
                utils::set_attributes_from_qos(patt, qos_);
            } else {
                // Trigger update of network interfaces by calling update_attributes with current
                // attributes
                patt = dispatcher_participant->getParticipantAttributes();
            }
        }
    }

    if (enabled) {
        (void)dispatcher_participant->update_attributes(patt);
    }

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_qos(vbs::DomainParticipantQosInner& qos) const {
    std::lock_guard<std::mutex> _(mtx_gs_);
    qos = qos_;
    return ReturnCode_t::RETCODE_OK;
}

const vbs::DomainParticipantQosInner& DomainParticipantImpl::get_qos() const {
    const std::lock_guard<std::mutex> _(mtx_gs_);
    return qos_;
}

DomainParticipantListenerInner* DomainParticipantImpl::get_listener() const {
    return listener_;
}

ReturnCode_t DomainParticipantImpl::delete_publisher(const Publisher* pub) {
    if (nullptr == pub) {
        return ReturnCode_t::RETCODE_ERROR;
    }

    if (this != pub->get_participant()) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    if (pub->batch_free_ == true) {
        return ReturnCode_t::RETCODE_OK;
    }
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    auto pit = publishers_by_handle_.find(pub->get_instance_handle());
    if (pit != publishers_by_handle_.end()) {
        if (pub->has_datawriters()) {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        (void)pit->second->set_listener(nullptr);
        delete pit->second;
        (void)publishers_by_handle_.erase(pit);
        return ReturnCode_t::RETCODE_OK;
    } else {
        return ReturnCode_t::RETCODE_ERROR;
    }
}

ReturnCode_t DomainParticipantImpl::delete_subscriber(Subscriber* sub) {
    if (nullptr == sub) {
        return ReturnCode_t::RETCODE_ERROR;
    }
    if (this != sub->get_participant()) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    if (sub->batch_free_ == true) {
        return ReturnCode_t::RETCODE_OK;
    }
    std::lock_guard<std::mutex> lock(mtx_subs_);
    if (subscribers_by_handle_.find(sub->get_instance_handle()) != subscribers_by_handle_.end()) {
        if (sub->has_datareaders()) {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        (void)sub->set_listener(nullptr);
        (void)subscribers_by_handle_.erase(subscribers_by_handle_.find(sub->get_instance_handle()));
        delete sub;
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_ERROR;
}

TopicImpl* DomainParticipantImpl::find_topic(const std::string& topic_name, const evbs::ertps::Duration_t& timeout) {
    auto find_fn = [this, &topic_name]() {
        return topics_.count(topic_name) > 0U;
    };

    std::unique_lock<std::mutex> lock(mtx_topics_);
    if (timeout.is_infinite()) {
        cond_topics_.wait(lock, find_fn);
    } else {
        auto duration = std::chrono::seconds(timeout.seconds) + std::chrono::nanoseconds(timeout.nanosec);
        if (!cond_topics_.wait_for(lock, duration, find_fn)) {
            return nullptr;
        }
    }

    TopicImpl* ret_val = topics_[topic_name]->create_topic()->get_topic();

    InstanceHandle_t topic_handle;
    create_instance_handle(topic_handle);
    ret_val->set_instance_handle(topic_handle);
    topics_by_handle_[topic_handle] = ret_val;

    return ret_val;
}

void DomainParticipantImpl::set_topic_listener(const TopicProxyFactory* factory, vbs::TopicCore* topic,
                                               TopicListener* const listener, const evbs::edds::dds::StatusMask& mask) {
    const std::lock_guard<std::mutex> lock(mtx_topics_);
    topic->set_listener(listener);
    factory->for_each([mask](const std::unique_ptr<TopicProxy>& proxy) { proxy->get_topic()->status_mask_ = mask; });
}

ReturnCode_t DomainParticipantImpl::delete_topic(const TopicImpl* topic) {
    if (topic == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "Topic is null to delete.");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(mtx_topics_);
    auto handle_it =
        std::find_if(topics_by_handle_.begin(), topics_by_handle_.end(),
                     [topic](const decltype(topics_by_handle_)::value_type& item) { return item.second == topic; });
    if (handle_it != topics_by_handle_.end()) {
        auto it = topics_.find(topic->get_name());
        if (it == topics_.end()) {
            elogError(PARTICIPANT, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                      "Topic found by handle but factory not found");
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        InstanceHandle_t handle = topic->get_instance_handle();

        logInfo(PARTICIPANT, guid_ << " Delete topic successfully."
                                   << " Topic " << topic->get_name());
        TopicProxy* proxy = dynamic_cast<TopicProxy*>(topic->get_impl());
        assert(nullptr != proxy);
        auto ret_code = it->second->delete_topic(proxy);
        if (ReturnCode_t::RETCODE_OK == ret_code) {
            (void)topics_by_handle_.erase(handle);

            if (it->second->can_be_deleted()) {
                auto factory = it->second;
                (void)topics_.erase(it);
                delete factory;
            }
        }
        return ReturnCode_t(ret_code());
    }
    elogError(PARTICIPANT, RetCode_t::RETCODE_PRECONDITION_NOT_MET, "Topic not found.");
    return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

vbs::ContentFilteredTopicInner* DomainParticipantImpl::create_contentfilteredtopic(
    const std::string& name, TopicImpl* related_topic, const std::string& filter_expression,
    const std::vector<std::string>& expression_parameters) {
    return create_contentfilteredtopic(name, related_topic, filter_expression, expression_parameters,
                                       EDDS_SQLFILTER_NAME);
}

vbs::ContentFilteredTopicInner* DomainParticipantImpl::create_contentfilteredtopic(
    const std::string& name, TopicImpl* related_topic, const std::string& filter_expression,
    const std::vector<std::string>& expression_parameters, const char* filter_class_name) {
    if ((nullptr == related_topic) || (nullptr == filter_class_name)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mtx_topics_);

    // Check there is no Topic with the same name
    if ((topics_.find(name) != topics_.end()) || (filtered_topics_.find(name) != filtered_topics_.end())) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Topic with name : " << name << " already exists");
        return nullptr;
    }

    if (related_topic->get_participant() != this) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Creating ContentFilteredTopic with name " << name << ": related_topic not from this participant");
        return nullptr;
    }

    evbs::ertps::rtps::IContentFilterFactory* filter_factory = find_content_filter_factory(filter_class_name);
    if (nullptr == filter_factory) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Could not find factory for filter class " << filter_class_name);
        return nullptr;
    }

    if (expression_parameters.size() > vbsutil::xmlparser::ContentFilterProperty::parameters_limit_) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Number of expression parameters exceeds maximum allocation limit: "
                      << expression_parameters.size() << " > "
                      << vbsutil::xmlparser::ContentFilterProperty::parameters_limit_);
        return nullptr;
    }

    if (expression_parameters.size() > 100) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER,
                  "Number of expression parameters exceeds maximum protocol limit: " << expression_parameters.size()
                                                                                     << " > 100");
        return nullptr;
    }

    LoanableSequence<const char*>::size_type n_params;
    n_params = static_cast<LoanableSequence<const char*>::size_type>(expression_parameters.size());
    LoanableSequence<const char*> filter_parameters(n_params);
    filter_parameters.length(n_params);
    while (n_params > 0) {
        n_params--;
        if (n_params >= static_cast<LoanableSequence<const char*>::size_type>(expression_parameters.size())) {
            continue;
        }
        filter_parameters[n_params] = expression_parameters[n_params].c_str();
    }

    vbs::ContentFilteredTopicInner* topic;
    topic = new vbs::ContentFilteredTopicInner(name, related_topic, filter_expression, expression_parameters);
    vbs::ContentFilteredTopicCore* content_topic_impl = static_cast<vbs::ContentFilteredTopicCore*>(topic->get_impl());
    content_topic_impl->filter_property.filter_class_name = filter_class_name;
    content_topic_impl->filter_factory = filter_factory;
    content_topic_impl->filter_instance = nullptr;
    content_topic_impl->update_signature();

    // Save the topic into the map
    filtered_topics_.emplace(std::make_pair(name, std::unique_ptr<vbs::ContentFilteredTopicInner>(topic)));
    std::string params;
    for (auto& param : expression_parameters) {
        params += param + ",";
    }
    logInfo(PARTICIPANT, "Content filter topic "
                             << name << " created, related topic:" << related_topic->get_impl()->get_rtps_topic_name()
                             << " expression:" << filter_expression << " params:" << params);
    return topic;
}

ReturnCode_t DomainParticipantImpl::delete_contentfilteredtopic(const vbs::ContentFilteredTopicInner* topic) {
    if (topic == nullptr) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_BAD_PARAMETER,
                  " Param is nullptr when deleting content filter topic.");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(mtx_topics_);
    auto it = filtered_topics_.find(topic->get_name());

    if (it != filtered_topics_.end()) {
        if (it->second->get_impl()->is_referenced()) {
            elogError(PARTICIPANT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
                      " Content filter topic " << topic->get_name() << " is in use.");
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
        filtered_topics_.erase(it);
        return ReturnCode_t::RETCODE_OK;
    }
    elogError(PARTICIPANT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
              " Content filter topic " << topic->get_name() << " not found.");
    return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

const InstanceHandle_t& DomainParticipantImpl::get_instance_handle() const {
    return static_cast<const InstanceHandle_t&>(guid_);
}

const GUID_t& DomainParticipantImpl::guid() const {
    return guid_;
}

Publisher* DomainParticipantImpl::create_publisher(const PublisherQos& qos, PublisherListener* listener,
                                                   const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    if (!Publisher::check_qos(qos)) {
        // The Publisher::check_qos() function is not yet implemented and always returns
        // ReturnCode_t::RETCODE_OK. It will be implemented in future releases of EDDS.
        // elogError(PARTICIPANT, ReturnCode_t::RETCODE_INCONSISTENT_POLICY, "PublisherQos inconsistent or not supported");
        // return nullptr;
    }

    // TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    Publisher* pub = new Publisher(this, qos, listener, mask);
    pub->dispatcher_participant_ = get_dispatcher_participant();
    bool enabled = get_dispatcher_participant() != nullptr;

    // Create InstanceHandle for the new publisher
    InstanceHandle_t pub_handle;
    create_instance_handle(pub_handle);
    pub->handle_ = pub_handle;

    // SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_pubs_);
    publishers_by_handle_[pub_handle] = pub;

    // Enable publisher if appropriate
    if (enabled && auto_enable) {
        (void)pub->enable();
    }

    return pub;
}

Publisher* DomainParticipantImpl::create_publisher_with_profile(const std::string& profile_name,
                                                                PublisherListener* listener,
                                                                const evbs::edds::dds::StatusMask& mask,
                                                                bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, *attr)) {
        PublisherQos qos = default_pub_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return create_publisher(qos, listener, mask, auto_enable);
    }

    return nullptr;
}

ReturnCode_t DomainParticipantImpl::ignore_participant(const InstanceHandle_t& /*handle*/) {
    return ReturnCode_t::RETCODE_OK;
#if 0
    return (nullptr == dispatcher_participant_) ? ReturnCode_t::RETCODE_NOT_ENABLED :
           dispatcher_participant_->ignore_participant(iHandle2GUID(handle).guidPrefix) ? ReturnCode_t::RETCODE_OK :
           ReturnCode_t::RETCODE_BAD_PARAMETER;
#endif
}

ReturnCode_t DomainParticipantImpl::ignore_topic(const InstanceHandle_t& handle) {
    static_cast<void>(handle);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_publication(const InstanceHandle_t& handle) {
    static_cast<void>(handle);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_subscription(const InstanceHandle_t& handle) {
    static_cast<void>(handle);
    return ReturnCode_t::RETCODE_UNSUPPORTED;
}

DomainId_t DomainParticipantImpl::get_domain_id() const {
    return domain_id_;
}

ReturnCode_t DomainParticipantImpl::delete_contained_entities() {
    bool can_be_deleted = true;

    std::lock_guard<std::mutex> lock_subscribers(mtx_subs_);

    for (auto& subscriber : subscribers_by_handle_) {
        can_be_deleted = subscriber.second->can_be_deleted();
        if (!can_be_deleted) {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
    }

    std::lock_guard<std::mutex> lock_publishers(mtx_pubs_);

    for (auto& publisher : publishers_by_handle_) {
        can_be_deleted = publisher.second->can_be_deleted();
        if (!can_be_deleted) {
            return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
        }
    }

    ReturnCode_t ret_code = ReturnCode_t::RETCODE_OK;

    for (auto& subscriber : subscribers_by_handle_) {
        ret_code = ReturnCode_t(subscriber.second->delete_contained_entities()());
        if (!ret_code) {
            return ReturnCode_t::RETCODE_ERROR;
        }
    }

    auto it_subs = subscribers_by_handle_.begin();
    while (it_subs != subscribers_by_handle_.end()) {
        (void)it_subs->second->set_listener(nullptr);
        delete it_subs->second;
        it_subs = subscribers_by_handle_.erase(it_subs);
    }

    for (auto& publisher : publishers_by_handle_) {
        ret_code = ReturnCode_t(publisher.second->delete_contained_entities()());
        if (!ret_code) {
            return ReturnCode_t::RETCODE_ERROR;
        }
    }

    auto it_pubs = publishers_by_handle_.begin();
    while (it_pubs != publishers_by_handle_.end()) {
        (void)it_pubs->second->set_listener(nullptr);
        delete it_pubs->second;
        it_pubs = publishers_by_handle_.erase(it_pubs);
    }

    std::lock_guard<std::mutex> lock_topics(mtx_topics_);

    filtered_topics_.clear();
    topics_by_handle_.clear();

    auto it_topics = topics_.begin();
    while (it_topics != topics_.end()) {
        delete it_topics->second;
        it_topics = topics_.erase(it_topics);
    }

    return ReturnCode_t::RETCODE_OK;
}

RetCode_t DomainParticipantImpl::assert_liveliness() {
    vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        return RetCode_t::RETCODE_NOT_ENABLED;
    }
    return dispatcher_participant->assert_liveliness()();
}

ReturnCode_t DomainParticipantImpl::set_default_publisher_qos(const PublisherQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultPublisherQos()) {
        reset_default_publisher_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = Publisher::check_qos(qos);
    if (!ret_val) {
        // The Publisher::check_qos() function is not yet implemented and always returns
        // ReturnCode_t::RETCODE_OK. It will be implemented in future releases of EDDS. return
        // ret_val;
    }
    Publisher::set_qos(default_pub_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_publisher_qos() {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    Publisher::set_qos(default_pub_qos_, vbsutil::xmlparser::getDefaultPublisherQos(), true);
    std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    XMLProfileManager::getDefaultPublisherAttributes(*attr);
    utils::set_qos_from_attributes(default_pub_qos_, *attr);
}

const PublisherQos& DomainParticipantImpl::get_default_publisher_qos() const {
    return default_pub_qos_;
}

ReturnCode_t DomainParticipantImpl::get_default_publisher_qos(PublisherQos& qos) const {
    qos = default_pub_qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_publisher_qos_from_profile(const std::string& profile_name,
                                                                   PublisherQos& qos) const {
    std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, *attr)) {
        qos = default_pub_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantImpl::set_default_subscriber_qos(const SubscriberQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultSubscriberQos()) {
        reset_default_subscriber_qos();
        return ReturnCode_t::RETCODE_OK;
    }
    ReturnCode_t check_result = ReturnCode_t(Subscriber::check_qos(qos)());
    if (!check_result) {
        // The Subscriber::check_qos() function is not yet implemented and always returns
        // ReturnCode_t::RETCODE_OK. It will be implemented in future releases of EDDS. return
        // check_result;
    }
    Subscriber::set_qos(default_sub_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_subscriber_qos() {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    Subscriber::set_qos(default_sub_qos_, vbsutil::xmlparser::getDefaultSubscriberQos(), true);
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    XMLProfileManager::getDefaultSubscriberAttributes(*attr);
    utils::set_qos_from_attributes(default_sub_qos_, *attr);
}

const SubscriberQos& DomainParticipantImpl::get_default_subscriber_qos() const {
    return default_sub_qos_;
}

ReturnCode_t DomainParticipantImpl::get_default_subscriber_qos(SubscriberQos& qos) const {
    qos = default_sub_qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_subscriber_qos_from_profile(const std::string& profile_name,
                                                                    SubscriberQos& qos) const {
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile_name, *attr)) {
        qos = default_sub_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

ReturnCode_t DomainParticipantImpl::set_default_topic_qos(const vbsutil::xmlparser::TopicQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultTopicQos()) {
        reset_default_topic_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t ret_val = vbs::TopicCore::check_qos(qos);
    if (!ret_val) {
        return ret_val;
    }

    vbs::TopicCore::set_qos(default_topic_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::reset_default_topic_qos() {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    vbs::TopicCore::set_qos(default_topic_qos_, vbsutil::xmlparser::getDefaultTopicQos(), true);
    std::unique_ptr<TopicAttributes> attr(new TopicAttributes);
    XMLProfileManager::getDefaultTopicAttributes(*attr);
    utils::set_qos_from_attributes(default_topic_qos_, *attr);
}

const vbsutil::xmlparser::TopicQos& DomainParticipantImpl::get_default_topic_qos() const {
    return default_topic_qos_;
}

ReturnCode_t DomainParticipantImpl::get_default_topic_qos(vbsutil::xmlparser::TopicQos& qos) const {
    qos = default_topic_qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_topic_qos_from_profile(const std::string& profile_name,
                                                               vbsutil::xmlparser::TopicQos& qos) const {
    std::unique_ptr<TopicAttributes> attr(new TopicAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillTopicAttributes(profile_name, *attr)) {
        qos = default_topic_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

bool DomainParticipantImpl::contains_entity(const InstanceHandle_t& handle, bool recursive) const {
    // Look for publishers
    {
        std::lock_guard<std::mutex> lock(mtx_pubs_);
        if (publishers_by_handle_.find(handle) != publishers_by_handle_.end()) {
            return true;
        }
    }

    // Look for subscribers
    {
        std::lock_guard<std::mutex> lock(mtx_subs_);
        if (subscribers_by_handle_.find(handle) != subscribers_by_handle_.end()) {
            return true;
        }
    }

    // Look for topics
    {
        std::lock_guard<std::mutex> lock(mtx_topics_);
        if (topics_by_handle_.find(handle) != topics_by_handle_.end()) {
            return true;
        }
    }

    if (recursive) {
        // Look into publishers
        {
            std::lock_guard<std::mutex> lock(mtx_pubs_);
            for (auto& pit : publishers_by_handle_) {
                if (pit.second->contains_entity(handle)) {
                    return true;
                }
            }
        }

        // Look into subscribers
        {
            std::lock_guard<std::mutex> lock(mtx_subs_);
            for (auto& sit : subscribers_by_handle_) {
                if (sit.second->contains_entity(handle)) {
                    return true;
                }
            }
        }
    }

    return false;
}

ReturnCode_t DomainParticipantImpl::get_current_time(evbs::ertps::Time_t& current_time) const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    current_time.seconds = static_cast<int32_t>(seconds.count());
    current_time.nanosec = static_cast<uint32_t>(nanos.count());

    return ReturnCode_t::RETCODE_OK;
}

std::vector<std::string> DomainParticipantImpl::get_participant_names() const {
    std::lock_guard<std::mutex> _(mtx_gs_);
    return dispatcher_participant_ == nullptr ? std::vector<std::string> {}
                                              : dispatcher_participant_->getParticipantNames();
}

Subscriber* DomainParticipantImpl::create_subscriber(const SubscriberQos& qos, SubscriberListener* listener,
                                                     const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    if (!Subscriber::check_qos(qos)) {
        // The Subscriber::check_qos() function is not yet implemented and always returns
        // ReturnCode_t::RETCODE_OK. It will be implemented in future releases of EDDS.
        // elogError(PARTICIPANT, ReturnCode_t::RETCODE_INCONSISTENT_POLICY, "SubscriberQos inconsistent or not supported");
        // return nullptr;
    }

    // TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    Subscriber* sub = new Subscriber(this, qos, listener, mask);
    sub->dispatcher_participant_ = get_dispatcher_participant();

    // Create InstanceHandle for the new subscriber
    InstanceHandle_t sub_handle;
    bool enabled = get_dispatcher_participant() != nullptr;

    // Create InstanceHandle for the new subscriber
    create_instance_handle(sub_handle);
    sub->handle_ = sub_handle;

    // SAVE THE PUBLISHER INTO MAPS
    std::lock_guard<std::mutex> lock(mtx_subs_);
    subscribers_by_handle_[sub_handle] = sub;

    // Enable subscriber if appropriate
    if (enabled && auto_enable) {
        ReturnCode_t ret_subscriber_enable = sub->enable();
        assert(ReturnCode_t::RETCODE_OK == ret_subscriber_enable);
        (void)ret_subscriber_enable;
    }

    return sub;
}

Subscriber* DomainParticipantImpl::create_subscriber_with_profile(const std::string& profile_name,
                                                                  SubscriberListener* listener,
                                                                  const evbs::edds::dds::StatusMask& mask,
                                                                  bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillSubscriberAttributes(profile_name, *attr)) {
        SubscriberQos qos = default_sub_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return create_subscriber(qos, listener, mask, auto_enable);
    }

    return nullptr;
}

TopicImpl* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name,
                                               const std::string& profile_name) {
    const auto dyn_builder = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name);
    vbs::VbsDynamicPubSubTypeImpl* dynamic_entity = nullptr;
    if (dyn_builder != nullptr) {
        dynamic_entity = new vbs::VbsDynamicPubSubTypeImpl(dyn_builder->build());
    } else {
        dynamic_entity = new vbs::VbsDynamicPubSubTypeImpl();
        dynamic_entity->setName(type_name.c_str());
    }
    register_type(TypeSupport(dynamic_entity));

    if (static_cast<size_t>(profile_name.length()) > static_cast<size_t>(0)) {
        auto qos = QosManager::Instance()->get_topic_xml_qos(profile_name);
        if (qos != nullptr) {
            return create_topic_with_profile(topic_name, type_name, *qos);
        }
    } else {
        const vbsutil::xmlparser::TopicQos dds_topic_qos;
        // coverity[cert_exp60_cpp_violation:SUPPRESS]
        return create_topic(topic_name, type_name, dds_topic_qos);
    }

    return nullptr;
}

TopicImpl* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name,
                                               const TypeSupport& local_type, const std::string& profile_name) {
    if (!type_discovery_enabled()) {
        register_type(local_type);
    } else {
        unregister_type(type_name);
        register_type(local_type);
    }

    if (static_cast<size_t>(profile_name.length()) > static_cast<size_t>(0)) {
        auto qos = QosManager::Instance()->get_topic_xml_qos(profile_name);
        if (qos != nullptr) {
            return create_topic_with_profile(topic_name, type_name, *qos);
        }
    } else {
        const vbsutil::xmlparser::TopicQos dds_topic_qos;
        // coverity[cert_exp60_cpp_violation:SUPPRESS]
        return create_topic(topic_name, type_name, dds_topic_qos);
    }

    return nullptr;
}

TopicImpl* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name,
                                               DynamicType_ptr* dyn_type, const std::string& profile_name) {
    auto const dyn_builder = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name);
    if (dyn_builder != nullptr) {
        *dyn_type = dyn_builder->build();
        auto type = TypeSupport(new VbsDynamicPubSubTypeImpl(*dyn_type));
        register_type(std::move(type), type_name);

        if (static_cast<size_t>(profile_name.length()) > static_cast<size_t>(0)) {
            auto qos = QosManager::Instance()->get_topic_xml_qos(profile_name);
            if (qos != nullptr) {
                return create_topic_with_profile(topic_name, type_name, *qos);
            }
        } else {
            const vbsutil::xmlparser::TopicQos dds_topic_qos;
            return create_topic(topic_name, type_name, dds_topic_qos);
        }

    } else {
        const std::string msg = " Get dynamic type:" + type_name + " fail.";
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << msg);
    }

    return nullptr;
}

TopicImpl* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name,
                                               const TypeSupport& local_type, const TopicQosImpl& local_topic_qos) {
    if (!type_discovery_enabled()) {
        register_type(local_type);
    } else {
        unregister_type(type_name);
        register_type(local_type);
    }

    return create_topic(topic_name, type_name, local_topic_qos.get_dds_topic_qos());
}

TopicImpl* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name,
                                               const vbsutil::xmlparser::TopicQos& qos, TopicListener* listener,
                                               const evbs::edds::dds::StatusMask& mask) {
    // Look for the correct type registration
    TypeSupport type_support = find_type(type_name);
    if (type_support.empty()) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET, "Type : " << type_name << " Not Registered");
        return nullptr;
    }

    if (!vbs::TopicCore::check_qos_including_resource_limits(qos, type_support)) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_INCONSISTENT_POLICY, "TopicQos inconsistent or not supported");
        return nullptr;
    }

    bool enabled = get_dispatcher_participant() != nullptr;

    std::lock_guard<std::mutex> lock(mtx_topics_);

    // Check there is no Topic with the same name
    if ((topics_.find(topic_name) != topics_.end())) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_ILLEGAL_OPERATION,
                  "Topic with name : " << topic_name << " already exists");
        return nullptr;
    }

    InstanceHandle_t topic_handle;
    create_instance_handle(topic_handle);

    TopicProxyFactory* factory =
        new TopicProxyFactory(this, topic_name, type_name, mask, std::move(type_support), qos, listener);
    TopicProxy* proxy = factory->create_topic();
    TopicImpl* topic = proxy->get_topic();
    topic->set_instance_handle(topic_handle);

    // SAVE THE TOPIC INTO MAPS
    topics_by_handle_[topic_handle] = topic;
    topics_[topic_name] = factory;

    // Enable topic if appropriate
    if (enabled) {
        ReturnCode_t ret_topic_enable = topic->enable();
        assert(ReturnCode_t::RETCODE_OK == ret_topic_enable);
        (void)ret_topic_enable;
    }

    logInfo(PARTICIPANT, "Topic " << topic_name << " created in " << guid_ << " successfully."
                                  << " Type " << type_name);
    cond_topics_.notify_all();

    discovered_topic_names_.insert(topic_name);
    return topic;
}

TopicImpl* DomainParticipantImpl::create_topic_with_profile(const std::string& topic_name, const std::string& type_name,
                                                            const std::string& profile_name, TopicListener* listener,
                                                            const evbs::edds::dds::StatusMask& mask) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    std::unique_ptr<TopicAttributes> attr(new TopicAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillTopicAttributes(profile_name, *attr)) {
        vbsutil::xmlparser::TopicQos qos = default_topic_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return create_topic(topic_name, type_name, qos, listener, mask);
    }

    return nullptr;
}

TopicImpl* DomainParticipantImpl::create_topic_with_profile(const std::string& topic_name, const std::string& type_name,
                                                            const vbsutil::xmlparser::TopicQos& qos,
                                                            TopicListener* listener,
                                                            const evbs::edds::dds::StatusMask& mask) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    return create_topic(topic_name, type_name, qos, listener, mask);
}

vbs::TopicDescription* DomainParticipantImpl::lookup_topicdescription(const std::string& topic_name) const {
    std::lock_guard<std::mutex> lock(mtx_topics_);

    auto it = topics_.find(topic_name);
    if (it != topics_.end()) {
        return it->second->get_topic()->get_topic();
    }

    return nullptr;
}

TypeSupport DomainParticipantImpl::find_type(const std::string& type_name) const {
    std::lock_guard<std::mutex> lock(mtx_types_);

    auto type_it = types_.find(type_name);

    if (type_it != types_.end()) {
        return type_it->second;
    }

    return TypeSupport(nullptr);
}

RetCode_t DomainParticipantImpl::register_type(const TypeSupport type, const std::string& type_name) {
    if (type_name.size() <= 0U) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "Registered Type must have a name");
        return RetCode_t::RETCODE_BAD_PARAMETER;
    }

    TypeSupport t = find_type(type_name);

    if (!t.empty()) {
        if (t == type) {
            return RetCode_t::RETCODE_OK;
        }

        elogError(PARTICIPANT, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Another type with the same name '" << type_name << "' is already registered.");
        return RetCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    logDebug(PARTICIPANT, "Type " << type_name << " registered to " << guid_ << " type_lookup_:" << type_lookup_);
    std::lock_guard<std::mutex> lock(mtx_types_);
    (void)types_.insert(std::make_pair(type_name, type));

    if (!(type.get()->is_dynamic_type())) {
        using namespace evbs::ertps::types;
        auto type_id = TypeObjectFactory::get_instance()->get_type_identifier(type.get()->getName());
        if (type_id) {
            const TypeObject* obj = TypeObjectFactory::get_instance()->get_type_object(type_name);
            if (obj == nullptr) {
                elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "Cannot get_type_object type " << type_name);
                return RetCode_t::RETCODE_BAD_PARAMETER;
            }
            bool is_mutable = obj->complete().struct_type().struct_flags().IS_MUTABLE();
            if (is_mutable) {
                type->setIdlCrc16("");
            }
            const_cast<TypeIdentifier*>(type_id)->set_string_crc16(type->getIdlCrc16());
            logDebug(PARTICIPANT, "<" << type_name << ", " << type->getIdlCrc16() << ">");
            logDebug(PARTICIPANT,
                     "Type " << type_name << " struct mutable is " << (is_mutable == true ? "true" : "false"));
        }
    } else if (type_lookup_) {
        (void)register_dynamic_type_to_factories(type);
    }
    return RetCode_t::RETCODE_OK;
}

RetCode_t DomainParticipantImpl::register_type(TypeSupport type) {
    return register_type(type, type.get_type_name());
}

bool DomainParticipantImpl::register_dynamic_type_to_factories(const TypeSupport& type) const {
    using namespace evbs::ertps::types;

    DynamicPubSubType* dpst = dynamic_cast<DynamicPubSubType*>(type.get());
    if ((dpst != nullptr) && (dpst->GetDynamicType() != nullptr))  // Registering a dynamic type.
    {
        try {
            logDebug(PARTICIPANT, "Dynamic type " << dpst->getName() << " register.");
            TypeObjectFactory* objectFactory = TypeObjectFactory::get_instance();
            DynamicTypeBuilderFactory* dynFactory = DynamicTypeBuilderFactory::get_instance();
            const TypeIdentifier* id = objectFactory->get_type_identifier(dpst->getName());
            if (id == nullptr) {
                TypeObject typeObj;
                dynFactory->build_type_object(dpst->GetDynamicType(), typeObj);
                const TypeIdentifier* type_id2 = objectFactory->get_type_identifier(dpst->getName());
                const TypeObject* type_obj = objectFactory->get_type_object(dpst->getName());
                if (type_id2 == nullptr) {
                    elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_ERROR,
                              "Cannot register dynamic type " << dpst->getName());
                    return false;
                } else {
                    objectFactory->add_type_object(dpst->getName(), type_id2, type_obj);
                }
            }
            const TypeObject* obj = objectFactory->get_type_object(dpst->getName());
            if (obj == nullptr) {
                elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_ERROR,
                          "Cannot get_type_object type " << dpst->getName());
                return false;
            }
            bool is_mutable = obj->complete().struct_type().struct_flags().IS_MUTABLE();
            if (is_mutable) {
                type.get()->setIdlCrc16("");
            } else {
                type.get()->setIdlCrc16(objectFactory->get_type_identifier(dpst->getName())->get_string_crc16());
            }
            logDebug(PARTICIPANT, "<" << dpst->getName() << ", " << type.get()->getIdlCrc16() << ">");
            logDebug(PARTICIPANT, "Type " << dpst->getName() << " struct mutable is "
                                          << (is_mutable == true ? "true" : "false") << " <" << dpst->getName() << ", "
                                          << type.get()->getIdlCrc16() << ">");
            return true;
        } catch (vbsutil::ecdr::exception::BadParamException& e) {
            elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_BAD_PARAMETER,
                      "Registered Type must have a name" << e.what());
            return false;
        } catch (std::range_error& e) {
            elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_OUT_OF_RESOURCES,
                      "Registered Type throw range_error" << e.what());
            return false;
        }
    }

    return false;  // Isn't a registered dynamic type.
}

vbsutil::elog::ReturnCode_t DomainParticipantImpl::unregister_type(const std::string& type_name) {

    if (type_name.size() <= 0U) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "Registered Type must have a name");
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    TypeSupport t = find_type(type_name);

    if (t.empty()) {
        return vbsutil::elog::ReturnCode_t::RETCODE_OK;  // Not registered, so unregistering complete.
    }

    {
        // Check is any subscriber is using the type
        std::lock_guard<std::mutex> lock(mtx_subs_);

        for (auto& sub : subscribers_by_handle_) {
            if (sub.second->type_in_use(type_name)) {
                return vbsutil::elog::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;  // Is in use
            }
        }
    }

    {
        // Check is any publisher is using the type
        std::lock_guard<std::mutex> lock(mtx_pubs_);

        for (auto& pit : publishers_by_handle_) {
            if (pit.second->type_in_use(type_name)) {
                return vbsutil::elog::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;  // Is in use
            }
        }
    }

    std::lock_guard<std::mutex> lock(mtx_types_);
    (void)types_.erase(type_name);

    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

void DomainParticipantImpl::MyParticipantListener::onParticipantDiscovery(
    vbs::dispatcher::SubstrateParticipant* participant, vbs::BaseParticipantDiscoveryInfo&& info) {
    DomainParticipantListenerInner* listener = nullptr;
    evbs::ertps::rtps::RTPSParticipant* rtps_participant =
        static_cast<evbs::ertps::rtps::RTPSParticipant*>(participant);
    ParticipantDiscoveryInfo&& info_ = dynamic_cast<ParticipantDiscoveryInfo&&>(info);

    Sentry sentinel(this);
    if (sentinel) {
        logDebug(RTPS_PDP_DISCOVERY, " Participant discovery status "
                                         << (int)info_.status << " guid: " << rtps_participant->getGuid()
                                         << " domainid: " << rtps_participant->get_domain_id()
                                         << " participant name: " << rtps_participant->prinfParticipantNames()
                                         << " participant id: " << rtps_participant->getRTPSParticipantID()
                                         << " dest guid: " << info_.info.m_guid
                                         << " participant name: " << info_.info.m_participantName);
        if ((listener = participant_->get_listener()) != nullptr) {
            listener->on_participant_discovery(participant_, std::move(info_));
        }
    }
}

#if HAVE_SECURITY
void DomainParticipantImpl::MyParticipantListener::onParticipantAuthentication(
    vbs::dispatcher::SubstrateParticipant*, vbs::ParticipantAuthenticationInfo&& info) {
    DomainParticipantListenerInner* listener = nullptr;
    Sentry sentinel(this);
    if (sentinel) {
        if ((listener = participant_->get_listener()) != nullptr) {
            listener->onParticipantAuthentication(participant_, std::move(info));
        }
    }
}

#endif  // if HAVE_SECURITY

void DomainParticipantImpl::notify_type_discovery(const std::string& topic_name, const std::string& type_name) {
    VbsLogD("DomainParticipant on_type_discovery type_name: " << type_name << " " << topic_name);

    if (user_listener_ == nullptr) {
        VbsLogE(vbs::VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " vbs_participant_listener_ unexpected nullptr " << topic_name);
        return;
    }

    user_listener_->on_type_discovery(user_participant_, type_name, user_participant_->get_dynamic_type(type_name));

    vbs::DynamicTypeInformation type_info(type_name, topic_name);
    user_listener_->on_type_info(user_participant_, type_name, type_info);
}

void DomainParticipantImpl::update_type_info(const std::string& topic_name, const std::string& type_name) {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    discovered_type_name_by_topic_[topic_name] = type_name;

    if (discovered_topic_names_.find(topic_name) != discovered_topic_names_.end()) {
        return;
    }
    if (discovered_type_topic_.find(type_name) != discovered_type_topic_.end()) {
        discovered_type_topic_[type_name].insert(topic_name);
    } else {
        discovered_type_topic_[type_name] = {topic_name};
    }

    if (!type_discovery_enabled()) {
        return;
    }

    if (receive_all_topic_) {
        if (discovered_type_info_.find(type_name) != discovered_type_info_.end()) {
            notify_type_discovery(topic_name, type_name);
            discovered_topic_names_.insert(topic_name);
        } else {
            register_remote_type(type_name);
        }
        VbsLogD("Participant has register remote sub type " << type_name);
    } else {
        if (listener_topics_.find(topic_name) != listener_topics_.end()) {
            if (discovered_type_info_.find(type_name) != discovered_type_info_.end()) {
                notify_type_discovery(topic_name, type_name);
                discovered_topic_names_.insert(topic_name);
            } else {
                register_remote_type(type_name);
            }
            VbsLogD("Participant has register remote sub type " << type_name);
        }
    }
}

void DomainParticipantImpl::MyParticipantListener::onReaderDiscovery(
    vbs::dispatcher::SubstrateParticipant* const participant, vbs::BaseReaderDiscoveryInfo&& info) {
    DomainParticipantListenerInner* listener = nullptr;
    evbs::ertps::rtps::RTPSParticipant* rtps_participant =
        static_cast<evbs::ertps::rtps::RTPSParticipant*>(participant);
    ReaderDiscoveryInfo&& reader_info = dynamic_cast<ReaderDiscoveryInfo&&>(info);

    participant_->update_type_info(reader_info.info.topicName().to_string(), reader_info.info.typeName().to_string());

    Sentry sentinel(this);
    if (sentinel) {
        logDebug(RTPS_EDP, "Reader discovery status: "
                               << (int)reader_info.status << " guid: " << rtps_participant->getGuid()
                               << " domain id: " << rtps_participant->get_domain_id()
                               << " participant name: " << rtps_participant->prinfParticipantNames()
                               << " participant id: " << rtps_participant->getRTPSParticipantID() << " dest guid: "
                               << reader_info.info.guid() << " topic name: " << reader_info.info.topicName());
        if ((listener = participant_->get_listener()) != nullptr) {
            listener->on_subscriber_discovery(participant_, std::move(reader_info));
        }
    }
}

void DomainParticipantImpl::MyParticipantListener::onWriterDiscovery(vbs::dispatcher::SubstrateParticipant* participant,
                                                                     vbs::BaseWriterDiscoveryInfo&& info) {
    DomainParticipantListenerInner* listener = nullptr;
    evbs::ertps::rtps::RTPSParticipant* rtps_participant =
        static_cast<evbs::ertps::rtps::RTPSParticipant*>(participant);
    WriterDiscoveryInfo&& writer_info = dynamic_cast<WriterDiscoveryInfo&&>(info);

    participant_->update_type_info(writer_info.info.topicName().to_string(), writer_info.info.typeName().to_string());

    Sentry sentinel(this);
    if (sentinel) {
        logDebug(RTPS_EDP, " Writer discovery status "
                               << (int)writer_info.status << " guid: " << rtps_participant->getGuid()
                               << " domain id: " << rtps_participant->get_domain_id()
                               << " participant name: " << rtps_participant->prinfParticipantNames()
                               << " participant id: " << rtps_participant->getRTPSParticipantID() << " dest guid: "
                               << writer_info.info.guid() << " topic name: " << writer_info.info.topicName());
        if ((listener = participant_->get_listener()) != nullptr) {
            listener->on_publisher_discovery(participant_, std::move(writer_info));
        }
    }
}

void DomainParticipantImpl::update_type_discovery(const std::string& type_name,
                                                  evbs::ertps::types::DynamicType_ptr dyn_type) {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    if (discovered_type_topic_.find(type_name) == discovered_type_topic_.end()) {
        VbsLogD("DomainParticipantImpl discovered_type_topic_ don't has type_name: " << type_name);
        return;
    }

    discovered_type_info_[type_name] = dyn_type;
    if (user_listener_ == nullptr) {
        return;
    }

    for (auto& topic_name : discovered_type_topic_[type_name]) {
        if (discovered_topic_names_.find(topic_name) != discovered_topic_names_.end()) {
            VbsLogD("DomainParticipantImpl udp registered_type_names has: " << topic_name);
            continue;
        }
        notify_type_discovery(topic_name, type_name);
        discovered_topic_names_.insert(topic_name);
    }
}

void DomainParticipantImpl::MyParticipantListener::on_type_discovery(
    vbs::dispatcher::SubstrateParticipant*, const /*vbs::common::*/ SampleIdentity& request_sample_id,
    const std::vector<vbs::common::types::TypeIdentifierTypeObjectPair>& type_pairs) {
    (void)participant_->check_get_type_request(request_sample_id, type_pairs);
}

void DomainParticipantImpl::MyParticipantListener::on_type_dependencies_reply(
    const GUID_t& dst_guid, vbs::dispatcher::SubstrateParticipant*,
    const vbs::common::SampleIdentity& request_sample_id,
    const vbs::common::types::TypeIdentifierWithSizeSeq& dependencies) {
    Sentry sentinel(this);
    if (sentinel) {
        (void)participant_->check_get_dependencies_request(dst_guid, request_sample_id, dependencies);
    }
}

void DomainParticipantImpl::MyParticipantListener::on_type_information_received(
    const GUID_t& dst_guid, const std::string& type_name, const vbs::common::types::TypeInformation& type_information) {
    Sentry sentinel(this);
    if (sentinel) {
        (void)participant_->register_remote_type(dst_guid, type_information, type_name);
    }
}

bool DomainParticipantImpl::new_remote_endpoint_discovered(const GUID_t& /*partguid*/, uint16_t /*endpointId*/,
                                                           EndpointKind_t /*kind*/) {
#if 0
    if (get_dispatcher_participant() != nullptr) {
        if (kind == evbs::ertps::rtps::WRITER) {
            return get_dispatcher_participant()->newRemoteWriterDiscovered(partguid, static_cast<int16_t>(endpointId));
        } else {
            return get_dispatcher_participant()->newRemoteReaderDiscovered(partguid, static_cast<int16_t>(endpointId));
        }
    }
#endif
    return false;
}

ResourceEvent& DomainParticipantImpl::get_resource_event() const {
    assert(nullptr != get_dispatcher_participant());
    return get_dispatcher_participant()->get_resource_event();
}

evbs::ertps::rtps::SampleIdentity DomainParticipantImpl::get_type_dependencies(
    const GUID_t& dst_guid, const evbs::ertps::types::TypeIdentifierSeq& in) const {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    return nullptr != dispatcher_participant
               ? dispatcher_participant->lookup_manager()->get_type_dependencies(dst_guid, in)
               : evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY;
}

evbs::ertps::rtps::SampleIdentity DomainParticipantImpl::get_types(
    const GUID_t& dst_guid, const evbs::ertps::types::TypeIdentifierSeq& in) const {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    return nullptr != dispatcher_participant ? dispatcher_participant->lookup_manager()->get_types(dst_guid, in)
                                             : evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY;
}

void DomainParticipantImpl::type_request(const GUID_t& dst_guid,
                                         const evbs::ertps::types::TypeInformation& type_information,
                                         const std::string& type_name) {
    using namespace evbs::ertps::types;
    TypeIdentifierSeq dependencies;
    TypeIdentifierSeq retrieve_objects;

    fill_pending_dependencies(type_information.complete().dependent_typeids(), dependencies, retrieve_objects);

    evbs::ertps::rtps::SampleIdentity request_dependencies;
    evbs::ertps::rtps::SampleIdentity request_objects;

    // Lock now, we don't want to process the reply before we add the requests' ID to the maps.
    std::lock_guard<std::mutex> lock(mtx_request_cb_);

    // If any pending dependency exists, retrieve it.
    if (!dependencies.empty()) {
        request_dependencies = get_type_dependencies(dst_guid, dependencies);
    }

    // If any pending TypeObject exists, retrieve it
    if (!retrieve_objects.empty()) {
        request_objects = get_types(dst_guid, retrieve_objects);
    }

    // If no more dependencies but failed to create, probably we only need the TypeObject
    dependencies.clear();  // Reuse the same vector.
    dependencies.push_back(type_information.complete().typeid_with_size().type_id());
    evbs::ertps::rtps::SampleIdentity requestId = get_types(dst_guid, dependencies);

    // Add everything to maps
    (void)register_types_.emplace(std::make_pair(requestId, type_name));
    std::vector<evbs::ertps::rtps::SampleIdentity> vector;
    vector.push_back(requestId);  // Add itself

    if (evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY != request_dependencies) {
        vector.push_back(request_dependencies);
        (void)child_requests_.emplace(std::make_pair(request_dependencies, requestId));
    }

    if (evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY != request_objects) {
        vector.push_back(request_objects);
        (void)child_requests_.emplace(std::make_pair(request_objects, requestId));
    }

    // Move the filled vector to the map
    (void)parent_requests_.emplace(std::make_pair(requestId, std::move(vector)));
}

ReturnCode_t DomainParticipantImpl::register_remote_type(const GUID_t& dst_guid,
                                                         const vbs::common::types::TypeInformation& type_information,
                                                         const std::string& type_name) {
    using namespace evbs::ertps::types;

    if (get_dispatcher_participant() == nullptr) {
        return vbs::ReturnCode_t::RETCODE_NOT_ENABLED;
    }

    TypeObjectFactory* factory = TypeObjectFactory::get_instance();
    // Check if plain
    if (type_information.complete().typeid_with_size().type_id()._d() < EK_COMPLETE) {
        DynamicType_ptr dyn =
            factory->build_dynamic_type(type_name, &type_information.complete().typeid_with_size().type_id());

        if (nullptr != dyn) {
            auto ret = register_dynamic_type(type_name, dyn);
            if ((ret == vbs::ReturnCode_t::RETCODE_OK) && listener_) {
                listener_->on_type_discovery(this, type_name, std::move(dyn));
            }
            logInfo(PARTICIPANT, guid() << "Plain type (" << type_name << ") discoveried.");
            return ret;
        }
        // If cannot create the dynamic type, probably is because it depend on unknown types.
        // We must continue.
    }

    // Check if already available
    TypeObject obj;
    (void)factory->typelookup_get_type(type_information.complete().typeid_with_size().type_id(), obj);

    if (obj._d() != 0U) {
        DynamicType_ptr dyn =
            factory->build_dynamic_type(type_name, &type_information.complete().typeid_with_size().type_id(), &obj);

        if (nullptr != dyn) {
            auto ret = register_dynamic_type(type_name, dyn);
            if ((ret == vbs::ReturnCode_t::RETCODE_OK) && listener_) {
                listener_->on_type_discovery(this, type_name, std::move(dyn));
            }
            logInfo(PARTICIPANT, guid() << " Type (" << type_name << ") discoveried from local.");
            return ret;
        }
    } else if (get_dispatcher_participant()->lookup_manager() != nullptr) {
        type_request(dst_guid, type_information, type_name);
        return vbs::ReturnCode_t::RETCODE_OK;
    }
    return vbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t DomainParticipantImpl::register_remote_type(const std::string& type_name) {
    using namespace evbs::ertps::types;

    {
        std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
        if (registered_type_names_.find(type_name) != registered_type_names_.end()) {
            return vbs::ReturnCode_t::RETCODE_OK;
        }
        registered_type_names_.insert(type_name);
    }

    TypeSupport type_support = find_type(type_name);
    if (!type_support.empty()) {
        logDebug(PARTICIPANT, guid() << " Typelookup type(" << type_name << ") exists.");
        return vbs::ReturnCode_t::RETCODE_OK;
    }

    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "rtps participant (" << type_name << ") is null.");
        return vbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    if (dispatcher_participant->lookup_manager()->register_remote_type(type_name) == false) {
        return vbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    (void)dispatcher_participant->lookup_manager()->get_type_info(type_name);
    return vbs::ReturnCode_t::RETCODE_OK;
}
void DomainParticipantImpl::fill_pending_dependencies(const evbs::ertps::types::TypeIdentifierWithSizeSeq& dependencies,
                                                      evbs::ertps::types::TypeIdentifierSeq& pending_identifiers,
                                                      evbs::ertps::types::TypeIdentifierSeq& pending_objects) const {
    using namespace evbs::ertps::types;
    for (const TypeIdentifierWithSize& tiws : dependencies) {
        TypeObjectFactory::get_instance()->fill_pending_dependencies(tiws.type_id(), pending_identifiers,
                                                                     pending_objects);
    }
}

bool DomainParticipantImpl::check_get_child_dependencies_request(
    const GUID_t& dst_guid, const evbs::ertps::rtps::SampleIdentity& request_id,
    const evbs::ertps::rtps::SampleIdentity& request_parent_id,
    const evbs::ertps::types::TypeIdentifierWithSizeSeq& dependencies) {
    using namespace evbs::ertps::types;
    TypeIdentifierSeq next_dependencies;
    TypeIdentifierSeq retrieve_objects;
    std::vector<evbs::ertps::rtps::SampleIdentity>& vector = parent_requests_[request_parent_id];

    fill_pending_dependencies(dependencies, next_dependencies, retrieve_objects);

    // If any pending dependency exists, retrieve it
    if (!next_dependencies.empty()) {
        evbs::ertps::rtps::SampleIdentity child_request = get_type_dependencies(dst_guid, next_dependencies);
        vector.push_back(child_request);
        (void)child_requests_.emplace(std::make_pair(child_request, request_parent_id));
    }

    // If any pending TypeObject exists, retrieve it
    if (!retrieve_objects.empty()) {
        evbs::ertps::rtps::SampleIdentity child_request = get_types(dst_guid, retrieve_objects);
        vector.push_back(child_request);
        (void)child_requests_.emplace(std::make_pair(child_request, request_parent_id));
    }
    remove_child_request(request_id);
    return true;
}

bool DomainParticipantImpl::check_get_dependencies_request(
    const GUID_t& dst_guid, const evbs::ertps::rtps::SampleIdentity& requestId,
    const evbs::ertps::types::TypeIdentifierWithSizeSeq& dependencies) {
    // Maybe we have a pending request?
    if (evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY != requestId) {
        std::lock_guard<std::mutex> lock(mtx_request_cb_);
        auto child_it = child_requests_.find(requestId);
        if (child_it != child_requests_.end()) {
            return check_get_child_dependencies_request(dst_guid, requestId, child_requests_[requestId], dependencies);
        }
    }
    return false;
}

ReturnCode_t DomainParticipantImpl::register_dynamic_type(const std::string& type_name,
                                                          evbs::ertps::types::DynamicType_ptr dyn_type) {
    TypeSupport type(new evbs::ertps::types::DynamicPubSubType(dyn_type));
    auto ret = register_type(std::move(type));
    if (ret == ReturnCode_t::RETCODE_OK) {
        update_type_discovery(type_name, std::move(dyn_type));
    }
    return ret;
}

bool DomainParticipantImpl::build_dynamic_type(std::string type_name) {
    auto it = types_id_obj_.find(type_name);
    if (it != types_id_obj_.end()) {
        const evbs::ertps::types::TypeIdentifier* identifier = &it->second.type_identifier();
        const evbs::ertps::types::TypeObject* object = &it->second.type_object();

        evbs::ertps::types::TypeObjectFactory::get_instance()->add_type_object(type_name, identifier, object);

        if (qos_.wire_protocol().builtin.discovery_config.discoveryProtocol ==
            evbs::ertps::rtps::DiscoveryProtocol::SERVER) {
            const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
            if (dispatcher_participant == nullptr) {
                elogError(PARTICIPANT, ReturnCode_t::RETCODE_PRECONDITION_NOT_MET,
                          "rtps participant (" << type_name << ") is null.");
                return false;
            }
            dispatcher_participant->lookup_manager()->remove_unreply_sample_id(type_name);
        }
        evbs::ertps::types::DynamicType_ptr dynamic =
            evbs::ertps::types::TypeObjectFactory::get_instance()->build_dynamic_type(type_name, identifier, object);

        if (nullptr != dynamic) {
            if (register_dynamic_type(type_name, dynamic) == ReturnCode_t::RETCODE_OK) {
                if (listener_) {
                    listener_->on_type_discovery(this, type_name, std::move(dynamic));
                }
                types_id_obj_.erase(it);
                logInfo(PARTICIPANT, guid() << " Type (" << type_name << ") discoveried from remote.");
                return true;
            }
        }
    }
    return false;
}

evbs::ertps::rtps::IContentFilterFactory* DomainParticipantImpl::find_content_filter_factory(
    const char* filter_class_name) {
    auto it = filter_factories_.find(filter_class_name);
    if (it != filter_factories_.end()) {
        return it->second;
    }

    if (0 != strcmp(filter_class_name, EDDS_SQLFILTER_NAME)) {
        return nullptr;
    }

    return evbs::ertps::rtps::IContentFilterFactory::get_default_filter_factory();
}

bool DomainParticipantImpl::check_get_type_request(
    const evbs::ertps::rtps::SampleIdentity& requestId,
    const std::vector<vbs::common::types::TypeIdentifierTypeObjectPair>& types) {
    if (requestId == evbs::edds::dds::builtin::INVALID_SAMPLE_IDENTITY) {
        return true;
    }
    // First level request?
    std::lock_guard<std::mutex> lock(mtx_request_cb_);

    auto cb_it = register_types_.find(requestId);

    if (cb_it != register_types_.end()) {
        std::string name = cb_it->second;
        if (types.size() != 1) {
            elogError(PARTICIPANT, ReturnCode_t::RETCODE_BAD_PARAMETER,
                      "Registered Type must have just a pair of type, size " << types.size());
            return false;
        }
        types_id_obj_.emplace(std::make_pair(name, types[0]));

        auto pending = parent_requests_.find(requestId);
        if ((pending != parent_requests_.end()) && (pending->second.size() < 2U)) {
            if (build_dynamic_type(std::move(name))) {
                remove_parent_request(requestId);
                return true;
            }
        }
        // Failed, cannot register the type yet, probably child request still pending.
        return false;
    }

    auto child_it = child_requests_.find(requestId);

    if (child_it != child_requests_.end()) {
        std::string name;
        for (auto& pair : types) {
            // Register received TypeObject into factory, remove the iterator from the map and check
            // our parent.
            name = evbs::ertps::types::TypeObjectFactory::get_instance()->get_type_name_by_object(&pair.type_object());
            evbs::ertps::types::TypeObjectFactory::get_instance()->add_type_object(name, &pair.type_identifier(),
                                                                                   &pair.type_object());
        }
        remove_child_request(requestId);
    }
    return true;
}

void DomainParticipantImpl::remove_parent_request(const evbs::ertps::rtps::SampleIdentity& request) {
    // If a parent request if going to be deleted, delete all its children too.
    auto cb_it = register_types_.find(request);
    auto parent_it = parent_requests_.find(request);

    if (parent_requests_.end() != parent_it) {
        for (const evbs::ertps::rtps::SampleIdentity& child_id : parent_it->second) {
            auto child_it = child_requests_.find(child_id);
            if (child_requests_.end() != child_it) {
                (void)child_requests_.erase(child_it);
            }
        }
        (void)parent_requests_.erase(parent_it);
    }

    if (register_types_.end() != cb_it) {
        (void)register_types_.erase(cb_it);
    }
}

void DomainParticipantImpl::remove_child_request(const evbs::ertps::rtps::SampleIdentity& request) {
    auto child_it = child_requests_.find(request);
    if (child_requests_.end() != child_it) {
        evbs::ertps::rtps::SampleIdentity parent_request = child_it->second;
        (void)child_requests_.erase(child_it);

        auto parent_it = parent_requests_.find(parent_request);
        if (parent_requests_.end() != parent_it) {
            std::vector<evbs::ertps::rtps::SampleIdentity>& pending = parent_it->second;
            (void)pending.erase(std::find(pending.begin(), pending.end(), request));
            if (pending.empty()) {
                (void)parent_requests_.erase(parent_it);
            }
        }

        on_child_requests_finished(parent_request);
    }
}

void DomainParticipantImpl::on_child_requests_finished(const evbs::ertps::rtps::SampleIdentity& parent) {
    auto pending_requests_it = parent_requests_.find(parent);
    // Do I have no more pending childs?
    if ((parent_requests_.end() == pending_requests_it) || pending_requests_it->second.empty()) {
        // Am I a children?
        auto child_it = child_requests_.find(parent);
        if (child_requests_.end() != child_it) {
            remove_child_request(parent);
        } else {
            // Or a top-level request?
            auto it = register_types_.find(parent);

            /*if xxx condition is not correct!!! */
            if ((parent_requests_.end() != pending_requests_it) && (pending_requests_it->second.size() < 2U)) {
                (void)parent_requests_.erase(pending_requests_it);
            }
            if (register_types_.end() != it) {
                build_dynamic_type(it->second);
                (void)register_types_.erase(it);
            }
        }
    } else if (pending_requests_it->second.size() < 2U) {
        auto it = register_types_.find(parent);
        if (it == register_types_.end()) {
            return;
        }
        if (build_dynamic_type(it->second)) {
            remove_parent_request(parent);
            return;
        }
    }
}

bool DomainParticipantImpl::has_active_entities() {
    if (!publishers_by_handle_.empty()) {
        return true;
    }
    if (!subscribers_by_handle_.empty()) {
        return true;
    }
    if (!topics_.empty()) {
        return true;
    }
    return false;
}

bool DomainParticipantImpl::set_qos(vbs::DomainParticipantQosInner& to, const vbs::DomainParticipantQosInner& from,
                                    bool first_time) {
    bool qos_should_be_updated = false;

    if (first_time && (!(to.allocation() == from.allocation()))) {
        to.allocation() = from.allocation();
    }
    if (first_time && (!(to.properties() == from.properties()))) {
        to.properties() = from.properties();
    }
    if (!(to.wire_protocol() == from.wire_protocol())) {
        to.wire_protocol() = from.wire_protocol();
        to.wire_protocol().hasChanged = true;
        if (!first_time) {
            qos_should_be_updated = true;
        }
    }
    if (first_time && (!(to.transport() == from.transport()))) {
        to.transport() = from.transport();
    }
    if (first_time && to.name() != from.name()) {
        to.name() = from.name();
    }

    return qos_should_be_updated;
}

ReturnCode_t DomainParticipantImpl::check_qos(const vbs::DomainParticipantQosInner& qos) {
    UNUSED_PARAMETER(qos);
    return ReturnCode_t::RETCODE_OK;
}

bool DomainParticipantImpl::can_qos_be_updated(const vbs::DomainParticipantQosInner& to,
                                               const vbs::DomainParticipantQosInner& from) {
    bool updatable = true;
    if (!(to.allocation() == from.allocation())) {
        updatable = false;
        logDebug(RTPS_QOS_CHECK, "ParticipantResourceLimitsQos cannot be changed after the participant is enabled");
    }
    if (!(to.properties() == from.properties())) {
        updatable = false;
        logDebug(RTPS_QOS_CHECK, "PropertyPolilyQos cannot be changed after the participant is enabled");
    }
    if (!(to.wire_protocol() == from.wire_protocol())) {
        updatable = false;
        logDebug(RTPS_QOS_CHECK, "WireProtocolConfigQos cannot be changed after the participant is enabled");
    }
    if (!(to.transport() == from.transport())) {
        updatable = false;
        logDebug(RTPS_QOS_CHECK, "TransportConfigQos cannot be changed after the participant is enabled");
    }
    if (!(to.name() == from.name())) {
        updatable = false;
        logDebug(RTPS_QOS_CHECK, "Participant name cannot be changed after the participant is enabled");
    }
    return updatable;
}

void DomainParticipantImpl::create_instance_handle(InstanceHandle_t& handle) {
    using evbs::ertps::rtps::octet;

    uint32_t id = ++next_instance_id_;
    handle = guid_;
    handle.value[15] = 0x01U;  // Vendor specific;
    handle.value[14] = static_cast<octet>(id & 0xFFU);
    handle.value[13] = static_cast<octet>((id >> 8) & 0xFFU);
    handle.value[12] = static_cast<octet>((id >> 16) & 0xFFU);
}

DomainParticipantListenerInner* DomainParticipantImpl::get_listener_for(const evbs::edds::dds::StatusMask& status) {
    if (get_status_mask().is_active(status)) {
        return get_listener();
    }
    return nullptr;
}

bool DomainParticipantImpl::start_initial_announcement() {
    uint32_t count;
    Duration_t period;

    count = qos_.wire_protocol().builtin.discovery_config.initial_announcements.count;
    period = qos_.wire_protocol().builtin.discovery_config.initial_announcements.period;

    return start_initial_announcement(count, period);
}

bool DomainParticipantImpl::start_initial_announcement(uint32_t count, evbs::ertps::Duration_t period) {
    if (((period.seconds == 0) && (period.nanosec == 0)) || (count == 0) || (period.seconds < 0)) {
        return false;
    }

    return dispatcher_participant_->startRTPSParticipantInitialAnnouncement(count, period);
}

void DomainParticipantImpl::stop_initial_announcement() {
    (void)start_initial_announcement(0U, {0, 100000000});
}

evbs::ertps::rtps::EvbsVersion_t DomainParticipantImpl::get_remote_evbsversion() {
    if (dispatcher_participant_ != nullptr) {
        return dispatcher_participant_->get_remote_evbsversion();
    }
    return evbs::edds::rtps::c_Version_vbs_unknown;
}

void DomainParticipantImpl::initToolHookGetInfoFunc() {
    evbs::edds::dds::builtin::LookupManager* manPtr = this->get_dispatcher_participant()->lookup_manager();
    if (manPtr != nullptr) {
        if (manPtr->get_builtin_protocols()->m_att.lookup_config.use_server == true) {
            evbs::edds::dds::builtin::LookupRequestListener* reqPtr =
                this->get_dispatcher_participant()->lookup_manager()->get_lookupReqListener();
            reqPtr->RegisterHook_get_writer_reader_infos(
                std::bind(&DomainParticipantImpl::get_writer_reader_infos, this));
            reqPtr->RegisterHook_get_remote_guids(
                std::bind(&DomainParticipantImpl::get_remote_guids, this, std::placeholders::_1));
            reqPtr->RegisterHook_get_statistic_send_info(std::bind(&DomainParticipantImpl::get_statistic_send_info,
                                                                   this, std::placeholders::_1, std::placeholders::_2));
            reqPtr->RegisterHook_get_statistic_recv_info(std::bind(&DomainParticipantImpl::get_statistic_recv_info,
                                                                   this, std::placeholders::_1, std::placeholders::_2));
            reqPtr->RegisterHook_get_statistic_writer_reader_qos(
                std::bind(&DomainParticipantImpl::get_statistic_writer_reader_qos, this, std::placeholders::_1,
                          std::placeholders::_2));
            reqPtr->RegisterHook_get_statistic_writer_reader_proxy_infos(std::bind(
                &DomainParticipantImpl::get_statistic_writer_reader_proxy_infos, this, std::placeholders::_1));
        }
    }
}

evbs::edds::dds::builtin::StatisticWrtierReaderInfos DomainParticipantImpl::get_writer_reader_infos() {
    evbs::edds::dds::builtin::StatisticWrtierReaderInfos writer_reader_infos;

    auto get_type_name_by_topic = [this](const std::string& topic_name) {
        vbs::TopicDescription* topic_desc = lookup_topicdescription(topic_name);
        return topic_desc ? topic_desc->get_type_name() : "";
    };

    std::lock_guard<std::mutex> _(mtx_gs_);
    if (dispatcher_participant_) {
        for (auto& writer : dispatcher_participant_->getAllWriters()) {
            if (writer->getTopicName().empty()) {
                continue;
            }
            evbs::edds::dds::builtin::WriterReaderInfo info;
            info.type = evbs::edds::dds::builtin::StatisticEntityType::STATICTIC_ENTITY_WRITER;
            info.guid = writer->getGuid();
            info.topic_name = writer->getTopicName();
            info.type_name = get_type_name_by_topic(info.topic_name);
            info.matched = writer->getMatchedReadersSize();
            writer_reader_infos.push_back(std::move(info));
        }
        for (auto& reader : dispatcher_participant_->getAllReaders()) {
            if (reader->getTopicName().empty()) {
                continue;
            }
            evbs::edds::dds::builtin::WriterReaderInfo info;
            info.type = evbs::edds::dds::builtin::StatisticEntityType::STATICTIC_ENTITY_READER;
            info.guid = reader->getGuid();
            info.topic_name = reader->getTopicName();
            info.type_name = get_type_name_by_topic(info.topic_name);
            info.matched = reader->getMatchedWritersSize();
            writer_reader_infos.push_back(std::move(info));
        }
    }
    return writer_reader_infos;
}

evbs::edds::dds::builtin::StatisticWrtierReaderInfos DomainParticipantImpl::lookup_writer_reader_infos(
    const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return evbs::edds::dds::builtin::StatisticWrtierReaderInfos();
    }

    return dispatcher_participant->lookup_manager()->lookup_writer_reader_infos(guid, timeout);
}

evbs::edds::dds::builtin::StatisticMatchGuids DomainParticipantImpl::get_remote_guids(
    const evbs::ertps::rtps::GUID_t& guid) {
    std::lock_guard<std::mutex> _(mtx_gs_);
    if (dispatcher_participant_) {
        for (auto& writer : dispatcher_participant_->getAllWriters()) {
            if (writer->getGuid() == guid) {
                return writer->get_remote_guids();
            }
        }
        for (auto& reader : dispatcher_participant_->getAllReaders()) {
            if (reader->getGuid() == guid) {
                return reader->get_remote_guids();
            }
        }
    }
    return evbs::edds::dds::builtin::StatisticMatchGuids();
}

evbs::edds::dds::builtin::StatisticMatchGuids DomainParticipantImpl::lookup_remote_guids(
    const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, ReturnCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return evbs::edds::dds::builtin::StatisticMatchGuids();
    }

    return dispatcher_participant->lookup_manager()->lookup_remote_guids(guid, timeout);
}

RetCode_t DomainParticipantImpl::get_statistic_send_info(const evbs::ertps::rtps::GUID_t& guid,
                                                         evbs::edds::dds::builtin::StatisticSendInfo* info) {
    std::lock_guard<std::mutex> lock_pubs(mtx_pubs_);
    for (auto& pub_it : publishers_by_handle_) {
        std::lock_guard<std::mutex> lock_subs(pub_it.second->mtx_writers_);
        for (auto& topic_writers : pub_it.second->writers_) {
            for (auto& dw : topic_writers.second) {
                if (dw->guid() == guid) {
                    dw->get_statistic_send_info(info);
                    return RetCode_t::RETCODE_OK;
                }
            }
        }
    }
    elogError(PARTICIPANT, ReturnCode_t::RETCODE_ERROR, "local writer " << guid << " not found.");
    return RetCode_t::RETCODE_ERROR;
}

RetCode_t DomainParticipantImpl::lookup_statistic_send_info(const evbs::ertps::rtps::GUID_t& guid,
                                                            evbs::edds::dds::builtin::StatisticSendInfo* info,
                                                            uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, vbs::ReturnCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return vbs::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    return dispatcher_participant->lookup_manager()->lookup_statistic_send_info(guid, info, timeout);
}

RetCode_t DomainParticipantImpl::get_statistic_recv_info(const evbs::ertps::rtps::GUID_t& guid,
                                                         evbs::edds::dds::builtin::StatisticRecvInfo* info) {
    std::lock_guard<std::mutex> lock_subs(mtx_subs_);
    for (auto& sub : subscribers_by_handle_) {
        std::lock_guard<std::mutex> lock_subs(sub.second->mtx_readers_);
        for (auto& topic_readers : sub.second->readers_) {
            for (auto& dr : topic_readers.second) {
                if (dr->guid() == guid) {
                    dr->get_statistic_recv_info(info);
                    return RetCode_t::RETCODE_OK;
                }
            }
        }
    }
    elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "local reader " << guid << " not found.");
    return RetCode_t::RETCODE_ERROR;
}

RetCode_t DomainParticipantImpl::lookup_statistic_recv_info(const evbs::ertps::rtps::GUID_t& guid,
                                                            evbs::edds::dds::builtin::StatisticRecvInfo* info,
                                                            uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return RetCode_t::RETCODE_BAD_PARAMETER;
    }

    return dispatcher_participant->lookup_manager()->lookup_statistic_recv_info(guid, info, timeout);
}

RetCode_t DomainParticipantImpl::get_statistic_writer_reader_qos(
    const evbs::ertps::rtps::GUID_t& guid, evbs::edds::dds::builtin::StatisticWriterReaderQos* qos) {
    {
        std::lock_guard<std::mutex> lock_pubs(mtx_pubs_);
        for (auto& pub_it : publishers_by_handle_) {
            std::lock_guard<std::mutex> lock_subs(pub_it.second->mtx_writers_);
            for (auto& topic_writers : pub_it.second->writers_) {
                for (auto& dw : topic_writers.second) {
                    if (dw->guid() == guid) {
                        const auto& writer_qos = dw->get_dds_qos();
                        qos->durability = writer_qos.durability().kind;
                        qos->deadline = writer_qos.deadline().period;
                        qos->liveliness_kind = writer_qos.liveliness().kind;
                        qos->liveliness_lease_duration = writer_qos.liveliness().lease_duration;
                        qos->liveliness_announcement_period = writer_qos.liveliness().announcement_period;
                        qos->reliability_kind = writer_qos.reliability().kind;
                        qos->reliability_max_blocking_time = writer_qos.reliability().max_blocking_time;
                        qos->history_kind = writer_qos.history().kind;
                        qos->history_depth = writer_qos.history().depth;
                        qos->max_samples = writer_qos.resource_limits().max_samples;
                        qos->max_instances = writer_qos.resource_limits().max_instances;
                        qos->max_samples_per_instance = writer_qos.resource_limits().max_samples_per_instance;
                        qos->allocated_samples = writer_qos.resource_limits().allocated_samples;
                        qos->extra_samples = writer_qos.resource_limits().extra_samples;
                        qos->lifespan = writer_qos.lifespan().duration;
                        qos->ownership = writer_qos.ownership().kind;
                        qos->pub_mode = writer_qos.publish_mode().kind;
                        qos->flowctlname = writer_qos.publish_mode().flowname;
                        qos->e2e_enable = writer_qos.e2e_protection().enabled;
                        qos->send_multi_enable = writer_qos.send_multi();
                        return ReturnCode_t::RETCODE_OK;
                    }
                }
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock_subs(mtx_subs_);
        for (auto& sub : subscribers_by_handle_) {
            std::lock_guard<std::mutex> lock_subs(sub.second->mtx_readers_);
            for (auto& topic_readers : sub.second->readers_) {
                for (auto& dr : topic_readers.second) {
                    if (dr->guid() == guid) {
                        const auto& reader_qos = dr->get_dds_qos();
                        qos->durability = reader_qos.durability().kind;
                        qos->deadline = reader_qos.deadline().period;
                        qos->liveliness_kind = reader_qos.liveliness().kind;
                        qos->liveliness_lease_duration = reader_qos.liveliness().lease_duration;
                        qos->liveliness_announcement_period = reader_qos.liveliness().announcement_period;
                        qos->reliability_kind = reader_qos.reliability().kind;
                        qos->reliability_max_blocking_time = reader_qos.reliability().max_blocking_time;
                        qos->history_kind = reader_qos.history().kind;
                        qos->history_depth = reader_qos.history().depth;
                        qos->max_samples = reader_qos.resource_limits().max_samples;
                        qos->max_instances = reader_qos.resource_limits().max_instances;
                        qos->max_samples_per_instance = reader_qos.resource_limits().max_samples_per_instance;
                        qos->allocated_samples = reader_qos.resource_limits().allocated_samples;
                        qos->extra_samples = reader_qos.resource_limits().extra_samples;
                        qos->lifespan = reader_qos.lifespan().duration;
                        qos->ownership = reader_qos.ownership().kind;
                        qos->e2e_enable = reader_qos.e2e_protection().enabled;
                        qos->send_multi_enable = reader_qos.send_multi();
                        return ReturnCode_t::RETCODE_OK;
                    }
                }
            }
        }
    }
    elogError(PARTICIPANT, RetCode_t::RETCODE_ERROR, "local writer qos " << guid << " not found.");
    return ReturnCode_t::RETCODE_ERROR;
}

RetCode_t DomainParticipantImpl::lookup_statistic_writer_reader_qos(
    const evbs::ertps::rtps::GUID_t& guid, evbs::edds::dds::builtin::StatisticWriterReaderQos* qos, uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    return dispatcher_participant->lookup_manager()->lookup_statistic_writer_reader_qos(guid, qos, timeout);
}

evbs::edds::dds::builtin::StatisticProxyInfos DomainParticipantImpl::get_statistic_writer_reader_proxy_infos(
    const evbs::ertps::rtps::GUID_t& guid) {
    std::lock_guard<std::mutex> _(mtx_gs_);
    if (dispatcher_participant_) {
        for (auto& writer : dispatcher_participant_->getAllWriters()) {
            if (writer->getGuid() == guid) {
                // std::cout << "------------writer proxy!" << std::endl;
                return writer->get_proxy_infos();
            }
        }
        for (auto& reader : dispatcher_participant_->getAllReaders()) {
            if (reader->getGuid() == guid) {
                // std::cout << "+++++++++++++reader proxy!" << std::endl;
                return reader->get_proxy_infos();
            }
        }
    }
    return evbs::edds::dds::builtin::StatisticProxyInfos();
}

evbs::edds::dds::builtin::StatisticProxyInfos DomainParticipantImpl::lookup_statistic_writer_reader_proxy_infos(
    const evbs::ertps::rtps::GUID_t& guid, uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "rtps participant (" << guid << ") is null.");
        return evbs::edds::dds::builtin::StatisticProxyInfos();
    }

    return dispatcher_participant->lookup_manager()->lookup_writer_reader_proxy_infos(guid, timeout);
}

vbsutil::elog::Log::GuidMessageBriefsMap DomainParticipantImpl::lookupMessageBrief(const std::string& host_name,
                                                                                   uint32_t pid,
                                                                                   const std::string& topic_name,
                                                                                   uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_BAD_PARAMETER, "remotConfig rtps participant is null.");
        return vbsutil::elog::Log::GuidMessageBriefsMap();
    }

    return dispatcher_participant->lookup_manager()->lookup_statistic_message_brief(host_name, pid, topic_name,
                                                                                    timeout);
}

RetCode_t DomainParticipantImpl::remoteConfig(const std::string& host_name, uint32_t pid, const std::string& params,
                                              uint32_t timeout) {
    const vbs::dispatcher::DispatcherParticipant* dispatcher_participant = get_dispatcher_participant();
    if (dispatcher_participant == nullptr) {
        elogError(PARTICIPANT, RetCode_t::RETCODE_PRECONDITION_NOT_MET, "lookupMessageBrief rtps participant is null.");
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    return dispatcher_participant->lookup_manager()->remoteConfig(host_name, pid, params, timeout)();
}

#if 0
template <EndpointKind_t kind, octet no_key, octet with_key>
bool DomainParticipantImpl::preprocess_endpoint_attributes(const EntityId_t& entity_id, std::atomic<uint32_t>& id_counter,
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
            entId.value[3] = ((-2 == att.getUserDefinedID()) && (0 < att.getEntityID())) ? (0x60U) | no_key : no_key;
        } else if (att.topicKind == WITH_KEY) {
            entId.value[3] = ((-2 == att.getUserDefinedID()) && (0 < att.getEntityID())) ? (0x60) | with_key : with_key;
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
                                                               << persistence_guid_property->c_str()
                                                               << "'. Wrong input");
                return false;
            }
        }
    }

    // Error log level can be disable. Avoid unused warning
    static_cast<void>(debug_label);

    return true;
}
#endif
// Get all the participants in DataBase
bool DomainParticipantImpl::get_participants_in_db(std::vector<GuidPrefix_t>& prefix_list) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    return sub_part->get_participants_in_db(prefix_list);
}

// Get the participant info by guid_prefix
bool DomainParticipantImpl::get_participant_info(const GuidPrefix_t& prefix, DiscoveryParticipantInfo& part_info) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_participant_info(prefix, part_info);
}

// Get the reader info by guid
bool DomainParticipantImpl::get_reader_info(const GUID_t& reader_guid, DiscoveryEndpointInfo& reader_info) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_reader_info(reader_guid, reader_info);
}

// Get the writer info by guid
bool DomainParticipantImpl::get_writer_info(const GUID_t& writer_guid, DiscoveryEndpointInfo& writer_info) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_writer_info(writer_guid, writer_info);
}

bool DomainParticipantImpl::get_topic_names(std::vector<std::string>& topic_list) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    return sub_part->get_topic_names(topic_list);
}

bool DomainParticipantImpl::has_writer(const std::string& topic_name) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    return sub_part->has_writer(topic_name);
}

bool DomainParticipantImpl::has_reader(const std::string& topic_name) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    return sub_part->has_reader(topic_name);
}

bool DomainParticipantImpl::get_writer_of_topic(const std::string& topic_name,
                                                std::vector<DiscoveryEndpointInfo>& writer_info_list) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_writer_of_topic(topic_name, writer_info_list);
}

bool DomainParticipantImpl::get_reader_of_topic(const std::string& topic_name,
                                                std::vector<DiscoveryEndpointInfo>& reader_info_list) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_reader_of_topic(topic_name, reader_info_list);
}

bool DomainParticipantImpl::has_participant(const std::string& participant_name) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    return sub_part->has_participant(participant_name);
}

bool DomainParticipantImpl::get_participant_of_name(const std::string& participant_name,
                                                    DiscoveryParticipantInfo& part_info) {
    const SubstrateParticipant* sub_part = get_dispatcher_participant()->getSubstrateParticipant(SUBSTRATE_TYPE_RTPS);
    RTPSParticipant* rtps_part = const_cast<RTPSParticipant*>(static_cast<const RTPSParticipant*>(sub_part));
    return rtps_part->get_participant_of_name(participant_name, part_info);
}

void DomainParticipantImpl::update_registered_topics() {
    if (!qos_.wire_protocol().builtin.lookup_config.use_client) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "use_client not set when register remote type. guid " << guid());
        return;
    }
    for (auto& it : discovered_type_name_by_topic_) {
        if (receive_all_topic_ || (listener_topics_.find(it.first) != listener_topics_.end())) {
            register_remote_type(it.second);
            if (discovered_topic_names_.find(it.first) != discovered_topic_names_.end()) {
                continue;
            }
            if (discovered_type_info_.find(it.second) != discovered_type_info_.end()) {
                notify_type_discovery(it.first, it.second);
                discovered_topic_names_.insert(it.first);
            }
        }
    }
}

void DomainParticipantImpl::register_remote_topic(const std::string& topic_name) {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    listener_topics_.insert(topic_name);
    receive_all_topic_ = false;
    update_registered_topics();
}

void DomainParticipantImpl::register_remote_topic() {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    receive_all_topic_ = true;
    update_registered_topics();
}

void DomainParticipantImpl::unregister_remote_topic() {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    receive_all_topic_ = false;
    listener_topics_.clear();
    set_listener(nullptr);
}

Topic* DomainParticipantImpl::find_topic(const std::string& topic_name, const std::string& topic_type) {
    std::string key = topic_name + "-" + topic_type;
    std::lock_guard<std::mutex> lock(mtx_topic_);
    if (topic_participant_infos_.find(key) == topic_participant_infos_.end()) {
        return nullptr;
    }
    return topic_participant_infos_[key];
}

void DomainParticipantImpl::remove_topic(const Topic* topic_ptr) {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    for (auto it = topic_participant_infos_.begin(); it != topic_participant_infos_.end(); ++it) {
        if (it->second == topic_ptr) {
            topic_participant_infos_.erase(it);
            return;
        }
    }
}

void DomainParticipantImpl::add_topic(const std::string& topic_name, const std::string& topic_type, Topic* topic_ptr) {

    std::string key = topic_name + "-" + topic_type;
    std::lock_guard<std::mutex> lock(mtx_topic_);
    topic_participant_infos_[key] = topic_ptr;
}

void DomainParticipantImpl::del_participant_infos() {
    std::lock_guard<std::mutex> lock(mtx_topic_);
    for (auto& i : topic_participant_infos_) {
        delete i.second;
    }
    topic_participant_infos_.clear();
}

void DomainParticipantImpl::delete_topic(ContentFilteredTopic* topic_ptr) {
    remove_topic(topic_ptr);
    delete topic_ptr;
}

void DomainParticipantImpl::remove_topic(ContentFilteredTopic* topic_ptr) {
    std::lock_guard<std::mutex> lock(mtx_content_filtered_topic_);
    content_filtered_topic_infos_.erase(topic_ptr);
}

void DomainParticipantImpl::add_topic(ContentFilteredTopic* topic_ptr) {
    std::unique_lock<std::mutex> lock(mtx_content_filtered_topic_);
    content_filtered_topic_infos_.insert(topic_ptr);
}

void DomainParticipantImpl::del_content_filtered_topics() {
    std::unique_lock<std::mutex> lock(mtx_content_filtered_topic_);
    for (auto topic_ptr : content_filtered_topic_infos_) {
        delete topic_ptr;
    }
    content_filtered_topic_infos_.clear();
}

vbs::common::types::DynamicType_ptr DomainParticipantImpl::get_dynamic_type(const std::string& type_name) {
    std::lock_guard<std::recursive_mutex> lock(mtx_type_discovery_);
    if (discovered_type_info_.find(type_name) == discovered_type_info_.end()) {
        return vbs::common::types::DynamicType_ptr(nullptr);
    }
    return discovered_type_info_[type_name];
}

}  // namespace vbs
