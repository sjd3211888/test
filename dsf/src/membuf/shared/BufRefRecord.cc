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

#include "BufRefRecord.h"
#include <unistd.h>
#include "DsfLog.hpp"
namespace mbuf {

bool BufRefRecord::IsRefBy(UserID userId) const {
    if (userId.IsInvalid()) {
        return false;
    }
    return userPids[userId] != 0U && userStates[userId] == RefState::USING;
}

UserID BufRefRecord::AddUserID(const PID pid) {
    // 由于owner不再共用这个函数 user的遍历开始索引从1开始
    // 出于性能的和多个槽位的重试考虑，这里还保持compare_exchange_weak的使用
    for (size_t i = 1U; i < userPids.size(); i++) {
        PID emptyPid {0U};
        if (userPids[i].compare_exchange_weak(emptyPid, pid)) {
            auto oldState = RefState::INVALID;
            // 虽然pid可以换槽位，但是userStates没有其他槽位可选，
            // 这里从compare_exchange_weak 改为 compare_exchange_strong 避免fail spuriously
            // 否则虽然pid槽位被占用但是无法被使用
            if (userStates[i].compare_exchange_strong(oldState, RefState::USING)) {
                return UserID {i};
            } else {
                DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_INVALID_TO_USING_FAILED,
                              "pid:" << pid << ",change userPids[" << i << "] from states:" << userStates[i]
                                     << " to USING(1) failed!!!");
                break;
            }
        } else {
            DSF_LOG_DEBUG(BufRefRecord,
                          "userPids[" << i << "]=" << userPids[i] << " is not empty,can not exchange to pid:" << pid);
        }
    }
    return UserID {userPids.size()};
}

UserID BufRefRecord::AddOwnerID(const PID pid) {
    PID emptyPid {0U};
    if (userPids[0].compare_exchange_strong(emptyPid, pid)) {
        auto oldState = RefState::INVALID;
        if (userStates[0].compare_exchange_strong(oldState, RefState::USING)) {
            return UserID {0U};
        } else {
            DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_INVALID_TO_USING_FAILED,
                          "AddOwnerID pid:" << pid << ",current states:" << userStates[0]
                                            << ",expect state:RefState::INVALID"
                                            << ",exchange state:RefState::USING failed!");
            return UserID {userPids.size()};
        }
    } else {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_ADD_PID_FAILED,
                      "AddOwnerID current pid:" << userPids[0] << ",expect pid:0"
                                                << ",exchange pid:" << pid << " failed!");
        return UserID {userPids.size()};
    }
}

bool BufRefRecord::UserCrash(PID pid, UserCrashHandler& userCrashHandler) {
    for (size_t i = 0U; i < userPids.size(); i++) {
        if (userPids[i].load() == pid) {
            if (UserCrashHandle(UserID(i), userCrashHandler)) {
                continue;
            } else {
                return false;
            }
        }
    }
    return true;
}

void BufRefRecord::OwnerCrash(UserCrashHandler& userCrashHandler) {
    UserCrashHandle(UserID(0U), userCrashHandler);
}

bool BufRefRecord::UserCrashHandle(UserID userId, UserCrashHandler& userCrashHandler) {
    auto& userState = userStates[userId];
    auto oldState = userState.load();
    if (oldState != RefState::USING) {
        // 这个分支正常情况不会进入，如果发生了异常处理后再次进入(比如收到2次通知一次crash，一次下线)，可能会进去这里，这里输出日志记录。
        DSF_LOG_WARNING(BufRefRecord, "user id:" << userId << ",oldState:" << oldState << ",current states:"
                                                 << userStates[userId] << ", is not using on crash handle");
        return true;
    }
    // 这里使用compare_exchange_weak如果失败会有槽位泄露的风险，改为strong，避免fail spuriously
    if (userState.compare_exchange_strong(oldState, RefState::CRASH)) {
        userCrashHandler.HandleUserCrash(userId);
        userState.store(RefState::INVALID);
        userPids[userId].store(0U);
        return true;
    } else {
        // 在oldState有竞争的情况下会进入这里
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_USING_TO_CRASH_FAILED,
                      "user id:" << userId << "userState cas,oldState:" << oldState
                                 << ",current states:" << userStates[userId] << ",change to RefState::CRASH failed");
        return false;
    }
}

bool BufRefRecord::ResetUser(UserID userId, UserCrashHandler& userCrashHandler) {
    if (userId.IsUserInvalid()) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_USERID_INVALID,
                      "ResetUser userId:" << userId << " IsInvalid");
        return false;
    }
    auto oldState = userStates[userId].load();
    if (oldState != RefState::USING) {
        // 这个分支正常情况不会进入，如果发生了异常处理后再次进入(比如收到2次通知一次crash，一次下线)，可能会进去这里，这里输出日志记录。
        DSF_LOG_WARNING(BufRefRecord, "user id:" << userId << ",oldState:" << oldState << ",current states:"
                                                 << userStates[userId] << ", is not using on ResetUser");
        return true;
    }
    // user offline,change userStates to INVALID whatever old userStates is what
    if (userStates[userId].compare_exchange_strong(oldState, RefState::INVALID)) {
        // reset userPids to 0
        userPids[userId].store(0U);
    } else {
        DSF_LOG_ERROR(
            DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_USING_TO_INVALID_FAILED,
            "ResetUser change userId:" << userId << " userStates to RefState::INVALID failed,oldState:" << oldState);
        return false;
    }
    // reset buffer node state to Free
    if (!userCrashHandler.HandleUserCrash(userId)) {
        DSF_LOG_ERROR(DSF_MEMBUF, ReturnCode_t::RETCODE_MEMBUF_CAS_ANY_TO_FREE_FAILED,
                      "ResetUser change userId:" << userId << " BufferNode refState to RefNState::Free failed!");
        return false;
    }
    return true;
}

void BufRefRecord::SetEnableDfx(uint8_t enable) {
    enableDfx.store(enable);
}

uint64_t BufRefRecord::GetAllocPosition() const {
    return allocPosition;
}

uint64_t BufRefRecord::GetFreePosition() const {
    return freePosition;
}

void BufRefRecord::UpdateAllocInfo(uint64_t size) {
    // 每次成功分配后更新最大最小值
    maxAllocSize = std::max(maxAllocSize, size);
    minAllocSize = minAllocSize == 0 ? size : std::min(minAllocSize, size);
    ++allocCount;
}

uint64_t BufRefRecord::GetMaxAllocSize() const {
    return maxAllocSize;
}

uint64_t BufRefRecord::GetMinAllocSize() const {
    return minAllocSize;
}

uint64_t BufRefRecord::GetAllocCount() const {
    return allocCount;
}

uint64_t BufRefRecord::GetUsedPoolSize() const {
    return usedPoolSize;
}

uint64_t BufRefRecord::GetUsedBufCount() const {
    return usedBufCount;
}

}  // namespace mbuf
