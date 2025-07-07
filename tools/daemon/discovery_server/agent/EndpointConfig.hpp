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

#include <agent/types/EndpointType.hpp>

#ifndef ENDPOINT_CONFIG_HPP
#define ENDPOINT_CONFIG_HPP

namespace discovery_server {
namespace agent {
namespace config {

static const std::string g_mvbs_qos_version = "mvbs.qos.250228";

static const composite_writer_attr dcps_composite_writers[] = {
    //composit writer
    {
        {
            // composite_ep_attr
            "MVBS::BZCU::COMPOSIT::TOPIC1",  // topic_name
            "MVBS::BZCU::COMPOSIT::TYPE",    // type_name ,可能不需要
            {
                // qos
                BEST_EFFORT,         //reliability_kind
                VOLATILE,            // durability_kind
                SHARED,              // ownership_kind
                QOS_LIVE_AUTOMATIC,  // liveliness_kind
                KEEP_ALL,            // history_kind
            },
            5,        // history.depth
            0,        // ownership_strength.value
            {10, 0},  // reliability.max_block_time
            {
                // liveliness
                {20, 0},  // lease_duration
                {10, 0},  // announcement_period
            },
            {
                // resource_limit
                5,  // max_samples
                1,  // max_instances
                5,  // max_samples_per_instance
            },
            {2147483647, 4294967295},  // lifespan.duration
            {
                // e2e
                0,      // e2e_p04_data_id
                0,      // e2e_p04_min_data_length
                4090,   // e2e_p04_max_data_length
                10,     // e2e_p04_max_delta_counter
                false,  // e2e_protection
            },
            {10, 0},  // deadline.duration
            1,        // entity_id
            100,      // user_defined_id
            0,        // locator_selector
            false,    // backup_enabled
            false,    // crc16_enabled
        },
        5,      // max_matched_readers
        false,  // enforce_unicast
        1500,   // max_sub_topic_size 后期由XML工具考虑如何生成
        {       //sub writer
         {
             "domain_0_topic_0Topic",
             "HelloWorld",
             2,
         }},
    },
};

static const composite_reader_attr dcps_composite_readers[] = {
    //composit reader
    {
        {
            // composite_ep_attr
            "MVBS::BZCU::COMPOSIT::TOPIC1",  // topic_name
            "MVBS::BZCU::COMPOSIT::TYPE",    // type_name ,可能不需要
            {
                // qos
                BEST_EFFORT,         //reliability_kind
                VOLATILE,            // durability_kind
                SHARED,              // ownership_kind
                QOS_LIVE_AUTOMATIC,  // liveliness_kind
                KEEP_ALL,            // history_kind
            },
            5,        // history.depth
            0,        // ownership_strength.value
            {10, 0},  // reliability.max_block_time
            {
                // liveliness
                {20, 0},  // lease_duration
                {10, 0},  // announcement_period
            },
            {
                // resource_limit
                5,  // max_samples
                1,  // max_instances
                5,  // max_samples_per_instance
            },
            {2147483647, 4294967295},  // lifespan.duration
            {
                // e2e
                0,      // e2e_p04_data_id
                0,      // e2e_p04_min_data_length
                4090,   // e2e_p04_max_data_length
                10,     // e2e_p04_max_delta_counter
                false,  // e2e_protection
            },
            {10, 0},  // deadline.duration
            1,        // entity_id
            100,      // user_defined_id
            0,        // locator_selector
            false,    // backup_enabled
            false,    // crc16_enabled
        },
        5,     // max_matched_readers
        1500,  // max_sub_topic_size 后期由XML工具考虑如何生成
        {      //sub writer
         {
             "domain_0_topic_0Topic",
             "HelloWorld",
             2,
         }},
    },
};

static const participant_attr dcps_ptcp_attr = {
    "app1_172.31.10.31",  // name

    /* 3. Configuration for Discovery */
    {          // discovery
     {20, 0},  // lease_duration
     {5, 0},   // lease_duration_announce_period
     {3, 0},   // initial_announce_period
     50},

    /* 4. Configuration of Endpoints */
    1,                       // composite_reader_num
    1,                       // composite_writer_num
    dcps_composite_readers,  // composite_reader_attrs
    dcps_composite_writers,  // composite_writer_attrs
};

static mvbs_profile dcps_profile = {0, &dcps_ptcp_attr};

static mvbs_profile* GetAgentProfile(void) {
    return &dcps_profile;
}

static const std::string& GetMvbsQosVersion() {
    return g_mvbs_qos_version;
}

}  // namespace config
}  // namespace agent
}  // namespace discovery_server

#endif  // ENDPOINT_CONFIG_HPP
