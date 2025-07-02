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

#ifndef VBS_DATAWRITERLISTENER_HPP_
#define VBS_DATAWRITERLISTENER_HPP_

#include "vbs/Global.hpp"
#include "vbs/status/PublicationMatchedStatus.hpp"
#include "vbs/status/DeadlineMissedStatus.hpp"
#include "vbs/status/BaseStatus.hpp"
#include "vbs/status/EndpointCrashStatus.hpp"

namespace vbs {

class DataWriter;

class DataWriterListener {
 public:
    /**
     * @brief Constructor
     */
    DataWriterListener();

    /**
     * @brief Destructor
     */
    virtual ~DataWriterListener();

    /**
     * This method is called when the Publisher is matched (or unmatched) against an endpoint.
     *
     * @param writer Pointer to the associated Publisher
     * @param info Information regarding the matched subscriber
     */
    virtual void on_publication_matched(DataWriter* const writer, const vbs::PublicationMatchedStatus& info);

    /**
     * A method called when a deadline is missed
     *
     * @param writer Pointer to the associated Publisher
     * @param status The deadline missed status
     */
    virtual void on_offered_deadline_missed(DataWriter* const writer, const vbs::OfferedDeadlineMissedStatus& status);

    /**
     * @brief Method called when the liveliness of a publisher is lost
     *
     * @param writer The publisher
     * @param status The liveliness lost status
     */
    virtual void on_liveliness_lost(DataWriter* const writer, const vbs::LivelinessLostStatus& status);

    /**
    * A method called when a endpoint is crashed
    *
    * @param writer Pointer to the associated DataWriter
    * @param status The endpoint crash status
    */
    virtual void on_endpoint_crash(DataWriter* const writer, const vbs::EndpointCrashStatus& status);
};

}  // namespace vbs

#endif  // VBS_DATAWRITERLISTENER_HPP_
