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

#ifndef _EDDS_ENTITYPUBLISHER_DATAWRITERHISTORY_HPP_
#define _EDDS_ENTITYPUBLISHER_DATAWRITERHISTORY_HPP_

#include <chrono>
#include <mutex>

#include "edds/rtps/common/InstanceHandle.h"
#include "edds/rtps/common/Time_t.h"
#include "history/WriterHistory.h"
#include "edds/rtps/resources/ResourceManagement.h"
#include "ertps/attributes/TopicAttributes.h"
#include "ertps/qos/QosPolicies.h"
#include "dispatcher/DispatcherWriter.hpp"
#include "pub/DataWriterInstance.hpp"

namespace vbs {
/**
 * Class DataWriterHistory, implementing a WriterHistory with support for keyed topics and
 * HistoryQOS. This class is created by the Publisher and should not be used by the user
 * directly.
 * @ingroup EDDS_MODULE
 */
class DataWriterHistory : public vbs::common::WriterHistory {
 public:
    /**
     * Constructor of the DataWriterHistory.
     * @param topic_att TopicAttributed
     * @param payloadMax Maximum payload size.
     * @param mempolicy Set whether the payloads ccan dynamically resized or not.
     * @param unack_sample_remove_functor Functor to call DDS listener callback on_unacknowledged_sample_removed
     */
    DataWriterHistory(const evbs::ertps::TopicAttributes& topic_att, uint32_t payloadMax,
                      evbs::ertps::rtps::MemoryManagementPolicy_t mempolicy,
                      std::function<void(const evbs::ertps::rtps::InstanceHandle_t&)> unack_sample_remove_functor,
                      const BatchQosPolicy& batch_qos);

    virtual ~DataWriterHistory();

    /**
     * Rebuild instances loaded from DB. Does nothing if the topic doesn't have key.
     */
    void rebuild_instances();

    /*!
     * @brief Tries to reserve resources for the new instance.
     *
     * @param [in]  instance_handle    Instance's key.
     * @param [in]  max_blocking_time  Maximum time the operation should be waiting.
     * @param [out] payload            Pointer to a serialized payload structure where the
     * serialized payload of the newly allocated instance should be written.
     *
     * @return True if resources were reserved successfully.
     */
    bool register_instance(const evbs::ertps::rtps::InstanceHandle_t& instance_handle,
                           evbs::ertps::rtps::SerializedPayload_t*& payload);

    /**
     * This operation can be used to retrieve the serialized payload of the instance key that
     * corresponds to an
     * @ref vbs::Entity::instance_handle_ "instance_handle".
     *
     * This operation will return @c nullptr if the InstanceHandle_t handle does not correspond to
     * an existing data-object known to the DataWriterHistory.
     *
     * @param[in] handle  Handle to the instance to retrieve the key values from.
     *
     * @return Pointer to the serialized payload of the sample with which the instance was
     * registered.
     */
    evbs::ertps::rtps::SerializedPayload_t* get_key_value(const evbs::ertps::rtps::InstanceHandle_t& handle);

    /**
     * Add a change comming from the DataWriter.
     * @param change Pointer to the change
     * @param wparams Extra write parameters.
     * @param lock
     * @param max_blocking_time
     * @return True if added.
     */
    bool add_pub_change(vbs::common::CacheChange_t* change, vbs::common::WriteParams& wparams,
                        std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
                        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time, bool add_history);

    template <typename PreCommitHook>
    bool add_pub_change_with_commit_hook(vbs::common::CacheChange_t* change, vbs::common::WriteParams& wparams,
                                         PreCommitHook pre_commit,
                                         std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
                                         const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time,
                                         bool add_history) {
        bool returnedValue = false;
        bool add = true;
        if (add_history) {
            add = prepare_change(change, lock, max_blocking_time);
        }

        if (add) {
            if (this->add_change_with_commit_hook(change, wparams, pre_commit, lock, max_blocking_time, add_history)) {
                logDebug(RTPS_HISTORY, " Change " << change->sequenceNumber
                                                  << " added with key: " << change->instanceHandle << " and "
                                                  << change->serializedPayload.length << " bytes");
                returnedValue = true;
            }
        }

        return returnedValue;
    }

    void add_persistence_change(CacheChange_t* a_change) override {
        auto rtps_writer = mp_writer->getSubstrateEndpoint(SUBSTRATE_TYPE_RTPS);
        if (rtps_writer) {
            rtps_writer->add_persistentWriter_change(a_change);
            logDebug(RTPS_HISTORY,
                     "Topic " << topicName << " statefulWriter add persistence change " << a_change->sequenceNumber);
        }
    }

    /**
     * Remove all change from the associated history.
     * @param removed Number of elements removed.
     * @return True if all elements were removed.
     */
    bool removeAllChange(size_t* removed);

    /**
     * Remove the change with the minimum sequence Number.
     * @return True if removed.
     */
    bool removeMinChange();

    /**
     * Remove a change by the publisher History.
     * @param change Pointer to the CacheChange_t.
     * @return True if removed.
     */
    bool remove_change_pub(vbs::common::CacheChange_t* change);
    bool remove_change_pub_nts(vbs::common::CacheChange_t* change);

    virtual bool remove_change_g_nts(vbs::common::CacheChange_t* a_change) override;

    bool remove_instance_changes(const evbs::ertps::rtps::InstanceHandle_t& handle,
                                 const evbs::ertps::rtps::SequenceNumber_t& seq_up_to);

    /**
     * @brief Sets the next deadline for the given instance
     * @param handle The instance handle
     * @param next_deadline_us The time point when the deadline will occur
     * @return True if deadline was set successfully
     */
    bool set_next_deadline_nts(const evbs::ertps::rtps::InstanceHandle_t& handle,
                               const std::chrono::steady_clock::time_point& next_deadline_us);

    /**
     * @brief Returns the deadline for the instance that is next going to 'expire'
     * @param handle The handle for the instance that will next miss the deadline
     * @param next_deadline_us The time point when the deadline will occur
     * @return True if deadline could be retrieved for the given instance
     */
    bool get_next_deadline_nts(evbs::ertps::rtps::InstanceHandle_t& handle,
                               std::chrono::steady_clock::time_point& next_deadline_us);

    /*!
     * @brief Checks if the instance's key is registered.
     * @param[in] handle Instance's key.
     * return `true` if instance's key is registered in the history.
     */
    bool is_key_registered(const evbs::ertps::rtps::InstanceHandle_t& handle);

    /**
     * Waits till the last change in the instance history has been acknowledged.
     * @param handle Instance's handle.
     * @param lock Lock which should be unlock in case the operation has to wait.
     * @param max_blocking_time Maximum time the operation should be waiting.
     * @return true when the last change of the instance history is acknowleged, false when timeout
     * is reached.
     */
    bool wait_for_acknowledgement_last_change(
        const evbs::ertps::rtps::InstanceHandle_t& handle, std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time);

    /**
     * @brief Check if a specific change has been acknowledged or fully delivered if disable positive ACKs QoS is
     *        enabled.
     *
     * @param change CacheChange to check
     * @return true if acknowledged or fully delivered. False otherwise.
     */
    bool change_is_acked_or_fully_delivered(const vbs::common::CacheChange_t* change);

    vbs::dispatcher::DispatcherWriter* mp_writer;

 private:
    typedef std::map<evbs::ertps::rtps::InstanceHandle_t, vbs::detail::DataWriterInstance> t_m_Inst_Caches;

    //! Map where keys are instance handles and values are vectors of cache changes associated
    t_m_Inst_Caches keyed_changes_;
    //! Time point when the next deadline will occur (only used for topics with no key)
    std::chrono::steady_clock::time_point next_deadline_us_;
    //! HistoryQosPolicy values.
    HistoryQosPolicy history_qos_;
    //! ResourceLimitsQosPolicy values.
    ResourceLimitsQosPolicy resource_limited_qos_;
    //! Topic Attributes
    evbs::ertps::TopicAttributes topic_att_;

    //! Unacknowledged sample removed functor
    std::function<void(const evbs::ertps::rtps::InstanceHandle_t&)> unacknowledged_sample_removed_functor_;

    /**
     * @brief Method that finds a key in the DataWriterHistory or tries to add it if not found
     * @param [in]  instance_handle  Instance of the key.
     * @param [in]  payload          Serialized payload of the sample for which the instance is
     * being registered.
     * @param [out] map_it           A map iterator to the given key.
     * @return True if the key was found or could be added to the map
     */
    bool find_or_add_key(const evbs::ertps::rtps::InstanceHandle_t& instance_handle,
                         const evbs::ertps::rtps::SerializedPayload_t& payload, t_m_Inst_Caches::iterator* map_it);

    inline bool history_capbility_check(const vbs::common::CacheChange_t* change,
                                        std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
                                        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time);

    inline bool history_capbility_check_with_key(vbs::common::CacheChange_t* change,
                                                 std::unique_lock<RecursiveTimedMutex>& lock,
                                                 t_m_Inst_Caches::iterator& vit);

    inline bool history_capbility_check_with_key(
        vbs::common::CacheChange_t* change, std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time, t_m_Inst_Caches::iterator& vit,
        bool& hasError);
    /**
     * Add a change comming from the Publisher.
     * @param change Pointer to the change
     * @param lock
     * @param max_blocking_time
     * @return True if added.
     */
    bool prepare_change(vbs::common::CacheChange_t* change, std::unique_lock<vbs::common::RecursiveTimedMutex>& lock,
                        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time);
};

}  // namespace vbs

#endif  // _EDDS_ENTITYPUBLISHER_DATAWRITERHISTORY_HPP_
