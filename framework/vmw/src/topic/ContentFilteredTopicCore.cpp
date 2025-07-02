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
// ------------------------------------------------------------------

#include <algorithm>

#include "ContentFilteredTopicCore.hpp"
#include "elog/ReturnCode.hpp"
#include "edds/dds/core/policy/ParameterList.hpp"
#include "edds/dds/core/policy/ParameterTypes.hpp"
#include "domain/DomainParticipantImpl.hpp"
#include "domain/qos/DomainParticipantQosInner.hpp"
#include "sub/DataReaderImpl.hpp"
#include "edds/rtps/messages/CDRMessage.hpp"
#include "edds/rtps/interfaces/ContentFilterUtils.hpp"

namespace vbs {

bool ContentFilteredTopicCore::is_relevant(const evbs::ertps::rtps::CacheChange_t& change,
                                           const evbs::ertps::rtps::GUID_t& reader_guid) const {
    (void)reader_guid;

    bool ret_val = true;
    if (check_filter_signature(change, ret_val)) {
        return ret_val;
    }
    return true;
}

vbsutil::elog::ReturnCode_t ContentFilteredTopicCore::set_expression_parameters(
    const char* new_expression, const std::vector<std::string>& new_expression_parameters) {
    vbs::DomainParticipantQosInner pqos;
    related_topic->get_participant()->get_qos(pqos);
    if (new_expression_parameters.size() > vbsutil::xmlparser::ContentFilterProperty::parameters_limit_) {
        elogError(CONTENT_FILTERED_TOPIC, vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER,
                  "Number of expression parameters exceeds maximum allocation limit: "
                      << new_expression_parameters.size() << " > "
                      << vbsutil::xmlparser::ContentFilterProperty::parameters_limit_);
        return vbsutil::elog::ReturnCode_t::RETCODE_BAD_PARAMETER;
    }

    vbs::LoanableSequence<const char*>::size_type n_params;
    n_params = static_cast<vbs::LoanableSequence<const char*>::size_type>(new_expression_parameters.size());
    vbs::LoanableSequence<const char*> filter_parameters(n_params);
    filter_parameters.length(n_params);
    while (n_params > 0) {
        n_params--;
        if (n_params >= static_cast<vbs::LoanableSequence<const char*>::size_type>(new_expression_parameters.size())) {
            continue;
        }
        filter_parameters[n_params] = new_expression_parameters[n_params].c_str();
    }

    filter_property.expression_parameters.assign(new_expression_parameters.begin(), new_expression_parameters.end());
    if (nullptr != new_expression) {
        filter_property.filter_expression = new_expression;
    }

    update_signature();

    // Inform data readers
    for (vbs::DataReaderImpl* reader : readers_) {
        reader->filter_has_been_updated();
    }
    return vbsutil::elog::ReturnCode_t::RETCODE_OK;
}

void ContentFilteredTopicCore::update_signature() {
    ContentFilterUtils::compute_signature(filter_property, filter_signature_);
}

bool ContentFilteredTopicCore::check_filter_signature(const evbs::ertps::rtps::CacheChange_t& change,
                                                      bool& filter_result) const {
    // Empty expressions always pass the filter
    if (filter_property.filter_expression.empty()) {
        filter_result = true;
        return true;
    }

    return ContentFilterUtils::check_filter_signature(change, filter_signature_, filter_result);
}

} /* namespace vbs */
