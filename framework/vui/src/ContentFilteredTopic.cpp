// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
#include "vbs/Topic.hpp"
#include "vbs/DomainParticipant.hpp"
#include "vbs/ContentFilteredTopic.hpp"
#include "topic/ContentFilteredTopicImpl.hpp"

namespace vbs {

ContentFilteredTopic::ContentFilteredTopic(DomainParticipant* local_participant, const std::string& name,
                                           Topic* related_topic, const std::string& filter_expression,
                                           const std::vector<std::string>& expression_parameters)
    : impl_(new ContentFilteredTopicImpl(local_participant->get_instance(), name, related_topic, filter_expression,
                                         expression_parameters)) {}

ContentFilteredTopic::~ContentFilteredTopic() {
    delete impl_;
    impl_ = nullptr;
}

Topic* ContentFilteredTopic::get_related_topic() const {
    return impl_->get_related_topic();
}

const std::string& ContentFilteredTopic::get_filter_expression() const {
    return impl_->get_filter_expression();
}

ReturnCode_t ContentFilteredTopic::get_expression_parameters(std::vector<std::string>& expression_parameters) const {
    return impl_->get_expression_parameters(expression_parameters);
}

bool ContentFilteredTopic::set_expression_parameters(const std::vector<std::string>& expression_parameters) {
    return impl_->set_expression_parameters(expression_parameters);
}

bool ContentFilteredTopic::set_filter_expression(const std::string& filter_expression,
                                                 const std::vector<std::string>& expression_parameters) {
    return impl_->set_filter_expression(filter_expression, expression_parameters);
}

ContentFilteredTopicImpl* ContentFilteredTopic::get_instance() const {
    return impl_;
}

bool ContentFilteredTopic::init_verify() const {
    return impl_->init_verify();
}

}  // namespace vbs
