/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_SUB_SAMPLEINFOIMPL_HPP_
#define INCLUDE_SUB_SAMPLEINFOIMPL_HPP_

#include <vector>
#include <variant>

#include "core/Global.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/SampleInfoInner.hpp"
#include "core/LoanableSequence.hpp"

namespace evbs {
namespace edds {
namespace dds {
using SampleInfoSeq = vbs::LoanableSequence<vbs::SampleInfoInner>;
}  // namespace dds
}  // namespace edds
}  // namespace evbs

namespace vbs {
class SampleInfoImpl {
    friend class DataReaderImpl;
    friend class LoanableCollectionFlatDataImpl;

 public:
    /**
     * SampleInfoImpl constructor.
     */
    SampleInfoImpl() {}

    /**
     * SampleInfoImpl deconstructor.
     */
    virtual ~SampleInfoImpl() { clear(); }

    /**
     * get the length of data vector
     *
     * @return length
     */
    size_t length() const {
        if (dny_dds_infos_.index() == 0) {
            return 1;
        } else {
            return std::get<1>(dny_dds_infos_).length();
        }
    }

    /**
     * get the data len before serialization.
     *
     * @param i the index of data
     * @return length of plain data.
     */
    uint32_t payload_len(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).payload_len;
        }
        return static_cast<uint32_t>(0);
    }

    /**
     * get the data is valid or not.
     *
     * @param i the index of data
     * @return true is valid, false is invalid
     */
    bool valid_data(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).valid_data;
        }
        return false;
    }

    /**
     * clear data vector
     */
    void clear() {}

    int64_t source_timestamp(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).source_timestamp.to_ns();
        }
        return 0;
    }

    int64_t reception_timestamp(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).reception_timestamp.to_ns();
        }
        return 0;
    }

    uint64_t publication_sequence_number(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).sample_identity.sequence_number().to64long();
        }
        return static_cast<uint64_t>(0);
    }

    uint64_t reception_sequence_number(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).reception_sequence_number.to64long();
        }
        return static_cast<uint64_t>(0);
    }

    uint64_t framework_sequence_number(const size_t& i) {
        if (i < length()) {
            return get_sample_from_index(i).related_sample_identity.sequence_number().to64long();
        }
        return static_cast<uint64_t>(0);
    }

    E2EExceptionStatus e2e_exception_status(const size_t& i) {
        if (i < length()) {
            return E2EExceptionStatus(get_sample_from_index(i).e2eStatus);
        }
        return E2EExceptionStatus();
    }

 private:
    vbs::SampleInfoInner& get_sample_from_index(int index) {
        if (dny_dds_infos_.index() == 0) {
            return std::get<0>(dny_dds_infos_);
        } else {
            return std::get<1>(dny_dds_infos_)[index];
        }
    }

 private:
    std::variant<vbs::SampleInfoInner, evbs::edds::dds::SampleInfoSeq> dny_dds_infos_ =
        evbs::edds::dds::SampleInfoSeq();
};

}  // namespace vbs
#endif  // INCLUDE_SUB_SAMPLEINFOIMPL_HPP_
