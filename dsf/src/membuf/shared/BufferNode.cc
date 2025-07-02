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

#include "BufferNode.h"

#include "DsfLog.hpp"

#include <unistd.h>
#include <cstdint>
#include <iostream>

namespace mbuf {
namespace {
constexpr size_t RefStateBitWidth = 2U;
constexpr uint64_t RefStateMask = 0x3U;

constexpr uint64_t SetAllUserStatePending() {
    uint64_t state = 0U;
    for (uint16_t id = 1U; id < MAX_REF_COUNT; id++) {
        state |= RefNState::Pending << (RefStateBitWidth * id);
    }
    return state;
}
// calculate all user free mask
constexpr uint64_t CalculateUserFreeMask(size_t bitWidth) {
    return ~((static_cast<size_t>(1U) << bitWidth) - 1);
}

constexpr uint64_t CalculateAnyUserUsingMask(size_t bitWidth) {
    uint64_t state = 0U;
    for (uint16_t id = 1U; id < MAX_REF_COUNT; id++) {
        state |= RefNState::Using << (bitWidth * id);
    }
    return state;
}

constexpr uint64_t AllUserStateFreeMask = CalculateUserFreeMask(RefStateBitWidth);
constexpr uint64_t AllUserStatePending = SetAllUserStatePending();
constexpr uint64_t AnyUserStateUsingMask = CalculateAnyUserUsingMask(RefStateBitWidth);

bool StateEqual(uint64_t refState, UserID userId, uint64_t cmpState) {
    return ((refState >> (RefStateBitWidth * userId)) & RefStateMask) == cmpState;
}

uint64_t CurrentState(uint64_t refState, UserID userId) {
    return (refState >> (RefStateBitWidth * userId)) & RefStateMask;
}

uint64_t ExpectState(uint64_t refState, UserID userId, uint64_t expect) {
    return (refState & (~(RefStateMask << (RefStateBitWidth * userId)))) | (expect << (RefStateBitWidth * userId));
}

bool Transit(std::atomic<uint64_t>& refState, UserID userId, uint64_t oldState, uint64_t newState) {
    while (true) {
        auto state = refState.load(std::memory_order_relaxed);
        if (StateEqual(state, userId, oldState)) {
            if (refState.compare_exchange_weak(state, ExpectState(state, userId, newState))) {
                return true;
            }
        } else {
            DSF_LOG_WARNING(Transit, "Transit failed: user id:"
                                         << userId << ",current state:" << CurrentState(refState, userId)
                                         << ",expect old state:" << oldState << ",transit new state:" << newState);
            return false;
        }
    }
}
}  // namespace

bool BufferNode::OnPending(UserID userId) {
    return Transit(refState, userId, RefNState::Free, RefNState::Pending);
}

bool BufferNode::OnUsing(UserID userId) {
    return Transit(refState, userId, RefNState::Pending, RefNState::Using);
}

bool BufferNode::OnReleased(UserID userId) {
    return Transit(refState, userId, RefNState::Using, RefNState::Free);
}

bool BufferNode::OnRevertToPending (UserID userId) {
    return Transit(refState, userId, RefNState::Using, RefNState::Pending);
  }  

bool BufferNode::OnExcept(UserID userId) {
    // There is a risk of failure if you try MAX_REF_COUNT times,
    // in this case because user have not released membuf on crash or offline
    constexpr size_t MAX_TRY_COUNT = MAX_REF_COUNT;
    for (size_t idx = 0U; idx < MAX_TRY_COUNT; idx++) {
        auto state = refState.load(std::memory_order_relaxed);
        if (refState.compare_exchange_weak(state, ExpectState(state, userId, RefNState::Free))) {
            return true;
        } else {
            DSF_LOG_DEBUG(BufferNode, "try index:" << idx << "user id:" << userId << ",cas old state:" << state
                                                   << ",expect state:" << ExpectState(state, userId, RefNState::Free)
                                                   << " failed");
        }
    }
    return false;
}

uint64_t BufferNode::QueryState(UserID userId) const {
    return CurrentState(refState, userId);
}

void BufferNode::GetState(std::ostream& ostream, UserID id) const {
    ostream << "user id:" << id << ",current state:" << CurrentState(refState, id) << "\n";
}

uint64_t BufferNode::GetState(UserID id) const {
    return CurrentState(refState, id);
}

bool BufferNode::OnRecycle() {
    auto state = refState.load(std::memory_order_relaxed);
    // the owner state must be Free, users state can be Free or Pending
    if ((state | AllUserStatePending) == AllUserStatePending) {
        return refState.compare_exchange_weak(state, 0U);
    } else {
        return false;
    }
}

bool BufferNode::IsFree() const {
    return refState.load() == 0x0U;
}

bool BufferNode::IsAllUserFree() const {
    return (refState.load() & AllUserStateFreeMask) == 0x0U;
}

bool BufferNode::IsAllUserNotUsing() const {
    return (refState.load() & AnyUserStateUsingMask) == 0x0U;
}

bool BufferNode::IsAllOwnerFree() const {
    return QueryState(0) == 0x0U;
}

void BufferNode::Reset() {
    refState.store(0U);
}

void BufferNode::Init(MemOffset offset, size_t size) {
    Reset();
    this->offset = offset;
    this->length = size;
    this->recycleNums = 0U;
}

std::string BufferNode::PrintRefState() const {
    std::stringstream ss;
    ss << refState.load() << ",";
    return ss.str();
}
}  // namespace mbuf
