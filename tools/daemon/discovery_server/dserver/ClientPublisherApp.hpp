// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef DISCOVERY_SERVER__CLIENTPUBLISHERAPP_HPP
#define DISCOVERY_SERVER__CLIENTPUBLISHERAPP_HPP

#include <condition_variable>

#include "domain/DomainParticipantImpl.hpp"
#include <pub/DataWriterListenerInner.hpp>
#include <deps/common/TypeSupport.hpp>
#include "topic/TopicImpl.hpp"

#include "Application.hpp"
#include "CLIParser.hpp"
#include "Client.hpp"

using namespace evbs::edds::dds;

namespace discovery_server {

class ClientPublisherApp : public Application, public vbs::DataWriterListenerInner {
 public:
    ClientPublisherApp(const CLIParser::client_publisher_config& config);

    ~ClientPublisherApp();

    //! Publisher matched method
    void on_publication_matched(vbs::DataWriterImpl* writer, const PublicationMatchedStatus& info) override;

    //! Run publisher
    void run() override;

    //! Stop publisher
    void stop() override;

 private:
    //! Return the current state of execution
    bool is_stopped();

    //! Publish a sample
    bool publish();

    Client hello_;

    vbs::DomainParticipantImpl* participant_;

    vbs::Publisher* publisher_;

    vbs::TopicImpl* topic_;

    vbs::DataWriterImpl* writer_;

    vbs::TypeSupport type_;

    int16_t matched_;

    uint16_t samples_;

    std::mutex mutex_;

    std::condition_variable cv_;

    std::atomic<bool> stop_;

    const uint16_t period_ms_ {100};  // in ms
};

}  // namespace discovery_server

#endif  // DISCOVERY_SERVER__CLIENTPUBLISHERAPP_HPP
