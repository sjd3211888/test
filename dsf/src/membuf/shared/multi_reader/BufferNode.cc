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

#include "multi_reader/BufferNode.h"

#include "DsfLog.hpp"

#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <pthread.h>
namespace mbuf {
namespace multiReader {

static inline int pthread_mutex_lock_private(pthread_mutex_t* ptr_mutex) {
    int ret = pthread_mutex_lock(ptr_mutex);
    if (EOWNERDEAD == ret) {
        DSF_LOG_WARNING(multireader, "detect mutex owner process crash!");
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
        ret = pthread_mutex_consistent(ptr_mutex);
#else
        ret = 0;
#endif
        if (0 != ret) {
            DSF_LOG_ERROR(DSF_MEMBUF, membuf_ret_t::RETCODE_MEMBUF_MUTEX_EXCEPTION,
                          "pthread_mutex_consistent failed, ret value is: " << strerror(ret));
            return membuf_ret_t::RETCODE_MEMBUF_MUTEX_EXCEPTION;
        }
        DSF_LOG_WARNING(multireader, "hold mutex crash, continue");
        return membuf_ret_t::RETCODE_MEMBUF_HOLDMUTEX_CRASH;
    } else if (0 != ret) {
        DSF_LOG_ERROR(DSF_MEMBUF, membuf_ret_t::RETCODE_MEMBUF_MUTEX_EXCEPTION,
                      "pthread_mutex_lock unexpectedly failed, ret value is: " << strerror(ret));
        return membuf_ret_t::RETCODE_MEMBUF_MUTEX_EXCEPTION;
    } else {
        // do nothing
    }
    return 0;
}

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

constexpr uint64_t CalculateAnyUsingMask(size_t bitWidth) {
    uint64_t state = 0U;
    for (uint16_t id = 0U; id < MAX_REF_COUNT; id++) {
        state |= RefNState::Using << (bitWidth * id);
    }
    return state;
}

constexpr uint64_t AllUserStateFreeMask = CalculateUserFreeMask(RefStateBitWidth);
constexpr uint64_t AllUserStatePending = SetAllUserStatePending();
constexpr uint64_t AnyUserStateUsingMask = CalculateAnyUserUsingMask(RefStateBitWidth);
constexpr uint64_t AnyStateUsingMask = CalculateAnyUsingMask(RefStateBitWidth);

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
            DSF_LOG_WARNING(Transit, "Transit failed, state:"
                                         << state << " refState:" << refState.load() << ",user id:" << userId
                                         << ",current state:" << CurrentState(refState, userId)
                                         << ",expect old state:" << oldState << ",transit new state:" << newState);
            return false;
        }
    }
}
}  // namespace

bool BufferNode::OnPending(UserID userId) {
    pthread_mutex_lock_private(&refStateMutex);
    // 计算userId所属的refState,user_id >> 5写法等价于 user_id / 32;  userId & 0x1F 等价于 userId%32
    DSF_LOG_DEBUG(OnPending, "OnPending1: user id:" << userId << ",array index:" << (userId >> 5) << ",statues:"
                                                    << refState[userId >> 5] << ",which user id:" << ((userId & 0x1F)));
    auto ret = Transit(refState[userId >> 5], (userId & 0x1F), RefNState::Free, RefNState::Pending);

    DSF_LOG_DEBUG(OnPending, "OnPending2: user id:" << userId << ",array index:" << (userId >> 5) << ",statues:"
                                                    << refState[userId >> 5] << ",which user id:" << ((userId & 0x1F)));
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::OnUsing(UserID userId) {
    pthread_mutex_lock_private(&refStateMutex);
    DSF_LOG_DEBUG(OnUsing, "OnUsing: user id:" << userId << ",array index:" << (userId >> 5) << ",statues:"
                                               << refState[userId >> 5] << ",which user id:" << ((userId & 0x1F)));
    auto ret = Transit(refState[userId >> 5], (userId & 0x1F), RefNState::Pending, RefNState::Using);
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::OnReleased(UserID userId) {
    pthread_mutex_lock_private(&refStateMutex);
    auto ret = Transit(refState[userId >> 5], (userId & 0x1F), RefNState::Using, RefNState::Free);
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::OnRevertToPending (UserID userId) {
    pthread_mutex_lock_private(&refStateMutex);
    auto ret = Transit(refState[userId >> 5], (userId & 0x1F), RefNState::Using, RefNState::Pending);
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::OnExcept(UserID userId) {
    pthread_mutex_lock_private(&refStateMutex);
    // There is a risk of failure if you try MAX_REF_COUNT times,
    // in this case because user have not released membuf on crash or offline
    constexpr size_t MAX_TRY_COUNT = MAX_REF_COUNT;
    for (size_t idx = 0U; idx < MAX_TRY_COUNT; idx++) {
        auto state = refState[userId >> 5].load(std::memory_order_relaxed);
        if (refState[userId >> 5].compare_exchange_weak(state, ExpectState(state, (userId & 0x1F), RefNState::Free))) {
            static_cast<void>(pthread_mutex_unlock(&refStateMutex));
            return true;
        } else {
            DSF_LOG_DEBUG(BufferNode,
                          "try index:" << idx << "user id:" << userId << ",cas old state:" << state << ",expect state:"
                                       << ExpectState(state, (userId & 0x1F), RefNState::Free) << " failed");
        }
    }
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return false;
}

uint64_t BufferNode::QueryState(UserID userId) const {
    pthread_mutex_lock_private(&refStateMutex);
    auto ret = CurrentState(refState[userId >> 5], (userId & 0x1F));
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

void BufferNode::GetState(std::ostream& ostream, UserID id) const {
    ostream << "user id:" << id << ",current state:" << CurrentState(refState[id >> 5], (id & 0x1F)) << "\n";
}

uint64_t BufferNode::GetState(UserID id) const {
    return CurrentState(refState[id >> 5], (id & 0x1F));
}

bool BufferNode::OnRecycle() {
    pthread_mutex_lock_private(&refStateMutex);
    for (auto& State : refState) {
        auto state = State.load(std::memory_order_relaxed);
        // the owner state must be Free, users state can be Free or Pending
        if ((state | AllUserStatePending) != AllUserStatePending) {
            static_cast<void>(pthread_mutex_unlock(&refStateMutex));
            return false;
        }
    }
    for (auto& State : refState) {
        State.store(0U);
    }
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return true;
}

bool BufferNode::IsFree() const {
    pthread_mutex_lock_private(&refStateMutex);
    for (auto& states : refState) {
        if (states.load() != 0x0U) {
            static_cast<void>(pthread_mutex_unlock(&refStateMutex));
            return false;
        }
    }
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return true;
}

bool BufferNode::IsAllUserFree() const {
    pthread_mutex_lock_private(&refStateMutex);
    auto ret = (refState[0].load() & AllUserStateFreeMask) == 0x0U;
    bool first = true;
    for (auto& states : refState) {
        if (!first && states.load() != 0x0U) {
            static_cast<void>(pthread_mutex_unlock(&refStateMutex));
            return false;
        }
        first = false;
    }
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::IsAllUserNotUsing() const {
    pthread_mutex_lock_private(&refStateMutex);
    auto ret = (refState[0].load() & AnyUserStateUsingMask) == 0x0U;
    bool first = true;
    for (auto& states : refState) {
        if (!first && (states.load() & AnyStateUsingMask) != 0x0U) {
            static_cast<void>(pthread_mutex_unlock(&refStateMutex));
            return false;
        }
        first = false;
    }
    static_cast<void>(pthread_mutex_unlock(&refStateMutex));
    return ret;
}

bool BufferNode::IsAllOwnerFree() const {
    return QueryState(0) == 0x0U;
}

void BufferNode::Reset() {
    for (auto& State : refState) {
        State.store(0U);
    }
}

void BufferNode::Init(MemOffset memOffset, size_t size) {
    Reset();
    this->offset = memOffset;
    this->length = size;
    this->recycleNums = 0U;
    pthread_mutexattr_t refStateMutexAttr;
    int ret_multi_write_mux = 0;
    /* set mutex shared between processes */
    static_cast<void>(pthread_mutexattr_init(&refStateMutexAttr));
    static_cast<void>(pthread_mutexattr_setpshared(&refStateMutexAttr, PTHREAD_PROCESS_SHARED));
#if !defined(NDK_BUILD) && !defined(AOSP_BUILD)
    static_cast<void>(pthread_mutexattr_setrobust(&refStateMutexAttr, PTHREAD_MUTEX_ROBUST));
#endif
    ret_multi_write_mux = pthread_mutex_init(&refStateMutex, &refStateMutexAttr);

    if (ret_multi_write_mux != 0) {
        DSF_LOG_ERROR(DSF_MEMBUF, membuf_ret_t::RETCODE_MEMBUF_PTHREAD_MUTEX_INIT_FAILED,
                      "writer pthread retcode: " << strerror(errno));
        static_cast<void>(pthread_mutex_destroy(&refStateMutex));
    }
}

std::string BufferNode::PrintRefState() const {
    std::stringstream ss;
    for (auto& it : refState) {
        ss << it.load() << ",";
    }
    return ss.str();
}
}  // namespace multiReader
}  // namespace mbuf
