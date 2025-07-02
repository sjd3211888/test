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
#ifndef _EDDS_GENERATED_910612643_HELLOWORLD_SUBSCRIBER_H_
#define _EDDS_GENERATED_910612643_HELLOWORLD_SUBSCRIBER_H_

#include "HelloWorldPubSubTypes.h"

#include <mutex>
#include <vbs/DomainParticipant.hpp>
#include <vbs/DataReader.hpp>
#include <vbs/DataReaderListener.hpp>
#include <vbs/Global.hpp>
#include <vbs/DataReaderQos.hpp>

class HelloWorldSubscriber {
 public:
    HelloWorldSubscriber();

    virtual ~HelloWorldSubscriber();

    bool init(const std::string& xml_config_file);

    void run();

 private:
    vbs::DomainParticipant* participant_;
    vbs::Topic* topic_;
    vbs::DataReader* reader_;
    vbs::TypeSupport type_;

    std::string xml_config_file_;
    // QoS Profiles
    vbs::DataReaderQos dr_qos_;

    class SubListener : public vbs::DataReaderListener {
     public:
        SubListener() {}
        ~SubListener() override {}

        void on_data_available(vbs::DataReader* reader) override;

        void on_subscription_matched(vbs::DataReader* reader, const vbs::SubscriptionMatchedStatus& info) override;

        int matched = 0;
        uint32_t samples = 0;
    } listener_;
};

#endif  // _EDDS_GENERATED_910612643_HELLOWORLD_SUBSCRIBER_H_