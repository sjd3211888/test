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

#ifndef BASE_DISCOVERER_HPP_
#define BASE_DISCOVERER_HPP_

#include <cstdint>
#include <vector>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include <edds/rtps/common/Guid.h>
#include <deps/common/Locator.h>
#include <edds/rtps/common/SequenceNumber.h>
#include <edds/rtps/common/RemoteLocators.hpp>
#include <ertps/rtps/RTPSDomain.h>
#include <edds/rtps/participant/ParticipantDiscoveryInfo.h>
#include <edds/rtps/reader/ReaderDiscoveryInfo.h>
#include <edds/rtps/writer/WriterDiscoveryInfo.h>
#include <ertps/types/DynamicDataPrinter.hpp>

#include "nlohmann/json.hpp"

namespace vbstoolsdk {

namespace ERTPS = evbs::ertps;
namespace ERTPS_RTPS = evbs::ertps::rtps;

constexpr const char* HOST_ENTITY_TAG = "host";
constexpr const char* USER_ENTITY_TAG = "user";
constexpr const char* PROCESS_ENTITY_TAG = "process";

class BaseDiscoverer {
 public:
    enum RWKind { DATAREADER = 0, DATAWRITER };

    struct RWInfo {
        uint32_t domain_id;
        ERTPS_RTPS::GUID_t guid;
        nlohmann::json qos;

        //endpoint data
        std::string topic_name;
        std::string type_name;
        ERTPS_RTPS::RemoteLocatorList locators;

        RWKind rw_kind;
        RWInfo() {}
        RWInfo(RWKind kind) : rw_kind(kind) {}
    };

    struct ParticipantInfo {
        uint32_t domain_id;
        ERTPS_RTPS::GUID_t guid;
        nlohmann::json qos;

        //participant data
        std::string address;
        std::string participant_name;
        std::vector<RWInfo> readers;
        std::vector<RWInfo> writers;

        //physical data
        std::string host;
        std::string user;
        std::string process;
    };

    struct TopicInfo {
        std::string topic_name;
        std::string type_name;
        std::vector<ERTPS_RTPS::GUID_t> readers;
        std::vector<ERTPS_RTPS::GUID_t> writers;
        bool type_discovered;
        float rate;
    };

    struct DataInfo {
        std::string topic_name;
        int64_t source_timestamp;
        int64_t reception_timestamp;
        uint64_t sequence_id;
        std::string out_json;
    };

    using participants_info_map_t = std::map<std::vector<unsigned char>, ParticipantInfo>;
    using rws_info_map_t = std::map<std::vector<unsigned char>, RWInfo>;
    using guid_info_map_t = std::map<std::vector<unsigned char>, ERTPS_RTPS::GUID_t>;

    BaseDiscoverer(uint32_t domain_id, bool type_discovery_flag = false, bool all_topics_observation = false)
        : domain_id_(domain_id),
          type_discovery_flag_(type_discovery_flag),
          all_topics_observation_(all_topics_observation),
          dyndata_printer_(std::make_shared<ERTPS::types::DynamicDataPrinter>()) {}

    virtual ~BaseDiscoverer() {}

    std::vector<ParticipantInfo> get_participants_list();
    std::vector<RWInfo> get_readers_list();
    std::vector<RWInfo> get_writers_list();
    std::vector<TopicInfo> get_topics_list();
    void set_all_topics_observation();
    void unset_all_topics_observation();
    void add_topic_observation(const std::string& topic_name);
    void delete_topic_observation(const std::string& topic_name);
    void update_topics_observation_list(std::unordered_set<std::string>&& list);
    void register_callback(std::function<void(DataInfo&&)> callback_function);
    void unregister_callback();

 protected:
    std::vector<unsigned char> guid_to_vector_(ERTPS_RTPS::GUID_t guid);
    std::vector<unsigned char> prefix_guid_to_vector_(ERTPS_RTPS::GUID_t guid);
    std::vector<unsigned char> entity_guid_to_vector_(ERTPS_RTPS::GUID_t guid);
    bool delimit_topic_type_(const std::string& topic_type, std::string& topic_name, std::string& type_name);

    uint32_t domain_id_;
    std::atomic_bool type_discovery_flag_;
    std::atomic_bool all_topics_observation_;
    std::mutex observation_mutex_;
    std::unordered_set<std::string> topics_observation_list_;
    participants_info_map_t participants_map_;
    std::shared_ptr<ERTPS::types::DynamicDataPrinter> dyndata_printer_;
    std::function<void(DataInfo&&)> callback_function_;

    //used to know r/w belong to which participant
    //prefix_id/entity_id
    std::map<std::vector<unsigned char>, rws_info_map_t> readers_map_;
    std::map<std::vector<unsigned char>, rws_info_map_t> writers_map_;

    //used to know r/w belong to which topic
    //topic_name::type_name
    std::unordered_map<std::string, guid_info_map_t> topics_readers_map_;
    std::unordered_map<std::string, guid_info_map_t> topics_writers_map_;

    //used to type discovery
    //type_name, std::vector<topic_name>
    std::unordered_map<std::string, std::unordered_set<std::string>> discovered_types_topics_;
    std::unordered_set<std::string> type_discovered_types_;
};

}  // namespace vbstoolsdk

#endif
