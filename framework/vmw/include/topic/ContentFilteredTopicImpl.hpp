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

#ifndef VBS_CONTENTFILTEREDTOPICIMPL_HPP_
#define VBS_CONTENTFILTEREDTOPICIMPL_HPP_

#include <map>
#include <string>

#include "core/Global.hpp"
#include "vbs/DomainParticipant.hpp"
#include "vbs/TopicQos.hpp"
#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class DomainParticipantImpl;

class ContentFilteredTopicImpl {
    friend ContentFilteredTopic;
    friend DataReaderImpl;

 private:
    ContentFilteredTopicImpl(DomainParticipantImpl* local_participant, const std::string& name, Topic* related_topic,
                             const std::string& filter_expression,
                             const std::vector<std::string>& expression_parameters);

 public:
    virtual ~ContentFilteredTopicImpl();

    Topic* get_related_topic() const;

    const std::string& get_filter_expression() const;

    ReturnCode_t get_expression_parameters(std::vector<std::string>& expression_parameters) const;

    bool set_expression_parameters(const std::vector<std::string>& expression_parameters);

    bool set_filter_expression(const std::string& filter_expression,
                               const std::vector<std::string>& expression_parameters);

    bool init_verify() const;

 private:
    vbs::ContentFilteredTopicInner* dds_topic_ = nullptr;
    bool dds_is_initialized_ = false;

    std::string name_;
    Topic* related_topic_ = nullptr;
    std::string filter_expression_;
    std::vector<std::string> expression_parameters_;
};

}  // namespace vbs

#endif  // VBS_CONTENTFILTEREDTOPICIMPL_HPP_
