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

#ifndef INCLUDE_EDDS_DDS_PUBLISHER_DATAWRITERLISTENER_HPP_
#define INCLUDE_EDDS_DDS_PUBLISHER_DATAWRITERLISTENER_HPP_

#include "edds/rtps/common/Types.h"
#include "deps/core/status/BaseStatus.hpp"
#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "deps/core/status/IncompatibleQosStatus.hpp"
#include "deps/core/status/DeadlineMissedStatus.hpp"
#include "deps/common/InstanceHandle.h"
#include "deps/common/Guid.h"
#include "elog/Log.hpp"

namespace vbs {
class DataWriterImpl;

/**
 * Class DataWriterListenerInner, allows the end user to implement callbacks triggered by certain events.
 * @ingroup EDDS_MODULE
 */
class RTPS_DllAPI DataWriterListenerInner {
 public:
    /**
     * @brief Constructor
     */
    DataWriterListenerInner() {}

    /**
     * @brief Destructor
     */
    virtual ~DataWriterListenerInner() {}

    /**
     * This method is called when the Publisher is matched (or unmatched) against an endpoint.
     *
     * @param writer Pointer to the associated Publisher
     * @param info Information regarding the matched subscriber
     */
    virtual void on_publication_matched(vbs::DataWriterImpl* writer,
                                        const evbs::edds::dds::PublicationMatchedStatus& info) {
        (void)writer;
        (void)info;
    }

    /**
     * A method called when a deadline is missed
     *
     * @param writer Pointer to the associated Publisher
     * @param status The deadline missed status
     */
    virtual void on_offered_deadline_missed(vbs::DataWriterImpl* writer,
                                            const evbs::edds::dds::OfferedDeadlineMissedStatus& status) {
        (void)writer;
        (void)status;
    }

    /**
     * A method called when an incompatible QoS is offered
     *
     * @param writer Pointer to the associated Publisher
     * @param status The deadline missed status
     */
    virtual void on_offered_incompatible_qos(vbs::DataWriterImpl* writer,
                                             const evbs::edds::dds::OfferedIncompatibleQosStatus& status) {
        (void)writer;
        (void)status;
    }

    /**
     * @brief Method called when the liveliness of a publisher is lost
     *
     * @param writer The publisher
     * @param status The liveliness lost status
     */
    virtual void on_liveliness_lost(vbs::DataWriterImpl* writer, const evbs::edds::dds::LivelinessLostStatus& status) {
        (void)writer;
        (void)status;
    }

    /**
     * @brief Method called when a sample has been removed unacknowledged
     *
     * @param writer Pointer to the associated DataWriter
     * @param instance Handle to the instance the sample was removed from
     */
    virtual void on_unacknowledged_sample_removed(vbs::DataWriterImpl* writer,
                                                  const vbsutil::xmlparser::InstanceHandle_t& instance) {
        static_cast<void>(writer);
        static_cast<void>(instance);
        logDebug(DATA_WRITER, "Remove an unacknowledged sample.");
    }

    virtual void on_endpoint_crash(const vbsutil::xmlparser::GUID_t& reader_guid) { static_cast<void>(reader_guid); }
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_PUBLISHER_DATAWRITERLISTENER_HPP_
