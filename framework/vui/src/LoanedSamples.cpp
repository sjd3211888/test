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

#include "vbs/LoanedSamples.hpp"

namespace vbs {

LoanedSampleInfo::LoanedSampleInfo()
    : valid_data_(false),
      payload_len_(0),
      source_timestamp_(0),
      reception_timestamp_(0),
      publication_sequence_number_(0),
      reception_sequence_number_(0),
      e2e_exception_status_() {}

LoanedSampleInfo::LoanedSampleInfo(const LoanedSampleInfo& sample_info)
    : valid_data_(sample_info.valid_data_),
      payload_len_(sample_info.payload_len_),
      source_timestamp_(sample_info.source_timestamp_),
      reception_timestamp_(sample_info.reception_timestamp_),
      publication_sequence_number_(sample_info.publication_sequence_number_),
      reception_sequence_number_(sample_info.reception_sequence_number_),
      e2e_exception_status_(sample_info.e2e_exception_status_) {}

LoanedSampleInfo::LoanedSampleInfo(bool valid, uint32_t len, int64_t stimestamp, int64_t rtimestamp,
                                   uint64_t pub_sequence_number, uint64_t recept_sequence_number,
                                   const E2EExceptionStatus& e2e_status)
    : valid_data_(valid),
      payload_len_(len),
      source_timestamp_(stimestamp),
      reception_timestamp_(rtimestamp),
      publication_sequence_number_(pub_sequence_number),
      reception_sequence_number_(recept_sequence_number),
      e2e_exception_status_(e2e_status) {}

LoanedSampleInfo::~LoanedSampleInfo() {}

bool LoanedSampleInfo::valid_data() {
    return valid_data_;
}

uint32_t LoanedSampleInfo::payload_len() {
    return payload_len_;
}

int64_t LoanedSampleInfo::source_timestamp() {
    return source_timestamp_;
}

int64_t LoanedSampleInfo::reception_timestamp() {
    return reception_timestamp_;
}

uint64_t LoanedSampleInfo::publication_sequence_number() {
    return publication_sequence_number_;
}

uint64_t LoanedSampleInfo::reception_sequence_number() {
    return publication_sequence_number_;
}

E2EExceptionStatus LoanedSampleInfo::e2e_exception_status() {
    return e2e_exception_status_;
}

}  // namespace vbs
