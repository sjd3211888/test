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
// feature: listener code decoupling
// feature: heartbeat optimize
// ------------------------------------------------------------------

#include <chrono>
#include <limits>
#include <mutex>

#include "pub/DataWriterHistory.hpp"
#include "edds/rtps/common/InstanceHandle.h"
#include "edds/rtps/common/Time_t.h"
#include "elog/log/Log.h"
#include "edds/rtps/writer/RTPSWriter.h"
#include "deps/common/TypeSupport.hpp"

namespace vbs {

using namespace evbs::ertps;
using namespace evbs::ertps::rtps;
using namespace vbs::common;

static HistoryAttributes to_history_attributes(const TopicAttributes& topic_att, uint32_t payloadMaxSize,
                                               MemoryManagementPolicy_t mempolicy) {
    auto initial_samples = topic_att.qos.resource_limits().allocated_samples;
    auto max_samples = topic_att.qos.resource_limits().max_samples;
    auto extra_samples = topic_att.qos.resource_limits().extra_samples;

    if (topic_att.qos.history().kind != KEEP_ALL_HISTORY_QOS) {
        max_samples = topic_att.qos.history().depth;
        if (topic_att.getTopicKind() != NO_KEY) {
            max_samples *= topic_att.qos.resource_limits().max_instances;
        }
        // hanjie bug：当max_samples配置值小于初始值100时，错误。
        // initial_samples = std::min(initial_samples, max_samples);
    }
    // hanjie and
    initial_samples = std::min(initial_samples, max_samples);

    return HistoryAttributes(mempolicy, payloadMaxSize, initial_samples, max_samples, extra_samples);
}

DataWriterHistory::DataWriterHistory(
    const TopicAttributes& topic_att, uint32_t payloadMaxSize, MemoryManagementPolicy_t mempolicy,
    std::function<void(const evbs::ertps::rtps::InstanceHandle_t&)> unack_sample_remove_functor,
    const BatchQosPolicy& batch_qos)
    : WriterHistory(to_history_attributes(topic_att, payloadMaxSize, mempolicy), batch_qos),
      mp_writer(nullptr),
      history_qos_(topic_att.qos.history()),
      resource_limited_qos_(topic_att.qos.resource_limits()),
      topic_att_(topic_att),
      unacknowledged_sample_removed_functor_(std::move(unack_sample_remove_functor)) {
    if (resource_limited_qos_.max_instances == 0) {
        resource_limited_qos_.max_instances = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_samples_per_instance == 0) {
        resource_limited_qos_.max_samples_per_instance = std::numeric_limits<int32_t>::max();
    }
}

DataWriterHistory::~DataWriterHistory() {}

void DataWriterHistory::rebuild_instances() {
    if (topic_att_.getTopicKind() == WITH_KEY) {
        for (CacheChange_t* change : m_changes) {
            t_m_Inst_Caches::iterator vit;
            if (find_or_add_key(change->instanceHandle, change->serializedPayload, &vit)) {
                vit->second.cache_changes.push_back(change);
            }
        }
    }
}

bool DataWriterHistory::register_instance(const InstanceHandle_t& instance_handle, SerializedPayload_t*& payload) {
    payload = nullptr;

    /// Preconditions
    if (topic_att_.getTopicKind() == NO_KEY) {
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(mp_mutex);
    t_m_Inst_Caches::iterator vit;
    bool result = find_or_add_key(instance_handle, {}, &vit);
    if (result && vit != keyed_changes_.end()) {
        payload = &vit->second.key_payload;
    }
    return result;
}

evbs::ertps::rtps::SerializedPayload_t* DataWriterHistory::get_key_value(
    const evbs::ertps::rtps::InstanceHandle_t& handle) {
    t_m_Inst_Caches::iterator vit = keyed_changes_.find(handle);
    if ((vit != keyed_changes_.end()) && vit->second.is_registered()) {
        return &vit->second.key_payload;
    }
    return nullptr;
}

inline bool DataWriterHistory::history_capbility_check(
    const CacheChange_t* change, std::unique_lock<RecursiveTimedMutex>& lock,
    const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) {
    UNUSED_PARAMETER(max_blocking_time);
    UNUSED_PARAMETER(lock);
    if (!m_isHistoryFull) {
        return true;
    }

    bool ret = false;

    next_change_length(change->serializedPayload.length);
    if (history_qos_.kind == KEEP_ALL_HISTORY_QOS) {
        logDebug(PUBLISHER, "resource limit has been exceeded, DataWriterHistory is full!");
        lock.unlock();
        ret = this->mp_writer->try_remove_change(max_blocking_time, lock);
        if (!ret) {
            this->mp_writer->acknack_request();
        }
        lock.lock();
    } else if (history_qos_.kind == KEEP_LAST_HISTORY_QOS) {
        bool is_acked = true;
        InstanceHandle_t instance;
        if (unacknowledged_sample_removed_functor_) {
            instance = topic_att_.getTopicKind() == NO_KEY ? vbs::HANDLE_NIL : m_changes.front()->instanceHandle;
            lock.unlock();
            is_acked = change_is_acked_or_fully_delivered(m_changes.front());
            lock.lock();
        }
#ifdef BATCH_SEND_ENABLE
        if (get_batch_enable()) {
            ret = this->remove_min_batch_change(m_changes.back()->sequenceNumber);
        } else {
#else
        {
#endif
            ret = this->remove_min_change_nts();
        }

        // Notify if change has been removed unacknowledged
        if (unacknowledged_sample_removed_functor_ && ret) {
            if (!is_acked) {
                unacknowledged_sample_removed_functor_(instance);
            }
        }
    }

    if (!ret) {
        elogErrorKeyT(RTPS_HISTORY, evbs::ReturnCode_t::RETCODE_ERROR, topic_att_.getTopicName().c_str(),
                      "Attempting to add Data " << next_sequence_number()
                                                << " to Full WriterCache: " << topic_att_.getTopicName());
        return false;
    }
    return true;
}

inline bool DataWriterHistory::history_capbility_check_with_key(CacheChange_t* change,
                                                                std::unique_lock<RecursiveTimedMutex>& lock,
                                                                t_m_Inst_Caches::iterator& vit) {
    bool add = false;
    if (vit == keyed_changes_.end()) {
        return false;
    }
    if (vit->second.cache_changes.size() < static_cast<size_t>(history_qos_.depth)) {
        add = true;
    } else {
        InstanceHandle_t instance = change->instanceHandle;
        add = remove_change_pub_nts(vit->second.cache_changes.front());

        // Notify if removed unacknowledged
        if (unacknowledged_sample_removed_functor_ && add) {
            lock.unlock();
            bool is_acked = change_is_acked_or_fully_delivered(vit->second.cache_changes.front());
            lock.lock();
            if (!is_acked) {
                unacknowledged_sample_removed_functor_(instance);
            }
        }
    }
    return add;
}
inline bool DataWriterHistory::history_capbility_check_with_key(
    CacheChange_t* change, std::unique_lock<RecursiveTimedMutex>& lock,
    const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time, t_m_Inst_Caches::iterator& vit,
    bool& hasError) {
    UNUSED_PARAMETER(max_blocking_time);
    UNUSED_PARAMETER(lock);
    bool add = false;
    if (vit == keyed_changes_.end()) {
        return false;
    }
    if (vit->second.cache_changes.size() < static_cast<size_t>(resource_limited_qos_.max_samples_per_instance)) {
        add = true;
    } else {
        SequenceNumber_t seq_to_remove = vit->second.cache_changes.front()->sequenceNumber;
        lock.unlock();
        if (!mp_writer->wait_for_acknowledgement(seq_to_remove, max_blocking_time, lock)) {
            // Timeout waiting. Will not add change to history.
            hasError = true;
            lock.lock();
            return false;
        }
        lock.lock();

        // vit may have been invalidated
        if (!find_or_add_key(change->instanceHandle, change->serializedPayload, &vit)) {
            hasError = true;
            return false;
        }

        // If the change we were trying to remove was already removed, try again
        if (vit->second.cache_changes.empty() || vit->second.cache_changes.front()->sequenceNumber != seq_to_remove) {
            return false;
        }

        // Remove change if still present
        add = remove_change_pub_nts(vit->second.cache_changes.front());
    }
    return add;
}

bool DataWriterHistory::prepare_change(CacheChange_t* change, std::unique_lock<RecursiveTimedMutex>& lock,
                                       const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) {
    if (!history_capbility_check(change, lock, max_blocking_time)) {
        return false;
    }
    assert(!m_isHistoryFull);

    // For NO_KEY we can directly add the change
    bool add = (topic_att_.getTopicKind() == NO_KEY);
    if (topic_att_.getTopicKind() == WITH_KEY) {
        t_m_Inst_Caches::iterator vit;
        // For WITH_KEY, we take into account the limits on the instance
        // In case we wait for a sequence to be acknowledged, we try several times
        // until we reach the max blocking timepoint
        while (!add) {
            // We should have the instance
            if (!find_or_add_key(change->instanceHandle, change->serializedPayload, &vit)) {
                break;
            }

            if (history_qos_.kind == KEEP_LAST_HISTORY_QOS) {
                add = history_capbility_check_with_key(change, lock, vit);
            } else if (history_qos_.kind == KEEP_ALL_HISTORY_QOS) {
                bool hasError = false;
                add = history_capbility_check_with_key(change, lock, max_blocking_time, vit, hasError);
                if (hasError) {
                    break;
                }
            }
        }

        if (add && vit != keyed_changes_.end()) {
            vit->second.cache_changes.push_back(change);
        }
    }

    return add;
}

bool DataWriterHistory::add_pub_change(CacheChange_t* change, WriteParams& wparams,
                                       std::unique_lock<RecursiveTimedMutex>& lock,
                                       const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time,
                                       bool add_history) {
    bool returnedValue = false;
    bool add = true;
    if (add_history) {
        add = prepare_change(change, lock, max_blocking_time);
    }
    if (add) {
        if (this->add_change_nts(change, wparams, max_blocking_time, add_history)) {
            logDebug(RTPS_HISTORY, topic_att_.getTopicDataType()
                                       << " Change " << change->sequenceNumber
                                       << " added with key: " << change->instanceHandle << " and "
                                       << change->serializedPayload.length << " bytes");
            returnedValue = true;
        }
    }

    return returnedValue;
}

bool DataWriterHistory::find_or_add_key(const InstanceHandle_t& instance_handle, const SerializedPayload_t& payload,
                                        t_m_Inst_Caches::iterator* vit_out) {
    static_cast<void>(payload);

    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(instance_handle);
    if (vit != keyed_changes_.end()) {
        *vit_out = vit;
        return true;
    }

    if (static_cast<int>(keyed_changes_.size()) < resource_limited_qos_.max_instances) {
        const auto ret = keyed_changes_.insert(std::make_pair(instance_handle, vbs::detail::DataWriterInstance()));
        if (!ret.second) {
            return false;
        }
        vit = ret.first;
        if (vit == keyed_changes_.end()) {
            return false;
        }
        (void)vit->second.key_payload.copy(&payload, false);
        *vit_out = vit;
        return true;
    }
    logWarning(RTPS_HISTORY, "Cannot find instance_handle and size(" << static_cast<int>(keyed_changes_.size())
                                                                     << ") >= resource_limited_qos_.max_instances("
                                                                     << resource_limited_qos_.max_instances << ").");
    return false;
}

bool DataWriterHistory::removeAllChange(size_t* removed) {
    size_t rem = 0U;
    std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);

    while (m_changes.size() > 0U) {
        if (remove_change_pub_nts(m_changes.front())) {
            ++rem;
        } else {
            break;
        }
    }
    if (removed != nullptr) {
        *removed = rem;
    }
    if (rem > 0U) {
        return true;
    }
    return false;
}

bool DataWriterHistory::removeMinChange() {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);
    if (m_changes.size() > 0U) {
        return remove_change_pub_nts(m_changes.front());
    }
    return false;
}

bool DataWriterHistory::remove_change_pub_nts(CacheChange_t* change) {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }

    MessageLog(topic_att_.getTopicName().to_string(), change->writerGUID, MSG_TRACE_TYPE_SEND_QUE_OUT,
               change->sequenceNumber.to64long(), change->sourceTimestamp, change->serializedPayload.length);

    if (topic_att_.getTopicKind() == NO_KEY) {
        if (remove_change_nts(change)) {
            m_isHistoryFull = false;
            return true;
        }

        return false;
    } else {
        t_m_Inst_Caches::iterator vit;
        vit = keyed_changes_.find(change->instanceHandle);
        if (vit == keyed_changes_.end()) {
            return false;
        }

        for (auto chit = vit->second.cache_changes.begin(); chit != vit->second.cache_changes.end(); ++chit) {
            if (((*chit)->sequenceNumber == change->sequenceNumber) && ((*chit)->writerGUID == change->writerGUID)) {
                if (remove_change_nts(change)) {
                    (void)vit->second.cache_changes.erase(chit);
                    m_isHistoryFull = false;
                    return true;
                }
            }
        }
        elogErrorT(PUBLISHER, RetCode_t::RETCODE_ERROR, "Change not found, something is wrong");
    }
    return false;
}

bool DataWriterHistory::remove_change_pub(CacheChange_t* change) {
    std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);
    return remove_change_pub_nts(change);
}

bool DataWriterHistory::remove_change_g_nts(CacheChange_t* a_change) {
    return remove_change_pub_nts(a_change);
}

bool DataWriterHistory::remove_instance_changes(const InstanceHandle_t& handle, const SequenceNumber_t& seq_up_to) {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }

    if (topic_att_.getTopicKind() == NO_KEY) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_ILLEGAL_OPERATION,
                   "Cannot be removed instance changes of a NO_KEY DataType");
        return false;
    }

    std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(handle);
    if (vit == keyed_changes_.end()) {
        return false;
    }

    auto chit = vit->second.cache_changes.begin();
    for (; chit != vit->second.cache_changes.end() && (*chit)->sequenceNumber <= seq_up_to; ++chit) {
        MessageLog(topic_att_.getTopicName().to_string(), (*chit)->writerGUID, MSG_TRACE_TYPE_SEND_QUE_OUT,
                   (*chit)->sequenceNumber.to64long(), (*chit)->sourceTimestamp, (*chit)->serializedPayload.length);
        if (remove_change_nts(*chit)) {
            m_isHistoryFull = false;
        }
    }

    (void)vit->second.cache_changes.erase(vit->second.cache_changes.begin(), chit);

    if (vit->second.cache_changes.empty()) {
        (void)keyed_changes_.erase(vit);
    }

    return true;
}

bool DataWriterHistory::set_next_deadline_nts(const InstanceHandle_t& handle,
                                              const std::chrono::steady_clock::time_point& next_deadline_us) {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }
    //std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);

    if (topic_att_.getTopicKind() == NO_KEY) {
        next_deadline_us_ = next_deadline_us;
        return true;
    } else if (topic_att_.getTopicKind() == WITH_KEY) {
        if (keyed_changes_.find(handle) == keyed_changes_.end()) {
            return false;
        }

        keyed_changes_[handle].next_deadline_us = next_deadline_us;
        return true;
    }

    return false;
}

bool DataWriterHistory::get_next_deadline_nts(InstanceHandle_t& handle,
                                              std::chrono::steady_clock::time_point& next_deadline_us) {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }
    //std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);

    if (topic_att_.getTopicKind() == WITH_KEY) {
        auto min = std::min_element(keyed_changes_.begin(), keyed_changes_.end(),
                                    [](const t_m_Inst_Caches::value_type& lhs, const t_m_Inst_Caches::value_type& rhs) {
                                        return lhs.second.next_deadline_us < rhs.second.next_deadline_us;
                                    });
        if (min != keyed_changes_.end()) {
            handle = min->first;
            next_deadline_us = min->second.next_deadline_us;
            return true;
        } else {
            elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_ERROR, "keyed_changes_ is empty.");
            return false;
        }
    } else if (topic_att_.getTopicKind() == NO_KEY) {
        next_deadline_us = next_deadline_us_;
        return true;
    }

    return false;
}

bool DataWriterHistory::is_key_registered(const InstanceHandle_t& handle) {
    if ((mp_writer == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogErrorT(RTPS_HISTORY, RetCode_t::RETCODE_NOT_ENABLED,
                   "You need to create a Writer with this History before using it");
        return false;
    }
    std::lock_guard<RecursiveTimedMutex> guard(this->mp_mutex);
    t_m_Inst_Caches::iterator vit;
    vit = keyed_changes_.find(handle);
    return (vit != keyed_changes_.end()) && vit->second.is_registered();
}

bool DataWriterHistory::wait_for_acknowledgement_last_change(
    const InstanceHandle_t& handle, std::unique_lock<RecursiveTimedMutex>& lock,
    const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time) {
    if (WITH_KEY == topic_att_.getTopicKind()) {
        // Find the instance
        t_m_Inst_Caches::iterator vit = keyed_changes_.find(handle);
        if (vit != keyed_changes_.end()) {
            SequenceNumber_t seq = vit->second.cache_changes.back()->sequenceNumber;
            lock.unlock();
            bool ret = mp_writer->wait_for_acknowledgement(seq, max_blocking_time, lock);
            lock.lock();
            return ret;
        }
    }
    return false;
}

bool DataWriterHistory::change_is_acked_or_fully_delivered(const CacheChange_t* change) {
    return mp_writer->change_is_acked_or_fully_delivered(change);
}

}  // namespace vbs
