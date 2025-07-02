// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: content filtered topic support set filter info dynamically
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// ------------------------------------------------------------------

#include "topic/ContentFilteredTopicImpl.hpp"
#include "vbs/Topic.hpp"
#include "domain/DomainParticipantImpl.hpp"

namespace vbs {

ContentFilteredTopicImpl::ContentFilteredTopicImpl(DomainParticipantImpl* local_participant, const std::string& name,
                                                   Topic* related_topic, const std::string& filter_expression,
                                                   const std::vector<std::string>& expression_parameters)
    : name_(name),
      related_topic_(related_topic),
      filter_expression_(filter_expression),
      expression_parameters_(expression_parameters) {
    if (local_participant != related_topic->get_instance()->get_participant()) {
        return;
    }
    dds_topic_ = local_participant->create_contentfilteredtopic(name_, related_topic_->get_instance(),
                                                                filter_expression_, expression_parameters_);

    if (dds_topic_ != nullptr) {
        dds_is_initialized_ = true;
    }
}

ContentFilteredTopicImpl::~ContentFilteredTopicImpl() {
    if (dds_topic_) {
        auto topic = get_related_topic();
        if (!topic || !topic->get_instance()) {
            return;
        }
        auto local_participant = topic->get_instance()->get_participant();
        if (local_participant != nullptr) {
            local_participant->delete_contentfilteredtopic(dds_topic_);
        }
    }
}

Topic* ContentFilteredTopicImpl::get_related_topic() const {
    return related_topic_;
}

const std::string& ContentFilteredTopicImpl::get_filter_expression() const {
    return filter_expression_;
}

ReturnCode_t ContentFilteredTopicImpl::get_expression_parameters(
    std::vector<std::string>& expression_parameters) const {
    expression_parameters = expression_parameters_;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

bool ContentFilteredTopicImpl::set_expression_parameters(const std::vector<std::string>& expression_parameters) {
    return dds_topic_->set_expression_parameters(expression_parameters) ==
           vbsutil::elog::ReturnCode_t(vbsutil::elog::ReturnCode_t::RETCODE_OK);
}

bool ContentFilteredTopicImpl::set_filter_expression(const std::string& filter_expression,
                                                     const std::vector<std::string>& expression_parameters) {
    return dds_topic_->set_filter_expression(filter_expression, expression_parameters) ==
           vbsutil::elog::ReturnCode_t(vbsutil::elog::ReturnCode_t::RETCODE_OK);
}

bool ContentFilteredTopicImpl::init_verify() const {
    if (dds_is_initialized_) {
        return true;
    }
    return false;
}

}  // namespace vbs
