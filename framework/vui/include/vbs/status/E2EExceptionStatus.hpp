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

#ifndef VBS_STATUS_E2EEXCEPTIONSTATUS_HPP_
#define VBS_STATUS_E2EEXCEPTIONSTATUS_HPP_

#include <cstdint>

namespace evbs {
namespace edds {
namespace dds {
class E2EExceptionStatus;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

typedef enum {
    E2E_STATUS_OK = 0x00,
    E2E_STATUS_NONEWDATA = 0x01,
    E2E_STATUS_ERROR = 0x02,
    E2E_STATUS_REPEATED = 0x08,
    E2E_STATUS_OKSOMELOST = 0x20,
    E2E_STATUS_WRONGSEQUENCE = 0x40,
    E2E_STATUS_ERRORADDRESS = 0x80,
    E2E_STATUS_ERRORCONFIG = 0x100
} E2E_StatusType;

using EVBSE2EExceptionStatus = evbs::edds::dds::E2EExceptionStatus;

class E2EExceptionStatus {
 public:
    E2EExceptionStatus();

    E2EExceptionStatus(const E2EExceptionStatus& status);

    E2EExceptionStatus(const EVBSE2EExceptionStatus& evbs_status);

    E2E_StatusType status() const;

    uint16_t counter() const;

    uint32_t crc() const;

 private:
    E2E_StatusType status_ = E2E_STATUS_OK;
    uint16_t counter_;
    uint32_t crc_;
};
}  // namespace vbs

#endif  // VBS_STATUS_E2EEXCEPTIONSTATUS_HPP_
