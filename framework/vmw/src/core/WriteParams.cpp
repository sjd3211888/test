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

#include "core/WriteParams.hpp"

namespace vbs {
VBSWriteParams::VBSWriteParams() : sequence_num_(0U) {}

VBSWriteParams::VBSWriteParams(uint64_t num) : sequence_num_(num) {}

VBSWriteParams::VBSWriteParams(uint64_t num, const uint16_t& id) : sequence_num_(num), guid_(id) {}

uint64_t VBSWriteParams::sequence_num() {
    return sequence_num_;
}

void VBSWriteParams::sequence_num(uint64_t num) {
    sequence_num_ = num;
}

uint16_t VBSWriteParams::guid() {
    return guid_;
}

void VBSWriteParams::guid(const uint16_t& id) {
    guid_ = id;
}

}  // namespace vbs
