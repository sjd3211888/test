// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _EDDS_ENTITYTOPICPROXY_HPP_
#define _EDDS_ENTITYTOPICPROXY_HPP_

#include <memory>
#include <string>

#include "deps/core/status/StatusMask.hpp"
#include "topic/TopicImpl.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"
#include "topic/TopicListener.hpp"
#include "topic/TopicDescriptionImpl.hpp"
#include "topic/TopicCore.hpp"
#include "ertps/types/TypesBase.h"

namespace vbs {

class DomainParticipantImpl;
class TypeSupport;

class TopicProxy : public vbs::TopicDescriptionImpl {
 public:
    TopicProxy(const std::string& topic_name, const std::string& type_name, const evbs::edds::dds::StatusMask& mask,
               vbs::TopicCore* topic_core) noexcept
        : topic_core_(topic_core),
          user_topic_(new vbs::TopicImpl(topic_core_->get_participant(), topic_name, type_name, topic_core_->get_type(),
                                         this, mask)) {}

    const vbsutil::xmlparser::TopicQos& get_qos() const { return topic_core_->get_qos(); }

    ReturnCode_t set_qos(const vbsutil::xmlparser::TopicQos& qos) { return topic_core_->set_qos(qos); }

    const TopicListener* get_listener() const { return topic_core_->get_listener(); }

    void set_listener(TopicListener* listener, const evbs::edds::dds::StatusMask& mask) {
        topic_core_->set_listener(listener, mask);
    }

    DomainParticipantImpl* get_participant() const { return topic_core_->get_participant(); }

    const TypeSupport& get_type() const { return topic_core_->get_type(); }

    TopicListener* get_listener_for(const evbs::edds::dds::StatusMask& status) {
        return topic_core_->get_listener_for(status, user_topic_.get());
    }

    vbs::TopicImpl* get_topic() const { return user_topic_.get(); }

    const std::string& get_rtps_topic_name() const override { return user_topic_->get_name(); }

 private:
    vbs::TopicCore* topic_core_ = nullptr;
    std::unique_ptr<vbs::TopicImpl> user_topic_;
};

}  // namespace vbs

#endif /* _EDDS_ENTITYTOPICPROXY_HPP_ */
