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

#ifndef INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBERLISTENER_HPP_
#define INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBERLISTENER_HPP_

#include "ertps/ertps_dll.h"
#include "ertps/qos/DeadlineMissedStatus.h"
#include "ertps/qos/LivelinessChangedStatus.h"
#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "sub/DataReaderListenerInner.hpp"

namespace vbs {
class Subscriber;

/**
 * Class SubscriberListener, it should be used by the end user to implement specific callbacks to
 * certain actions. It also inherits all DataReaderListenerInner callbacks.
 *
 * @ingroup EDDS_MODULE
 */
class SubscriberListener : public vbs::DataReaderListenerInner {
 public:
    /**
     * @brief Constructor
     */
    RTPS_DllAPI SubscriberListener() {}

    /**
     * @brief Destructor
     */
    RTPS_DllAPI virtual ~SubscriberListener() {}

    /**
     * Virtual function to be implemented by the user containing the actions to be performed when a
     * new Data Message is available on any reader.
     *
     * @param sub Subscriber
     */
    RTPS_DllAPI virtual void on_data_on_readers(Subscriber* sub) { (void)sub; }
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_SUBSCRIBER_SUBSCRIBERLISTENER_HPP_
