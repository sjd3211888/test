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
// feature: support agent for vbs
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_SUBSCRIBER_SAMPLEINFO_HPP_
#define INCLUDE_EDDS_DDS_SUBSCRIBER_SAMPLEINFO_HPP_

#include "edds/rtps/common/SequenceNumber.h"
#include "edds/rtps/common/SampleIdentity.h"
#include "edds/rtps/common/Time_t.h"
#include "edds/rtps/common/Types.h"
#include "sub/InstanceState.hpp"
#include "sub/SampleState.hpp"
#include "sub/ViewState.hpp"
#include "deps/common/TypeSupport.hpp"
#include "edds/dds/common/InstanceHandle.hpp"
#include "deps/core/status/E2EExceptionStatus.hpp"

namespace vbs {

/*!
 * @brief SampleInfo is the information that accompanies each sample that is ‘read’ or ‘taken.’
 */
struct SampleInfoInner {
    //! indicates whether or not the corresponding data sample has already been read
    vbs::SampleStateKind sample_state;

    //! indicates whether the DataReader has already seen samples for the most-current generation of
    //! the related instance.
    vbs::ViewStateKind view_state;

    //! indicates whether the instance is currently in existence or, if it has been disposed, the
    //! reason why it was disposed.
    vbs::InstanceStateKind instance_state;

    //! number of times the instance had become alive after it was disposed
    int32_t disposed_generation_count;

    //! number of times the instance had become alive after it was disposed because no writers
    int32_t no_writers_generation_count;

    //! number of samples related to the same instance that follow in the collection
    int32_t sample_rank;

    //! the generation difference between the time the sample was received, and the time the most
    //! recent sample in the collection was received.
    int32_t generation_rank;

    //! the generation difference between the time the sample was received, and the time the most
    //! recent sample was received. The most recent sample used for the calculation may or may not
    //! be in the returned collection
    int32_t absolute_generation_rank;

    //! time provided by the DataWriter when the sample was written
    evbs::ertps::rtps::Time_t source_timestamp;

    //! time provided by the DataReader when the sample was added to its history
    evbs::ertps::rtps::Time_t reception_timestamp;

    //! identifies locally the corresponding instance
    vbs::InstanceHandle_t instance_handle;

    //! identifies locally the DataWriter that modified the instance
    //!
    //! Is the same InstanceHandle_t that is returned by the operation get_matched_publications on
    //! the DataReader
    vbs::InstanceHandle_t publication_handle;

    //! whether the DataSample contains data or is only used to communicate of a change in the
    //! instance
    bool valid_data;

    //! Sample Identity (Extension for RPC)
    evbs::ertps::rtps::SampleIdentity sample_identity;

    //! Related Sample Identity (Extension for RPC)
    evbs::ertps::rtps::SampleIdentity related_sample_identity;

    evbs::edds::dds::E2EExceptionStatus e2eStatus;

    // serialization header size(4) + length of user`s serialized data.
    uint32_t payload_len;

    evbs::ertps::rtps::SequenceNumber_t reception_sequence_number;

    uint32_t topic_id;
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_SUBSCRIBER_SAMPLEINFO_HPP_
