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

#include "vbs/status/E2EExceptionStatus.hpp"
#include "deps/core/status/E2EExceptionStatus.hpp"

namespace vbs {

E2EExceptionStatus::E2EExceptionStatus() : status_(E2E_STATUS_OK), counter_(0U), crc_(0U) {}

E2EExceptionStatus::E2EExceptionStatus(const E2EExceptionStatus& status) {
    status_ = status.status_;
    counter_ = status.counter_;
    crc_ = status.crc_;
}

E2EExceptionStatus::E2EExceptionStatus(const EVBSE2EExceptionStatus& evbs_status) {
    status_ = static_cast<E2E_StatusType>(static_cast<int>(evbs_status.status));
    counter_ = evbs_status.counter;
    crc_ = evbs_status.crc;
}

E2E_StatusType E2EExceptionStatus::status() const {
    return status_;
}

uint16_t E2EExceptionStatus::counter() const {
    return counter_;
}

uint32_t E2EExceptionStatus::crc() const {
    return crc_;
}

}  // namespace vbs
