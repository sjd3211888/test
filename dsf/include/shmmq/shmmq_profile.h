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

#ifndef SHMMQ_PROFILE_H
#define SHMMQ_PROFILE_H

#include <stdint.h>

namespace mbuf {
class BufferDescriptor;
}
namespace shmmq {

struct profile;
typedef struct profile ProfileStruct;

class ShmmqProfile {
 private:
    ProfileStruct* profile_;

 public:
    void Init(ProfileStruct* profile);

    static ShmmqProfile& getInstance();
    void OnEnqueued();
    void OnDequeued(const mbuf::BufferDescriptor* buf_desc);

    ShmmqProfile(const ShmmqProfile&) = delete;
    ShmmqProfile& operator=(const ShmmqProfile&) = delete;

 protected:
    ShmmqProfile() = default;
    ~ShmmqProfile() = default;
};

} /* namespace shmmq */

#endif  // SHMMQ_STATISTIC_H
