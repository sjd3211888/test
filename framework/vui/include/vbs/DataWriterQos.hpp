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

#ifndef VBS_DATAWRITERQOS_HPP_
#define VBS_DATAWRITERQOS_HPP_

#include "vbs/Global.hpp"

namespace vbs {

class DataWriterQosImpl;
class DataWriterQos {
 public:
    /**
    * @brief Constructor
    */
    DataWriterQos();

    /**
    * DataWriterQos deconstructor.
    */
    ~DataWriterQos();

    /**
    * Sets the instance for the DataWriter Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param writer_qos_impl_ptr Shared pointer to the DataWriterQosImpl instance to be set.
    */
    void set_instance(const std::shared_ptr<vbs::DataWriterQosImpl>& writer_qos_impl_ptr);

    /**
    * Gets the instance for the DataWriter Qos implementation.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    */
    std::shared_ptr<vbs::DataWriterQosImpl> get_instance() const;

    /**
    * Overload for the equality operator to compare two DataWriterQos objects.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param b The DataWriterQos object to compare against.
    * @return True if both DataWriterQos objects are equal, otherwise false.
    */
    bool operator==(const DataWriterQos& b) const;

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
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
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
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_resource_limits The new resource_limits QoS value.
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

    /**
    * @return A constant reference to the ownership_strength Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    const VbsOwnershipStrengthQos& ownership_strength() const;

    /**
    * @return A reference to the ownership_strength Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsOwnershipStrengthQos& ownership_strength();

    /**
    * Sets the ownership_strength Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_ownershipstrength The new ownership_strength QoS value.
    */
    void ownership_strength(const VbsOwnershipStrengthQos& new_ownershipstrength);

    /**
    * @return A constant reference to the publish_mode Qos setting.
    */
    const VbsPublishModeQos& publish_mode() const;

    /**
    * @return A reference to the publish_mode Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    */
    VbsPublishModeQos& publish_mode();

    /**
    * Sets the publish_mode Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_publish_mode The new publish_mode QoS value.
    */
    void publish_mode(const VbsPublishModeQos& new_publish_mode);

    /**
    * @return A constant reference to the e2e_protection Qos setting.
    */
    const VbsE2EProtectionQos& e2e_protection() const;

    /**
    * @return A reference to the e2e_protection Qos setting.
    */
    VbsE2EProtectionQos& e2e_protection();

    /**
    * Sets the e2e_protection Qos setting.
    * 
    * @note @li Thread-Safe: No
    * @note @li Lock-Free: No
    * 
    * @param new_writer_e2e_protection The new e2e_protection QoS value.
    */
    void e2e_protection(const VbsE2EProtectionQos& new_writer_e2e_protection);

 private:
    mutable std::shared_ptr<vbs::DataWriterQosImpl> writer_qos_ptr_;
};

}  // namespace vbs

#endif  // VBS_DATAWRITERQOS_HPP_
