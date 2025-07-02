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

#ifndef _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERINSTANCE_HPP_
#define _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERINSTANCE_HPP_

#include <chrono>
#include <cstdint>

#include "sub/InstanceState.hpp"
#include "sub/ViewState.hpp"
#include "ertps/utils/collections/ResourceLimitedVector.hpp"
#include "edds/rtps/common/Guid.h"
#include "sub/DataReaderCacheChange.hpp"
#include "sub/DataReaderHistoryCounters.hpp"

namespace vbs {
namespace detail {

/// Book-keeping information for an instance
struct DataReaderInstance {
    using ChangeCollection = evbs::ertps::ResourceLimitedVector<vbs::detail::DataReaderCacheChange, std::true_type>;
    using WriterOwnership = std::pair<evbs::ertps::rtps::GUID_t, uint32_t>;
    using WriterCollection = evbs::ertps::ResourceLimitedVector<WriterOwnership, std::false_type>;

    //! A vector of DataReader changes belonging to the same instance
    ChangeCollection cache_changes;
    //! The list of alive writers for this instance
    WriterCollection alive_writers;
    //! GUID and strength of the current maximum strength writer
    WriterOwnership current_owner {{}, std::numeric_limits<uint32_t>::max()};
    //! The time when the group will miss the deadline
    std::chrono::steady_clock::time_point next_deadline_us;
    //! Current view state of the instance
    vbs::ViewStateKind view_state = vbs::ViewStateKind::NEW_VIEW_STATE;
    //! Current instance state of the instance
    vbs::InstanceStateKind instance_state = vbs::InstanceStateKind::ALIVE_INSTANCE_STATE;
    //! Current disposed generation of the instance
    int32_t disposed_generation_count = 0;
    //! Current no_writers generation of the instance
    int32_t no_writers_generation_count = 0;

    DataReaderInstance(const evbs::ertps::ResourceLimitedContainerConfig& changes_allocation,
                       const evbs::ertps::ResourceLimitedContainerConfig& writers_allocation)
        : cache_changes(changes_allocation), alive_writers(writers_allocation) {}

    void writer_update_its_ownership_strength(const evbs::ertps::rtps::GUID_t& writer_guid,
                                              const uint32_t ownership_strength) {
        if (writer_guid == current_owner.first) {
            // Check it is an "alive" writer.
            auto writer_it =
                std::find_if(alive_writers.begin(), alive_writers.end(),
                             [&writer_guid](const WriterOwnership& item) { return item.first == writer_guid; });

            if (alive_writers.end() == writer_it) {
                std::cerr << "writer_update_its_ownership_strength() cannot find alive writer." << std::endl;
                return;
            }

            // Update writer info
            (*writer_it).second = ownership_strength;
            current_owner.second = ownership_strength;
            update_owner();
        }

        return;
    }

    bool update_state(vbs::detail::DataReaderHistoryCounters& counters, const vbs::common::ChangeKind_t change_kind,
                      const evbs::ertps::rtps::GUID_t& writer_guid, const uint32_t ownership_strength) {
        bool ret_val = false;

        if (!has_been_accounted_) {
            has_been_accounted_ = true;
            assert(vbs::ViewStateKind::NEW_VIEW_STATE == view_state);
            ++counters.instances_new;
            assert(vbs::InstanceStateKind::ALIVE_INSTANCE_STATE == instance_state);
            ++counters.instances_alive;
        }

        switch (change_kind) {
            case vbs::common::ALIVE:
                ret_val = writer_alive(counters, writer_guid, ownership_strength);
                break;

            case vbs::common::NOT_ALIVE_DISPOSED:
                ret_val = writer_dispose(counters, writer_guid, ownership_strength);
                break;

            case vbs::common::NOT_ALIVE_DISPOSED_UNREGISTERED:
                ret_val = writer_dispose(counters, writer_guid, ownership_strength);
                ret_val |= writer_unregister(counters, writer_guid);
                break;

            case vbs::common::NOT_ALIVE_UNREGISTERED:
                ret_val = writer_unregister(counters, writer_guid);
                break;

            default:
                // TODO (Miguel C): log error / assert
                break;
        }

        return ret_val;
    }

    bool writer_removed(vbs::detail::DataReaderHistoryCounters& counters,
                        const evbs::ertps::rtps::GUID_t& writer_guid) {
        return has_been_accounted_ && writer_unregister(counters, writer_guid);
    }

    void deadline_missed() {
        if (evbs::ertps::rtps::c_Guid_Unknown != current_owner.first) {
            if (alive_writers.remove_if(
                    [&](const WriterOwnership& item) { return item.first == current_owner.first; })) {
                current_owner.second = 0U;
                current_owner.first = evbs::ertps::rtps::c_Guid_Unknown;
                if (alive_writers.empty() && (vbs::InstanceStateKind::ALIVE_INSTANCE_STATE == instance_state)) {
                    instance_state = vbs::InstanceStateKind::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
                }
                if (vbs::ALIVE_INSTANCE_STATE == instance_state) {
                    update_owner();
                }
            }
        }
    }

 private:
    //! Whether this instance has ever been included in the history counters
    bool has_been_accounted_ = false;

    bool writer_alive(vbs::detail::DataReaderHistoryCounters& counters, const evbs::ertps::rtps::GUID_t& writer_guid,
                      const uint32_t ownership_strength) {
        bool ret_val = false;

        if (writer_guid == current_owner.first)  // Accept sample of current owner.
        {
            current_owner.second = ownership_strength;
            ret_val = true;
        } else if (ownership_strength > current_owner.second)  // Accept sample of greater strength writer
        {
            current_owner.first = writer_guid;
            current_owner.second = ownership_strength;
            ret_val = true;
        } else if ((ownership_strength == current_owner.second) &&
                   (writer_guid < current_owner.first))  // Check if new writer has lower GUID.
        {
            current_owner.first = writer_guid;
            ret_val = true;
        } else if (std::numeric_limits<uint32_t>::max() ==
                   ownership_strength)  // uint32_t::max indicates we are in SHARED_OWNERSHIP_QOS.
        {
            assert(evbs::ertps::rtps::c_Guid_Unknown == current_owner.first);
            assert(std::numeric_limits<uint32_t>::max() == current_owner.second);
            ret_val = true;
        } else if (evbs::ertps::rtps::c_Guid_Unknown == current_owner.first)  // Without owner.
        {
            current_owner.first = writer_guid;
            current_owner.second = ownership_strength;
            ret_val = true;
        }

        if (ret_val) {
            if (vbs::InstanceStateKind::NOT_ALIVE_DISPOSED_INSTANCE_STATE == instance_state) {
                counters_update(counters.instances_disposed, counters.instances_alive, counters, true);

                ++disposed_generation_count;
                alive_writers.clear();
                view_state = vbs::ViewStateKind::NEW_VIEW_STATE;
            } else if (vbs::InstanceStateKind::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE == instance_state) {
                counters_update(counters.instances_no_writers, counters.instances_alive, counters, true);

                ++no_writers_generation_count;
                assert(0 == alive_writers.size());
                view_state = vbs::ViewStateKind::NEW_VIEW_STATE;
            }

            instance_state = vbs::InstanceStateKind::ALIVE_INSTANCE_STATE;
        }

        writer_set(writer_guid, ownership_strength);

        return ret_val;
    }

    bool writer_dispose(vbs::detail::DataReaderHistoryCounters& counters, const evbs::ertps::rtps::GUID_t& writer_guid,
                        const uint32_t ownership_strength) {
        bool ret_val = false;

        if ((ownership_strength >= current_owner.second) ||
            ((ownership_strength == current_owner.second) && (writer_guid < current_owner.first))) {
            if (std::numeric_limits<uint32_t>::max() != ownership_strength)  // Not SHARED_OWNERSHIP_QOS
            {
                current_owner.first = writer_guid;
                current_owner.second = ownership_strength;
            }

            if (vbs::InstanceStateKind::ALIVE_INSTANCE_STATE == instance_state) {
                ret_val = true;
                instance_state = vbs::InstanceStateKind::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
                counters_update(counters.instances_alive, counters.instances_disposed, counters, false);
            }
        }

        writer_set(writer_guid, ownership_strength);

        return ret_val;
    }

    bool writer_unregister(vbs::detail::DataReaderHistoryCounters& counters,
                           const evbs::ertps::rtps::GUID_t& writer_guid) {
        bool ret_val = false;

        if (alive_writers.remove_if(
                [&writer_guid](const WriterOwnership& item) { return item.first == writer_guid; })) {
            if (writer_guid == current_owner.first) {
                current_owner.second = 0U;
                current_owner.first = evbs::ertps::rtps::c_Guid_Unknown;
                if (vbs::ALIVE_INSTANCE_STATE == instance_state) {
                    update_owner();
                }
            }

            if (alive_writers.empty() && (vbs::InstanceStateKind::ALIVE_INSTANCE_STATE == instance_state)) {
                instance_state = vbs::InstanceStateKind::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
                counters_update(counters.instances_alive, counters.instances_no_writers, counters, false);
            }

            ret_val = true;
        }

        return ret_val;
    }

    void writer_set(const evbs::ertps::rtps::GUID_t& writer_guid, const uint32_t ownership_strength) {
        auto it = std::find_if(alive_writers.begin(), alive_writers.end(),
                               [&writer_guid](const WriterOwnership& item) { return item.first == writer_guid; });
        if (it == alive_writers.end()) {
            (void)alive_writers.emplace_back(writer_guid, ownership_strength);
        } else {
            it->second = ownership_strength;
        }
    }

    void counters_update(uint64_t& decremented_counter, uint64_t& incremented_counter,
                         vbs::detail::DataReaderHistoryCounters& counters, bool set_as_new_view_state) {
        --decremented_counter;
        ++incremented_counter;
        if (set_as_new_view_state && (vbs::ViewStateKind::NEW_VIEW_STATE != view_state)) {
            ++counters.instances_new;
            --counters.instances_not_new;
        }
    }

    void update_owner() {
        (void)std::for_each(alive_writers.begin(), alive_writers.end(), [&](const WriterOwnership& item) {
            if ((item.second > current_owner.second) ||
                ((item.second == current_owner.second) && (item.first < current_owner.first))) {
                current_owner = item;
            }
        });
    }
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYSUBSCRIBER_HISTORY_DATAREADERCACHECHANGE_HPP_
