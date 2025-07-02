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
#include "HelloWorldPublisher.h"

#include <vbs/DomainParticipantFactory.hpp>
#include <vbs/DataWriter.hpp>
#include <vbs/DataWriterQos.hpp>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <chrono>


int64_t GetCurTime(){
    // 获取当前时间点（UTC 时间，自 1970-01-01 00:00:00）
    auto now = std::chrono::system_clock::now();
    
    // 转换为微秒精度的时间戳
    auto micro_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    );
    int64_t microseconds = micro_since_epoch.count();
    return microseconds;
    //std::cout << "微秒级时间戳: " << microseconds << std::endl;
}

HelloWorldPublisher::HelloWorldPublisher()
    : participant_(nullptr), topic_(nullptr), writer_(nullptr), type_(new HelloWorldPubSubType()) {}

HelloWorldPublisher::~HelloWorldPublisher() {
    if (participant_ != nullptr && writer_ != nullptr) {
        participant_->delete_data_writer(writer_);
    }
    if (participant_ != nullptr && topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }
    delete participant_;
    participant_ = nullptr;
}

bool HelloWorldPublisher::init(const std::string& xml_config_file) {
    /* Initialize data_ here */

    /* Create DomainParticipant*/
    std::string participant_profile_name = "pub_participant_profile";
    std::string topic_profile_name = "HelloWorldTopic";
    std::string writer_profile_name = "test_writer_prof";
    xml_config_file_ = xml_config_file;

    participant_ = vbs::DomainParticipantFactory::create_participant("test_xml_pub.xml", participant_profile_name);

    if (participant_ == nullptr) {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("Example HelloWorld", type_.get_type_name(), type_, topic_profile_name);

    if (topic_ == nullptr) {
        return false;
    }
    // CREATE THE WRITER
    writer_ = participant_->create_data_writer(topic_, writer_profile_name, &listener_);
    if (writer_ == nullptr) {
        return false;
    }
    std::cout << "HelloWorld DataWriter created." << std::endl;
    return true;
}

void HelloWorldPublisher::PubListener::on_publication_matched(vbs::DataWriter*,
                                                              const vbs::PublicationMatchedStatus& info) {
    if (info.current_count_change() == 1) {
        matched = info.total_count();
        std::cout << "DataWriter matched. matched is " << matched << std::endl;
    } else if (info.current_count_change() == -1) {
        matched = info.total_count();
        std::cout << "DataWriter unmatched." << std::endl;

    } else {
        std::cout << info.current_count_change()
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void HelloWorldPublisher::run() {
    std::cout << "HelloWorld DataWriter waiting for DataReaders." << std::endl;
    while (listener_.matched == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));  // Sleep 250 ms
    }
    // Publication code

    HelloWorld st;
    st.index(0);
    //st.message("helloworld");
    std::string str;
    int isize = 1024*1024*40; //*1024*40
    str.resize(isize);
    for(int i=0 ; i<isize ; ++i) str[i] = 't';
    str[isize] = '\0';
    st.message(str);
    /* Initialize your structure here */

    int msgsent = 0;

    while (msgsent < 1000) {
        int64_t t1 = GetCurTime();
        st.index(st.index() + 1);
        writer_->write(&st);
        ++msgsent;
        //std::cout << "Sending sample, count=" << msgsent << std::endl;
        std::cout << t1 << ", Sending sample, count=" << msgsent << "   " << st.message().length() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Sleep 300 ms
    }
}