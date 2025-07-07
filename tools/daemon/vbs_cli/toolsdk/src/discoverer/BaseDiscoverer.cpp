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

#include "discoverer/BaseDiscoverer.hpp"

namespace vbstoolsdk {

std::vector<BaseDiscoverer::ParticipantInfo> BaseDiscoverer::get_participants_list() {
    participants_info_map_t participants_map_copy(participants_map_);
    std::vector<ParticipantInfo> return_vector;

    for (const auto& participant : participants_map_copy) {
        ParticipantInfo participant_info {participant.second};
        std::vector<unsigned char> prefix_id = prefix_guid_to_vector_(participant_info.guid);
        auto writers_it = writers_map_.find(prefix_id);
        auto readers_it = readers_map_.find(prefix_id);

        if (writers_it != writers_map_.end()) {
            for (const auto& writer : writers_it->second) {
                participant_info.writers.emplace_back(writer.second);
            }
        }

        if (readers_it != readers_map_.end()) {
            for (const auto& reader : readers_it->second) {
                participant_info.readers.emplace_back(reader.second);
            }
        }
        return_vector.emplace_back(std::move(participant_info));
    }

    return return_vector;
}

std::vector<BaseDiscoverer::RWInfo> BaseDiscoverer::get_readers_list() {
    std::map<std::vector<unsigned char>, rws_info_map_t> readers_map_copy {readers_map_};
    std::vector<RWInfo> return_vector;

    for (const auto& readers : readers_map_copy) {
        for (const auto& reader : readers.second) {
            return_vector.emplace_back(reader.second);
        }
    }
    return return_vector;
}

std::vector<BaseDiscoverer::RWInfo> BaseDiscoverer::get_writers_list() {
    std::map<std::vector<unsigned char>, rws_info_map_t> writers_map_copy {writers_map_};
    std::vector<RWInfo> return_vector;

    for (const auto& writers : writers_map_copy) {
        for (const auto& writer : writers.second) {
            return_vector.emplace_back(writer.second);
        }
    }
    return return_vector;
}

std::vector<BaseDiscoverer::TopicInfo> BaseDiscoverer::get_topics_list() {
    std::unordered_map<std::string, guid_info_map_t> topics_readers_map_copy {topics_readers_map_};
    std::unordered_map<std::string, guid_info_map_t> topics_writers_map_copy {topics_writers_map_};
    std::vector<TopicInfo> return_vector;

    for (auto& topic_readers : topics_readers_map_copy) {
        std::string topic_name;
        std::string type_name;
        TopicInfo topic_info;
        delimit_topic_type_(topic_readers.first, topic_name, type_name);
        topic_info.topic_name = topic_name;
        topic_info.type_name = type_name;
        topic_info.type_discovered = false;
        topic_info.rate = 0;

        if (type_discovered_types_.find(type_name) != type_discovered_types_.end()) {
            auto it = discovered_types_topics_.find(type_name);
            if (it != discovered_types_topics_.end()) {
                if (it->second.find(topic_name) != it->second.end()) {
                    topic_info.type_discovered = true;
                }
            }
        }

        for (auto& reader : topic_readers.second) {
            topic_info.readers.emplace_back(reader.second);
        }

        auto writers_it = topics_writers_map_copy.find(topic_readers.first);
        if (writers_it != topics_writers_map_copy.end()) {
            for (auto& writer : writers_it->second) {
                topic_info.writers.emplace_back(writer.second);
            }
            topics_writers_map_copy.erase(topic_readers.first);
        }
        return_vector.emplace_back(std::move(topic_info));
    }

    for (auto& topic_writers : topics_writers_map_copy) {
        std::string topic_name;
        std::string type_name;
        TopicInfo topic_info;
        delimit_topic_type_(topic_writers.first, topic_name, type_name);
        topic_info.topic_name = topic_name;
        topic_info.type_name = type_name;
        topic_info.rate = 0;
        for (auto& writer : topic_writers.second) {
            topic_info.writers.emplace_back(writer.second);
        }
        return_vector.emplace_back(std::move(topic_info));
    }
    return return_vector;
}

void BaseDiscoverer::set_all_topics_observation() {
    all_topics_observation_ = true;
}

void BaseDiscoverer::unset_all_topics_observation() {
    all_topics_observation_ = false;
}

void BaseDiscoverer::add_topic_observation(const std::string& topic_name) {
    std::lock_guard lk(observation_mutex_);
    topics_observation_list_.emplace(topic_name);
}

void BaseDiscoverer::delete_topic_observation(const std::string& topic_name) {
    std::lock_guard lk(observation_mutex_);
    topics_observation_list_.erase(topic_name);
}

void BaseDiscoverer::update_topics_observation_list(std::unordered_set<std::string>&& list) {
    std::lock_guard lk(observation_mutex_);
    topics_observation_list_ = std::move(list);
}

void BaseDiscoverer::register_callback(std::function<void(DataInfo&&)> callback_function) {
    std::lock_guard lk(observation_mutex_);
    callback_function_ = callback_function;
}

void BaseDiscoverer::unregister_callback() {
    std::lock_guard lk(observation_mutex_);
    callback_function_ = nullptr;
}

std::vector<unsigned char> BaseDiscoverer::guid_to_vector_(ERTPS_RTPS::GUID_t guid) {
    std::vector<unsigned char> temp;
    temp.insert(temp.end(), guid.guidPrefix.value, guid.guidPrefix.value + 12);
    temp.insert(temp.end(), guid.entityId.value, guid.entityId.value + 4);
    return temp;
}

std::vector<unsigned char> BaseDiscoverer::prefix_guid_to_vector_(ERTPS_RTPS::GUID_t guid) {
    std::vector<unsigned char> temp;
    temp.insert(temp.end(), guid.guidPrefix.value, guid.guidPrefix.value + 12);
    return temp;
}

std::vector<unsigned char> BaseDiscoverer::entity_guid_to_vector_(ERTPS_RTPS::GUID_t guid) {
    std::vector<unsigned char> temp;
    temp.insert(temp.end(), guid.entityId.value, guid.entityId.value + 4);
    return temp;
}

bool BaseDiscoverer::delimit_topic_type_(const std::string& topic_type, std::string& topic_name,
                                         std::string& type_name) {
    size_t delimiter_pos = topic_type.find("&&");
    if (delimiter_pos != std::string::npos) {
        topic_name = topic_type.substr(0, delimiter_pos);
        type_name = topic_type.substr(delimiter_pos + 2);
        return true;
    }
    return false;
}

}  // namespace vbstoolsdk
