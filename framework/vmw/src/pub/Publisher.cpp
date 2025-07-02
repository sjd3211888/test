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

#include <functional>

#include "edds/rtps/participant/RTPSParticipant.h"
#include "ertps/xmlparser/XMLProfileManager.h"
#include "ertps/attributes/PublisherAttributes.h"
#include "domain/DomainParticipantImpl.hpp"
#include "topic/TopicDescriptionImpl.hpp"
#include "utils/QosConverters.hpp"
#include "pub/Publisher.hpp"
#include "pub/PublisherListener.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "deps/common/TypeSupport.hpp"
#include "pub/DataWriterImpl.hpp"
#include "elog/Log.hpp"

namespace evbs {
namespace edds {
namespace dds {

using ertps::Duration_t;
using ertps::rtps::InstanceHandle_t;
using ertps::xmlparser::XMLP_ret;
using ertps::xmlparser::XMLProfileManager;
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {
Publisher::Publisher(vbs::DomainParticipantImpl* p, const PublisherQos& qos, PublisherListener* listen,
                     const evbs::edds::dds::StatusMask& mask)
    : DomainEntity(mask),
      participant_(p),
      qos_((&qos == &vbsutil::xmlparser::getDefaultPublisherQos()) ? participant_->get_default_publisher_qos() : qos),
      listener_(listen),
      publisher_listener_(this),
      dispatcher_participant_(p->get_dispatcher_participant()),
      default_datawriter_qos_(vbsutil::xmlparser::getDefaultDataWriteQos()) {
    const std::unique_ptr<PublisherAttributes> pub_attr(new PublisherAttributes);
    XMLProfileManager::getDefaultPublisherAttributes(*pub_attr);
    utils::set_qos_from_attributes(default_datawriter_qos_, *pub_attr);
}

vbsutil::elog::ReturnCode_t Publisher::enable() {
    if (enable_) {
        return vbsutil::elog::ReturnCode_t::RETCODE_OK;
    }

    if (false == get_participant()->is_enabled()) {
        return vbsutil::elog::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    enable_ = true;
    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& topic_writers : writers_) {
        for (DataWriterImpl* dw : topic_writers.second) {
            (void)dw->enable();
        }
    }

    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

void Publisher::disable() {
    (void)set_listener(nullptr);

    {
        std::lock_guard<std::mutex> lock(mtx_writers_);
        for (auto it = writers_.begin(); it != writers_.end(); ++it) {
            for (DataWriterImpl* dw : it->second) {
                dw->disable();
            }
        }
    }
}

Publisher::~Publisher() {
    {
        std::lock_guard<std::mutex> lock(mtx_writers_);
        for (auto it = writers_.begin(); it != writers_.end(); ++it) {
            for (DataWriterImpl* dw : it->second) {
                dw->batch_free_ = true;
                delete dw;
            }
        }
        writers_.clear();
    }

    participant_ = nullptr;
    listener_ = nullptr;
    dispatcher_participant_ = nullptr;
}

const PublisherQos& Publisher::get_qos() const {
    return qos_;
}

ReturnCode_t Publisher::get_qos(PublisherQos& qos) const {
    qos = qos_;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

ReturnCode_t Publisher::set_qos(const PublisherQos& qos) {
    bool enabled = is_enabled();

    const PublisherQos& qos_to_set =
        (&qos == &vbsutil::xmlparser::getDefaultPublisherQos()) ? participant_->get_default_publisher_qos() : qos;

    if (&qos != &vbsutil::xmlparser::getDefaultPublisherQos()) {
        ReturnCode_t ret_val = check_qos(qos_to_set);
        if (!ret_val) {
            return ret_val;
        }
    }

    if (enabled && (!can_qos_be_updated(qos_, qos_to_set))) {
        return ReturnCode_t(ReturnCode_t::RETCODE_IMMUTABLE_POLICY);
    }
    set_qos(qos_, qos_to_set, !enabled);

    if (enabled) {
        std::lock_guard<std::mutex> lock(mtx_writers_);
        for (auto& topic_writers : writers_) {
            for (auto writer : topic_writers.second) {
                writer->publisher_qos_updated();
            }
        }
    }

    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

const PublisherListener* Publisher::get_listener() const {
    return listener_;
}

ReturnCode_t Publisher::set_listener(PublisherListener* listener) {
    return set_listener(listener, evbs::edds::dds::StatusMask::all());
}

ReturnCode_t Publisher::set_listener(PublisherListener* listener, const evbs::edds::dds::StatusMask& mask) {
    listener_ = listener;
    status_mask_ = mask;

    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

void Publisher::PublisherWriterListener::on_publication_matched(DataWriterImpl* writer,
                                                                const evbs::edds::dds::PublicationMatchedStatus& info) {
    if (publisher_->listener_ != nullptr) {
        publisher_->listener_->on_publication_matched(writer, info);
    }
}

void Publisher::PublisherWriterListener::on_liveliness_lost(DataWriterImpl* writer,
                                                            const evbs::edds::dds::LivelinessLostStatus& status) {
    if (publisher_->listener_ != nullptr) {
        publisher_->listener_->on_liveliness_lost(writer, status);
    }
}

void Publisher::PublisherWriterListener::on_offered_deadline_missed(
    DataWriterImpl* writer, const evbs::ertps::OfferedDeadlineMissedStatus& status) {
    if (publisher_->listener_ != nullptr) {
        publisher_->listener_->on_offered_deadline_missed(writer, status);
    }
}

DataWriterImpl* Publisher::create_datawriter(vbs::TopicImpl* topic, const vbsutil::xmlparser::DataWriterQos& qos,
                                             DataWriterListenerInner* listener, const evbs::edds::dds::StatusMask& mask,
                                             bool auto_enable) {
    if (nullptr == topic) {
        return nullptr;
    }

    // Look for the correct type registration
    TypeSupport type_support = participant_->find_type(topic->get_type_name());

    /// Preconditions
    // Check the type was registered.
    if (type_support.empty()) {
        elogError(PUBLISHER, RetCode_t::RETCODE_PRECONDITION_NOT_MET,
                  "Type: " << topic->get_type_name() << " Not Registered");
        return nullptr;
    }

    if (!DataWriterImpl::check_qos_including_resource_limits(qos, type_support)) {
        return nullptr;
    }

    DataWriterImpl* writer = new DataWriterImpl(this, std::move(type_support), topic, qos, listener, mask);
    qos.exportDataWriterQosInfo();

    {
        std::lock_guard<std::mutex> lock(mtx_writers_);
        writers_[topic->get_name()].push_back(writer);
    }

    if (is_enabled() && auto_enable) {
        if (ReturnCode_t::RETCODE_OK != writer->enable()) {
            (void)delete_datawriter(writer);
            return nullptr;
        }
    }

    if (writer) {
        logInfo(PUBLISHER, "Writer " << writer->guid() << " created in topic: " << topic->get_name()
                                     << " mask:" << get_status_mask() << " enable:" << auto_enable
                                     << " listener:" << (listener != nullptr));
    }

    return writer;
}

DataWriterImpl* Publisher::create_datawriter_with_profile(vbs::TopicImpl* topic, const std::string& profile_name,
                                                          DataWriterListenerInner* listener,
                                                          const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    const std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, *attr)) {
        vbsutil::xmlparser::DataWriterQos qos = default_datawriter_qos_;
        utils::set_qos_from_attributes(qos, *attr);
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
        logInfo(PUBLISHER, "Writer creating in topic " << topic->get_name() << " with profile" << profile_name);
        return create_datawriter(topic, qos, listener, mask, auto_enable);
    }

    return nullptr;
}

DataWriterImpl* Publisher::create_datawriter_with_profile(vbs::TopicImpl* topic,
                                                          const vbsutil::xmlparser::DataWriterQos& qos_input,
                                                          DataWriterListenerInner* listener,
                                                          const evbs::edds::dds::StatusMask& mask, bool auto_enable) {
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
    return create_datawriter(topic, qos, listener, mask, auto_enable);
}

ReturnCode_t Publisher::add_datawriter(const vbs::TopicImpl* topic, DataWriterImpl* writer) {
    if (nullptr == topic || nullptr == writer) {
        return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
    }
    std::lock_guard<std::mutex> lock(mtx_writers_);
    writers_[topic->get_name()].push_back(writer);
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

ReturnCode_t Publisher::del_datawriter(const DataWriterImpl* writer) {
    if (nullptr == writer) {
        return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
    }

    if (this != writer->get_publisher()) {
        return ReturnCode_t(ReturnCode_t::RETCODE_PRECONDITION_NOT_MET);
    }
    std::unique_lock<std::mutex> lock(mtx_writers_);
    auto vit = writers_.find(writer->get_topic()->get_name());
    if (vit != writers_.end()) {
        auto dw_it = std::find(vit->second.begin(), vit->second.end(), writer);
        if (dw_it != vit->second.end()) {
            // First extract the writer from the maps to free the mutex
            DataWriterImpl* writer = *dw_it;
            ReturnCode_t ret_code = writer->check_delete_preconditions();
            if (!ret_code) {
                return ret_code;
            }
            (void)writer->set_listener(nullptr);
            (void)vit->second.erase(dw_it);
            if (vit->second.empty()) {
                (void)writers_.erase(vit);
            }
            lock.unlock();
            return ReturnCode_t(ReturnCode_t::RETCODE_OK);
        }
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
}

ReturnCode_t Publisher::delete_datawriter(const DataWriterImpl* writer) {
    if (nullptr == writer) {
        return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
    }

    if (this != writer->get_publisher()) {
        return ReturnCode_t(ReturnCode_t::RETCODE_PRECONDITION_NOT_MET);
    }
    std::unique_lock<std::mutex> lock(mtx_writers_);
    auto vit = writers_.find(writer->get_topic()->get_name());
    if (vit != writers_.end()) {
        auto dw_it = std::find(vit->second.begin(), vit->second.end(), writer);
        if (dw_it != vit->second.end()) {
            // First extract the writer from the maps to free the mutex
            DataWriterImpl* writer = *dw_it;
            ReturnCode_t ret_code = writer->check_delete_preconditions();
            if (!ret_code) {
                return ret_code;
            }
            (void)writer->set_listener(nullptr);
            (void)vit->second.erase(dw_it);
            if (vit->second.empty()) {
                (void)writers_.erase(vit);
            }
            lock.unlock();
            delete (writer);
            return ReturnCode_t(ReturnCode_t::RETCODE_OK);
        }
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
}

DataWriterImpl* Publisher::lookup_datawriter(const std::string& topic_name) const {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    auto it = writers_.find(topic_name);
    if ((it != writers_.end()) && (it->second.size() > 0U)) {
        return it->second.front();
    }
    return nullptr;
}

bool Publisher::get_datawriters(std::vector<DataWriterImpl*>& writers) const {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& vit : writers_) {
        for (DataWriterImpl* dw : vit.second) {
            writers.push_back(dw);
        }
    }
    return true;
}

bool Publisher::has_datawriters() const {
    if (writers_.empty()) {
        return false;
    }
    return true;
}

bool Publisher::contains_entity(const evbs::ertps::rtps::InstanceHandle_t& handle) const {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& vit : writers_) {
        for (DataWriterImpl* dw : vit.second) {
            InstanceHandle_t h(dw->guid());
            if (h == handle) {
                return true;
            }
        }
    }
    return false;
}

ReturnCode_t Publisher::set_default_datawriter_qos(const vbsutil::xmlparser::DataWriterQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultDataWriteQos()) {
        reset_default_datawriter_qos();
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }

    ReturnCode_t ret_val = DataWriterImpl::check_qos(qos);
    if (!ret_val) {
        return ret_val;
    }
    DataWriterImpl::set_qos(default_datawriter_qos_, qos, true);
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

void Publisher::reset_default_datawriter_qos() {
    // TODO (ILG): Change when we have full XML support for DDS QoS profiles
    DataWriterImpl::set_qos(default_datawriter_qos_, vbsutil::xmlparser::getDefaultDataWriteQos(), true);
    const std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    XMLProfileManager::getDefaultPublisherAttributes(*attr);
    utils::set_qos_from_attributes(default_datawriter_qos_, *attr);
}

const vbsutil::xmlparser::DataWriterQos& Publisher::get_default_datawriter_qos() const {
    return default_datawriter_qos_;
}

ReturnCode_t Publisher::get_default_datawriter_qos(vbsutil::xmlparser::DataWriterQos& qos) const {
    qos = default_datawriter_qos_;
    return ReturnCode_t::RETCODE_OK;
}

ReturnCode_t Publisher::get_datawriter_qos_from_profile(const std::string& profile_name,
                                                        vbsutil::xmlparser::DataWriterQos& qos) const {
    const std::unique_ptr<PublisherAttributes> attr(new PublisherAttributes);
    if (XMLP_ret::XML_OK == XMLProfileManager::fillPublisherAttributes(profile_name, *attr, false)) {
        qos = default_datawriter_qos_;
        utils::set_qos_from_attributes(qos, *attr);
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }

    return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
}

ReturnCode_t Publisher::wait_for_acknowledgments(const Duration_t& max_wait) {
    Duration_t current = max_wait;
    Duration_t begin, end;
    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& vit : writers_) {
        for (DataWriterImpl* dw : vit.second) {
            (void)participant_->get_current_time(begin);
            if (!dw->wait_for_acknowledgments(current)) {
                return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
            }
            // Check ellapsed time and decrement
            (void)participant_->get_current_time(end);
            current = current - (end - begin);
            if (current < evbs::ertps::c_TimeZero) {
                return ReturnCode_t(ReturnCode_t::RETCODE_TIMEOUT);
            }
        }
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

vbs::DomainParticipantImpl* Publisher::get_participant() const {
    return participant_;
}

ReturnCode_t Publisher::delete_contained_entities() {
    // Let's be optimistic
    ReturnCode_t result = ReturnCode_t(ReturnCode_t::RETCODE_OK);

    bool can_be_deleted = true;

    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& writer : writers_) {
        for (DataWriterImpl* dw : writer.second) {
            can_be_deleted = dw->check_delete_preconditions() == ReturnCode_t(ReturnCode_t::RETCODE_OK);
            if (!can_be_deleted) {
                return ReturnCode_t(ReturnCode_t::RETCODE_PRECONDITION_NOT_MET);
            }
        }
    }

    // We traverse the map trying to delete all writers;
    auto writer_iterator = writers_.begin();
    while (writer_iterator != writers_.end()) {
        // First extract the writer from the maps to free the mutex
        auto it = writer_iterator->second.begin();
        if (it != writer_iterator->second.end()) {
            DataWriterImpl* writer = *it;
            if (!writer->check_delete_preconditions()) {
                return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
            }
            (void)writer->set_listener(nullptr);
            it = writer_iterator->second.erase(it);
            writer->batch_free_ = true;
            delete (writer);
        }
        if (writer_iterator->second.empty()) {
            writer_iterator = writers_.erase(writer_iterator);
        }
    }
    return result;
}

bool Publisher::can_be_deleted() {
    bool can_be_deleted = true;

    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& topic_writers : writers_) {
        for (DataWriterImpl* dw : topic_writers.second) {
            // coverity[misra_cpp_2008_rule_0_1_2_violation:SUPPRESS]
            can_be_deleted =
                can_be_deleted && (dw->check_delete_preconditions() == ReturnCode_t(ReturnCode_t::RETCODE_OK));
            if (!can_be_deleted) {
                return can_be_deleted;
            }
        }
    }

    return can_be_deleted;
}

const InstanceHandle_t& Publisher::get_instance_handle() const {
    return handle_;
}

bool Publisher::type_in_use(const std::string& type_name) const {
    for (auto& it : writers_) {
        for (DataWriterImpl* writer : it.second) {
            if (writer->get_topic()->get_type_name() == type_name) {
                return true;  // Is in use
            }
        }
    }
    return false;
}

void Publisher::set_qos(PublisherQos& to, const PublisherQos& from, bool first_time) {
    UNUSED_PARAMETER(first_time);
    if (!(to.partition() == from.partition())) {
        to.partition() = from.partition();
        to.partition().hasChanged = true;
    }
}

ReturnCode_t Publisher::check_qos(const PublisherQos& qos) {
    (void)qos;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

bool Publisher::can_qos_be_updated(const PublisherQos& to, const PublisherQos& from) {
    (void)to;
    (void)from;
    return true;
}

PublisherListener* Publisher::get_listener_for(const evbs::edds::dds::StatusMask& status) {
    if ((listener_ != nullptr) && get_status_mask().is_active(status)) {
        return listener_;
    }
    return participant_->get_listener_for(status);
}

}  // namespace vbs
