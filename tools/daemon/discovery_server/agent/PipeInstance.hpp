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

#ifndef PIPE_INSTANCE_HPP
#define PIPE_INSTANCE_HPP

#include "CLIParser.hpp"

#include <agent/types/EndpointType.hpp>

#include "domain/DomainParticipantImpl.hpp"
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>
#include "pub/DataWriterImpl.hpp"
#include <pub/Publisher.hpp>
#include <xmlparser/qos/XMLDataWriterQos.hpp>
#include <sub/DataReaderImpl.hpp>
#include <sub/Subscriber.hpp>
#include <xmlparser/qos/XMLDataReaderQos.hpp>

#include <thread>
#include <mutex>

#define ENTITY_KEY_TO_TOPIC_ID(key) (((key & 0xff0000) >> 16) | ((key & 0xff00)) | ((key & 0xff) << 16))

namespace discovery_server {
namespace agent {

using namespace evbs::edds::dds;
/**
 * @brief   Singleton class to manage the launch process of a MicroXRCE-DDS Agent.
 */
class PipeInstance {
 private:
    /**
     * @brief   Default constructor.
     */
    PipeInstance();
    /**
     * @brief   PipeInstance class shall not be copy constructible.
     */
    PipeInstance(const PipeInstance&) = delete;

    PipeInstance(PipeInstance&&) = delete;

    /**
     * @brief   PipeInstance class shall not be copy assignable.
     */
    PipeInstance& operator=(const PipeInstance&) = delete;

    PipeInstance& operator=(PipeInstance&&) = delete;

    void qos_convert(DataWriterQos& qos, const config::composite_endpoint_attr& composite_attr);
    void qos_convert(DataReaderQos& qos, const config::composite_endpoint_attr& composite_attr);

    vbs::DataReaderImpl* create_sub_writer(vbs::Subscriber* subscriber,
                                           const config::composite_endpoint_attr& composite_attr,
                                           const config::sub_endpoint_attr& sub_endpoint_attr);

    vbs::DataWriterImpl* create_sub_reader(vbs::Publisher* publisher,
                                           const config::composite_endpoint_attr& composite_attr,
                                           const config::sub_endpoint_attr& sub_endpoint_attr);

    bool create_pipe_writer(const config::composite_writer_attr* writer_attr);

    bool create_pipe_reader(const config::composite_reader_attr* reader_attr);

    vbs::TopicImpl* get_topic(vbs::DomainParticipantImpl* partticipant, const std::string& topic_name,
                              const std::string& type_name);

 public:
    static PipeInstance& getInstance();

    bool create(const CLIParser::server_config& server_config);

    void clear();

 private:
    std::mutex mutex_;
    vbs::DomainParticipantImpl* participant_ {nullptr};
    vbs::DataWriterImpl* pipe_writer_ {nullptr};
    vbs::DataReaderImpl* pipe_reader_ {nullptr};
    std::unordered_map<std::string, vbs::TopicImpl*> dds_topics_;
    // key:composite reader, value:[topic id, dds writer]
    std::unordered_map<vbs::DataReaderImpl*, std::unordered_map<uint32_t, vbs::DataWriterImpl*>> dds_writers_;
    // key:dds reader, value:topic id
    std::unordered_map<vbs::DataReaderImpl*, uint32_t> dds_readers_;

    class DDSReaderListener : public vbs::DataReaderListenerInner {
     public:
        DDSReaderListener(PipeInstance* pipe_obj);

        DDSReaderListener() = delete;

        void on_data_available(vbs::DataReaderImpl* reader) override;

        PipeInstance* pipe_obj_;
    } dds_reader_listener_;

    class PipeReaderListener : public vbs::DataReaderListenerInner {
     public:
        PipeReaderListener(PipeInstance* pipe_obj);

        PipeReaderListener() = delete;

        void on_data_available(vbs::DataReaderImpl* reader) override;

        PipeInstance* pipe_obj_;
    } pipe_listener_;
};
}  // namespace agent
}  // namespace discovery_server

#endif  // PIPE_INSTANCE_HPP
