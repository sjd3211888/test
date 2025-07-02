// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: report ContentFilterProperty in on_publication_matched in DataWriterListenerInner
// feature: Ensure that the writer does not miss the callback function firing after registering the first listener
// ------------------------------------------------------------------

#ifndef VBS_STATUS_PUBLICATIONMATCHEDSTATUS_HPP_
#define VBS_STATUS_PUBLICATIONMATCHEDSTATUS_HPP_

#include <cstdint>

#include "vbs/status/MatchedStatus.hpp"

namespace evbs {
namespace edds {
namespace dds {
class PublicationMatchedStatus;
}
}  // namespace edds
}  // namespace evbs

namespace vbs {

using EVBSPublicationMatchedStatus = evbs::edds::dds::PublicationMatchedStatus;

class ContentFilterProperty {
 public:
    std::string content_filtered_topic_name;
    std::string related_topic_name;
    std::string filter_class_name;
    std::string filter_expression;
    std::vector<std::string> expression_parameters;
};

class PublicationMatchedStatus {
 public:
    explicit PublicationMatchedStatus(const EVBSPublicationMatchedStatus& info);

    PublicationMatchedStatus();

    PublicationMatchedStatus(const PublicationMatchedStatus& status);

    PublicationMatchedStatus& operator=(const PublicationMatchedStatus& status) {
        status_match_status_ = status.status_match_status_;
        status_last_subscription_handle_ = status.status_last_subscription_handle_;
        content_filter_property_ = status.content_filter_property_;
        return *this;
    }

    int32_t total_count() const { return status_match_status_.total_count(); }

    int32_t total_count_change() const { return status_match_status_.total_count_change(); }

    int32_t current_count() const { return status_match_status_.current_count(); }

    int32_t current_count_change() const { return status_match_status_.current_count_change(); }

    const ContentFilterProperty& content_filter_property() const { return content_filter_property_; }

 private:
    /**
     * Handle to the last reader that matched the writer causing the status to change
     */
    MatchedStatus status_match_status_;
    /**
     * The content filter property of the reader that matched the writer
     */
    uint32_t status_last_subscription_handle_;
    ContentFilterProperty content_filter_property_;
};

}  // namespace vbs

#endif  // VBS_STATUS_PUBLICATIONMATCHEDSTATUS_HPP_
