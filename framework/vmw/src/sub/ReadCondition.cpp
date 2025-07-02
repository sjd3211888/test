// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "sub/ReadConditionImpl.hpp"

namespace vbs {

ReadCondition::ReadCondition() {}

ReadCondition::~ReadCondition() {}

bool ReadCondition::get_trigger_value() const noexcept {
    assert((bool)impl_);
    return impl_->get_trigger_value();
}
vbs::DataReaderImpl& ReadCondition::get_datareader() const noexcept {
    assert((bool)impl_);
    return impl_->get_datareader();
}

vbs::SampleStateMask ReadCondition::get_sample_state_mask() const noexcept {
    assert((bool)impl_);
    return impl_->get_sample_state_mask();
}

vbs::ViewStateMask ReadCondition::get_view_state_mask() const noexcept {
    assert((bool)impl_);
    return impl_->get_view_state_mask();
}

vbs::InstanceStateMask ReadCondition::get_instance_state_mask() const noexcept {
    assert((bool)impl_);
    return impl_->get_instance_state_mask();
}

}  // namespace vbs
