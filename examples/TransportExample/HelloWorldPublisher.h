// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: Modify test cases
// ------------------------------------------------------------------
#ifndef _EDDS_GENERATED_1238148503_HELLOWORLD_PUBLISHER_H_
#define _EDDS_GENERATED_1238148503_HELLOWORLD_PUBLISHER_H_

#include "HelloWorldPubSubTypes.h"

#include <mutex>
#include <vbs/DomainParticipant.hpp>
#include <vbs/DataWriter.hpp>
#include <vbs/DataWriterListener.hpp>
#include <vbs/Global.hpp>
#include <vbs/DataWriterQos.hpp>

class HelloWorldPublisher {
 public:
    HelloWorldPublisher();

    virtual ~HelloWorldPublisher();

    bool init(const std::string& xml_config_file);

    void run();

 private:
    vbs::DomainParticipant* participant_;
    vbs::Topic* topic_;
    vbs::DataWriter* writer_;
    vbs::TypeSupport type_;

    std::string xml_config_file_;
    class PubListener : public vbs::DataWriterListener {
     public:
        PubListener() {}

        ~PubListener() override {}

        void on_publication_matched(vbs::DataWriter* writer, const vbs::PublicationMatchedStatus& info) override;

        int matched = 0;
    } listener_;
};

#endif  // _EDDS_GENERATED_1238148503_HELLOWORLD_PUBLISHER_H_