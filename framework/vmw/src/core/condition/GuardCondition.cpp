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

#include "core/condition/GuardCondition.hpp"
#include "core/condition/ConditionNotifier.hpp"

namespace vbs {

GuardCondition::GuardCondition() : trigger_value_(false) {}

GuardCondition::~GuardCondition() {}

bool GuardCondition::get_trigger_value() const {
    return trigger_value_.load();
}

vbsutil::elog::ReturnCode_t GuardCondition::set_trigger_value(const bool value) {
    const bool old_value = trigger_value_.exchange(value);
    if ((!old_value) && value) {
        notifier_->notify();
    }

    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

}  // namespace vbs
