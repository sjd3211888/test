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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef INCLUDE_EDDS_DDS_TOPIC_TOPICLISTENER_HPP_
#define INCLUDE_EDDS_DDS_TOPIC_TOPICLISTENER_HPP_

#include "ertps/ertps_dll.h"
#include "deps/core/status/BaseStatus.hpp"

namespace vbs {
class TopicImpl;

/**
 * Class TopicListener, it should be used by the end user to implement specific callbacks to certain
 * actions.
 *
 * @ingroup EDDS_MODULE
 */
class RTPS_DllAPI TopicListener {
 public:
    /**
     * @brief Constructor
     */
    TopicListener() {}

    /**
     * @brief Destructor
     */
    virtual ~TopicListener() {}

    /**
     * Virtual function to be implemented by the user containing the actions to be performed when
     * another topic exists with the same name but different characteristics.
     *
     * @param topic Topic
     * @param status The inconsistent topic status
     */
    virtual void on_inconsistent_topic(vbs::TopicImpl* topic, evbs::edds::dds::InconsistentTopicStatus status) {
        (void)topic;
        (void)status;
    }
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_TOPIC_TOPICLISTENER_HPP_
