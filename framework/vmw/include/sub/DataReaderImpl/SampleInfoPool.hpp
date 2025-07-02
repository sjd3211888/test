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
// feature: evbs compilation does not depend on framework header files
// ------------------------------------------------------------------

#ifndef _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLEINFOPOOL_HPP_
#define _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLEINFOPOOL_HPP_

#include <algorithm>
#include <cassert>

#include "deps/base/LoanableCollection.hpp"
#include "core/LoanableSequence.hpp"
#include "sub/SampleInfoInner.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "ertps/utils/collections/ResourceLimitedContainerConfig.hpp"
#include "ertps/utils/collections/ResourceLimitedVector.hpp"

namespace vbs {
namespace detail {

struct SampleInfoPool {
    explicit SampleInfoPool(const vbsutil::xmlparser::DataReaderQos& qos)
        : free_items_(qos.reader_resource_limits().sample_infos_allocation),
          used_items_(qos.reader_resource_limits().sample_infos_allocation) {
        UNUSED_PARAMETER(qos);
        for (size_t n = 0; n < qos.reader_resource_limits().sample_infos_allocation.initial; ++n) {
            (void)free_items_.push_back(new vbs::SampleInfoInner());
        }
    }

    ~SampleInfoPool() {
        for (vbs::SampleInfoInner* it : free_items_) {
            delete it;
        }
    }

    size_t num_allocated() { return used_items_.size(); }

    vbs::SampleInfoInner* get_item() {
        vbs::SampleInfoInner** result = nullptr;

        if (free_items_.empty()) {
            result = used_items_.push_back(new vbs::SampleInfoInner());
        } else {
            result = used_items_.push_back(free_items_.back());
            static_cast<void>(result);
            assert(result != nullptr);
            free_items_.pop_back();
        }

        return result ? *result : nullptr;
    }

    void return_item(vbs::SampleInfoInner* item) {
        bool removed = used_items_.remove(item);
        static_cast<void>(removed);
        assert(removed);

        vbs::SampleInfoInner** result = free_items_.push_back(item);
        static_cast<void>(result);
        assert(result != nullptr);
    }

 private:
    using collection_type = evbs::ertps::ResourceLimitedVector<vbs::SampleInfoInner*>;

    collection_type free_items_;
    collection_type used_items_;
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLEINFOPOOL_HPP_
