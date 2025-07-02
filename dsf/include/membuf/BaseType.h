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

#ifndef MEMBUF_INC_BASE_TYPE_H
#define MEMBUF_INC_BASE_TYPE_H

#include <cstdint>
#include "common/types/TypesBase.h"

namespace mbuf {
using ReturnCode_t = li::dsfdds::ReturnCode_t;
constexpr uint16_t MAX_REF_COUNT = 32U;
constexpr uint16_t MAX_MULTI_REF_COUNT = 128U;

// constexpr uint16_t UNIMEM_HEAP_BUF_NAME_LEN = 128;
using PID = uint64_t;
using PoolID = uint32_t;
using MemOffset = uint64_t;

struct UserID {
    UserID(const uint64_t v) : index(v) {}

    bool IsInvalid() const { return index >= MAX_REF_COUNT; }
    bool IsUserInvalid() const { return index == MAX_REF_COUNT || index == 0U; }
    bool IsMultiUserInvalid() const { return index == MAX_MULTI_REF_COUNT || index == 0U; }
    bool IsOwnerInvalid() const { return index != 0U; }
    void set_id(uint64_t index) { this->index = index; }

    operator uint64_t() const { return index; }

 private:
    uint64_t index;
};

enum AllocateMode : size_t {
    Fixed,
    Dynamic,
    MultiReader,
};

}  // namespace mbuf

#endif  //  MEMBUF_INC_BASE_TYPE_H
