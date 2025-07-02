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

#ifndef DSFDDS_SUBSCRIBER_SAMPLEINFO_HPP_
#define DSFDDS_SUBSCRIBER_SAMPLEINFO_HPP_

#include <ostream>
#include "common/utils/SequenceNumber.h"
#include "common/all_common.h"

namespace li {
namespace dsfdds {
/*!
 * @brief SampleInfo is the information that accompanies each sample that is ‘read’ or ‘taken.’
 */
struct SampleInfo {
    //! time provided by the DataWriter when the sample was written
    Time_t source_timestamp;
    //! time provided by the DataReader when the sample was added to its history
    Time_t reception_timestamp;
    SequenceNumber_t publication_sequence_number;
    SequenceNumber_t reception_sequence_number;
    //! whether the DataSample contains data or is only used to communicate of a change in the instance
    bool valid_data;
    SampleInfo(const Time_t& source_timestamp_, const Time_t& reception_timestamp_,
               const SequenceNumber_t& publication_sequence_number_, const SequenceNumber_t& reception_sequence_number_,
               bool valid_data_)
        : source_timestamp(source_timestamp_),
          reception_timestamp(reception_timestamp_),
          publication_sequence_number(publication_sequence_number_),
          reception_sequence_number(reception_sequence_number_),
          valid_data(valid_data_) {}
    SampleInfo()
        : source_timestamp(Time_t()),
          reception_timestamp(Time_t()),
          publication_sequence_number(),
          reception_sequence_number(),
          valid_data(false) {}
    friend std::ostream& operator<<(std::ostream& os, const SampleInfo& info) {
        std::ios_base::fmtflags original_flags = os.flags();
        os << "source_timestamp: " << info.source_timestamp << " reception_timestamp: " << info.reception_timestamp
           << " publication_sequence_number: " << info.publication_sequence_number.to64long()
           << " reception_sequence_number: " << info.reception_sequence_number.to64long() << std::boolalpha
           << " valid_data: " << info.valid_data;
        os.flags(original_flags);
        return os;
    }
};

}  // namespace dsfdds
}  // namespace li

#endif /* DSFDDS_SUBSCRIBER_SAMPLEINFO_HPP_*/
