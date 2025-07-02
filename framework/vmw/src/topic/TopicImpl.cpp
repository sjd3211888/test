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
// feature: remove reactor
// feature: change name NewusContext to NexusContext
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: VBS framework adapt to system test tool
// feature: separating interface and implementation about Topic
// feature: Support dump topic traffic to disk
// feature: shm dynamic type discovery
// feature: record-replay dsf dynamic discovery with xml
// ------------------------------------------------------------------

#include "ertps/xmlparser/XMLProfileManager.h"
#include "topic/TopicImpl.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "vbs/types/VBSDynamicData.h"
#include "context/NexusContext.hpp"
#include "context/QosManager.hpp"
#include "topic/TopicProxy.hpp"

namespace vbs {

VBSDynamicDataPrinter TopicImpl::printer_;

TopicImpl::TopicImpl(DomainParticipantImpl* const local_participant, const std::string& topic_name,
                     const std::string& type_name, const TypeSupport& local_type, TopicProxy* p,
                     const evbs::edds::dds::StatusMask& mask)
    : DomainEntity(mask),
      TopicDescription(topic_name, type_name),
      type_(local_type),
      dymanic_type_(new VbsFlatDataPubSubType(type_name)),
      participant_(local_participant),
      topic_proxy_(p) {
    if (local_participant == nullptr) {
        return;
    }

    history_send_throughput_.direction = vbs::TRAFFIC_TYPE::SEND_TRAFFIC;
    history_recv_throughput_.direction = vbs::TRAFFIC_TYPE::RECV_TRAFFIC;

    topic_name_ = topic_name;
    data_type_name_ = type_name;
    framework_seq_id_.store(1U);

    if (topic_proxy_ != nullptr) {
        dds_is_initialized_ = true;
        qos_.dds_topic_qos(topic_proxy_->get_qos());
    }
}

TopicImpl::~TopicImpl() {}

bool TopicImpl::GetQos(TopicQosImpl& qos) {
    qos = qos_;
    return true;
}

bool TopicImpl::SetQos(const TopicQosImpl& qos) {
    qos_ = qos;
    return true;
}

const TopicQosImpl& TopicImpl::get_qos_impl() const {
    return qos_;
}

void* TopicImpl::create_data() {
    return type_.create_data();
}

void TopicImpl::delete_data(void* data) {
    return type_.delete_data(data);
}

bool TopicImpl::init_verify() {
    return dds_is_initialized_;
}

bool TopicImpl::get_topic_throughput(NetTrafficInfoImpl& send, NetTrafficInfoImpl& recv, int interval) {
    NetTrafficInfoImpl old_send, old_recv, new_send, new_recv;
    {
        std::lock_guard<std::mutex> lk(send_traffic_mtx_);
        old_send = history_send_throughput_;
    }
    {
        std::lock_guard<std::mutex> lk(recv_traffic_mtx_);
        old_recv = history_recv_throughput_;
    }
    std::this_thread::sleep_for(std::chrono::seconds(interval));
    auto current_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    uint64_t now_in_millis = static_cast<uint64_t>(current_timestamp);
    {
        std::lock_guard<std::mutex> lk(send_traffic_mtx_);
        new_send = history_send_throughput_;
    }
    {
        std::lock_guard<std::mutex> lk(recv_traffic_mtx_);
        new_recv = history_recv_throughput_;
    }
    send.direction = old_send.direction;
    send.pkg = new_send.pkg - old_send.pkg;
    send.throughput = new_send.throughput - old_send.throughput;
    send.timestamp = now_in_millis;
    recv.direction = old_recv.direction;
    recv.pkg = new_recv.pkg - old_recv.pkg;
    recv.throughput = new_recv.throughput - old_recv.throughput;
    recv.timestamp = now_in_millis;
    return true;
}

void TopicImpl::dump_network_traffic(vbs::NetTrafficInfoImpl (&traffic)[2]) {
    NetTrafficInfoImpl old_send, old_recv, new_send, new_recv;
    auto current_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    uint64_t now_in_millis = static_cast<uint64_t>(current_timestamp);
    {
        std::lock_guard<std::mutex> lk(send_traffic_mtx_);
        traffic[0] = history_send_throughput_;
        traffic[0].timestamp = now_in_millis;
    }
    {
        std::lock_guard<std::mutex> lk(recv_traffic_mtx_);
        traffic[1] = history_recv_throughput_;
        traffic[1].timestamp = now_in_millis;
    }
}

void TopicImpl::update_network_traffic(int pgk_size, int pkg_count, TRAFFIC_TYPE type) {
    if (type == vbs::TRAFFIC_TYPE::SEND_TRAFFIC) {
        std::lock_guard<std::mutex> lk(send_traffic_mtx_);
        history_send_throughput_.pkg += pkg_count;
        history_send_throughput_.throughput += pgk_size;
        history_send_throughput_.direction = type;
    } else if (type == vbs::TRAFFIC_TYPE::RECV_TRAFFIC) {
        std::lock_guard<std::mutex> lk(recv_traffic_mtx_);
        history_recv_throughput_.pkg += pkg_count;
        history_recv_throughput_.throughput += pgk_size;
        history_recv_throughput_.direction = type;
    }
}

std::string TopicImpl::to_string(const VBSDynamicData* data, int enum_mode) {
    std::ostringstream oss;
    printer_.print_to_sstream(oss, data, enum_mode);
    return oss.str();
}

std::ostream& TopicImpl::to_string(std::ostream& out, const VBSDynamicData* data, int enum_mode) {
    std::ostringstream oss;
    printer_.print_to_sstream(oss, data, enum_mode);
    out << oss.str();
    return out;
}

DomainParticipantImpl* TopicImpl::get_participant() const {
    return participant_;
}

vbs::TopicDescriptionImpl* TopicImpl::get_impl() const {
    return topic_proxy_;
}

const vbsutil::xmlparser::TopicQos& TopicImpl::get_qos() const {
    return topic_proxy_->get_qos();
}

ReturnCode_t TopicImpl::get_qos(vbsutil::xmlparser::TopicQos& qos) const {
    qos = topic_proxy_->get_qos();
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

ReturnCode_t TopicImpl::set_qos(const vbsutil::xmlparser::TopicQos& qos) {
    return ReturnCode_t(topic_proxy_->set_qos(qos)());
}

const vbs::TopicListener* TopicImpl::get_listener() const {
    return topic_proxy_->get_listener();
}

ReturnCode_t TopicImpl::set_listener(vbs::TopicListener* listener, const evbs::edds::dds::StatusMask& mask) {
    topic_proxy_->set_listener(listener, mask);
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

}  // namespace vbs
