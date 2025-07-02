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

#ifndef MEMBUF_INC_BUFFER_DESC_H
#define MEMBUF_INC_BUFFER_DESC_H

#include <cstdint>
#include <type_traits>
#include <ostream>
#include "membuf/BaseType.h"

namespace mbuf {
using BufferNodeOffset = uint64_t;

struct BufferDesc {
    BufferDesc(PoolID id, UserID userId, BufferNodeOffset offset, uint64_t count)
        : poolID_(id), userID_(userId), bufferNodeOffset(offset), bufferNodeRecycleNums(count) {}
    BufferDesc() : poolID_(0U), userID_(0U), bufferNodeOffset(0U), bufferNodeRecycleNums(0U) {}

    bool operator==(const BufferDesc& rhs) const {
        return poolID_ == rhs.poolID_ && userID_ == rhs.userID_ && bufferNodeOffset == rhs.bufferNodeOffset &&
               bufferNodeRecycleNums == rhs.bufferNodeRecycleNums;
    }

    BufferDesc(const BufferDesc& other)
        : poolID_(other.poolID_),
          userID_(other.userID_),
          bufferNodeOffset(other.bufferNodeOffset),
          bufferNodeRecycleNums(other.bufferNodeRecycleNums) {}

    BufferDesc& operator=(const BufferDesc& other) {
        if (this != &other) {
            poolID_ = other.poolID_;
            userID_ = other.userID_;
            bufferNodeOffset = other.bufferNodeOffset;
            bufferNodeRecycleNums = other.bufferNodeRecycleNums;
        }
        return *this;
    }

    PoolID GetPoolID() const { return poolID_; }

    UserID GetUserID() const { return userID_; }

    void SetUserId(UserID userId) { userID_ = userId; }

    BufferNodeOffset GetNodeOffset() const { return bufferNodeOffset; }

    uint64_t GetRecycleNums() const { return bufferNodeRecycleNums; }

    void SetBufferNodeRecycleNums(uint64_t bufferNodeRecycleNums) {
        BufferDesc::bufferNodeRecycleNums = bufferNodeRecycleNums;
    }

    bool IsValid() const { return poolID_ || bufferNodeOffset; }

    bool IsValid(uint64_t recycleNums) const { return bufferNodeRecycleNums == recycleNums; }

    friend std::ostream& operator<<(std::ostream& os, const BufferDesc& obj) {
        return os << "poolID_: " << obj.poolID_ << " userID_: " << obj.userID_
                  << " bufferNodeOffset: " << obj.bufferNodeOffset
                  << " bufferNodeRecycleNums: " << obj.bufferNodeRecycleNums;
    }

 private:
    PoolID poolID_;
    UserID userID_;
    BufferNodeOffset bufferNodeOffset;
    uint64_t bufferNodeRecycleNums;
};

static_assert(std::is_standard_layout<BufferDesc>::value, "BufferDesc must be a standard layout");
}  // namespace mbuf
#endif  // MEMBUF_INC_BUFFER_DESC_H
