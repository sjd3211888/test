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
// feature: Remove redundant code 1. Remove MuxType(the source of data)
// 2. Remove unused member variables in DataWriterImpl/DataReadImpl/DomainParticipantListener
// 3. Remove member variables about shm type discovery 4. Delete some commented code
// ------------------------------------------------------------------

#ifndef VBS_SAMPLEINFO_HPP_
#define VBS_SAMPLEINFO_HPP_

#include <vector>

#include "vbs/Utils.hpp"
#include "vbs/status/E2EExceptionStatus.hpp"

namespace vbs {
class SampleInfoImpl;
class SampleInfo {
 public:
    /**
     * Default constructor for SampleInfo.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    SampleInfo();

    /**
     * Destructor for SampleInfo. It's declared as virtual to ensure correct 
     * behavior when deleting objects through pointers-to-base.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    virtual ~SampleInfo();

    /**
     * Returns the number of data samples currently stored.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return Number of data samples.
     */
    size_t length() const;

    /**
     * Retrieves the length of data before it was serialized.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return Length of the data in its original (unserialized) form.
     */
    uint32_t payload_len(const size_t& i);

    /**
     * Determines if the data at a given index is valid.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return true if data is valid; otherwise, false.
     */
    bool valid_data(const size_t& i);

    /**
     * Clears all data samples currently stored.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    void clear();

    /**
     * Retrieves the timestamp when the data was originally created.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return Timestamp of the data's creation.
     */
    int64_t source_timestamp(const size_t& i);

    /**
     * Retrieves the timestamp when the data was received.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return Timestamp of the data's reception.
     */
    int64_t reception_timestamp(const size_t& i);

    /**
     * Retrieves the sequence number for the data's publication.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return Publication sequence number.
     */
    uint64_t publication_sequence_number(const size_t& i);

    /**
     * Retrieves the sequence number for when the data was received.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return Reception sequence number.
     */
    uint64_t reception_sequence_number(const size_t& i);

    /**
     * Retrieves the e2e exception status for when the data was received.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param i Index of the desired data sample.
     * @return e2e exception status.
     */
    E2EExceptionStatus e2e_exception_status(const size_t& i);

    /**
     * Returns a pointer to the underlying SampleInfoImpl instance.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return Pointer to the SampleInfoImpl instance.
     */
    std::shared_ptr<SampleInfoImpl>& get_instance() { return sample_info_; }

 private:
    std::shared_ptr<SampleInfoImpl> sample_info_;
};

}  // namespace vbs

#endif  // VBS_SAMPLEINFO_HPP_
