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

#include "core/condition/WaitSet.hpp"
#include "ertps/types/TypesBase.h"
#include "core/condition/WaitSetImpl.hpp"

namespace vbs {

WaitSet::WaitSet() : impl_(new detail::WaitSetImpl()) {}

WaitSet::~WaitSet() {}

vbsutil::elog::ReturnCode_t WaitSet::attach_condition(const vbs::Condition& cond) {
    return impl_->attach_condition(cond);
}

vbsutil::elog::ReturnCode_t WaitSet::detach_condition(const vbs::Condition& cond) {
    return impl_->detach_condition(cond);
}

vbsutil::elog::ReturnCode_t WaitSet::wait(vbs::ConditionSeq& active_conditions,
                                          const evbs::ertps::Duration_t timeout) const {
    return impl_->wait(active_conditions, timeout);
}

vbsutil::elog::ReturnCode_t WaitSet::get_conditions(vbs::ConditionSeq& attached_conditions) const {
    return impl_->get_conditions(attached_conditions);
}

}  // namespace vbs
