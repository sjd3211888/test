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
// feature: use Listeners and NexusUnits as singleton
// ------------------------------------------------------------------

#ifndef VBS_DATAREADERLISTENER_HPP_
#define VBS_DATAREADERLISTENER_HPP_

#include "vbs/DataReader.hpp"
#include "vbs/status/SubscriptionMatchedStatus.hpp"
#include "vbs/status/DeadlineMissedStatus.hpp"
#include "vbs/status/LivelinessChangedStatus.hpp"
#include "vbs/status/BaseStatus.hpp"
#include "vbs/status/EndpointCrashStatus.hpp"
#include "vbs/status/E2EExceptionStatus.hpp"

namespace vbs {

class DataReader;

class DataReaderListener {
 public:
    /**
     * @brief Constructor
     */
    DataReaderListener();

    /**
     * @brief Destructor
     */
    virtual ~DataReaderListener();

    /**
     * This method is called when the Subscriber read the data.
     *
     * @param reader Pointer to the associated Subscriber
     */
    virtual void on_data_available(DataReader* const reader);

    /**
     * This method is called when the c is matched (or unmatched) against an endpoint.
     *
     * @param reader Pointer to the associated Subscriber
     * @param info Information regarding the matched publisher
     */
    virtual void on_subscription_matched(DataReader* const reader, const vbs::SubscriptionMatchedStatus& info);

    /**
     * Virtual method to be called when a topic misses the deadline period
     *
     * @param reader DataReader
     * @param status The requested deadline missed status
     */
    virtual void on_requested_deadline_missed(DataReader* const reader,
                                              const vbs::RequestedDeadlineMissedStatus& status);

    /**
     * @brief Method called when the liveliness status associated to a subscriber changes
     *
     * @param reader The DataReader
     * @param status The liveliness changed status
     */
    virtual void on_liveliness_changed(DataReader* const reader, const vbs::LivelinessChangedStatus& status);

    /**
     * @brief Method called when a sample was lost.
     *
     * @param reader The DataReader
     * @param status The sample lost status
     */
    virtual void on_sample_lost(DataReader* const reader, const vbs::SampleLostStatus& status);

    /**
     * A method called when a endpoint is crashed
     *
     * @param reader Pointer to the associated Publisher
     * @param status The endpoint crash status
     */
    virtual void on_endpoint_crash(DataReader* const reader, const vbs::EndpointCrashStatus& status);

    /**
     * A method called when on_e2e_exception
     *
     * @param status on_e2e_exception
     */
    virtual void on_e2e_exception(const E2EExceptionStatus& status);
};

}  // namespace vbs

#endif  // VBS_DATAREADERLISTENER_HPP_
