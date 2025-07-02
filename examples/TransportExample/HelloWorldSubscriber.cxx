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
#include <vbs/DomainParticipantFactory.hpp>
#include <vbs/DataReader.hpp>
#include <vbs/DataReaderQos.hpp>
#include <fstream>

#include "HelloWorldSubscriber.h"
#include "HelloWorldPubSubTypes.h"

int64_t GetCurTime();

HelloWorldSubscriber::HelloWorldSubscriber()
    : participant_(nullptr), topic_(nullptr), reader_(nullptr), type_(new HelloWorldPubSubType()) {}

HelloWorldSubscriber::~HelloWorldSubscriber() {
    if (participant_ != nullptr && reader_ != nullptr) {
        participant_->delete_data_reader(reader_);
    }
    if (participant_ != nullptr && topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }
    delete participant_;
    participant_ = nullptr;
}

bool HelloWorldSubscriber::init(const std::string& xml_config_file) {

    /* Create DomainParticipant*/
    (void)xml_config_file;
    std::string participant_profile_name = "sub_participant_profile";
    std::string topic_profile_name = "HelloWorldTopic";
    std::string participant_profile_file_name = "participant_profile_file_name";
    participant_ = vbs::DomainParticipantFactory::create_participant("test_xml_sub.xml", participant_profile_name);
    if (participant_ == nullptr) {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("Example HelloWorld", type_.get_type_name(), type_, topic_profile_name);
    if (topic_ == nullptr) {
        return false;
    }

    reader_ = participant_->create_data_reader(topic_, "test_reader_prof", &listener_);
    if (reader_ == nullptr) {
        return false;
    }

    return true;
}

void HelloWorldSubscriber::SubListener::on_subscription_matched(vbs::DataReader*,
                                                                const vbs::SubscriptionMatchedStatus& info) {
    if (info.current_count_change() == 1) {
        matched = info.total_count();
        std::cout << "Subscriber matched." << std::endl;

    } else if (info.current_count_change() == -1) {
        matched = info.total_count();
        std::cout << "Subscriber unmatched." << std::endl;

    } else {
        std::cout << info.current_count_change()
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void HelloWorldSubscriber::run() {
    std::cout << "Waiting for Data, press Enter to stop the DataReader. " << std::endl;
    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == "exit" || input == "quit") {
            std::cout << "Shutting down the Subscriber." << std::endl;
            break;
        }
    }
    std::cout << "Shutting down the Subscriber." << std::endl;
}

void HelloWorldSubscriber::SubListener::on_data_available(vbs::DataReader* reader) {
    vbs::LoanableCollection<HelloWorld, std::false_type> st;
    vbs::SampleInfo info;
    reader->take(&st, &info);
    // Print your structure data here.
    for (int i = 0; i < static_cast<int>(st.length()); ++i) {
        int64_t t1 = GetCurTime();
        const HelloWorld& out_example = st[i];
        std::cout << t1 << ", Reseive sample, count=" << out_example.index() << "  " << out_example.message().length() << std::endl;

        //std::cout << out_example << std::endl;
    }
}
