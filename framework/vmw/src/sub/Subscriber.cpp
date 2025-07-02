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

#include "edds/rtps/common/Property.h"
#include "edds/rtps/participant/RTPSParticipant.h"
#include "ertps/attributes/SubscriberAttributes.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "topic/TopicDescriptionImpl.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "sub/Subscriber.hpp"
#include "sub/SubscriberListener.hpp"
#include "sub/DataReaderImpl.hpp"
#include "deps/common/TypeSupport.hpp"
#include "utils/QosConverters.hpp"
#include "elog/Log.hpp"

namespace vbs {

Subscriber::Subscriber(vbs::DomainParticipantImpl* dp, const vbsutil::xmlparser::SubscriberQos& qos,
                       SubscriberListener* listener, const evbs::edds::dds::StatusMask& mask)
    : DomainEntity(mask),
      participant_(dp),
      qos_(&qos == &vbsutil::xmlparser::getDefaultSubscriberQos() ? dp->get_default_subscriber_qos() : qos),
      listener_(listener),
      subscriber_listener_(this),
      dispatcher_participant_(dp->get_dispatcher_participant()),
      default_datareader_qos_(vbsutil::xmlparser::getDefaultDataReaderQos()) {
    std::shared_ptr<SubscriberAttributes> sub_attr(new SubscriberAttributes);
    evbs::ertps::xmlparser::XMLProfileManager::getDefaultSubscriberAttributes(*sub_attr);
    utils::set_qos_from_attributes(default_datareader_qos_, *sub_attr);
}

Subscriber::~Subscriber() {
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto it = readers_.begin(); it != readers_.end(); ++it) {
            for (DataReaderImpl* dr : it->second) {
                dr->batch_free_ = true;
                delete dr;
            }
        }
        readers_.clear();
    }

    participant_ = nullptr;
    listener_ = nullptr;
    dispatcher_participant_ = nullptr;
}

evbs::ReturnCode_t Subscriber::enable() {
    if (enable_) {
        return evbs::ReturnCode_t::RETCODE_OK;
    }

    if (false == get_participant()->is_enabled()) {
        return evbs::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    enable_ = true;
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& topic_readers : readers_) {
        for (DataReaderImpl* dr : topic_readers.second) {
            (void)dr->enable();
        }
    }

    return evbs::ReturnCode_t::RETCODE_OK;
}

void Subscriber::disable() {
    (void)set_listener(nullptr);

    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto it = readers_.begin(); it != readers_.end(); ++it) {
            for (DataReaderImpl* dr : it->second) {
                dr->disable();
            }
        }
    }
}

const vbsutil::xmlparser::SubscriberQos& Subscriber::get_qos() const {
    return qos_;
}

ReturnCode_t Subscriber::get_qos(vbsutil::xmlparser::SubscriberQos& qos) const {
    qos = qos_;
    return ReturnCode_t::RETCODE_OK;
}

void Subscriber::set_qos(vbsutil::xmlparser::SubscriberQos& to, const vbsutil::xmlparser::SubscriberQos& from,
                         bool first_time) {
    UNUSED_PARAMETER(first_time);
    if (!(to.partition() == from.partition())) {
        to.partition() = from.partition();
        to.partition().hasChanged = true;
    }
}

ReturnCode_t Subscriber::set_qos(const vbsutil::xmlparser::SubscriberQos& qos) {
    bool enabled = is_enabled();
    const vbsutil::xmlparser::SubscriberQos& qos_to_set =
        (&qos == &vbsutil::xmlparser::getDefaultSubscriberQos()) ? participant_->get_default_subscriber_qos() : qos;

    if (&qos != &vbsutil::xmlparser::getDefaultSubscriberQos()) {
        ReturnCode_t check_result = check_qos(qos_to_set);
        if (!check_result) {
            return check_result;
        }
    }

    if (enabled && (!can_qos_be_updated(qos_, qos_to_set))) {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }
    set_qos(qos_, qos_to_set, !enabled);

    if (enabled) {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        for (auto& topic_readers : readers_) {
            for (auto reader : topic_readers.second) {
                reader->subscriber_qos_updated();
            }
        }
    }

    return ReturnCode_t::RETCODE_OK;
}

const SubscriberListener* Subscriber::get_listener() const {
    return listener_;
}

ReturnCode_t Subscriber::set_listener(SubscriberListener* listener) {
    listener_ = listener;
    status_mask_ = evbs::edds::dds::StatusMask::all();

    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t Subscriber::set_listener(SubscriberListener* listener, const evbs::edds::dds::StatusMask& mask) {
    listener_ = listener;
    status_mask_ = mask;

    return ReturnCode_t::RETCODE_OK;
}

DataReaderImpl* Subscriber::create_datareader(vbs::TopicDescription* topic,
                                              const vbsutil::xmlparser::DataReaderQos& reader_qos,
                                              DataReaderListenerInner* listener,
                                              const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    if (nullptr == topic) {
        return nullptr;
    }

    // Look for the correct type registration
    TypeSupport type_support = participant_->find_type(topic->get_type_name());

    /// Preconditions
    // Check the type was registered.
    if (type_support.empty()) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Type : " << topic->get_type_name() << " Not Registered");
        return nullptr;
    }

    if (!DataReaderImpl::check_qos_including_resource_limits(reader_qos, type_support)) {
        return nullptr;
    }

    DataReaderImpl* reader = new DataReaderImpl(this, type_support, topic, reader_qos, listener, mask);
    reader_qos.exportDataReaderQosInfo();

    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        readers_[topic->get_name()].push_back(reader);
    }

    if (is_enabled() && auto_enable) {
        if (ReturnCode_t::RETCODE_OK != reader->enable()) {
            (void)delete_datareader(reader);
            return nullptr;
        }
    }

    if (reader) {
        logInfo(SUBSCRIBER, "Reader " << reader->guid() << " created in topic: " << topic->get_name() << " enable:"
                                      << auto_enable << " mask:" << mask << " listener:" << (listener != nullptr));
    }
    return reader;
}

vbs::TopicImpl* Subscriber::get_topic_from_desc(vbs::TopicDescription* topic_desc) {
    vbs::TopicImpl* topic = dynamic_cast<vbs::TopicImpl*>(topic_desc);
    if (topic) {
        return topic;
    }

    vbs::ContentFilteredTopicInner* content_filter_topic = dynamic_cast<vbs::ContentFilteredTopicInner*>(topic_desc);
    if (content_filter_topic) {
        return content_filter_topic->get_related_topic();
    }

    return nullptr;
}

DataReaderImpl* Subscriber::create_datareader_with_profile(vbs::TopicDescription* topic_desc,
                                                           const std::string& profile_name,
                                                           DataReaderListenerInner* listener,
                                                           const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    if (XMLP_ret::XML_OK == evbs::ertps::xmlparser::XMLProfileManager::fillSubscriberAttributes(profile_name, *attr)) {
        vbsutil::xmlparser::DataReaderQos qos = default_datareader_qos_;
        utils::set_qos_from_attributes(qos, *attr);

        vbs::TopicImpl* topic = get_topic_from_desc(topic_desc);
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
        DataReaderImpl* reader = create_datareader(topic_desc, qos, listener, mask, auto_enable);
        if (reader) {
            logInfo(SUBSCRIBER,
                    "Reader created in topic: " << topic_desc->get_name() << " with profile " << profile_name);
        }
        return reader;
    }

    return nullptr;
}

DataReaderImpl* Subscriber::create_datareader_with_profile(vbs::TopicDescription* topic_desc,
                                                           const vbsutil::xmlparser::DataReaderQos& qos_input,
                                                           DataReaderListenerInner* listener,
                                                           const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    vbsutil::xmlparser::DataReaderQos qos = qos_input;
    vbs::TopicImpl* topic = get_topic_from_desc(topic_desc);
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
    DataReaderImpl* reader = create_datareader(topic_desc, qos, listener, mask, auto_enable);
    if (reader) {
        logInfo(SUBSCRIBER, "Reader created in topic: " << topic_desc->get_name());
    }
    return reader;
}

ReturnCode_t Subscriber::add_datareader(const vbs::TopicImpl* topic, DataReaderImpl* reader) {
    if (nullptr == topic || nullptr == reader) {
        return ReturnCode_t::RETCODE_ERROR;
    }
    std::lock_guard<std::mutex> lock(mtx_readers_);
    readers_[topic->get_name()].push_back(reader);
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t Subscriber::delete_datareader(const DataReaderImpl* reader, bool free) {
    if (nullptr == reader) {
        return ReturnCode_t::RETCODE_ERROR;
    }
    if (this != reader->get_subscriber()) {
        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }
    std::unique_lock<std::mutex> lock(mtx_readers_);
    auto it = readers_.find(reader->get_topicdescription()->get_name());
    if (it != readers_.end()) {
        auto dr_it = std::find(it->second.begin(), it->second.end(), reader);
        if (dr_it != it->second.end()) {
            // First extract the reader from the maps to free the mutex
            DataReaderImpl* reader_impl = *dr_it;
            if (!reader_impl->can_be_deleted(false)) {
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
            }

            (void)it->second.erase(dr_it);
            if (it->second.empty()) {
                (void)readers_.erase(it);
            }
            lock.unlock();

            // Now we can delete it
            if (free) {
                delete (reader_impl);
            }

            return ReturnCode_t::RETCODE_OK;
        }
    }
    return ReturnCode_t::RETCODE_ERROR;
}

DataReaderImpl* Subscriber::lookup_datareader(const std::string& topic_name) const {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    auto it = readers_.find(topic_name);
    if ((it != readers_.end()) && (it->second.size() > 0U)) {
        return it->second.front();
    }
    return nullptr;
}

ReturnCode_t Subscriber::get_datareaders(std::vector<DataReaderImpl*>& readers) const {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& it : readers_) {
        for (DataReaderImpl* dr : it.second) {
            readers.push_back(dr);
        }
    }
    return ReturnCode_t::RETCODE_OK;
}

bool Subscriber::has_datareaders() const {
    if (readers_.empty()) {
        return false;
    }
    return true;
}

ReturnCode_t Subscriber::notify_datareaders() const {
    for (auto& it : readers_) {
        for (DataReaderImpl* dr : it.second) {
            dr->listener_->on_data_available(dr);
        }
    }
    return ReturnCode_t::RETCODE_OK;
}

void Subscriber::reset_default_datareader_qos() {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    DataReaderImpl::set_qos(default_datareader_qos_, vbsutil::xmlparser::getDefaultDataReaderQos(), true);
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    evbs::ertps::xmlparser::XMLProfileManager::getDefaultSubscriberAttributes(*attr);
    utils::set_qos_from_attributes(default_datareader_qos_, *attr);
}

ReturnCode_t Subscriber::set_default_datareader_qos(const vbsutil::xmlparser::DataReaderQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultDataReaderQos()) {
        reset_default_datareader_qos();
        return ReturnCode_t::RETCODE_OK;
    }

    ReturnCode_t check_result = DataReaderImpl::check_qos(qos);
    if (!check_result) {
        return check_result;
    }

    DataReaderImpl::set_qos(default_datareader_qos_, qos, true);
    return ReturnCode_t::RETCODE_OK;
}

const vbsutil::xmlparser::DataReaderQos& Subscriber::get_default_datareader_qos() const {
    return default_datareader_qos_;
}

vbsutil::xmlparser::DataReaderQos& Subscriber::get_default_datareader_qos() {
    return default_datareader_qos_;
}

ReturnCode_t Subscriber::get_default_datareader_qos(vbsutil::xmlparser::DataReaderQos& qos) const {
    qos = default_datareader_qos_;
    return ReturnCode_t::RETCODE_OK;
}

bool Subscriber::contains_entity(const evbs::ertps::rtps::InstanceHandle_t& handle) const {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& vit : readers_) {
        for (DataReaderImpl* dw : vit.second) {
            InstanceHandle_t h(dw->guid());
            if (h == handle) {
                return true;
            }
        }
    }
    return false;
}

ReturnCode_t Subscriber::get_datareader_qos_from_profile(const std::string& profile_name,
                                                         vbsutil::xmlparser::DataReaderQos& qos) const {
    std::unique_ptr<SubscriberAttributes> attr(new SubscriberAttributes);
    if (XMLP_ret::XML_OK ==
        evbs::ertps::xmlparser::XMLProfileManager::fillSubscriberAttributes(profile_name, *attr, false)) {
        qos = default_datareader_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return ReturnCode_t::RETCODE_OK;
    }

    return ReturnCode_t::RETCODE_BAD_PARAMETER;
}

vbs::DomainParticipantImpl* Subscriber::get_participant() const {
    return participant_;
}

const evbs::ertps::rtps::InstanceHandle_t& Subscriber::get_instance_handle() const {
    return handle_;
}

void Subscriber::SubscriberReaderListener::on_data_available(DataReaderImpl* reader) {
    if (subscriber_->listener_ != nullptr) {
        subscriber_->listener_->on_data_available(reader);
        subscriber_->listener_->on_data_on_readers(subscriber_);
    }
}

void Subscriber::SubscriberReaderListener::on_subscription_matched(
    DataReaderImpl* reader, const evbs::edds::dds::SubscriptionMatchedStatus& info) {
    if (subscriber_->listener_ != nullptr) {
        subscriber_->listener_->on_subscription_matched(reader, info);
    }
}

void Subscriber::SubscriberReaderListener::on_requested_deadline_missed(
    DataReaderImpl* reader, const evbs::ertps::RequestedDeadlineMissedStatus& status) {
    if (subscriber_->listener_ != nullptr) {
        subscriber_->listener_->on_requested_deadline_missed(reader, status);
    }
}

void Subscriber::SubscriberReaderListener::on_liveliness_changed(DataReaderImpl* reader,
                                                                 const evbs::ertps::LivelinessChangedStatus& status) {
    (void)status;

    SubscriberListener* listener = subscriber_->listener_;
    if ((listener == nullptr) ||
        (!subscriber_->get_status_mask().is_active(evbs::edds::dds::StatusMask::liveliness_changed()))) {
        auto participant = subscriber_->get_participant();
        auto part_listener = const_cast<DomainParticipantListenerInner*>(participant->get_listener());
        listener = static_cast<SubscriberListener*>(part_listener);
        if (!participant->get_status_mask().is_active(evbs::edds::dds::StatusMask::liveliness_changed())) {
            listener = nullptr;
        }
    }

    if (listener != nullptr) {
        evbs::edds::dds::LivelinessChangedStatus callback_status;
        (void)reader->get_liveliness_changed_status(callback_status);
        listener->on_liveliness_changed(reader, callback_status);
    }
}

void Subscriber::SubscriberReaderListener::on_sample_rejected(DataReaderImpl* /*reader*/,
                                                              const evbs::ertps::SampleRejectedStatus& /*status*/) {
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_sample_rejected(reader, status);
       }
     */
}

void Subscriber::SubscriberReaderListener::on_requested_incompatible_qos(
    DataReaderImpl* /*reader*/, const evbs::edds::dds::RequestedIncompatibleQosStatus& /*status*/) {
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_requested_incompatible_qos(reader, status);
       }
     */
}

void Subscriber::SubscriberReaderListener::on_sample_lost(DataReaderImpl* /*reader*/,
                                                          const evbs::edds::dds::SampleLostStatus& /*status*/) {
    /* TODO
       if (subscriber_->listener_ != nullptr)
       {
        subscriber_->listener_->on_sample_rejected(reader, status);
       }
     */
}

bool Subscriber::type_in_use(const std::string& type_name) const {
    for (auto& it : readers_) {
        for (DataReaderImpl* reader : it.second) {
            if (reader->topic_attributes()->getTopicDataType() == type_name) {
                return true;  // Is in use
            }
        }
    }
    return false;
}

ReturnCode_t Subscriber::check_qos(const vbsutil::xmlparser::SubscriberQos& qos) {
    (void)qos;
    return ReturnCode_t::RETCODE_OK;
}

bool Subscriber::can_qos_be_updated(const vbsutil::xmlparser::SubscriberQos& to,
                                    const vbsutil::xmlparser::SubscriberQos& from) {
    (void)to;
    (void)from;
    return true;
}

SubscriberListener* Subscriber::get_listener_for(const evbs::edds::dds::StatusMask& status) {
    if ((listener_ != nullptr) && get_status_mask().is_active(status)) {
        return listener_;
    }
    return participant_->get_listener_for(status);
}

ReturnCode_t Subscriber::delete_contained_entities() {
    // Let's be optimistic
    ReturnCode_t result = ReturnCode_t::RETCODE_OK;

    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& reader : readers_) {
        for (DataReaderImpl* dr : reader.second) {
            if (!dr->can_be_deleted()) {
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
            }
        }
    }

    // We traverse the map trying to delete all readers;
    auto reader_iterator = readers_.begin();
    while (reader_iterator != readers_.end()) {
        // First extract the reader from the maps to free the mutex
        auto it = reader_iterator->second.begin();
        if (it == reader_iterator->second.end()) {
            return ReturnCode_t::RETCODE_ERROR;
        }
        DataReaderImpl* reader_impl = *it;
        bool ret_code = reader_impl->can_be_deleted();
        if (!ret_code) {
            return ReturnCode_t::RETCODE_ERROR;
        }
        (void)reader_impl->set_listener(nullptr);
        it = reader_iterator->second.erase(it);
        if (reader_iterator->second.empty()) {
            reader_iterator = readers_.erase(reader_iterator);
        }

        reader_impl->batch_free_ = true;
        delete (reader_impl);
    }
    return result;
}

bool Subscriber::can_be_deleted() const {
    bool return_status = true;

    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& topic_readers : readers_) {
        for (DataReaderImpl* dr : topic_readers.second) {
            return_status = dr->can_be_deleted();
            if (!return_status) {
                return false;
            }
        }
    }
    return true;
}

} /* namespace vbs */
