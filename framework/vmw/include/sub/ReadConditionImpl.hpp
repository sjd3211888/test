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

#ifndef _EDDS_ENTITYREADCONDITIONIMPL_HPP_
#define _EDDS_ENTITYREADCONDITIONIMPL_HPP_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <algorithm>
#include <forward_list>
#include <memory>
#include <mutex>

#include "core/condition/ConditionNotifier.hpp"
#include "sub/InstanceState.hpp"
#include "sub/ReadCondition.hpp"
#include "sub/SampleState.hpp"
#include "sub/ViewState.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/DataReaderImpl/StateFilter.hpp"

namespace vbs {
namespace detail {

class ReadConditionImpl : public std::enable_shared_from_this<ReadConditionImpl> {
    vbs::DataReaderImpl& data_reader_;
    const vbs::detail::StateFilter state_;
    vbs::detail::StateFilter value_;
    mutable std::mutex value_mtx_;
    std::recursive_mutex& mutex_;
    std::forward_list<const vbs::ReadCondition*> conditions_;

    using length = std::forward_list<const vbs::ReadCondition*>::difference_type;

 public:
    ReadConditionImpl(vbs::DataReaderImpl& data_reader, const vbs::detail::StateFilter& state)
        : data_reader_(data_reader), state_(state), value_(), mutex_(data_reader.get_conditions_mutex()) {}

    ~ReadConditionImpl() {
        // On destruction no ReadCondition should be associated
        assert(conditions_.empty());
    }

    /**
     * Detach all ReadConditions from this object.
     */
    void detach_all_conditions() noexcept {
        std::lock_guard<std::recursive_mutex> _(mutex_);

        if (conditions_.empty()) {
            return;
        }

        try {
            auto keep_alive_in_stack = shared_from_this();
        } catch (const std::bad_weak_ptr& e) {
            return;
        }

        for (const vbs::ReadCondition* cond : conditions_) {
            delete cond;
        }

        conditions_.clear();
    }

    bool get_trigger_value(const vbs::detail::StateFilter& state) const noexcept {
        return (state.sample_states & state_.sample_states) && (state.view_states & state_.view_states) &&
               (state.instance_states & state_.instance_states);
    }

    bool get_trigger_value() const noexcept {
        std::lock_guard<std::mutex> _(value_mtx_);
        return get_trigger_value(value_);
    }

    vbs::DataReaderImpl& get_datareader() const noexcept { return data_reader_; }

    const vbs::SampleStateMask& get_sample_state_mask() const noexcept { return state_.sample_states; }

    const vbs::ViewStateMask& get_view_state_mask() const noexcept { return state_.view_states; }

    const vbs::InstanceStateMask& get_instance_state_mask() const noexcept { return state_.instance_states; }

    /**
     * Attach a new ReadCondition to this object.
     * @param [in] pRC reader to attach
     * @return RETCODE_OK on success
     */
    ReturnCode_t attach_condition(vbs::ReadCondition* pRC) {
        using namespace std;

        lock_guard<recursive_mutex> _(mutex_);

        auto it = conditions_.begin();
        auto pit = conditions_.before_begin();

        while (it != conditions_.end()) {
            if (*it < pRC) {
                pit = it++;
            } else if (*it == pRC) {
                // already there
                return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
            } else {
                break;
            }
        }

        // insert
        (void)conditions_.insert_after(pit, pRC);

        // associate
        std::shared_ptr<detail::ReadConditionImpl> tmp_impl;
        try {
            tmp_impl = shared_from_this();
        } catch (const std::bad_weak_ptr& e) {
            std::cerr << e.what() << '\n';
            return ReturnCode_t::RETCODE_ERROR;
        }
        pRC->impl_ = std::move(tmp_impl);

        return ReturnCode_t::RETCODE_OK;
    }

    /**
     * Detach a ReadCondition from this object.
     * @param [in] pRC reader to detach
     * @return RETCODE_OK on success
     */
    ReturnCode_t detach_condition(vbs::ReadCondition* pRC) noexcept {
        using namespace std;

        lock_guard<recursive_mutex> _(mutex_);

        auto it = conditions_.begin();
        auto pit = conditions_.before_begin();

        while (it != conditions_.end()) {
            if (*it < pRC) {
                pit = it++;
            } else if (*it == pRC) {
                (void)conditions_.erase_after(pit);

                return ReturnCode_t::RETCODE_OK;
            } else {
                break;
            }
        }

        return ReturnCode_t::RETCODE_PRECONDITION_NOT_MET;
    }

    void set_trigger_value(const vbs::detail::StateFilter& value) noexcept {
        std::lock_guard<std::mutex> _(value_mtx_);
        value_ = value;
    }

    /**
     * Notify all the associated ReadConditions
     */
    void notify() const noexcept {
        std::lock_guard<std::recursive_mutex> _(mutex_);

        for (auto cond : conditions_) {
            auto pN = cond->get_notifier();
            assert(nullptr != pN);
            pN->notify();
        }
    }
};

}  // namespace detail
}  // namespace vbs

#endif  // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif  /* _EDDS_ENTITYREADCONDITIONIMPL_HPP_ */
