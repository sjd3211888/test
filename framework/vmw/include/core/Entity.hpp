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

#ifndef INCLUDE_EDDS_DDS_CORE_ENTITY_HPP_
#define INCLUDE_EDDS_DDS_CORE_ENTITY_HPP_

#include "ertps/types/TypesBase.h"
#include "core/condition/StatusCondition.hpp"
#include "deps/core/status/StatusMask.hpp"
#include "deps/common/TypeSupport.hpp"

namespace vbs {

/**
 * @brief The Entity class is the abstract base class for all the objects that support QoS policies,
 * a listener and a status condition.
 *
 */
class Entity {
 public:
    virtual ~Entity() = default;
    /**
     * @brief Constructor
     *
     * @param mask StatusMask (default: all)
     */
    RTPS_DllAPI Entity(const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all())
        : status_mask_(mask), status_condition_(this), enable_(false) {}

    /**
     * @brief This operation enables the Entity
     *
     * @return RETCODE_OK
     */
    virtual vbsutil::elog::ReturnCode_t enable() {
        enable_ = true;
        return vbsutil::elog::ReturnCode_t::RETCODE_OK;
    }

    /**
     * @brief This operation disables the Entity before closing it
     */
    void close() { enable_ = false; }

    /**
     * @brief Retrieves the set of relevant statuses for the Entity
     *
     * @return Reference to the StatusMask with the relevant statuses set to 1
     */
    RTPS_DllAPI const evbs::edds::dds::StatusMask& get_status_mask() const { return status_mask_; }

    /**
     * @brief Retrieves the set of triggered statuses in the Entity
     *
     * Triggered statuses are the ones whose value has changed
     * since the last time the application read the status.
     * When the entity is first created or if the entity is not enabled,
     * all communication statuses are in the non-triggered state,
     * so the list returned by the get_status_changes operation will be empty.
     * The list of statuses returned by the get_status_changes operation
     * refers to the status that are triggered on the Entity itself
     * and does not include statuses that apply to contained entities.
     *
     * @return const reference to the StatusMask with the triggered statuses set to 1
     */
    RTPS_DllAPI const evbs::edds::dds::StatusMask& get_status_changes() const;

    /**
     * @brief Retrieves the instance handler that represents the Entity
     *
     * @return Reference to the InstanceHandle
     */
    const InstanceHandle_t& get_instance_handle() const { return instance_handle_; }

    /**
     * @brief Checks if the Entity is enabled
     *
     * @return true if enabled, false if not
     */
    RTPS_DllAPI bool is_enabled() const { return enable_; }

    RTPS_DllAPI bool operator==(const Entity& other) const {
        return (this->instance_handle_ == other.instance_handle_);
    }

    /**
     * @brief Allows access to the StatusCondition associated with the Entity
     *
     * @return Reference to StatusCondition object
     */
    RTPS_DllAPI StatusCondition& get_statuscondition() { return status_condition_; }

 protected:
    /**
     * @brief Setter for the Instance Handle
     *
     * @param handle Instance Handle
     */
    RTPS_DllAPI void set_instance_handle(const InstanceHandle_t& handle) { instance_handle_ = handle; }

    //! StatusMask with relevant statuses set to 1
    evbs::edds::dds::StatusMask status_mask_;

    //! Condition associated to the Entity
    StatusCondition status_condition_;

    //! InstanceHandle associated to the Entity
    InstanceHandle_t instance_handle_;

    //! Boolean that states if the Entity is enabled or disabled
    bool enable_;
};

/**
 * @brief The DomainEntity class is a subclass of Entity created in order to differentiate between
 * DomainParticipants and the rest of Entities
 */
class DomainEntity : public Entity {
 public:
    /**
     * @brief Constructor
     *
     * @param mask StatusMask (default: all)
     */
    DomainEntity(const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all()) : Entity(mask) {}
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_CORE_ENTITY_HPP_
