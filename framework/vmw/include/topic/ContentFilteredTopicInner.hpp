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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef EDDS_DDS_TOPIC_CONTENTFILTEREDTOPIC_HPP
#define EDDS_DDS_TOPIC_CONTENTFILTEREDTOPIC_HPP

#include "elog/ReturnCode.hpp"
#include "topic/TopicDescription.hpp"

#define EDDS_SQLFILTER_NAME vbs::sqlfilter_name

namespace vbs {

class DomainParticipantImpl;
class TopicImpl;
class ContentFilteredTopicCore;

constexpr const char* const sqlfilter_name = "DDSSQL";

/**
 * Specialization of TopicDescription that allows for content-based subscriptions.
 *
 * @ingroup EDDS_MODULE
 */
class ContentFilteredTopicInner : public vbs::TopicDescription {
    friend class vbs::DomainParticipantImpl;

 private:
    ContentFilteredTopicInner(const std::string& name, vbs::TopicImpl* related_topic,
                              const std::string& filter_expression,
                              const std::vector<std::string>& expression_parameters);

 public:
    virtual ~ContentFilteredTopicInner();

    /**
     * @brief Getter for the related topic.
     *
     * This operation returns the Topic associated with the ContentFilteredTopicInner.
     * That is, the Topic specified when the ContentFilteredTopicInner was created.
     */
    vbs::TopicImpl* get_related_topic() const;

    /**
     * @brief Get the filter expression.
     *
     * This operation returns filter expression associated with this ContentFilteredTopicInner.
     * It will return the @c filter_expression specified on the last successful call to @c set_expression or,
     * if that method is never called, the expression specified when the ContentFilteredTopicInner was created.
     *
     * @return the @c filter_expression.
     */
    const std::string& get_filter_expression() const;

    /**
     * @brief Get the expression parameters.
     *
     * This operation returns expression parameters associated with this ContentFilteredTopicInner.
     * These will be the @c expression_parameters specified on the last successful call to @c set_expression or
     * @c set_expression_parameters.
     * If those methods have never been called, the expression parameters specified when the ContentFilteredTopicInner
     * was created will be returned.
     *
     * @param [out] expression_parameters  The expression parameters currently associated with the
     *                                     ContentFilteredTopicInner.
     *
     * @return RETCODE_OK
     */
    vbsutil::elog::ReturnCode_t get_expression_parameters(std::vector<std::string>& expression_parameters) const;

    /**
     * @brief Set the expression parameters.
     *
     * This operation changes expression parameters associated with this ContentFilteredTopicInner.
     *
     * @param [in] expression_parameters  The expression parameters to set.
     *
     * @return RETCODE_OK             if the expression parameters where correctly updated.
     * @return RETCODE_BAD_PARAMETER  if the expression parameters do not match with the current @c filter_expression.
     */
    vbsutil::elog::ReturnCode_t set_expression_parameters(const std::vector<std::string>& expression_parameters);

    /**
     * @brief Set the filter expression and the expression parameters.
     *
     * This operation changes the filter expression and the expression parameters associated with this
     * ContentFilteredTopicInner.
     *
     * @param [in] filter_expression      The filter expression to set.
     * @param [in] expression_parameters  The expression parameters to set.
     *
     * @return RETCODE_OK             if the expression and parameters where correctly updated.
     * @return RETCODE_BAD_PARAMETER  if @c filter_expression is not valid for this ContentFilteredTopicInner.
     * @return RETCODE_BAD_PARAMETER  if the expression parameters do not match with the @c filter_expression.
     */
    vbsutil::elog::ReturnCode_t set_filter_expression(const std::string& filter_expression,
                                                      const std::vector<std::string>& expression_parameters);

    /**
     * @brief Getter for the DomainParticipant
     * @return DomainParticipant pointer
     */
    vbs::DomainParticipantImpl* get_participant() const override;

    vbs::TopicDescriptionImpl* get_impl() const override;

 protected:
    vbs::ContentFilteredTopicCore* impl_;
};

}  // namespace vbs

#endif  // EDDS_DDS_TOPIC_CONTENTFILTEREDTOPIC_HPP
