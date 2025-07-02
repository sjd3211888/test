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

#include "membuf/ShareBuffer.h"
#include "BufferNode.h"
#include "multi_reader/BufferNode.h"
#include "membuf/BufferDesc.h"
#include "membuf/buffer_info.h"
#include "DsfLog.hpp"

namespace mbuf {
ShareBuffer::ShareBuffer(void* bufferNode, uint8_t* data, size_t size, const UserID userId, bool multiReader)
    : bufferNode(bufferNode), data_(data), size_(size), userId(userId), multiReader(multiReader) {}

ShareBuffer::~ShareBuffer() {
    if (multiReader) {
        reinterpret_cast<multiReader::BufferNode*>(bufferNode)->OnReleased(userId);
    } else {
        reinterpret_cast<BufferNode*>(bufferNode)->OnReleased(userId);
    }
}

void* ShareBuffer::data() {
    return data_;
}

uint32_t ShareBuffer::size() {
    return size_;
}

void* ShareBuffer::GetBufferNode() const {
    return bufferNode;
}

ShareBufferOwner::ShareBufferOwner(void* bufferNode, uint8_t* data, size_t size, PoolID poolId, BufferNodeOffset offset,
                                   bool multiReader)
    : ShareBuffer(bufferNode, data, size, 0U, multiReader), poolId(poolId), offset(offset) {
    if (multiReader) {
        reinterpret_cast<multiReader::BufferNode*>(bufferNode)->OnUsing(0U);
    } else {
        reinterpret_cast<BufferNode*>(bufferNode)->OnUsing(0U);
    }
}

BufferDesc ShareBufferOwner::BuildDescFor(UserID userId) const {
    bool ret;
    if (multiReader) {
        ret = reinterpret_cast<multiReader::BufferNode*>(bufferNode)->OnPending(userId);
        DSF_LOG_DEBUG(ShareBufferOwner, "BuildDescFor user id:"
                                            << userId << " buffer node status:"
                                            << reinterpret_cast<multiReader::BufferNode*>(bufferNode)->PrintRefState());
    } else {
        ret = reinterpret_cast<BufferNode*>(bufferNode)->OnPending(userId);
    }
    if (!ret) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_BUILD_DESC_ON_PENDING_ERR,
                      "BuildDescFor usedId:" << userId << ",onPending failed");
        return BufferDesc {0U, multiReader ? multiReader::MaxMultiReader : MAX_REF_COUNT, 0U, 0U};
    }
    auto recycleNum = multiReader ? reinterpret_cast<multiReader::BufferNode*>(bufferNode)->GetRecycleNums()
                                  : reinterpret_cast<BufferNode*>(bufferNode)->GetRecycleNums();
    return BufferDesc {poolId, userId, offset, recycleNum};
}

// 在大buf零拷贝的场景下，不使用user id获取des的元信息
BufferDesc ShareBufferOwner::GetDescMetaInfo() const {
    auto recycleNum = multiReader ? reinterpret_cast<multiReader::BufferNode*>(bufferNode)->GetRecycleNums()
                                  : reinterpret_cast<BufferNode*>(bufferNode)->GetRecycleNums();
    return BufferDesc {poolId, multiReader ? multiReader::MaxMultiReader : MAX_REF_COUNT, offset, recycleNum};
}

void ShareBufferOwner::ResetToFree(UserID userId) {
    bool ret;
    if (multiReader) {
        ret = reinterpret_cast<multiReader::BufferNode*>(bufferNode)->OnExcept(userId);
    } else {
        ret = reinterpret_cast<BufferNode*>(bufferNode)->OnExcept(userId);
    }
    if (!ret) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_RESET_TO_FREE_ON_EXCEPT_ERR,
                      "can not reset userId:" << userId << " to free");
    }
}

}  // namespace mbuf
