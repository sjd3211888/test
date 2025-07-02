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
// feature: remove dependency about old ZerocopyDataType and membuf in fwk.
// feature: framework fit QosManager 1. create topic、reader、writer with profile use QosManager API
// 2. QosManager add clear xml file API 3. UT fit QosManager
// feature: Reduce level of DomainParticipantListener
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// feature: Optimize big buffer zero copy
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: separating interface and implementation about DataReader
// feature: use Listeners and NexusUnits as singleton
// feature: add VBSFramework exception code
// feature: judging this pointer
// feature: VBSFramework log collection
// feature: VBS framework add performance test
// feature: Support DataReader get SampleInfo through take_next_sample()
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// ------------------------------------------------------------------

#include "sub/DataReaderImpl.hpp"
#include "vbs/DataReader.hpp"
#include "vbs/ContentFilteredTopic.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "context/NexusUnits.hpp"
#include "context/QosManager.hpp"

namespace vbs {

DataReader::DataReader(DomainParticipant* const local_participant, Topic* const local_topic,
                       const std::string& profile_name, DataReaderListener* const listener)
    : DataReader(local_participant, local_topic, profile_name, listener, StatusMask::all()) {}

DataReader::DataReader(DomainParticipant* const local_participant, Topic* const local_topic,
                       const std::string& profile_name, DataReaderListener* const listener, const StatusMask& mask)
    : reader_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataReader local_participant or local_topic unexpected nullptr " << profile_name);
        return;
    }
    reader_ptr_ = new vbs::DataReaderImpl(local_participant->get_instance(), local_topic->get_instance(), profile_name,
                                          listener, mask);
    reader_ptr_->user_datareader_ = this;
    if (listener != nullptr) {
        reader_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataReader(local_participant->get_instance(), reader_ptr_);
    QosManager::Instance()->add_reader(reader_ptr_->get_guid(), reader_ptr_);
}

DataReader::DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
                       const std::string& profile_name, DataReaderListener* const listener)
    : DataReader(local_participant, local_topic, profile_name, listener, StatusMask::all()) {}

DataReader::DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
                       const std::string& profile_name, DataReaderListener* const listener, const StatusMask& mask)
    : reader_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataReader local_participant or local_topic unexpected nullptr " << profile_name);
        return;
    }
    reader_ptr_ = new vbs::DataReaderImpl(local_participant->get_instance(), local_topic->get_instance(), profile_name,
                                          listener, mask);
    reader_ptr_->user_datareader_ = this;
    if (listener != nullptr) {
        reader_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataReader(local_participant->get_instance(), reader_ptr_);
    QosManager::Instance()->add_reader(reader_ptr_->get_guid(), reader_ptr_);
}

DataReader::DataReader(DomainParticipant* const local_participant, Topic* const local_topic,
                       const DataReaderQos& reader_qos, DataReaderListener* const listener)
    : DataReader(local_participant, local_topic, reader_qos, listener, StatusMask::all()) {}

DataReader::DataReader(DomainParticipant* const local_participant, Topic* const local_topic,
                       const DataReaderQos& reader_qos, DataReaderListener* const listener, const StatusMask& mask)
    : reader_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataReader local_participant or local_topic unexpected nullptr ");
        return;
    }
    reader_ptr_ = new vbs::DataReaderImpl(local_participant->get_instance(), local_topic->get_instance(),
                                          *reader_qos.get_instance(), listener, mask);
    reader_ptr_->user_datareader_ = this;
    if (listener != nullptr) {
        reader_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataReader(local_participant->get_instance(), reader_ptr_);
    QosManager::Instance()->add_reader(reader_ptr_->get_guid(), reader_ptr_);
}

DataReader::DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
                       const DataReaderQos& reader_qos, DataReaderListener* const listener)
    : DataReader(local_participant, local_topic, reader_qos, listener, StatusMask::all()) {}

DataReader::DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
                       const DataReaderQos& reader_qos, DataReaderListener* const listener, const StatusMask& mask)
    : reader_ptr_(nullptr) {

    if ((local_participant == nullptr) || (local_topic == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " DataReader local_participant or local_topic unexpected nullptr ");
        return;
    }
    reader_ptr_ = new vbs::DataReaderImpl(local_participant->get_instance(), local_topic->get_instance(),
                                          *reader_qos.get_instance(), listener, mask);
    reader_ptr_->user_datareader_ = this;
    if (listener != nullptr) {
        reader_ptr_->enable();
    }
    NexusUnits::get_unit().InsertDataReader(local_participant->get_instance(), reader_ptr_);
    QosManager::Instance()->add_reader(reader_ptr_->get_guid(), reader_ptr_);
}

DataReader::~DataReader() {
    if (reader_ptr_) {
        QosManager::Instance()->remove_reader(reader_ptr_->get_guid());
        NexusUnits::get_unit().DeleteDataReader(reader_ptr_->reader_participant_, reader_ptr_);
        reader_ptr_ = nullptr;
    }
}

ReturnCode_t DataReader::take_next_sample(void* data) {
    if ((reader_ptr_ == nullptr) || (data == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " reader_ptr_ or data unexpected nullptr");
        return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
    }

    return reader_ptr_->take_next_sample(data);
}

ReturnCode_t DataReader::take_next_sample(void* data, vbs::SampleInfo& info) {
    if ((reader_ptr_ == nullptr) || (data == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " reader_ptr_ or data unexpected nullptr");
        return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
    }

    return reader_ptr_->take_next_sample(data, info.get_instance().get());
}

ReturnCode_t DataReader::take_next_sample(vbs::VBSDynamicData* data) {
    if ((reader_ptr_ == nullptr) || (data == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "reader_ptr_ or data unexpected nullptr");
        return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
    }

    return reader_ptr_->take_next_sample(data);
}

ReturnCode_t DataReader::take_next_sample(vbs::VBSDynamicData* data, vbs::SampleInfo& info) {
    if ((reader_ptr_ == nullptr) || (data == nullptr)) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "reader_ptr_ or data unexpected nullptr");
        return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
    }

    return reader_ptr_->take_next_sample(data, info.get_instance().get());
}

void DataReader::set_listener(DataReaderListener* const listener) {
    set_listener(listener, StatusMask::all());
}

void DataReader::set_listener(DataReaderListener* const listener, const StatusMask& mask) {
    if (reader_ptr_) {
        {
#if !defined(_WIN32)
            const std::lock_guard<std::shared_mutex> lock(reader_ptr_->reader_mutex_);
#else
            const std::lock_guard<std::mutex> lock(reader_ptr_->reader_mutex_);
#endif
            reader_ptr_->set_user_listener(listener, mask);
        }
        if (listener != nullptr) {
            reader_ptr_->enable();
        }
    } else {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "reader_ptr_ unexpected nullptr");
    }
}

const DataReaderQos DataReader::get_qos() const {
    DataReaderQos reader_qos;
    if (reader_ptr_) {
        reader_qos.set_instance(std::make_shared<vbs::DataReaderQosImpl>(reader_ptr_->get_qos()));
        return reader_qos;
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "get qos reader_ptr_ unexpected nullptr");
    return reader_qos;
}

bool DataReader::init_verify() {
    if (reader_ptr_) {
        return reader_ptr_->init_verify();
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "reader_ptr_ unexpected nullptr");
    return false;
}

bool DataReader::deserialize(void* payload_buffer, const uint32_t payload_len, void* data) {
    if ((payload_buffer != nullptr) && (data != nullptr) && (reader_ptr_ != nullptr)) {
        return reader_ptr_->deserialize(payload_buffer, payload_len, data);
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << " payload_buffer/data/reader_ptr_ unexpected nullptr");
    return false;
}

ReturnCode_t DataReader::take_flatdata(std::shared_ptr<LoanableCollectionFlatDataImpl>& datas,
                                       std::shared_ptr<SampleInfoImpl>& infos) {
    if (reader_ptr_) {
        return reader_ptr_->take_flatdata(datas, infos);
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "take flatdata reader_ptr_ unexpected nullptr");
    return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
}

ReturnCode_t DataReader::take_buffer(std::shared_ptr<LoanableCollectionFlatDataImpl>& datas,
                                     std::shared_ptr<SampleInfoImpl>& infos) {
    if (reader_ptr_) {
        return reader_ptr_->take_buffer(datas, infos);
    }
    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER) << "take buffer reader_ptr_ unexpected nullptr");
    return ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
}

TypeSupport DataReader::get_type() {
    if (reader_ptr_) {
        return reader_ptr_->type_;
    }
    return TypeSupport(nullptr);
}

void DataReader::set_data_reader_listener(DataReaderListener* listener) {
    if (reader_ptr_ != nullptr) {
        reader_ptr_->set_user_listener(listener);
    }
}

}  // namespace vbs
