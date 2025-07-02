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

#include <cstdio>
#include <map>

#include "context/NexusUnits.hpp"
#include "context/NexusContext.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "topic/TopicImpl.hpp"
#include "pub/DataWriterImpl.hpp"
#include "sub/DataReaderImpl.hpp"

namespace vbs {
NexusUnits::NexusUnits() {
    dds_participant_factory_ = DomainParticipantFactoryInner::get_shared_instance();
}

NexusUnits::~NexusUnits() {
    DeleteDataWriter();
    DeleteDataReader();
    DeleteTopic();
    DeleteParticipant();
    dds_participant_factory_ = nullptr;
}

void NexusUnits::InsertParticipant(DomainParticipantImpl* participant) {
    {
        std::lock_guard<std::mutex> lock(mtx_participants_);
        participants_.insert(participant);
    }
    {
        std::lock_guard<std::mutex> lock(mtx_topics_);
        topics_[participant] = std::set<TopicImpl*>();
    }
    {
        std::lock_guard<std::mutex> lock(mtx_writers_);
        writers_[participant] = std::set<DataWriterImpl*>();
    }
    {
        std::lock_guard<std::mutex> lock(mtx_readers_);
        readers_[participant] = std::set<DataReaderImpl*>();
    }
}

void NexusUnits::dump_topic_traffic(const char* filepath) {
    FILE* fd = fopen(filepath, "a");
    if (fd == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "open " << filepath << " failed");
        return;
    }
    if (fd) {
        std::fseek(fd, 0, SEEK_END);
        size_t fileSize = std::ftell(fd);
        if (fileSize > 10 * 1024 * 1024) {
            VbsLogW("dump file " << filepath << " size too large, erase all content");
            std::fclose(fd);
            fd = fopen(filepath, "w");
        }
    }
    vbs::NetTrafficInfoImpl traffics[2];
    std::map<std::string, std::array<vbs::NetTrafficInfoImpl, 2>> all_topic_traffic;
    {
        std::lock_guard<std::mutex> lock(mtx_topics_);
        for (const auto& member : topics_) {
            std::set<vbs::TopicImpl*> tSets = member.second;
            for (auto topic : tSets) {
                if (topic != nullptr) {
                    topic->dump_network_traffic(traffics);
                    all_topic_traffic[topic->get_topic_name_()] =
                        std::array<vbs::NetTrafficInfoImpl, 2>({traffics[0], traffics[1]});
                }
            }
        }
    }
    int ret = 0;
    for (auto t : all_topic_traffic) {
        ret = fputs(t.first.c_str(), fd);
        if (ret == EOF) {
            break;
        }
        ret = fputs("\n", fd);
        if (ret == EOF) {
            break;
        }
        std::array<vbs::NetTrafficInfoImpl, 2>& traffic_array = t.second;
        ret = fputs(traffic_array[0].to_string().c_str(), fd);
        if (ret == EOF) {
            break;
        }
        ret = fputs("\n", fd);
        if (ret == EOF) {
            break;
        }
        ret = fputs(traffic_array[1].to_string().c_str(), fd);
        if (ret == EOF) {
            break;
        }
        ret = fputs("\n", fd);
        if (ret == EOF) {
            break;
        }
    }
    all_topic_traffic.clear();

    if (ret == EOF) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "write data to " << filepath << " failed");
    }
    fclose(fd);
    return;
}

void NexusUnits::InsertTopic(DomainParticipantImpl* participant, TopicImpl* topic) {
    std::lock_guard<std::mutex> lock(mtx_topics_);
    topics_[participant].insert(topic);
}

void NexusUnits::InsertDataWriter(DomainParticipantImpl* participant, DataWriterImpl* writer) {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    writers_[participant].insert(writer);
}

void NexusUnits::InsertDataReader(DomainParticipantImpl* participant, DataReaderImpl* reader) {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    readers_[participant].insert(reader);
}

void NexusUnits::DeleteParticipant(DomainParticipantImpl* participant) {
    DeleteDataWriter(participant);
    DeleteDataReader(participant);
    DeleteTopic(participant);

    std::lock_guard<std::mutex> lock(mtx_participants_);
    if (participants_.find(participant) == participants_.end())
        return;
    delete participant;
    participants_.erase(participant);
}

void NexusUnits::DeleteParticipant() {
    std::lock_guard<std::mutex> lock(mtx_participants_);
    for (auto& i : participants_) {
        delete i;
    }
    participants_.clear();
}

void NexusUnits::DeleteTopic(DomainParticipantImpl* participant, TopicImpl* topic) {
    std::lock_guard<std::mutex> lock(mtx_topics_);
    if (topics_.find(participant) == topics_.end())
        return;
    if (topics_[participant].find(topic) == topics_[participant].end())
        return;
    participant->delete_topic(topic);
    topics_[participant].erase(topic);
}

void NexusUnits::DeleteTopic(DomainParticipantImpl* participant) {
    std::lock_guard<std::mutex> lock(mtx_topics_);
    if (topics_.find(participant) == topics_.end())
        return;
    for (auto& i : topics_[participant]) {
        delete i;
    }
    topics_.erase(participant);
}

void NexusUnits::DeleteTopic() {
    std::lock_guard<std::mutex> lock(mtx_topics_);
    for (auto& i : topics_) {
        for (auto& j : i.second) {
            delete j;
        }
    }
    topics_.clear();
}

void NexusUnits::DeleteDataWriter(DomainParticipantImpl* participant, DataWriterImpl* writer) {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    if (writers_.find(participant) == writers_.end())
        return;
    if (writers_[participant].find(writer) == writers_[participant].end())
        return;
    delete writer;
    writers_[participant].erase(writer);
}

void NexusUnits::DeleteDataWriter(DomainParticipantImpl* participant) {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    if (writers_.find(participant) == writers_.end())
        return;
    for (auto& i : writers_[participant]) {
        delete i;
    }
    writers_.erase(participant);
}

void NexusUnits::DeleteDataWriter() {
    std::lock_guard<std::mutex> lock(mtx_writers_);
    for (auto& i : writers_) {
        for (auto& j : i.second) {
            delete j;
        }
    }
    writers_.clear();
}

void NexusUnits::DeleteDataReader(DomainParticipantImpl* participant, DataReaderImpl* reader) {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    if (readers_.find(participant) == readers_.end())
        return;
    if (readers_[participant].find(reader) == readers_[participant].end())
        return;
    delete reader;
    readers_[participant].erase(reader);
}

void NexusUnits::DeleteDataReader(DomainParticipantImpl* participant) {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    if (readers_.find(participant) == readers_.end())
        return;
    for (auto& i : readers_[participant]) {
        delete i;
    }
    readers_.erase(participant);
}

void NexusUnits::DeleteDataReader() {
    std::lock_guard<std::mutex> lock(mtx_readers_);
    for (auto& i : readers_) {
        for (auto& j : i.second) {
            delete j;
        }
    }
    readers_.clear();
}

NexusUnits& NexusUnits::get_unit() {
    return NexusContext::get_context().get_unit();
}

}  // namespace vbs
