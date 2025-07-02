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

#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/LoanedDynamicData.hpp"

namespace vbs {

LoanedDynamicData::LoanedDynamicData(LoanedDynamicData&& other) {
    if (this != &other) {
        parent_data_ = other.parent_data_;
        data_ = other.data_;
        other.parent_data_ = nullptr;
        other.data_ = nullptr;
    }
}

LoanedDynamicData& LoanedDynamicData::operator=(LoanedDynamicData&& other) {
    if (this != &other) {
        if ((parent_data_ != nullptr) && (data_ != nullptr)) {
            parent_data_->return_loaned_value(data_);
        }
        parent_data_ = other.parent_data_;
        data_ = other.data_;
        other.parent_data_ = nullptr;
        other.data_ = nullptr;
    }
    return *this;
}

LoanedDynamicData::~LoanedDynamicData() {
    if (data_ == nullptr) {
        return;
    }
    if (parent_data_ == nullptr) {
        return;
    }
    parent_data_->return_loaned_value(data_);
    parent_data_ = nullptr;
    data_ = nullptr;
}

LoanedDynamicData::LoanedDynamicData(VBSDynamicData* data, VBSDynamicData* parent_data)
    : data_(data), parent_data_(parent_data) {}

void LoanedDynamicData::return_loan() {
    parent_data_->return_loaned_value(data_);
    data_ = nullptr;
}

VBSDynamicData& LoanedDynamicData::get() {
    return *data_;
}

const VBSDynamicData& LoanedDynamicData::get() const {
    return *data_;
}

LoanedDynamicData::operator VBSDynamicData&() {
    return *data_;
}

LoanedDynamicData::operator const VBSDynamicData&() const {
    return *data_;
}

}  // namespace vbs
