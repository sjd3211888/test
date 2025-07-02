// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: support e2e deadline and liveliness 1. config deadline sendPeriod 2. enable deadline event
// feature: remove dependency about old ZerocopyDataType and membuf in fwk.
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API
// 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: Reduce level of DomainParticipantListener
// feature: Optimize big buffer zero copy
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: separating interface and implementation about DataWriter
// feature: use Listeners and NexusUnits as singleton
// feature: add VBSFramework exception code
// feature: judging this pointer
// feature: VBSFramework log collection
// feature: VBSDynamicData interface constness support
// feature: VBS framework add performance test
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// ------------------------------------------------------------------

#include "pub/DataWriterImpl.hpp"
#include "vbs/DataWriter.hpp"
#include "vbs/DataWriterListener.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "context/NexusUnits.hpp"
#include "context/QosManager.hpp"

namespace vbs {

DataWriter::DataWriter(DomainParticipant* const local_participant, Topic* const local_topic,
                       const std::string& profile_name, DataWriterListener* const listener)
    : DataWriter(local_participant, local_topic, profile_name, listener, StatusMask::all()) {}

DataWriter::DataWriter(DomainParticipant* const local_participant, Topic* const local_topic,
                       const std::string& profile_name, DataWriterListener* const listener, const StatusMask& mask)
    : writer_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataWriter local_participant or local_topic is nullptr, profile_name = " << profile_name);
        return;
    }
    writer_ptr_ = new vbs::DataWriterImpl(local_participant->get_instance(), local_topic->get_instance(), profile_name,
                                          listener, mask);
    writer_ptr_->user_datawriter_ = this;
    if (listener != nullptr) {
        writer_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataWriter(local_participant->get_instance(), writer_ptr_);
    QosManager::Instance()->add_writer(writer_ptr_->get_guid(), writer_ptr_);
}

DataWriter::DataWriter(DomainParticipant* const local_participant, Topic* const local_topic,
                       const DataWriterQos& writer_qos, DataWriterListener* const listener)
    : DataWriter(local_participant, local_topic, writer_qos, listener, StatusMask::all()) {}

DataWriter::DataWriter(DomainParticipant* const local_participant, Topic* const local_topic,
                       const DataWriterQos& writer_qos, DataWriterListener* const listener, const StatusMask& mask)
    : writer_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataWriter local_participant or local_topic is nullptr");
        return;
    }
    writer_ptr_ = new vbs::DataWriterImpl(local_participant->get_instance(), local_topic->get_instance(),
                                          *writer_qos.get_instance(), listener, mask);
    writer_ptr_->user_datawriter_ = this;
    if (listener != nullptr) {
        writer_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataWriter(local_participant->get_instance(), writer_ptr_);
    QosManager::Instance()->add_writer(writer_ptr_->get_guid(), writer_ptr_);
}

DataWriter::~DataWriter() {
    if (writer_ptr_) {
        QosManager::Instance()->remove_writer(writer_ptr_->get_guid());
        NexusUnits::get_unit().DeleteDataWriter(writer_ptr_->get_paritcipantimpl(), writer_ptr_);
        writer_ptr_ = nullptr;
    }
}

void DataWriter::set_listener(DataWriterListener* const listener) {
    set_listener(listener, StatusMask::all());
}

void DataWriter::set_listener(DataWriterListener* const listener, const StatusMask& mask) {
    if (writer_ptr_) {
        writer_ptr_->set_user_listener(listener, mask);
        if (listener != nullptr) {
            writer_ptr_->enable();
        }
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " writer_ptr_ unexpected nullptr");
    }
}

bool DataWriter::write(VBSDynamicData* data) {
    const auto* const_data = data;
    return write(const_data);
}

bool DataWriter::write(const VBSDynamicData* data) {
    if ((data == nullptr) || (writer_ptr_ == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " writer_ptr_ or data unexpected nullptr");
        return false;
    }

    bool sample = writer_ptr_->write(data, data->get_size());
    return sample;
}

bool DataWriter::write_buffer(void* const data, const uint32_t& len) {
    if ((data == nullptr) && (writer_ptr_ == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " writer_ptr_ or data unexpected nullptr");
        return false;
    }

    bool sample = writer_ptr_->write_buffer(data, len);
    return sample;
}

bool DataWriter::loan_sample(void*& sample) {
    if (writer_ptr_) {
        return writer_ptr_->loan_sample(sample) == ReturnCode_t::RETCODE_OK;
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " loan sample writer_ptr_ unexpected nullptr");
        return false;
    }
}

bool DataWriter::loan_sample(void*& sample, uint32_t size) {
    if (writer_ptr_) {
        return writer_ptr_->loan_sample(sample, size) == ReturnCode_t::RETCODE_OK;
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " loan sample writer_ptr_ unexpected nullptr");
        return false;
    }
}

bool DataWriter::discard_loan(void*& sample) {
    if (writer_ptr_) {
        return writer_ptr_->discard_loan(sample) == ReturnCode_t::RETCODE_OK;
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " discard loan writer_ptr_ unexpected nullptr");
        return false;
    }
}

bool DataWriter::assert_liveliness() {
    if (writer_ptr_) {
        return writer_ptr_->assert_liveliness();
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " assert liveliness writer_ptr_ unexpected nullptr");
        return false;
    }
}

bool DataWriter::serialize(void* const data, void* const dest_buffer, uint32_t* const dest_len) {
    if ((data == nullptr) || (dest_buffer == nullptr) || (dest_len == nullptr) || (writer_ptr_ == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " data/dest_buffer/dest_len/writer_ptr_ unexpected nullptr");
        return false;
    }
    return writer_ptr_->serialize(data, dest_buffer, dest_len);
}

const DataWriterQos DataWriter::get_qos() const {
    DataWriterQos writer_qos;
    if (writer_ptr_) {
        writer_qos.set_instance(std::make_shared<vbs::DataWriterQosImpl>(writer_ptr_->get_qos()));
        return writer_qos;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " get qos writer_ptr_ unexpected nullptr");
    return writer_qos;
}

bool DataWriter::init_verify() {
    if (writer_ptr_) {
        return writer_ptr_->init_verify();
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " writer_ptr_ unexpected nullptr");
    return false;
}

bool DataWriter::write(void* const data, const uint32_t& len) {
    //std::cout << "DataWriter::write(void* const data, const uint32_t& len) " << len << " " << data << std::endl;
    if ((data != nullptr) && (writer_ptr_ != nullptr)) {

        bool sample = writer_ptr_->write(data, len);
        return sample;
    }

    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " writer_ptr_ or data unexpected nullptr");
    return false;
}
}  // namespace vbs
