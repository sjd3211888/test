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

#ifndef _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERHISTORYCOUNTERS_HPP_
#define _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERHISTORYCOUNTERS_HPP_

#include <cstdint>

namespace vbs {
namespace detail {

/// Book-keeping information for a DataReaderHistory
struct DataReaderHistoryCounters {
    /// Total number of read samples accesible from the history
    uint64_t samples_read = 0U;
    /// Total number of unread samples accesible from the history
    uint64_t samples_unread = 0U;

    /// Total number of instances with NEW_VIEW_STATE
    uint64_t instances_new = 0U;
    /// Total number of instances with NOT_NEW_VIEW_STATE
    uint64_t instances_not_new = 0U;

    /// Total number of instances with ALIVE_INSTANCE_STATE
    uint64_t instances_alive = 0U;
    /// Total number of instances with NOT_ALIVE_DISPOSED_INSTANCE_STATE
    uint64_t instances_disposed = 0U;
    /// Total number of instances with NOT_ALIVE_NO_WRITERS_INSTANCE_STATE
    uint64_t instances_no_writers = 0U;
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERHISTORYCOUNTERS_HPP_
