// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <functional>

#include "topic/TopicCore.hpp"
#include "topic/TopicImpl.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "deps/common/TypeSupport.hpp"
#include "elog/Log.hpp"

namespace vbs {

TopicCore::TopicCore(vbs::TopicProxyFactory* factory, vbs::DomainParticipantImpl* p, TypeSupport type_support,
                     const vbsutil::xmlparser::TopicQos& qos, vbs::TopicListener* listen)
    : factory_(factory),
      participant_(p),
      type_support_(std::move(type_support)),
      qos_(&qos == &vbsutil::xmlparser::getDefaultTopicQos() ? participant_->get_default_topic_qos() : qos),
      listener_(listen) {}

TopicCore::~TopicCore() {}

vbsutil::elog::ReturnCode_t TopicCore::check_qos_including_resource_limits(const vbsutil::xmlparser::TopicQos& qos,
                                                                           const TypeSupport& type) {
    vbsutil::elog::ReturnCode_t check_qos_return = check_qos(qos);
    if (vbsutil::elog::ReturnCode_t::RETCODE_OK == check_qos_return && type->m_isGetKeyDefined) {
        check_qos_return = check_allocation_consistency(qos);
    }
    return check_qos_return;
}

vbsutil::elog::ReturnCode_t TopicCore::check_qos(const vbsutil::xmlparser::TopicQos& qos) {
    if ((AUTOMATIC_LIVELINESS_QOS == qos.liveliness().kind) ||
        (MANUAL_BY_PARTICIPANT_LIVELINESS_QOS == qos.liveliness().kind)) {
        if ((qos.liveliness().lease_duration < evbs::ertps::c_TimeInfinite) &&
            (qos.liveliness().lease_duration <= qos.liveliness().announcement_period)) {
            elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY, "lease_duration <= announcement period.");
            return vbsutil::elog::ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
        }
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

vbsutil::elog::ReturnCode_t TopicCore::check_allocation_consistency(const vbsutil::xmlparser::TopicQos& qos) {
    if ((qos.resource_limits().max_samples > 0) &&
        (qos.resource_limits().max_samples <
         (qos.resource_limits().max_instances * qos.resource_limits().max_samples_per_instance))) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be greater than max_instances * max_samples_per_instance");
        return vbsutil::elog::ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    if (((qos.resource_limits().max_instances <= 0) || (qos.resource_limits().max_samples_per_instance <= 0)) &&
        (qos.resource_limits().max_samples > 0)) {
        elogError(RTPS_QOS_CHECK, RetCode_t::RETCODE_INCONSISTENT_POLICY,
                  "max_samples should be infinite when max_instances or "
                  "max_samples_per_instance are infinite");
        return vbsutil::elog::ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

void TopicCore::set_qos(vbsutil::xmlparser::TopicQos& to, const vbsutil::xmlparser::TopicQos& from, bool first_time) {
    (void)first_time;
    to = from;

    // Topic Qos is only used to create other Qos, so it can always be updated
}

bool TopicCore::can_qos_be_updated(const vbsutil::xmlparser::TopicQos& to, const vbsutil::xmlparser::TopicQos& from) {
    (void)to;
    (void)from;

    return true;
}

const vbsutil::xmlparser::TopicQos& TopicCore::get_qos() const {
    return qos_;
}

vbsutil::elog::ReturnCode_t TopicCore::set_qos(const vbsutil::xmlparser::TopicQos& qos) {
    if (&qos == &vbsutil::xmlparser::getDefaultTopicQos()) {
        const vbsutil::xmlparser::TopicQos& default_qos = participant_->get_default_topic_qos();
        if (!can_qos_be_updated(qos_, default_qos)) {
            return vbsutil::elog::ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
        }

        set_qos(qos_, default_qos, false);
        return vbsutil::elog::ReturnCode_t::RETCODE_OK;
    }

    vbsutil::elog::ReturnCode_t ret_val = check_qos_including_resource_limits(qos, type_support_);
    if (!ret_val) {
        return ret_val;
    }

    if (!can_qos_be_updated(qos_, qos)) {
        return vbsutil::elog::ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }

    set_qos(qos_, qos, false);
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

const vbs::TopicListener* TopicCore::get_listener() const {
    return listener_;
}

void TopicCore::set_listener(vbs::TopicListener* listener) {
    listener_ = listener;
}

void TopicCore::set_listener(vbs::TopicListener* listener, const StatusMask& mask) {
    participant_->set_topic_listener(factory_, this, listener, mask);
}

vbs::DomainParticipantImpl* TopicCore::get_participant() const {
    return participant_;
}

const TypeSupport& TopicCore::get_type() const {
    return type_support_;
}

vbs::TopicListener* TopicCore::get_listener_for(const StatusMask& status, const vbs::TopicImpl* topic) {
    if ((listener_ != nullptr) && topic->get_status_mask().is_active(status)) {
        return listener_;
    }
    return participant_->get_listener_for(status);
}

}  // namespace vbs
