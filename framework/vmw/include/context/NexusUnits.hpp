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

#ifndef INCLUDE_CONTEXT_NEXUS_UNITS_H_
#define INCLUDE_CONTEXT_NEXUS_UNITS_H_

#include <set>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

#include "core/Global.hpp"
#include "core/Macros.h"

namespace vbs {
class DomainParticipantImpl;
class TopicImpl;
class DataWriterImpl;
class DataReaderImpl;
#if !defined(_WIN32)
class TraceOnline;
#endif
class NexusUnits {
    friend class DomainParticipant;
    friend class Topic;
    friend class DataWriter;
    friend class DataReader;
    friend class TopicImpl;
    friend class NexusContext;

 public:
    ~NexusUnits();

    void InsertParticipant(DomainParticipantImpl* participant);

    void InsertTopic(DomainParticipantImpl* participant, TopicImpl* topic);

    void InsertDataWriter(DomainParticipantImpl* participant, DataWriterImpl* writer);

    void InsertDataReader(DomainParticipantImpl* participant, DataReaderImpl* reader);

    void DeleteParticipant(DomainParticipantImpl* participant);

    void DeleteParticipant();

    void DeleteTopic(DomainParticipantImpl* participant, TopicImpl* topic);

    void DeleteTopic(DomainParticipantImpl* participant);

    void DeleteTopic();

    void DeleteDataWriter(DomainParticipantImpl* participant, DataWriterImpl* writer);

    void DeleteDataWriter(DomainParticipantImpl* participant);

    void DeleteDataWriter();

    void DeleteDataReader(DomainParticipantImpl* participant, DataReaderImpl* reader);

    void DeleteDataReader(DomainParticipantImpl* participant);

    void DeleteDataReader();

    void dump_topic_traffic(const char* filepath);

    static NexusUnits& get_unit();

 private:
    NexusUnits();
    std::shared_ptr<DomainParticipantFactoryInner> dds_participant_factory_;
    std::mutex mtx_participants_;
    std::mutex mtx_topics_;
    std::mutex mtx_writers_;
    std::mutex mtx_readers_;
    std::set<DomainParticipantImpl*> participants_;
    std::map<DomainParticipantImpl*, std::set<TopicImpl*>> topics_;
    std::map<DomainParticipantImpl*, std::set<DataWriterImpl*>> writers_;
    std::map<DomainParticipantImpl*, std::set<DataReaderImpl*>> readers_;

    DISALLOW_COPY_AND_ASSIGN(NexusUnits)
};
}  // namespace vbs

#endif  // INCLUDE_CONTEXT_NEXUS_UNITS_H_
