/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <string>
#include <vector>

#ifndef TYPES_ENDPOINT_TYPE_HPP
#define TYPES_ENDPOINT_TYPE_HPP

namespace discovery_server {
namespace agent {
namespace config {

enum {
    BEST_EFFORT = 1,
    RELIABLE = 2  // Specification says 3 but eprosima sends 2
};

typedef struct duration {
    int32_t seconds;   // seconds
    uint32_t nanosec;  // nanoseconds
} duration_t;

struct reliability_policy {
    duration_t max_blocking_time;
};

enum { VOLATILE = 0, TRANSIENT_LOCAL = 1, TRANSIENT = 2, PERSISTENT = 3 };

enum { KEEP_LAST, KEEP_ALL };

struct history_policy {
    // enum history_kind kind;
    uint16_t depth;
};

struct resource_limit_policy {
    uint16_t max_samples;
    uint8_t max_instances;
    uint8_t max_samples_per_instance;
};

struct lifespan_policy {
    duration_t duration;
};

struct deadline_policy {
    duration_t duration;
};

enum {
    SHARED,
    EXCLUSIVE,
};

struct ownership_strength_policy {
    uint16_t value;
};

enum { QOS_LIVE_AUTOMATIC, QOS_LIVE_MANUAL_BY_PTCP, QOS_LIVE_MANUAL_BY_TOPIC };

enum {
    //! Writer is matched but liveliness has not been asserted yet
    NOT_ASSERTED = 0,
    //! Writer is alive
    ALIVING = 1,
    //! Writer is not alive
    NOT_ALIVE = 2
};

struct liveliness_policy {
    /*! Period within which liveliness should be asserted.
     *  On a DataWriter it represents the period it commits to signal its liveliness.
     *  On a DataReader it represents the period without assertion after which a DataWriter is
     * considered inactive.
     */
    duration_t lease_duration;
    /*! The period for automatic assertion of liveliness.
     *  Only used for DataWriters with QOS_LIVE_AUTOMATIC liveliness.
     *
     * @warning When not infinite, must be < lease_duration, and it is advisable to be less than
     * 0.7*lease_duration.
     */
    duration_t announcement_period;
};

struct heartbeat_policy {
    uint8_t heartbeats_per_max_samples;
    uint8_t period_sec;
    uint16_t period_msec;
};

struct e2e_policy {
    uint32_t e2e_p04_data_id;
    uint16_t e2e_p04_min_data_length;
    uint16_t e2e_p04_max_data_length;
    uint16_t e2e_p04_max_delta_counter;
    bool e2e_protection;
};

struct sub_endpoint_attr {
    std::string topic_name;
    std::string type_name;
    uint16_t entity_id;
};

struct composite_endpoint_attr {
    std::string topic_name;
    std::string type_name;

    struct {
        uint16_t reliability_kind : 2;
        uint16_t durability_kind : 2;
        uint16_t ownership_kind : 2;
        uint16_t liveliness_kind : 2;
        uint16_t history_kind : 2;
    } qos;

    history_policy history;
    ownership_strength_policy ownership_strength;
    reliability_policy reliability;
    liveliness_policy liveliness;
    resource_limit_policy resource_limit;
    lifespan_policy lifespan;
    e2e_policy e2e;
    deadline_policy deadline;

    uint16_t entity_id;
    int16_t user_defined_id;  // Used by static discovery

    uint16_t locator_selector : 4;
    uint16_t backup_enabled : 1;
    uint16_t crc16_enabled : 1;
};

struct composite_reader_attr {
    composite_endpoint_attr composite_ep_attr;
    uint16_t max_matched_writers;  //define the size for writerproxy
    uint16_t max_sub_topic_size;
    std::vector<sub_endpoint_attr> sub_reader_ep_attr;
};

struct composite_writer_attr {
    composite_endpoint_attr composite_ep_attr;

    uint16_t max_matched_readers;
    bool enforce_unicast;
    uint16_t max_sub_topic_size;
    std::vector<sub_endpoint_attr> sub_writer_ep_attr;
};

typedef struct participant_attr {
    char name[64];
    /* 3. Configuration for Discovery */
    struct {
        duration_t lease_duration;
        duration_t lease_duration_announce_period;
        duration_t initial_announce_period;
        uint16_t initial_announce_count;
    } discovery;

    /* 4. Configuration of Endpoints */
    uint8_t composite_writer_num;
    uint8_t composite_reader_num;

    const composite_reader_attr* composite_reader_attrs;
    const composite_writer_attr* composite_writer_attrs;
} participant_attr_t;

struct mvbs_profile {
    uint32_t domain_id;

    const participant_attr_t* ptcp_attr;
};

}  // namespace config
}  // namespace agent
}  // namespace discovery_server

#endif  // TYPES_ENDPOINT_HPP
