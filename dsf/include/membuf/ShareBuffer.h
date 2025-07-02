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

#ifndef MEMBUF_INC_SHARE_BUFFER_H
#define MEMBUF_INC_SHARE_BUFFER_H

#include "membuf/BaseType.h"
#include "membuf/buffer.h"
#include "membuf/BufferDesc.h"
namespace mbuf {
struct BufferNode;

struct ShareBuffer : Buffer {
    ShareBuffer(void* bufferNode, uint8_t* data, size_t size, UserID, bool multiReader = false);
    ~ShareBuffer() override;
    void* GetBufferNode() const;
    bool IsMultiReader() const { return multiReader; }

 private:
    void* data() override;
    uint32_t size() override;

 protected:
    void* bufferNode;

 private:
    void* data_;
    size_t size_;
    const UserID userId;

 protected:
    bool multiReader;
};

struct ShareBufferOwner : ShareBuffer {
    ShareBufferOwner(void* bufferNode, uint8_t* data, size_t size, PoolID poolId, BufferNodeOffset offset,
                     bool multiReader = false);
    BufferDesc BuildDescFor(UserID) const;
    BufferDesc GetDescMetaInfo() const;
    void ResetToFree(UserID);

 private:
    PoolID poolId;
    BufferNodeOffset offset;
};

}  // namespace mbuf

#endif  // MEMBUF_INC_SHARE_BUFFER_H
