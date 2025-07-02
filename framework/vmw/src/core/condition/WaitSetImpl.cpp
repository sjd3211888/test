// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#include <mutex>

#include "WaitSetImpl.hpp"
#include "edds/rtps/common/Time_t.h"
#include "ertps/types/TypesBase.h"
#include "condition_variable"
#include "core/condition/Condition.hpp"
#include "core/condition/ConditionNotifier.hpp"

namespace vbs {
namespace detail {

WaitSetImpl::~WaitSetImpl() {
    evbs::utilities::collections::unordered_vector<const vbs::Condition*> old_entries;
    {
        // We only need to protect access to the collection.
        const std::lock_guard<std::mutex> guard(mutex_);
        old_entries = entries_;
        entries_.clear();
    }

    for (const vbs::Condition* const c : old_entries) {
        c->get_notifier()->detach_from(this);
    }
}

vbsutil::elog::ReturnCode_t WaitSetImpl::attach_condition(const vbs::Condition& condition) {
    bool was_there = false;

    {
        // We only need to protect access to the collection.
        const std::lock_guard<std::mutex> guard(mutex_);

        was_there = entries_.remove(&condition);
        (void)entries_.emplace_back(&condition);
    }

    if (!was_there) {
        // This is a new condition. Inform the notifier of our interest.
        condition.get_notifier()->attach_to(this);

        {
            // Might happen that a wait changes is_waiting_'s status. Protect it.
            const std::lock_guard<std::mutex> guard(mutex_);

            // Should wake_up when adding a new triggered condition
            if (is_waiting_ && condition.get_trigger_value()) {
                cond_.notify_one();
            }
        }
    }

    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

vbsutil::elog::ReturnCode_t WaitSetImpl::detach_condition(const vbs::Condition& condition) {
    bool was_there = false;

    {
        // We only need to protect access to the collection.
        const std::lock_guard<std::mutex> guard(mutex_);
        was_there = entries_.remove(&condition);
    }

    if (was_there) {
        // Inform the notifier we are not interested anymore.
        condition.get_notifier()->detach_from(this);
        return vbsutil::elog::ReturnCode_t::RETCODE_OK;
    }

    // Condition not found
    return vbsutil::elog::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
}

vbsutil::elog::ReturnCode_t WaitSetImpl::wait(vbs::ConditionSeq& active_conditions,
                                              const evbs::ertps::Duration_t& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (is_waiting_) {
        return vbsutil::elog::ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    const auto fill_active_conditions = [&]() {
        bool ret_val = false;
        active_conditions.clear();
        for (const vbs::Condition* c : entries_) {
            if (c->get_trigger_value()) {
                ret_val = true;
                active_conditions.push_back(const_cast<vbs::Condition*>(c));
            }
        }
        return ret_val;
    };

    bool condition_value = false;
    is_waiting_ = true;
    if (evbs::ertps::c_TimeInfinite == timeout) {
        cond_.wait(lock, fill_active_conditions);
        condition_value = true;
    } else {
        const auto ns = timeout.to_ns();
        condition_value = cond_.wait_for(lock, std::chrono::nanoseconds(ns), fill_active_conditions);
    }
    is_waiting_ = false;

    return condition_value ? vbsutil::elog::ReturnCode_t::RETCODE_OK : vbsutil::elog::ReturnCode_t::RETCODE_TIMEOUT;
}

vbsutil::elog::ReturnCode_t WaitSetImpl::get_conditions(vbs::ConditionSeq& attached_conditions) const {
    const std::lock_guard<std::mutex> guard(mutex_);
    attached_conditions.reserve(entries_.size());
    attached_conditions.clear();
    for (const vbs::Condition* c : entries_) {
        attached_conditions.push_back(const_cast<vbs::Condition*>(c));
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

void WaitSetImpl::wake_up() {
    const std::lock_guard<std::mutex> guard(mutex_);
    cond_.notify_one();
}

void WaitSetImpl::will_be_deleted(const vbs::Condition& condition) {
    const std::lock_guard<std::mutex> guard(mutex_);
    (void)entries_.remove(&condition);
}

}  // namespace detail
}  // namespace vbs
