// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef VBS_TOPICQOS_HPP_
#define VBS_TOPICQOS_HPP_

#include "vbs/Global.hpp"

namespace vbs {
class TopicQosImpl;
class TopicQos {
 public:
    /**
    * @brief Constructor
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    TopicQos();
    /**
    * TopicQos deconstructor.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    ~TopicQos();

    /**
    * Sets the instance for the Topic Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param topic_qos_impl_ptr Shared pointer to the TopicQosImpl instance to be set.
    */
    void set_instance(const std::shared_ptr<vbs::TopicQosImpl>& topic_qos_impl_ptr);

    /**
    * Gets the instance for the Topic Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    */
    std::shared_ptr<vbs::TopicQosImpl> get_instance() const;

    /**
    * Overload for the equality operator to compare two TopicQos objects.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param b The TopicQos object to compare against.
    * @return True if both TopicQos objects are equal, otherwise false.
    */
    bool operator==(const TopicQos& b) const;

    /**
    * @return A constant reference to the durability Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsDurabilityQos& durability() const;

    /**
    * @return A reference to the durability Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsDurabilityQos& durability();

    /**
    * Sets the durability Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_durability The new durability QoS value.
    */
    void durability(const VbsDurabilityQos& new_durability);

    /**
    * @return A constant reference to the deadline Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsDeadLineQos& deadline() const;

    /**
    * @return A reference to the deadline Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsDeadLineQos& deadline();

    /**
    * Sets the deadline Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_deadline The new deadline QoS value.
    */
    void deadline(const VbsDeadLineQos& new_deadline);

    /**
    * @return A constant reference to the liveliness Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsLiveLinessQos& liveliness() const;

    /**
    * @return A reference to the liveliness Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsLiveLinessQos& liveliness();

    /**
    * Sets the liveliness Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_liveliness The new liveliness QoS value.
    */
    void liveliness(const VbsLiveLinessQos& new_liveliness);

    /**
    * @return A constant reference to the reliability Qos setting.
    */
    const VbsReliabilityQos& reliability() const;

    /**
    * @return A reference to the reliability Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsReliabilityQos& reliability();

    /**
    * Sets the reliability Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_reliability The new reliability QoS value.
    */
    void reliability(const VbsReliabilityQos& new_reliability);

    /**
    * @return A constant reference to the history Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsHistoryQos& history() const;

    /**
    * @return A reference to the history Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsHistoryQos& history();

    /**
    * Sets the history Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_history The new history QoS value.
    */
    void history(const VbsHistoryQos& new_history);

    /**
    * @return A constant reference to the resource_limits Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsResourceLimitsQos& resource_limits() const;

    /**
    * @return A reference to the resource_limits Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsResourceLimitsQos& resource_limits();

    /**
    * Sets the resource_limits Qos setting.
    * 
    * @param new_resource_limits The new resource_limits QoS value.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    void resource_limits(const VbsResourceLimitsQos& new_resource_limits);

    /**
    * @return A constant reference to the lifespan Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsLifespanQos& lifespan() const;

    /**
    * @return A reference to the lifespan Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsLifespanQos& lifespan();

    /**
    * Sets the lifespan Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_lifespan The new lifespan QoS value.
    */
    void lifespan(const VbsLifespanQos& new_lifespan);

    /**
    * @return A constant reference to the ownership Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsOwnershipQos& ownership() const;

    /**
    * @return A reference to the ownership Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsOwnershipQos& ownership();

    /**
    * Sets the ownership Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_ownership The new ownership QoS value.
    */
    void ownership(const VbsOwnershipQos& new_ownership);

 private:
    mutable std::shared_ptr<vbs::TopicQosImpl> topic_qos_ptr_;
};

}  // namespace vbs

#endif  // VBS_TOPICQOS_HPP_
