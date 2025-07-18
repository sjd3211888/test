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

#ifndef INCLUDE_EDDS_DDS_SUBSCRIBER_READCONDITION_HPP_
#define INCLUDE_EDDS_DDS_SUBSCRIBER_READCONDITION_HPP_

#include <cassert>

#include "core/condition/Condition.hpp"
#include "sub/InstanceState.hpp"
#include "sub/SampleState.hpp"
#include "sub/ViewState.hpp"
#include "ertps/ertps_dll.h"

namespace vbs {
class DataReaderImpl;

namespace detail {
class ReadConditionImpl;
}  // namespace detail

/**
 * @brief A Condition specifically dedicated to read operations and attached to one DataReader.
 *
 * ReadCondition objects allow an application to specify the data samples it is interested in (by
 * specifying the desired sample_states, view_states, and instance_states). The condition will only
 * be triggered when suitable information is available. They are to be used in conjunction with a
 * WaitSet as normal conditions. More than one ReadCondition may be attached to the same DataReader.
 */
class ReadCondition : public vbs::Condition {
    friend class vbs::detail::ReadConditionImpl;

 public:
    ReadCondition();

    ~ReadCondition() override;

    // Non-copyable
    ReadCondition(const ReadCondition&) = delete;
    ReadCondition& operator=(const ReadCondition&) = delete;

    // Non-movable
    ReadCondition(ReadCondition&&) = delete;
    ReadCondition& operator=(ReadCondition&&) = delete;

    /**
     * @brief Retrieves the trigger_value of the Condition
     * @return true if trigger_value is set to 'true', 'false' otherwise
     */
    RTPS_DllAPI bool get_trigger_value() const noexcept override;

    /**
     * @brief Retrieves the DataReader associated with the ReadCondition.
     *
     * Note that there is exactly one DataReader associated with each ReadCondition.
     *
     * @return pointer to the DataReader associated with this ReadCondition.
     */
    RTPS_DllAPI vbs::DataReaderImpl& get_datareader() const noexcept;

    /**
     * @brief Retrieves the set of sample_states taken into account to determine the trigger_value
     * of this condition.
     *
     * @return the sample_states specified when the ReadCondition was created.
     */
    RTPS_DllAPI vbs::SampleStateMask get_sample_state_mask() const noexcept;

    /**
     * @brief Retrieves the set of view_states taken into account to determine the trigger_value of
     * this condition.
     *
     * @return the view_states specified when the ReadCondition was created.
     */
    RTPS_DllAPI vbs::ViewStateMask get_view_state_mask() const noexcept;

    /**
     * @brief Retrieves the set of instance_states taken into account to determine the trigger_value
     * of this condition.
     *
     * @return the instance_states specified when the ReadCondition was created.
     */
    RTPS_DllAPI vbs::InstanceStateMask get_instance_state_mask() const noexcept;

    vbs::detail::ReadConditionImpl* get_impl() const noexcept {
        assert((bool)impl_);
        return impl_.get();
    }

 protected:
    //! Class implementation
    std::shared_ptr<vbs::detail::ReadConditionImpl> impl_;
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_SUBSCRIBER_READCONDITION_HPP_
