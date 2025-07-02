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
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API
// 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: Add to_string mode for enum type
// feature: separating interface and implementation about Topic
// feature: use Listeners and NexusUnits as singleton
// feature: Support Topic throughput collection
// feature: judging this pointer
// feature: VBSFramework log collection
// feature: VBSDynamic interface constness support
// feature: VBS framework add performance test
// feature: Static type to dynamic type
// ------------------------------------------------------------------

#include "core/Global.hpp"
#include "topic/TopicImpl.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "context/NexusUnits.hpp"
#include "context/QosManager.hpp"
#include "vbs/types/VBSDynamicDataPrinter.h"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/VBSDynamicType.h"
#include "vbs/Topic.hpp"

namespace vbs {

Topic::Topic(DomainParticipant* const local_participant, const std::string& topic_name, const std::string& type_name,
             const TypeSupport& local_type, const std::string& profile_name)
    : topic_ptr_(nullptr) {
    if (local_participant == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Local_participant is nullptr, topic_name = " << topic_name << ", type_name = " << type_name
                << ", profile_name = " << profile_name);
        return;
    }
    topic_ptr_ = local_participant->get_instance()->create_topic(topic_name, type_name, local_type, profile_name);
    if (topic_ptr_ != nullptr) {
        NexusUnits::get_unit().InsertTopic(local_participant->get_instance(), topic_ptr_);
        QosManager::Instance()->add_topic(topic_ptr_->get_participant()->guid(), topic_name, type_name, topic_ptr_);
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Failed to create topic, topic_name = " << topic_name << ", type_name = " << type_name
                << ", profile_name = " << profile_name);
    }
}

Topic::Topic(DomainParticipant* const local_participant, const std::string& topic_name, const std::string& type_name,
             VBSDynamicType& dyn_type, const std::string& profile_name)
    : topic_ptr_(nullptr) {
    if (local_participant == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Local_participant is nullptr, topic_name = " << topic_name << ", type_name = " << type_name
                << ", profile_name = " << profile_name);
        return;
    }
    topic_ptr_ = local_participant->get_instance()->create_topic(topic_name, type_name, dyn_type.dynamic_type_.get(),
                                                                 profile_name);
    if (topic_ptr_ != nullptr) {
        NexusUnits::get_unit().InsertTopic(local_participant->get_instance(), topic_ptr_);
        QosManager::Instance()->add_topic(topic_ptr_->get_participant()->guid(), topic_name, type_name, topic_ptr_);
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Failed to create topic, topic_name = " << topic_name << ", type_name = " << type_name
                << ", profile_name = " << profile_name);
    }
}

Topic::Topic(DomainParticipant* const local_participant, const std::string& topic_name, const std::string& type_name,
             const TypeSupport& local_type, const TopicQos& topic_qos)
    : topic_ptr_(nullptr) {
    if (local_participant == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Local_participant is nullptr, topic_name = " << topic_name << ", type_name = " << type_name);
        return;
    }
    topic_ptr_ =
        local_participant->get_instance()->create_topic(topic_name, type_name, local_type, *topic_qos.get_instance());
    if (topic_ptr_ != nullptr) {
        NexusUnits::get_unit().InsertTopic(local_participant->get_instance(), topic_ptr_);
        QosManager::Instance()->add_topic(topic_ptr_->get_participant()->guid(), topic_name, type_name, topic_ptr_);
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " Failed to create topic, topic_name = " << topic_name << ", type_name = " << type_name);
    }
}

Topic::~Topic() {
    if (topic_ptr_) {
        QosManager::Instance()->remove_topic(topic_ptr_->get_participant()->guid(), topic_ptr_->get_topic_name_(),
                                             topic_ptr_->get_data_type_name());
        NexusUnits::get_unit().DeleteTopic(topic_ptr_->get_participant(), topic_ptr_);
        topic_ptr_ = nullptr;
    }
}

const TopicQos Topic::get_qos() const {
    TopicQos topic_qos;
    if (topic_ptr_) {
        topic_qos.set_instance(std::make_shared<vbs::TopicQosImpl>(topic_ptr_->get_qos_impl()));
        return topic_qos;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " topic_ptr_ unexpected nullptr");
    return topic_qos;
}

std::string Topic::get_topic_name() const {
    if (topic_ptr_ == nullptr) {
        return "";
    }
    return topic_ptr_->get_topic_name_();
}

VBSDynamicData* Topic::create_data() {
    vbs::DynamicData* data = static_cast<vbs::DynamicData*>(topic_ptr_->create_data());
    VBSDynamicData* vbs_data = new VBSDynamicData();
    vbs_data->vbs_dynamic_data_ = data;
    vbs_data->type_ = *(topic_ptr_->get_type());
    vbs_data->is_complex_data_ = false;
    vbs_data->type_name_ = topic_ptr_->get_data_type_name();
    return vbs_data;
}

ReturnCode_t Topic::delete_data(VBSDynamicData* data) {
    delete data;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

bool Topic::init_verify() {
    if (topic_ptr_) {
        return topic_ptr_->init_verify();
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " topic_ptr_ unexpected nullptr");
    return false;
}

std::string Topic::to_string(const VBSDynamicData* data, int enum_mode) {
    return TopicImpl::to_string(data, enum_mode);
}

std::ostream& Topic::to_string(std::ostream& out, const VBSDynamicData* data, int enum_mode) {
    return TopicImpl::to_string(out, data, enum_mode);
}

VBSDynamicType Topic::get_dynamic_type_inner(const std::string& type_name) {
    VBSDynamicType type;
    vbs::DynamicType_ptr dyn;
    vbs::TypeObjectFactory* factory = vbs::TypeObjectFactory::get_instance();
    const evbs::ertps::types::TypeInformation* type_information = factory->get_type_information(type_name);
    if (type_information == nullptr) {
        VbsLogW(VbsDecToHex(ReturnCode_t::RETCODE_NO_DATA)
                << " get_dynamic_type_inner type_information is nullptr, type_name: " << type_name);
        return type;
    }
    if (type_information->complete().typeid_with_size().type_id()._d() < evbs::ertps::types::EK_COMPLETE) {
        dyn = factory->build_dynamic_type(type_name, &type_information->complete().typeid_with_size().type_id());
    }
    evbs::ertps::types::TypeObject obj;
    (void)factory->typelookup_get_type(type_information->complete().typeid_with_size().type_id(), obj);
    if (obj._d() != 0U) {
        dyn = factory->build_dynamic_type(type_name, &type_information->complete().typeid_with_size().type_id(), &obj);
    }
    type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dyn);
    return type;
}

bool Topic::get_topic_throughput(NetTrafficInfo (&info)[2], int interval) {
    vbs::NetTrafficInfoImpl send, recv;
    if (!topic_ptr_) {
        return false;
    }
    if (topic_ptr_->get_topic_throughput(send, recv, interval)) {
        info[0].pkg = send.pkg;
        info[0].throughput = send.throughput;
        info[0].timestamp = send.timestamp;
        info[0].direction = static_cast<uint8_t>(send.direction);

        info[1].pkg = recv.pkg;
        info[1].throughput = recv.throughput;
        info[1].timestamp = recv.timestamp;
        info[1].direction = static_cast<uint8_t>(recv.direction);
        return true;
    }
    return false;
}

}  // namespace vbs
