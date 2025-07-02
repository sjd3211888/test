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
// feature: Code style modification
// ------------------------------------------------------------------

#include <vector>

#include "sub/SampleInfoImpl.hpp"
#include "vbs/SampleInfo.hpp"

namespace vbs {

SampleInfo::SampleInfo() : sample_info_(new SampleInfoImpl()) {}

SampleInfo::~SampleInfo() {
    if (sample_info_) {
        // delete sample_info_;
        sample_info_ = nullptr;
    }
}

size_t SampleInfo::length() const {
    if (sample_info_) {
        return sample_info_->length();
    }
    return 0;
}

uint32_t SampleInfo::payload_len(const size_t& i) {
    if (sample_info_) {
        return sample_info_->payload_len(i);
    }
    return 0;
}

bool SampleInfo::valid_data(const size_t& i) {
    if (sample_info_) {
        return sample_info_->valid_data(i);
    }
    return false;
}

void SampleInfo::clear() {
    if (sample_info_) {
        return sample_info_->clear();
    }
}

int64_t SampleInfo::source_timestamp(const size_t& i) {
    if (sample_info_) {
        return sample_info_->source_timestamp(i);
    }
    return 0;
}

int64_t SampleInfo::reception_timestamp(const size_t& i) {
    if (sample_info_) {
        return sample_info_->reception_timestamp(i);
    }
    return 0;
}

uint64_t SampleInfo::publication_sequence_number(const size_t& i) {
    if (sample_info_) {
        return sample_info_->publication_sequence_number(i);
    }
    return 0;
}

uint64_t SampleInfo::reception_sequence_number(const size_t& i) {
    if (sample_info_) {
        return sample_info_->reception_sequence_number(i);
    }
    return 0;
}

E2EExceptionStatus SampleInfo::e2e_exception_status(const size_t& i) {
    if (sample_info_) {
        return sample_info_->e2e_exception_status(i);
    }
    return E2EExceptionStatus();
}

}  // namespace vbs
