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

#include "ConditionNotifier.hpp"
#include "core/condition/Condition.hpp"
#include "core/condition/WaitSetImpl.hpp"

namespace vbs {
namespace detail {

void ConditionNotifier::attach_to(vbs::detail::WaitSetImpl* wait_set) {
    if (nullptr != wait_set) {
        const std::lock_guard<std::mutex> guard(mutex_);
        (void)entries_.remove(wait_set);
        (void)entries_.emplace_back(wait_set);
    }
}

void ConditionNotifier::detach_from(vbs::detail::WaitSetImpl* const wait_set) {
    if (nullptr != wait_set) {
        const std::lock_guard<std::mutex> guard(mutex_);
        (void)entries_.remove(wait_set);
    }
}

void ConditionNotifier::notify() {
    const std::lock_guard<std::mutex> guard(mutex_);
    for (vbs::detail::WaitSetImpl* const wait_set : entries_) {
        wait_set->wake_up();
    }
}

void ConditionNotifier::will_be_deleted(const vbs::Condition& condition) {
    std::lock_guard<std::mutex> guard(mutex_);
    for (vbs::detail::WaitSetImpl* wait_set : entries_) {
        wait_set->will_be_deleted(condition);
    }
}

}  // namespace detail
}  // namespace vbs
