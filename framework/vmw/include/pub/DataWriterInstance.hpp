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

#ifndef _EDDS_ENTITYPUBLISHER_HISTORY_DATAWRITERINSTANCE_HPP_
#define _EDDS_ENTITYPUBLISHER_HISTORY_DATAWRITERINSTANCE_HPP_

#include <chrono>

namespace vbs {

namespace common {
class ChaneKind;
class CacheChange;
}  // namespace common

namespace detail {

/// Book-keeping information for an instance
struct DataWriterInstance {
    //! A vector of cache changes
    std::vector<vbs::common::CacheChange_t*> cache_changes;
    //! The time when the group will miss the deadline
    std::chrono::steady_clock::time_point next_deadline_us;
    //! Serialized payload for key holder
    vbsutil::xmlparser::SerializedPayload_t key_payload;

    DataWriterInstance() = default;

    DataWriterInstance(const DataWriterInstance&) = delete;
    DataWriterInstance& operator=(const DataWriterInstance&) = delete;

    DataWriterInstance(DataWriterInstance&&) = default;
    DataWriterInstance& operator=(DataWriterInstance&&) = default;

    bool is_registered() const {
        return cache_changes.empty() || ((vbs::common::NOT_ALIVE_UNREGISTERED != cache_changes.back()->kind) &&
                                         (vbs::common::NOT_ALIVE_DISPOSED_UNREGISTERED != cache_changes.back()->kind));
    }
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYPUBLISHER_HISTORY_DATAWRITERINSTANCE_HPP_
