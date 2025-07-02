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

#ifndef MEMBUF_INC_BUFFER_H
#define MEMBUF_INC_BUFFER_H

#include <cstdint>
#include <memory>

namespace mbuf {

class Buffer {
 public:
    virtual void* data() = 0;
    virtual uint32_t size() = 0;
    virtual ~Buffer() = default;
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(Buffer&&) = delete;

 protected:
    Buffer() = default;
};

using BufferPtr = std::shared_ptr<Buffer>;
}  // namespace mbuf

#endif  // MEMBUF_INC_BUFFER_H
