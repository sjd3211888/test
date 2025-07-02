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
// feature: remove dependency about old ZerocopyDataType and membuf in fwk.
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: Reduce level of DataReaderListener
// feature: implement ContentFilteredTopic
// feature: FWK adjusts timestamp tracking 1. FWK adjusts timestamp tracking and increases packetsize tracking
// feature: VBS framework adapt to system test tool
// feature: private constructor
// feature: set get_qos is private
// feature: separating interface and implementation about DataReader
// feature: use Listeners and NexusUnits as singleton
// feature: Support DataReader get SampleInfo through take_next_sample()
// feature: Encapsulation LoanableCollection and Samplelnfo
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// feature: add api take_next_sample
// ------------------------------------------------------------------

#ifndef VBS_DATAREADER_HPP_
#define VBS_DATAREADER_HPP_

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include "vbs/Global.hpp"
#include "vbs/Topic.hpp"
#include "vbs/DataReaderQos.hpp"
#include "vbs/DomainParticipant.hpp"
#include "vbs/DataReaderListener.hpp"
#include "vbs/status/StatusMask.hpp"
#include "vbs/status/SubscriptionMatchedStatus.hpp"
#include "vbs/LoanableCollection.hpp"
#include "vbs/SampleInfo.hpp"
#include "vbs/LoanedSamples.hpp"
#include "vbs/types/VBSDynamicData.h"

namespace vbs {
class DataReaderListener;
class DomainParticipant;
class Topic;
class ContentFilteredTopic;
class DomainParticipantImpl;
class DataReaderImpl;
class NexusUnits;
class VBSDynamicData;
class DataReader {
    friend class DomainParticipant;

 public:
    /**
     * @brief Destructor for the DataReader class.
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * This destructor cleans up and releases the resources held by the DataReader object.
     * It safely deallocates the memory assigned to the underlying reader implementation
     * and the associated listener, setting their pointers to nullptr to prevent dangling pointer access.
     */
    virtual ~DataReader();

    /**
     * take datas from reader if it has.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param datas vbs::LoanableCollectionImpl pointer. T is a type of reading data, data in it.
     * @param infos vbs::SampleInfo pointer. It has some infomation that whether the data valid and data's length.
     * @return vbs::ReturnCode_t, success is RETCODE_OK, failed is RETCODE_ERROR
     */
    template <typename T, typename _NonConstEnabler = std::true_type>
    ReturnCode_t take(LoanableCollection<T, _NonConstEnabler>* datas, SampleInfo* infos) {
        if (this->take_flatdata(datas->get_flat_data()->get_instance(), infos->get_instance()) !=
            ReturnCode_t(ReturnCode_t::RETCODE_OK)) {
            return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
        }

        std::shared_ptr<LoanableCollectionFlatDataImpl> samples = datas->get_flat_data()->get_instance();
        std::shared_ptr<SampleInfoImpl> samples_infos = infos->get_instance();
        for (size_t i = 0; i < datas->get_flat_data()->length(); i++) {
            T* raw_pointer = static_cast<T*>((*(datas->get_flat_data()))[i]);
            std::shared_ptr<T> pData = std::shared_ptr<T>(raw_pointer, [samples, samples_infos](T*) {
                (void)samples;
                (void)samples_infos;
            });
            datas->dds_data_.push_back(std::move(pData));
        }
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }

    template <typename T, typename _NonConstEnabler = std::true_type>
    ReturnCode_t take(LoanedSamples<T, _NonConstEnabler>* datas) {
        datas->is_flatdata_ = false;
        return take(datas->datas_.get(), datas->infos_.get());
    }

    /**
     * take datas from reader if it has.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param datas LoanableCollectionPtr pointer.
     * @param infos SampleInfo pointer. It has some infomation that whether the data valid and data's length.
     * @return ReturnCode_t, success is RETCODE_OK, failed is others
     * @note only used for take dynamic type msg with medadata.
     */
    template <typename _NonConstEnabler = std::true_type>
    ReturnCode_t take_buffer(LoanableCollectionPtr<void*, _NonConstEnabler>* datas, SampleInfo* infos) {
        if (this->take_buffer(datas->get_flat_data()->get_instance(), infos->get_instance()) !=
            ReturnCode_t(ReturnCode_t::RETCODE_OK)) {
            return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
        }
        for (size_t i = 0; i < datas->get_flat_data()->length(); i++) {
            datas->data_.push_back((*(datas->get_flat_data()))[i]);
        }
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }

    template <typename _NonConstEnabler = std::true_type>
    ReturnCode_t take_buffer(LoanedSamples<void*, _NonConstEnabler>* datas) {
        datas->is_flatdata_ = true;
        return take_buffer(datas->flat_datas_.get(), datas->infos_.get());
    }

    /**
     * take next sample data.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param data void pointer.
     * @return vbs::ReturnCode_t, success is RETCODE_OK, failed is others
     */
    ReturnCode_t take_next_sample(void* data);

    /**
     * take next sample data.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param data void pointer.
     * @param info VBSDynamicData SampleInfo.
     * @return vbs::ReturnCode_t, success is RETCODE_OK, failed is others
     */
    ReturnCode_t take_next_sample(void* data, vbs::SampleInfo& info);

    /**
     * take next sample data.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param data void pointer.
     * @return vbs::ReturnCode_t, success is RETCODE_OK, failed is others
     */
    ReturnCode_t take_next_sample(vbs::VBSDynamicData* data);

    /**
     * take next sample data.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param data VBSDynamicData pointer.
     * @param info VBSDynamicData SampleInfo.
     * @return vbs::ReturnCode_t, success is RETCODE_OK, failed is others
     */
    ReturnCode_t take_next_sample(vbs::VBSDynamicData* data, vbs::SampleInfo& info);

    /**
     * set DataReaderListener.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param listener DataReaderListener
     * mask     StatusMask Default is StatusMask::all(), indicating all statuses.
     */
    void set_listener(DataReaderListener* const listener);

    /**
     * set DataReaderListener.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param listener DataReaderListener
     * @param mask     StatusMask
     */
    void set_listener(DataReaderListener* const listener, const vbs::StatusMask& mask);

    /**
     * deserialize data from byte buffer to struct.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param payload_buffer byte buffer.
     * @param payload_len length of byte buffer.
     * @param data Pointer to the struct data
     * @return ReturnCode_t, true if success.
     */
    bool deserialize(void* payload_buffer, const uint32_t payload_len, void* data);

    /**
     * Retrieve the instance of the DataReader.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This member function provides access to the internal DataReader instance represented by the reader_ptr_ member.
     *
     * @return vbs::DataReader* Pointer to the internal DataReader instance. 
     */
    vbs::DataReaderImpl* get_instance() { return reader_ptr_; }

    /**
     * @brief Check if initialization was successful.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This function verifies whether the previous initialization process
     * completed successfully. It should be called after the initialization
     * routine to ensure that the system or object is ready for use.
     *
     * @return true if the initialization was successful, false otherwise.
     */
    bool init_verify();

 private:
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * mask Status mask for the data reader. Default is StatusMask::all(), indicating all statuses.
     */
    DataReader(DomainParticipant* const local_participant, Topic* const local_topic, const std::string& profile_name,
               DataReaderListener* const listener);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    DataReader(DomainParticipant* const local_participant, Topic* const local_topic, const std::string& profile_name,
               DataReaderListener* const listener, const vbs::StatusMask& mask);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * mask Status mask for the data reader. Default is StatusMask::all(), indicating all statuses.
     */
    DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
               const std::string& profile_name, DataReaderListener* const listener);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param profile_name datareader profile name.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
               const std::string& profile_name, DataReaderListener* const listener, const vbs::StatusMask& mask);

    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param reader_qos Qos for the DataReader.
     * @param listener linstener pointer.
     * mask Status mask for the data reader. Default is StatusMask::all(), indicating all statuses.
     */
    DataReader(DomainParticipant* const local_participant, Topic* const local_topic, const DataReaderQos& reader_qos,
               DataReaderListener* const listener);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param reader_qos Qos for the DataReader.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    DataReader(DomainParticipant* const local_participant, Topic* const local_topic, const DataReaderQos& reader_qos,
               DataReaderListener* const listener, const vbs::StatusMask& mask);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param reader_qos Qos for the DataReader.
     * @param listener linstener pointer.
     * mask Status mask for the data reader. Default is StatusMask::all(), indicating all statuses.
     */
    DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
               const DataReaderQos& reader_qos, DataReaderListener* const listener);
    /**
     * DataReader static constructor.
     * 
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @param local_participant Domain Id.
     * @param local_topic reader_topic pointer.
     * @param reader_qos Qos for the DataReader.
     * @param listener linstener pointer.
     * @param mask StatusMask.
     */
    DataReader(DomainParticipant* const local_participant, ContentFilteredTopic* const local_topic,
               const DataReaderQos& reader_qos, DataReaderListener* const listener, const vbs::StatusMask& mask);

    /**
     * @brief Getter for the DataReaderQos.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Pointer to the DataReaderQos.
     */
    const DataReaderQos get_qos() const;

    ReturnCode_t take_flatdata(std::shared_ptr<LoanableCollectionFlatDataImpl>& datas,
                               std::shared_ptr<SampleInfoImpl>& infos);

    ReturnCode_t take_buffer(std::shared_ptr<LoanableCollectionFlatDataImpl>& datas,
                             std::shared_ptr<SampleInfoImpl>& infos);

    TypeSupport get_type();

    void set_data_reader_listener(DataReaderListener* listener);

    vbs::DataReaderImpl* reader_ptr_;
};

}  // namespace vbs

#endif  // VBS_DATAREADER_HPP_
