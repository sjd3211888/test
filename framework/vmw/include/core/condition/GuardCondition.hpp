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

#ifndef INCLUDE_EDDS_DDS_CORE_CONDITION_GUARDCONDITION_HPP_
#define INCLUDE_EDDS_DDS_CORE_CONDITION_GUARDCONDITION_HPP_

#include <atomic>

#include "ertps/ertps_dll.h"
#include "ertps/types/TypesBase.h"
#include "core/condition/Condition.hpp"

namespace vbs {

/**
 * @brief The GuardCondition class is a specific Condition whose trigger_value is completely under
 * the control of the application.
 *
 * The purpose of the GuardCondition is to provide the means for the application to manually wakeup
 * a WaitSet. This is accomplished by attaching the GuardCondition to the WaitSet and then setting
 * the trigger_value by means of the set_trigger_value operation.
 *
 */
class GuardCondition : public Condition {
 public:
    RTPS_DllAPI GuardCondition();

    RTPS_DllAPI ~GuardCondition();

    RTPS_DllAPI bool get_trigger_value() const override;

    /**
     * @brief Set the trigger_value
     * @param value new value for trigger
     * @return RETURN_OK
     */
    RTPS_DllAPI vbsutil::elog::ReturnCode_t set_trigger_value(const bool value);

 private:
    std::atomic<bool> trigger_value_;
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_CORE_CONDITION_GUARDCONDITION_HPP_
