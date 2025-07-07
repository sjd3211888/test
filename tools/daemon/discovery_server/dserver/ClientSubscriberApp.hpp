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

#ifndef DISCOVERY_SERVER__CLIENTSUBSCRIBERAPP_HPP
#define DISCOVERY_SERVER__CLIENTSUBSCRIBERAPP_HPP

#include <condition_variable>

#include "domain/DomainParticipantImpl.hpp"
#include <sub/DataReaderListenerInner.hpp>
#include <deps/common/TypeSupport.hpp>
#include "topic/TopicImpl.hpp"

#include "CLIParser.hpp"
#include "Client.hpp"
#include "Application.hpp"

using namespace evbs::edds::dds;

namespace discovery_server {

class ClientSubscriberApp : public Application, public vbs::DataReaderListenerInner {
 public:
    ClientSubscriberApp(const CLIParser::client_subscriber_config& config);

    ~ClientSubscriberApp();

    //! Subscription callback
    void on_data_available(vbs::DataReaderImpl* reader) override;

    //! Subscriber matched method
    void on_subscription_matched(vbs::DataReaderImpl* reader, const SubscriptionMatchedStatus& info) override;

    //! Run subscriber
    void run() override;

    //! Trigger the end of execution
    void stop() override;

 private:
    //! Return the current state of execution
    bool is_stopped();

    Client hello_;

    vbs::DomainParticipantImpl* participant_;

    vbs::Subscriber* subscriber_;

    vbs::TopicImpl* topic_;

    vbs::DataReaderImpl* reader_;

    vbs::TypeSupport type_;

    uint16_t samples_;

    uint16_t received_samples_;

    std::atomic<bool> stop_;

    mutable std::mutex terminate_cv_mtx_;

    std::condition_variable terminate_cv_;
};

}  // namespace discovery_server

#endif  // DISCOVERY_SERVER__CLIENTSUBSCRIBERAPP_HPP
