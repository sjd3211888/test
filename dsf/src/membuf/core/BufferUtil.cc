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

#include "membuf/buffer_info.h"
#include "membuf/ShareBuffer.h"
#include "membuf/BufferPoolUserManager.h"
namespace mbuf {
BufferInfo BufferUtil::GetBufferInfoFromBuffer(const std::shared_ptr<Buffer>& buffer) {
    return std::dynamic_pointer_cast<ShareBufferOwner>(buffer)->GetDescMetaInfo();
}

// 为了防止并发修改，这里的第一个入参直接传值，这样在函数调用的时候就会拷贝一份BufferInfo
std::shared_ptr<Buffer> BufferUtil::GetBufferFromBufferInfo(BufferInfo buffer_info,
                                                            const mbuf::BuildMbufContext& context) {
    // 如果入参是BufferInfo& buffer_info不能这么写，这样修改会有并发的风险，在一写多场景下，这里会把其他reader的user id修改掉
    buffer_info.SetUserId(context.userId);
    return reinterpret_cast<BufferPoolUserManager*>(context.manager)->RebuildBuffer(buffer_info);
}

BuildMbufContext::BuildMbufContext() : manager(nullptr), userId(UserID {MAX_REF_COUNT}) {}

BuildMbufContext::BuildMbufContext(void* manager, const UserID& userId) : manager(manager), userId(userId) {}

}  // namespace mbuf