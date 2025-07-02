/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>

#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "vbs/status/PublicationMatchedStatus.hpp"
#include "vbs/status/MatchedStatus.hpp"

namespace vbs {
PublicationMatchedStatus::PublicationMatchedStatus(const EVBSPublicationMatchedStatus& info)
    : status_match_status_(info.total_count, info.total_count_change, info.current_count, info.current_count_change),
      status_last_subscription_handle_() {
    content_filter_property_.content_filtered_topic_name = info.content_filter_property.content_filtered_topic_name;
    content_filter_property_.expression_parameters = info.content_filter_property.expression_parameters;
    content_filter_property_.filter_class_name = info.content_filter_property.filter_class_name;
    content_filter_property_.filter_expression = info.content_filter_property.filter_expression;
    content_filter_property_.related_topic_name = info.content_filter_property.related_topic_name;
}

PublicationMatchedStatus::PublicationMatchedStatus() : status_last_subscription_handle_(0) {}

PublicationMatchedStatus::PublicationMatchedStatus(const PublicationMatchedStatus& status) {
    *this = status;
}

}  // namespace vbs
