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
// feature: Reduce level of DomainParticipantListener
// feature: private constructor
// feature: set get_qos is private
// feature: separating interface and implementation about DataWriter
// feature: use Listeners and NexusUnits as singleton
// feature: VBSDynamicData interface constness support
// feature: VBS framework add performance test
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// ------------------------------------------------------------------

#ifndef VBS_DATAWRITER_HPP_
#define VBS_DATAWRITER_HPP_

#include <map>
#include <vector>
#include <memory>
#include <string>

#include "vbs/Global.hpp"
#include "vbs/Topic.hpp"
#include "vbs/DataWriterQos.hpp"
#include "vbs/DomainParticipant.hpp"
#include "vbs/status/StatusMask.hpp"
#include "vbs/types/VBSDynamicData.h"

namespace vbs {

class DataWriterListener;
class DomainParticipant;
class Topic;
class DomainParticipantImpl;
class DataWriterImpl;
class NexusUnits;
class VBSDynamicData;
class DataWriter {
    friend class DomainParticipant;

 public:
    /**
     * DataWriter deconstructor.
     */
    virtual ~DataWriter();

    /**
     * Write data to the topic.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param data Pointer to the data
     * @return True if correct, false otherwise
     */
    template <typename T>
    // coverity[definition_tu:SUPPRESS]
    bool write(T* const data) {
        return write(static_cast<void*>(data), 0U);
    }

    /**
     * set DataWriterListener.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * @param listener DataWriterListener
     * mask StatusMask Default is StatusMask::all(), indicating all statuses.
     */
    void set_listener(DataWriterListener* const listener);

    /**
     * set DataWriterListener.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param listener DataWriterListener
     * @param mask StatusMask
     */
    void set_listener(DataWriterListener* const listener, const vbs::StatusMask& mask);

    /**
     * Write dynamic type data.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param data data
     * @return True if correct, false otherwise
     * @note only used for dynamic type data.
     */
    bool write(VBSDynamicData* data);

    /**
     * Write dynamic type data.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param data data
     * @return True if correct, false otherwise
     * @note only used for dynamic type data.
     */
    bool write(const VBSDynamicData* data);

    /**
     * Write data to the topic.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param data serialized data or no-deserialized data.
     * @param len length of data.
     * @return True if correct, false otherwise
     * @see take_buffer
     */
    bool write_buffer(void* const data, const uint32_t& len);

    /**
     * @brief Get a pointer to the internal pool where the user could directly write.
     *
     * This method can only be used on a DataWriter for a plain data type. It will provide the
     * user with a pointer to an internal buffer where the data type can be prepared for sending.
     *
     * Once the sample has been prepared, it can then be published by calling @ref write.
     * After a successful call to @ref write, the middleware takes ownership of the loaned pointer
     * again, and the user should not access that memory again.
     *
     * If, for whatever reason, the sample is not published, the loan can be returned by calling
     * @ref discard_loan.
     *
     * @param [out] sample          Pointer to the sample on the internal pool.
     *
     * @return True if correct, false otherwise
     */
    bool loan_sample(void*& sample);

    /**
     * @brief Get a pointer to the internal pool where the user could directly write.
     *
     * This method can be used on a DataWriter for a data type which is not plain(if size is not zero). It will provide the
     * user with a pointer to an internal buffer where the data type can be prepared for sending.
     *
     * Once the sample has been prepared, it can then be published by calling @ref write.
     * After a successful call to @ref write, the middleware takes ownership of the loaned pointer
     * again, and the user should not access that memory again.
     *
     * If, for whatever reason, the sample is not published, the loan can be returned by calling
     * @ref discard_loan.
     *
     * @param [out] sample          Pointer to the sample on the internal pool.
     * @param [in]  size            size of sample, if equals 0, use type`size
     *
     * @return True if correct, false otherwise
     */
    bool loan_sample(void*& sample, uint32_t size);

    /**
     * @brief Discards a loaned sample pointer.
     *
     * See the description on @ref loan_sample for how and when to call this method.
     *
     * @param [in,out] sample  Pointer to the previously loaned sample.
     *
     * @return True if correct, false otherwise
     */
    bool discard_loan(void*& sample);

    /**
     * @brief This operation manually asserts the liveliness of the DataWriter. This is used in
     * combination with the LivelinessQosPolicy to indicate to the Service that the entity remains
     * active. This operation need only be used if the LIVELINESS setting is either
     * MANUAL_BY_PARTICIPANT or MANUAL_BY_TOPIC. Otherwise, it has no effect.
     *
     * @note Writing data via the write operation on a DataWriter asserts liveliness on the
     * DataWriter itself and its DomainParticipant. Consequently the use of assert_liveliness is
     * only needed if the application is not writing data regularly.
     *
     * @return True if asserted, false otherwise
     */
    bool assert_liveliness();

    /**
     * serialize data to raw buffer.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param data [IN] user data to be serialized.
     * @param dest_buffer [IN/OUT] the pointer to serialized data.
     * @param dest_len [IN/OUT] input the max length of payload buffer, output length of serialized data.
     * @return True if correct, false otherwise
     */
    bool serialize(void* const data, void* const dest_buffer, uint32_t* const dest_len);

    /**
     * Retrieve the instance of the DataWriter.
     *
     * @note @li Thread-Safe: Yes
     *
     * This member function provides access to the internal DataWriter instance represented by the writer_ptr_ member.
     *
     * @return vbs::DataWriter* Pointer to the internal DataWriter instance. 
     */
    vbs::DataWriterImpl* get_instance() { return writer_ptr_; }

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
     * DataWriter constructor.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * Constructs a DataWriter with the specified participant, topic, profile name, listener, and status mask.
     *
     * @param local_participant Pointer to the associated DomainParticipant.
     * @param local_topic Pointer to the associated topic for this writer.
     * @param profile_name Name of the profile for this data writer.
     * @param listener Pointer to the associated DataWriterListener. Default is nullptr, indicating no listener.
     * mask Status mask for the data writer. Default is StatusMask::all(), indicating all statuses.
     */
    DataWriter(DomainParticipant* const local_participant, Topic* const local_topic, const std::string& profile_name,
               DataWriterListener* const listener = nullptr);

    /**
     * DataWriter constructor.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * Constructs a DataWriter with the specified participant, topic, profile name, listener, and status mask.
     *
     * @param local_participant Pointer to the associated DomainParticipant.
     * @param local_topic Pointer to the associated topic for this writer.
     * @param profile_name Name of the profile for this data writer.
     * @param listener Pointer to the associated DataWriterListener. Default is nullptr, indicating no listener.
     * @param mask Status mask for the data writer. Default is StatusMask::all(), indicating all statuses.
     */
    DataWriter(DomainParticipant* const local_participant, Topic* const local_topic, const std::string& profile_name,
               DataWriterListener* const listener, const vbs::StatusMask& mask);

    /**
     * DataWriter constructor.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * Constructs a DataWriter with the specified participant, topic, profile name, listener, and status mask.
     *
     * @param local_participant Pointer to the associated DomainParticipant.
     * @param local_topic Pointer to the associated topic for this writer.
     * @param writer_qos Qos for the DataWriter.
     * @param listener Pointer to the associated DataWriterListener. Default is nullptr, indicating no listener.
     * mask Status mask for the data writer. Default is StatusMask::all(), indicating all statuses.
     */
    DataWriter(DomainParticipant* const local_participant, Topic* const local_topic, const DataWriterQos& writer_qos,
               DataWriterListener* const listener = nullptr);

    /**
     * DataWriter constructor.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     *
     * Constructs a DataWriter with the specified participant, topic, profile name, listener, and status mask.
     *
     * @param local_participant Pointer to the associated DomainParticipant.
     * @param local_topic Pointer to the associated topic for this writer.
     * @param writer_qos Qos for the DataWriter.
     * @param listener Pointer to the associated DataWriterListener. Default is nullptr, indicating no listener.
     * @param mask Status mask for the data writer. Default is StatusMask::all(), indicating all statuses.
     */
    DataWriter(DomainParticipant* const local_participant, Topic* const local_topic, const DataWriterQos& writer_qos,
               DataWriterListener* const listener, const vbs::StatusMask& mask);

    /**
     * Retrieves the DataWriterQos for this DataWriter.
     *
     * @note @li Thread-Safe: Yes
     *
     * @return Reference to the current DataWriterQos
     */
    const DataWriterQos get_qos() const;

    /**
     * Write DynamicData_ptr data to the topic.
     *
     * @param data pointer to the DynamicData
     * @param len length of data
     * @return True if correct, false otherwise
     * @note only used for writer dynamic type with medadata.
     */
    bool write(void* const data, const uint32_t& len);

    vbs::DataWriterImpl* writer_ptr_;
};

}  // namespace vbs

#endif  // VBS_DATAWRITER_HPP_
