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

#ifndef INCLUDE_EDDS_DDS_PUBLISHER_PUBLISHERLISTENER_HPP_
#define INCLUDE_EDDS_DDS_PUBLISHER_PUBLISHERLISTENER_HPP_

#include "ertps/qos/DeadlineMissedStatus.h"
#include "deps/core/status/BaseStatus.hpp"
#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "pub/DataWriterListenerInner.hpp"

namespace vbs {

class Publisher;

/**
 * Class PublisherListener, allows the end user to implement callbacks triggered by certain events.
 * It inherits all the DataWriterListenerInner callbacks.
 * @ingroup EDDS_MODULE
 */
class PublisherListener : public DataWriterListenerInner {
 public:
    /**
     * @brief Constructor
     */
    RTPS_DllAPI PublisherListener() {}

    /**
     * @brief Destructor
     */
    RTPS_DllAPI virtual ~PublisherListener() {}
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_PUBLISHER_PUBLISHERLISTENER_HPP_
