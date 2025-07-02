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

#include "core/condition/StatusCondition.hpp"
#include "ertps/types/TypesBase.h"
#include "core/condition/StatusConditionImpl.hpp"

namespace vbs {

using evbs::ReturnCode_t;

StatusCondition::StatusCondition(vbs::Entity* const parent)
    : Condition(), entity_(parent), impl_(new vbs::detail::StatusConditionImpl(notifier_.get())) {}

StatusCondition::~StatusCondition() {}

bool StatusCondition::get_trigger_value() const {
    return impl_->get_trigger_value();
}

vbsutil::elog::ReturnCode_t StatusCondition::set_enabled_statuses(const evbs::edds::dds::StatusMask& mask) {
    return impl_->set_enabled_statuses(mask);
}

const evbs::edds::dds::StatusMask& StatusCondition::get_enabled_statuses() const {
    return impl_->get_enabled_statuses();
}

vbs::Entity* StatusCondition::get_entity() const {
    return entity_;
}

}  // namespace vbs
