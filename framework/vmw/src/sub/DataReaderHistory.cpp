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
// feature: listener code decoupling
// feature: heartbeat optimize
// ------------------------------------------------------------------

#include <limits>
#include <memory>
#include <mutex>

#include "sub/DataReaderHistory.hpp"
#include "elog/log/Log.h"
#include "edds/rtps/reader/RTPSReader.h"
#include "topic/TopicDescription.hpp"
#include "deps/common/TypeSupport.hpp"
#include "deps/base/ChangeComparison.hpp"
#include "deps/common/sorted_vector_insert.hpp"

using namespace evbs::ertps::rtps;
using namespace vbs::common;
namespace vbs {
namespace detail {

// evbs::ertps::ResourceLimitedContainerConfig DEFAULT_MATCHED_PUBLISHER_ALLOCATION{0U, 200U, 1U};

static HistoryAttributes to_history_attributes(const TypeSupport& type, const vbsutil::xmlparser::DataReaderQos& qos) {
    auto initial_samples = qos.resource_limits().allocated_samples;
    auto max_samples = qos.resource_limits().max_samples;

    auto mempolicy = qos.endpoint().history_memory_policy;
    auto extra_samples = qos.resource_limits().extra_samples;

    const uint32_t PossibleAlignment = 3;
    uint32_t payloadMaxSize = type->m_typeSize;
    uint32_t spaceRemaining = std::numeric_limits<uint32_t>::max() - payloadMaxSize;

    if (spaceRemaining >= PossibleAlignment) {
        payloadMaxSize += PossibleAlignment;
    } else {
        payloadMaxSize = std::numeric_limits<uint32_t>::max();
    }
    return HistoryAttributes(mempolicy, payloadMaxSize, initial_samples, max_samples, extra_samples);
}

DataReaderHistory::DataReaderHistory(const TypeSupport& type, const vbs::TopicDescription& topic,
                                     const vbsutil::xmlparser::DataReaderQos& qos)
    : ReaderHistory(to_history_attributes(type, qos)),
      has_keys_(type->m_isGetKeyDefined),
      destination_order_(qos.destination_order()),
      key_writers_allocation_(qos.reader_resource_limits().matched_publisher_allocation),
      history_qos_(qos.history()),
      resource_limited_qos_(qos.resource_limits()),
      topic_name_(topic.get_name()),
      type_name_(topic.get_type_name()),
      type_(type.get()),
      get_key_object_(nullptr) {
    if (resource_limited_qos_.max_samples == 0) {
        resource_limited_qos_.max_samples = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_instances == 0) {
        resource_limited_qos_.max_instances = std::numeric_limits<int32_t>::max();
    }

    if (resource_limited_qos_.max_samples_per_instance == 0) {
        resource_limited_qos_.max_samples_per_instance = std::numeric_limits<int32_t>::max();
    }

    if (type_->m_isGetKeyDefined) {
        get_key_object_ = type_->createData();

        if (resource_limited_qos_.max_samples_per_instance < std::numeric_limits<int32_t>::max()) {
            key_changes_allocation_.maximum = static_cast<size_t>(resource_limited_qos_.max_samples_per_instance);
        }
    } else {
        resource_limited_qos_.max_instances = 1;
        resource_limited_qos_.max_samples_per_instance = resource_limited_qos_.max_samples;
        key_changes_allocation_.initial = 0;
        key_changes_allocation_.maximum = 0;

        (void)instances_.emplace(c_InstanceHandle_Unknown, std::make_shared<vbs::detail::DataReaderInstance>(
                                                               key_changes_allocation_, key_writers_allocation_));
        auto instances_copy = instances_[c_InstanceHandle_Unknown];
        data_available_instances_[c_InstanceHandle_Unknown] = std::move(instances_copy);
    }

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;

    receive_fn_ = qos.history().kind == KEEP_ALL_HISTORY_QOS
                      ? std::bind(&DataReaderHistory::received_change_keep_all, this, _1, _2, _3)
                      : std::bind(&DataReaderHistory::received_change_keep_last, this, _1, _2, _3);

    complete_fn_ = qos.history().kind == KEEP_ALL_HISTORY_QOS
                       ? std::bind(&DataReaderHistory::completed_change_keep_all, this, _1, _2, _3, _4)
                       : std::bind(&DataReaderHistory::completed_change_keep_last, this, _1, _2, _3, _4);

    if (!has_keys_) {
        compute_key_for_change_fn_ = [](CacheChange_t* change) {
            change->instanceHandle = c_InstanceHandle_Unknown;
            return true;
        };
    } else {
        compute_key_for_change_fn_ = [this](CacheChange_t* a_change) {
            if (!a_change->is_fully_assembled()) {
                return false;
            }

            if (a_change->instanceHandle.isDefined()) {
                return true;
            }

            if (type_ != nullptr) {
                logDebug(SUBSCRIBER, "Getting Key of change with no Key transmitted");
                void* key_object;

                if (a_change->serializedPayload.data[2] == 1) {
                    key_object = a_change->serializedPayload.data + 4u;
                } else {
                    (void)type_->deserialize(&a_change->serializedPayload, get_key_object_);
                    key_object = get_key_object_;
                }
                bool is_key_protected = false;
#if HAVE_SECURITY
                is_key_protected = mp_reader->getAttributes().endpoint.security_attributes().is_key_protected;
#endif  // if HAVE_SECURITY
                return type_->getKey(key_object, &a_change->instanceHandle, is_key_protected);
            }

            logWarning(SUBSCRIBER, "NO KEY in topic: " << topic_name_ << " and no method to obtain it";);
            return false;
        };
    }
}

DataReaderHistory::~DataReaderHistory() {
    if (type_->m_isGetKeyDefined) {
        type_->deleteData(get_key_object_);
    }
}

bool DataReaderHistory::received_change(CacheChange_t* change, size_t unknown_missing_changes_up_to,
                                        bool update_instance) {
    evbs::edds::dds::SampleRejectedStatusKind rejection_reason;
    return received_change(change, unknown_missing_changes_up_to, rejection_reason, update_instance);
}

bool DataReaderHistory::received_change(CacheChange_t* a_change, size_t unknown_missing_changes_up_to,
                                        evbs::edds::dds::SampleRejectedStatusKind& rejection_reason,
                                        bool update_instance) {
    bool ret_value = false;
    rejection_reason = NOT_REJECTED;
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    if (mp_reader == nullptr) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_NOT_ENABLED,
                  "You need to create a Reader with this History before using it");
        return false;
    }

    if ((0U == unknown_missing_changes_up_to) ||
        (m_changes.size() + unknown_missing_changes_up_to < static_cast<size_t>(resource_limited_qos_.max_samples))) {
        ret_value = receive_fn_(a_change, unknown_missing_changes_up_to, rejection_reason);
        //best_effort加入history之后就处理instance状态，不等on_data_available
        if (ret_value && update_instance) {
            if (!update_instance_nts(a_change)) {
                (void)remove_change_sub_nts(a_change, false);
                rejection_reason = REJECTED_BY_OWERSHIP;
                ret_value = false;
            }
        }
    } else {
        rejection_reason = REJECTED_BY_SAMPLES_LIMIT;
        elogWarningKeyT(SUBSCRIBER, topic_name_.to_string(),
                        "Topic " << topic_name_.to_string() << ", resource limit:max samples has been exceeded! "
                                 << resource_limited_qos_.max_samples);
    }

    return ret_value;
}

bool DataReaderHistory::received_change_keep_all(CacheChange_t* a_change, size_t unknown_missing_changes_up_to,
                                                 evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    if (!has_keys_ || !compute_key_for_change_fn_(a_change)) {
        // Store the sample temporally only in ReaderHistory. When completed it will be stored in
        // DataReaderHistory too.
        return add_to_reader_history_if_not_full(a_change, rejection_reason);
    }

    bool ret_value = false;
    InstanceCollection::iterator vit;
    if (find_key(a_change->instanceHandle, vit)) {
        vbs::detail::DataReaderInstance::ChangeCollection& instance_changes = vit->second->cache_changes;
        size_t total_size = instance_changes.size() + unknown_missing_changes_up_to;
        if (total_size < static_cast<size_t>(resource_limited_qos_.max_samples_per_instance)) {
            ret_value = add_received_change_with_key(a_change, *vit->second, rejection_reason);
        } else {
            logDebug(SUBSCRIBER, "Change not added due to maximum number "
                                     << resource_limited_qos_.max_samples_per_instance << " of samples per instance");
            rejection_reason = REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT;
        }

    } else {
        rejection_reason = REJECTED_BY_INSTANCES_LIMIT;
    }

    return ret_value;
}

bool DataReaderHistory::received_change_keep_last(CacheChange_t* a_change, size_t,
                                                  evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    if (!compute_key_for_change_fn_(a_change)) {
        // Store the sample temporally only in ReaderHistory. When completed it will be stored in
        // SubscriberHistory too.
        return add_to_reader_history_if_not_full(a_change, rejection_reason);
    }
    bool ret_value = false;
    if (!has_keys_) {
        if (m_changes.size() < static_cast<size_t>(history_qos_.depth)) {
            ret_value = true;
        } else {
            // Try to substitute the oldest sample.
            CacheChange_t* first_change = m_changes.at(0U);
            if (a_change->sourceTimestamp >= first_change->sourceTimestamp) {
                // As the instance is ordered by source timestamp, we can always remove the first
                // one.
                if (!first_change->isRead) {
                    logDebug(DATA_READER, "Topic " << topic_name_.to_string() << " keep last: remove the oldest cache "
                                                   << first_change->sequenceNumber);
                }
                ret_value = remove_change_sub_nts(first_change);
            } else {
                // Received change is older than oldest, and should be discarded
                logDebug(DATA_READER, "Topic " << topic_name_.to_string() << " keep last: receive older change seq:"
                                               << a_change->sequenceNumber << " time:" << a_change->sourceTimestamp
                                               << " first seq:" << first_change->sequenceNumber
                                               << " time:" << first_change->sourceTimestamp);
                rejection_reason = REJECTED_BY_DEPTH_LIMIT;
                return false;
            }
        }

        if (ret_value) {
            ret_value = add_to_reader_history_if_not_full(a_change, rejection_reason);
        }
    } else {
        InstanceCollection::iterator vit;
        if (find_key(a_change->instanceHandle, vit)) {
            vbs::detail::DataReaderInstance::ChangeCollection& instance_changes = vit->second->cache_changes;
            if (instance_changes.size() < static_cast<size_t>(history_qos_.depth)) {
                ret_value = true;
            } else {
                // Try to substitute the oldest sample.
                CacheChange_t* first_change = instance_changes.at(0U);
                if (a_change->sourceTimestamp >= first_change->sourceTimestamp) {
                    // As the instance is ordered by source timestamp, we can always remove the first
                    // one.
                    if (!first_change->isRead) {
                        logDebug(DATA_READER, "Topic " << topic_name_.to_string()
                                                       << " keep last: remove the oldest cache "
                                                       << first_change->sequenceNumber);
                    }
                    ret_value = remove_change_sub_nts(first_change);
                } else {
                    // Received change is older than oldest, and should be discarded
                    logDebug(DATA_READER, "Topic " << topic_name_.to_string() << " keep last: receive older change seq:"
                                                   << a_change->sequenceNumber << " time:" << a_change->sourceTimestamp
                                                   << " first seq:" << first_change->sequenceNumber
                                                   << " time:" << first_change->sourceTimestamp);
                    rejection_reason = REJECTED_BY_DEPTH_LIMIT;
                    return false;
                }
            }

            if (ret_value) {
                ret_value = add_received_change_with_key(a_change, *vit->second, rejection_reason);
            }
        } else {
            rejection_reason = REJECTED_BY_INSTANCES_LIMIT;
        }
    }

    return ret_value;
}

bool DataReaderHistory::add_received_change_with_key(CacheChange_t* a_change, vbs::detail::DataReaderInstance& instance,
                                                     evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    bool ret_value = add_to_reader_history_if_not_full(a_change, rejection_reason);

    if (ret_value) {
        add_to_instance(a_change, instance);
    }

    return ret_value;
}

bool DataReaderHistory::add_to_reader_history_if_not_full(CacheChange_t* a_change,
                                                          evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    if (m_isHistoryFull) {
        // Discarding the sample.
        logDebug(SUBSCRIBER, "Attempting to add Data to Full ReaderHistory: " << type_name_);
        rejection_reason = REJECTED_BY_SAMPLES_LIMIT;
        return false;
    }

    bool ret_value = add_change_nts(a_change);
    if (m_changes.size() == static_cast<size_t>(m_att.maximumReservedCaches)) {
        m_isHistoryFull = true;
    }
    return ret_value;
}

inline void DataReaderHistory::add_to_instance(CacheChange_t* a_change, vbs::detail::DataReaderInstance& instance) {
    // ADD TO KEY VECTOR
    vbs::detail::DataReaderCacheChange item = a_change;

    if (destination_order_.kind == BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS) {
        evbs::utilities::collections::sorted_vector_insert_reception(instance.cache_changes, item,
                                                                     vbs::common::history_order_reception_cmp);
    } else {
        evbs::utilities::collections::sorted_vector_insert(instance.cache_changes, item,
                                                           vbs::common::history_order_cmp);
    }
    auto instances_copy = instances_[a_change->instanceHandle];
    data_available_instances_[a_change->instanceHandle] = std::move(instances_copy);

    MessageLog(topic_name_.to_string(), a_change->writerGUID, MSG_TRACE_TYPE_RECV_QUE_IN,
               a_change->sequenceNumber.to64long(), a_change->sourceTimestamp, a_change->serializedPayload.length);
    logDebug(SUBSCRIBER, topic_name_.to_string()
                             << " " << guid.entityId << ": Change " << a_change->sequenceNumber
                             << " added from: " << a_change->writerGUID << " with KEY: " << a_change->instanceHandle;);
}

bool DataReaderHistory::find_key(const InstanceHandle_t& handle, InstanceCollection::iterator& vit_out) {
    InstanceCollection::iterator vit;
    vit = instances_.find(handle);
    if (vit != instances_.end()) {
        vit_out = vit;
        return true;
    }

    if (instances_.size() < static_cast<size_t>(resource_limited_qos_.max_instances)) {
        vit_out = instances_
                      .emplace(handle, std::make_shared<vbs::detail::DataReaderInstance>(key_changes_allocation_,
                                                                                         key_writers_allocation_))
                      .first;
        return true;
    }

    for (vit = instances_.begin(); vit != instances_.end(); ++vit) {
        if (vbs::InstanceStateKind::ALIVE_INSTANCE_STATE != vit->second->instance_state) {
            (void)data_available_instances_.erase(vit->first);
            (void)instances_.erase(vit);
            vit_out = instances_
                          .emplace(handle, std::make_shared<vbs::detail::DataReaderInstance>(key_changes_allocation_,
                                                                                             key_writers_allocation_))
                          .first;
            return true;
        }
    }

    logDebug(SUBSCRIBER, "History has reached the maximum number of instances");
    return false;
}

void DataReaderHistory::writer_unmatched(const GUID_t& writer_guid, const SequenceNumber_t& last_notified_seq) {
    // Remove all future changes from the unmatched writer
    remove_changes_with_pred([&writer_guid, &last_notified_seq](CacheChange_t* ch) {
        return (writer_guid == ch->writerGUID) && (last_notified_seq < ch->sequenceNumber);
    });
}

bool DataReaderHistory::remove_change_sub_nts(CacheChange_t* change, bool release) {
    if ((mp_reader == nullptr) /* || (mp_mutex == nullptr)*/) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_NOT_ENABLED,
                  "You need to create a Reader with this History before using it");
        return false;
    }

    MessageLog(topic_name_.to_string(), change->writerGUID, MSG_TRACE_TYPE_RECV_QUE_OUT,
               change->sequenceNumber.to64long(), change->sourceTimestamp, change->serializedPayload.length);

    if (has_keys_) {
        bool found = false;
        InstanceCollection::iterator vit;
        if (find_key(change->instanceHandle, vit)) {
            for (auto chit = vit->second->cache_changes.begin(); chit != vit->second->cache_changes.end(); ++chit) {
                if (((*chit)->sequenceNumber == change->sequenceNumber) &&
                    ((*chit)->writerGUID == change->writerGUID)) {
                    (void)vit->second->cache_changes.erase(chit);
                    found = true;

                    if (change->isRead) {
                        --counters_.samples_read;
                    }
                    break;
                }
            }
        }
        if (!found) {
            elogError(SUBSCRIBER, RetCode_t::RETCODE_ERROR, "Change not found on this key, something is wrong");
        }
    }
    if (ReaderHistory::remove_change_nts(change, release)) {
        counters_.samples_unread = get_unread_count_inner(false);
        return true;
    }

    return false;
}

bool DataReaderHistory::remove_change_sub(CacheChange_t* change, bool release) {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    return remove_change_sub_nts(change, release);
}

bool DataReaderHistory::remove_change_sub_no_key_nts(CacheChange_t* change, std::vector<CacheChange_t*>::iterator& it) {
    MessageLog(topic_name_.to_string(), change->writerGUID, MSG_TRACE_TYPE_RECV_QUE_OUT,
               change->sequenceNumber.to64long(), change->sourceTimestamp, change->serializedPayload.length);
    if (change->isRead) {
        --counters_.samples_read;
    }
    it = ReaderHistory::remove_change_nts(it, true, false);
    m_isHistoryFull = false;
    counters_.samples_unread = get_unread_count_inner(false);
    return true;
}

bool DataReaderHistory::remove_change_sub_nts(CacheChange_t* change,
                                              vbs::detail::DataReaderInstance::ChangeCollection::iterator& it) {
    if ((mp_reader == nullptr) /* || (mp_mutex == nullptr)*/) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_NOT_ENABLED,
                  "You need to create a Reader with this History before using it");
        return false;
    }

    MessageLog(topic_name_.to_string(), change->writerGUID, MSG_TRACE_TYPE_RECV_QUE_OUT,
               change->sequenceNumber.to64long(), change->sourceTimestamp, change->serializedPayload.length);

    if (has_keys_) {
        bool found = false;
        InstanceCollection::iterator vit;
        if (find_key(change->instanceHandle, vit)) {
            for (auto chit = vit->second->cache_changes.begin(); chit != vit->second->cache_changes.end(); ++chit) {
                if ((*chit)->sequenceNumber == change->sequenceNumber && (*chit)->writerGUID == change->writerGUID) {
                    assert(it == chit);
                    it = vit->second->cache_changes.erase(chit);
                    found = true;

                    if (change->isRead) {
                        --counters_.samples_read;
                    }
                    break;
                }
            }
        }
        if (!found) {
            elogError(SUBSCRIBER, RetCode_t::RETCODE_ERROR, "Change not found on this key, something is wrong");
        }
    }

    const_iterator chit = find_change_nts(change);
    if (chit == changesEnd()) {
        logDebug(RTPS_WRITER_HISTORY, "Trying to remove a change not in history");
        return false;
    }

    m_isHistoryFull = false;
    (void)ReaderHistory::remove_change_nts(chit, true, false);

    counters_.samples_unread = get_unread_count_inner(false);
    return true;
}

bool DataReaderHistory::remove_change_sub(CacheChange_t* change,
                                          vbs::detail::DataReaderInstance::ChangeCollection::iterator& it) {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    return remove_change_sub_nts(change, it);
}

bool DataReaderHistory::set_next_deadline_nts(const InstanceHandle_t& handle,
                                              const std::chrono::steady_clock::time_point& next_deadline_us,
                                              bool deadline_missed) {
    if ((mp_reader == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_NOT_ENABLED,
                  "You need to create a Reader with this History before using it");
        return false;
    }

    auto it = instances_.find(handle);
    if (it == instances_.end()) {
        return false;
    }

    if (deadline_missed) {
        it->second->deadline_missed();
    }
    it->second->next_deadline_us = next_deadline_us;
    return true;
}

bool DataReaderHistory::get_next_deadline_nts(InstanceHandle_t& handle,
                                              std::chrono::steady_clock::time_point& next_deadline_us) {
    if ((mp_reader == nullptr) /*|| (mp_mutex == nullptr)*/) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_NOT_ENABLED,
                  "You need to create a Reader with this History before using it");
        return false;
    }

    auto min =
        std::min_element(instances_.begin(), instances_.end(),
                         [](const InstanceCollection::value_type& lhs, const InstanceCollection::value_type& rhs) {
                             return lhs.second->next_deadline_us < rhs.second->next_deadline_us;
                         });

    if (min == instances_.end()) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_ERROR, "The instances_ may be empty.");
        return false;
    }
    handle = min->first;
    next_deadline_us = min->second->next_deadline_us;
    return true;
}

uint64_t DataReaderHistory::get_unread_count_inner(bool mark_as_read) {
    uint64_t ret_val = total_unread_;

    if (mark_as_read) {
        for (auto it = changesBegin(); (0U < total_unread_) && (it != changesEnd()); ++it) {
            CacheChange_t* change = *it;
            if ((!change->isRead))  // && (get_last_notified(change->writerGUID) >= change->sequenceNumber))
            {
                change->isRead = true;
                assert(0 < total_unread_);
                --total_unread_;
            }
        }
        assert(0 == total_unread_);
    }

    return ret_val;
}

uint64_t DataReaderHistory::get_unread_count(bool mark_as_read) {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    uint64_t ret_val = get_unread_count_inner(mark_as_read);
    assert(ret_val == counters_.samples_unread);
    if (mark_as_read) {
        counters_.samples_read += ret_val;
        counters_.samples_unread = 0U;
    }
    return ret_val;
}

bool DataReaderHistory::is_instance_present(const InstanceHandle_t& handle) const {
    return has_keys_ && instances_.find(handle) != instances_.end();
}

std::pair<bool, DataReaderHistory::instance_info> DataReaderHistory::lookup_available_instance(
    const InstanceHandle_t& handle, bool exact) {
    InstanceCollection::iterator it = data_available_instances_.end();

    if (!has_keys_) {
        // NO_KEY topics can only return the fictitious instance.
        // Execution can only get here for two reasons:
        // - Looking for a specific instance (exact = true)
        // - Looking for the next instance to the ficticious one (exact = false)
        // In both cases, no instance should be returned
        if ((!handle.isDefined()) && (!exact)) {
            // Looking for the first instance, return the ficticious one containing all changes
            it = data_available_instances_.begin();
        }
    } else {
        if (exact) {
            // Looking for a specific instance on a topic with key
            it = data_available_instances_.find(handle);
        } else {
            if (!handle.isDefined()) {
                // Looking for the first instance on a topic with key
                it = data_available_instances_.begin();
            } else {
                // Looking for an instance with a handle greater than the one on the input
                auto comp = [](const InstanceHandle_t& h, const InstanceCollection::value_type& it) {
                    return h < it.first;
                };
                it = std::upper_bound(data_available_instances_.begin(), data_available_instances_.end(), handle, comp);
            }
        }
    }

    return {it != data_available_instances_.end(), it};
}

std::pair<bool, DataReaderHistory::instance_info> DataReaderHistory::next_available_instance_nts(
    const InstanceHandle_t& handle, const DataReaderHistory::instance_info& current_info) {
    if (current_info == data_available_instances_.end()) {
        return {false, current_info};
    }
    instance_info it = current_info;
    if (it->first == handle) {
        ++it;
    }

    return {it != data_available_instances_.end(), it};
}

void DataReaderHistory::check_and_remove_instance(DataReaderHistory::instance_info& instance_info) {
    vbs::detail::DataReaderInstance* instance = instance_info->second.get();
    if (instance->cache_changes.empty() && (vbs::InstanceStateKind::ALIVE_INSTANCE_STATE != instance->instance_state) &&
        instance->alive_writers.empty() && instance_info->first.isDefined()) {
        (void)instances_.erase(instance_info->first);
        instance_info = data_available_instances_.erase(instance_info);
    }
}

ReaderHistory::iterator DataReaderHistory::remove_change_nts(ReaderHistory::const_iterator removal, bool release,
                                                             bool notify) {
    if (removal != changesEnd()) {
        CacheChange_t* p_sample = *removal;

        if ((!has_keys_) || (p_sample->is_fully_assembled())) {
            // clean any references to this CacheChange in the key state collection
            auto it = instances_.find(p_sample->instanceHandle);

            // if keyed and in history must be in the map
            // There is a case when the sample could not be in the keyed map. The first received
            // fragment of a fragmented sample is stored in the history, and when it is completed it
            // is stored in the keyed map. But it can occur it is rejected when the sample is
            // completed and removed without being stored in the keyed map.
            if (it != instances_.end()) {
                (void)it->second->cache_changes.remove(p_sample);
                if (p_sample->isRead) {
                    --counters_.samples_read;
                }
            }
        }
    }

    // call the base class
    auto ret_val = ReaderHistory::remove_change_nts(removal, release, notify);
    counters_.samples_unread = get_unread_count_inner(false);
    return ret_val;
}

bool DataReaderHistory::completed_change(CacheChange_t* change) {
    evbs::edds::dds::SampleRejectedStatusKind reason;
    return completed_change(change, 0U, reason);
}

bool DataReaderHistory::completed_change(CacheChange_t* change, size_t unknown_missing_changes_up_to,
                                         evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    bool ret_value = false;
    rejection_reason = REJECTED_BY_INSTANCES_LIMIT;
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    if (compute_key_for_change_fn_(change)) {
        InstanceCollection::iterator vit;
        if (find_key(change->instanceHandle, vit)) {
            ret_value = !change->instanceHandle.isDefined() ||
                        complete_fn_(change, *vit->second, unknown_missing_changes_up_to, rejection_reason);
        }
    }

    if (ret_value) {
        rejection_reason = NOT_REJECTED;
        if (!update_instance_nts(change)) {
            (void)remove_change_sub_nts(change, false);
            rejection_reason = REJECTED_BY_OWERSHIP;
            ret_value = false;
        }
    }

    return ret_value;
}

bool DataReaderHistory::completed_change_keep_all(CacheChange_t* change, vbs::detail::DataReaderInstance& instance,
                                                  size_t unknown_missing_changes_up_to,
                                                  evbs::edds::dds::SampleRejectedStatusKind& rejection_reason) {
    bool ret_value = false;
    vbs::detail::DataReaderInstance::ChangeCollection& instance_changes = instance.cache_changes;
    if (instance_changes.size() + unknown_missing_changes_up_to <
        static_cast<size_t>(resource_limited_qos_.max_samples_per_instance)) {
        add_to_instance(change, instance);
        ret_value = true;
    } else {
        logDebug(SUBSCRIBER, "Change not added due to maximum number of samples per instance");
        rejection_reason = REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT;
    }

    return ret_value;
}

bool DataReaderHistory::completed_change_keep_last(CacheChange_t* change, vbs::detail::DataReaderInstance& instance,
                                                   size_t, evbs::edds::dds::SampleRejectedStatusKind&) {
    bool ret_value = false;
    vbs::detail::DataReaderInstance::ChangeCollection& instance_changes = instance.cache_changes;
    if (instance_changes.size() < static_cast<size_t>(history_qos_.depth)) {
        ret_value = true;
    } else {
        // Try to substitute the oldest sample.
        CacheChange_t* first_change = instance_changes.at(0U);
        if (change->sourceTimestamp >= first_change->sourceTimestamp) {
            if (!first_change->isRead) {
                logDebug(DATA_READER, "Topic " << topic_name_.to_string() << " keep last: remove the oldest cache "
                                               << first_change->sequenceNumber);
            }
            // As the instance is ordered by source timestamp, we can always remove the first one.
            ret_value = remove_change_sub_nts(first_change);
        } else {
            // Received change is older than oldest, and should be discarded
            return true;
        }
    }

    if (ret_value) {
        add_to_instance(change, instance);
    }

    return ret_value;
}

void DataReaderHistory::change_was_processed_nts(CacheChange_t* const change, bool is_going_to_be_mark_as_read) {
    if ((!change->isRead) && is_going_to_be_mark_as_read) {
        ++counters_.samples_read;
        --counters_.samples_unread;
    }
}

void DataReaderHistory::instance_viewed_nts(const InstanceCollection::mapped_type& instance) {
    if (vbs::ViewStateKind::NEW_VIEW_STATE == instance->view_state) {
        instance->view_state = vbs::ViewStateKind::NOT_NEW_VIEW_STATE;
        --counters_.instances_new;
        ++counters_.instances_not_new;
    }
}

bool DataReaderHistory::update_instance_nts(CacheChange_t* const change) {
    InstanceCollection::iterator vit;

    vit = instances_.find(change->instanceHandle);
    if ((vit == instances_.end()) || (false != change->isRead)) {
        elogError(SUBSCRIBER, RetCode_t::RETCODE_ERROR,
                  "update_instance_nts() cannot find instance or the cache has been read.");
        return false;
    }

    bool ret = vit->second->update_state(counters_, change->kind, change->writerGUID,
                                         change->reader_info.writer_ownership_strength);
    change->reader_info.disposed_generation_count = vit->second->disposed_generation_count;
    change->reader_info.no_writers_generation_count = vit->second->no_writers_generation_count;

    return ret;
}

void DataReaderHistory::writer_not_alive(const GUID_t& writer_guid) {
    for (auto& it : instances_) {
        (void)it.second->writer_removed(counters_, writer_guid);
    }
}

vbs::detail::StateFilter DataReaderHistory::get_mask_status() const noexcept {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());

    return {static_cast<vbs::SampleStateMask>((counters_.samples_read ? vbs::READ_SAMPLE_STATE : 0) |
                                              (counters_.samples_unread ? vbs::NOT_READ_SAMPLE_STATE : 0)),
            static_cast<vbs::ViewStateMask>((counters_.instances_not_new ? vbs::NOT_NEW_VIEW_STATE : 0) |
                                            (counters_.instances_new ? vbs::NEW_VIEW_STATE : 0)),
            static_cast<vbs::InstanceStateMask>(
                (counters_.instances_alive ? vbs::ALIVE_INSTANCE_STATE : 0) |
                (counters_.instances_disposed ? vbs::NOT_ALIVE_DISPOSED_INSTANCE_STATE : 0) |
                (counters_.instances_no_writers ? vbs::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE : 0))};
}

void DataReaderHistory::writer_update_its_ownership_strength(const GUID_t& writer_guid,
                                                             const uint32_t ownership_strength) {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    for (auto& instance : instances_) {
        instance.second->writer_update_its_ownership_strength(writer_guid, ownership_strength);
    }
}

void DataReaderHistory::total_unread_increase() {
    std::lock_guard<RecursiveTimedMutex> guard(getMutex());
    total_unread_increase_nts();
}

void DataReaderHistory::total_unread_increase_nts() {
    ++counters_.samples_unread;
    ++total_unread_;
}

}  // namespace detail
}  // namespace vbs
