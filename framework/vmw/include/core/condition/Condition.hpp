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

#ifndef INCLUDE_EDDS_DDS_CORE_CONDITION_CONDITION_HPP_
#define INCLUDE_EDDS_DDS_CORE_CONDITION_CONDITION_HPP_

#include <memory>
#include <vector>

#include "ertps/ertps_dll.h"
#include "elog/Log.hpp"

namespace vbs {

namespace detail {
struct ConditionNotifier;
}  // namespace detail

/**
 * @brief The Condition class is the root base class for all the conditions that may be attached to
 * a WaitSet.
 */
class Condition {
 public:
    /**
     * @brief Retrieves the trigger_value of the Condition
     * @return true if trigger_value is set to 'true', 'false' otherwise
     */
    RTPS_DllAPI virtual bool get_trigger_value() const {
        logWarning(CONDITION, "get_trigger_value public member function not implemented");
        return false;  // TODO return trigger value
    }

    vbs::detail::ConditionNotifier* get_notifier() const { return notifier_.get(); }

 protected:
    Condition();
    virtual ~Condition();

    std::unique_ptr<detail::ConditionNotifier> notifier_;
};

using ConditionSeq = std::vector<Condition*>;

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_CORE_CONDITION_CONDITION_HPP_
