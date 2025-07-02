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

#include "StatusConditionImpl.hpp"
#include "core/condition/ConditionNotifier.hpp"

namespace vbs {
namespace detail {

StatusConditionImpl::StatusConditionImpl(ConditionNotifier* const notifier)
    : mask_(evbs::edds::dds::StatusMask::all()), status_(evbs::edds::dds::StatusMask::none()), notifier_(notifier) {}

StatusConditionImpl::~StatusConditionImpl() {}

bool StatusConditionImpl::get_trigger_value() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return (mask_ & status_).any();
}

vbsutil::elog::ReturnCode_t StatusConditionImpl::set_enabled_statuses(const evbs::edds::dds::StatusMask& mask) {
    bool notify = false;
    {
        const std::lock_guard<std::mutex> guard(mutex_);
        const bool old_trigger = (mask_ & status_).any();
        mask_ = mask;
        if (!old_trigger) {
            const bool new_trigger = (mask_ & status_).any();
            notify = new_trigger;
        }
    }

    if (notify) {
        notifier_->notify();
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

const evbs::edds::dds::StatusMask& StatusConditionImpl::get_enabled_statuses() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return mask_;
}

void StatusConditionImpl::set_status(const evbs::edds::dds::StatusMask& status, const bool trigger_value) {
    if (trigger_value) {
        bool notify = false;
        {
            const std::lock_guard<std::mutex> guard(mutex_);
            const bool old_trigger = (mask_ & status_).any();
            status_ |= status;
            if (!old_trigger) {
                const bool new_trigger = (mask_ & status_).any();
                notify = new_trigger;
            }
        }

        if (notify) {
            notifier_->notify();
        }
    } else {
        const std::lock_guard<std::mutex> guard(mutex_);
        status_ &= ~status;
    }
}

}  // namespace detail
}  // namespace vbs
