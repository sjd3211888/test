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

#ifndef INCLUDE_EDDS_DDS_SUBSCRIBER_DATAREADERLISTENER_HPP_
#define INCLUDE_EDDS_DDS_SUBSCRIBER_DATAREADERLISTENER_HPP_

#include "ertps/ertps_dll.h"
#include "ertps/qos/DeadlineMissedStatus.h"
#include "ertps/qos/LivelinessChangedStatus.h"
#include "ertps/qos/SampleRejectedStatus.hpp"
#include "ertps/qos/SampleLostStatus.hpp"
#include "deps/core/status/IncompatibleQosStatus.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "deps/common/Guid.h"
#include "elog/Log.hpp"

namespace vbs {
class DataReaderImpl;

/**
 * Class DataReaderListenerInner, it should be used by the end user to implement specific callbacks to
 * certain actions.
 * @ingroup EDDS_MODULE
 */
class DataReaderListenerInner {
 public:
    /**
     * @brief Constructor
     */
    RTPS_DllAPI DataReaderListenerInner() {}

    /**
     * @brief Destructor
     */
    RTPS_DllAPI virtual ~DataReaderListenerInner() {}

    /**
     * Virtual function to be implemented by the user containing the actions to be performed when
     * new Data Messages are received.
     *
     * @param reader DataReader
     */
    RTPS_DllAPI virtual void on_data_available(DataReaderImpl* reader) { (void)reader; }

    /**
     * Virtual method to be called when the subscriber is matched with a new Writer (or unmatched);
     * i.e., when a writer publishing in the same topic is discovered.
     *
     * @param reader DataReader
     * @param info The subscription matched status
     */
    RTPS_DllAPI virtual void on_subscription_matched(DataReaderImpl* reader,
                                                     const evbs::edds::dds::SubscriptionMatchedStatus& info) {
        (void)reader;
        (void)info;
    }

    /**
     * Virtual method to be called when a topic misses the deadline period
     *
     * @param reader DataReader
     * @param status The requested deadline missed status
     */
    RTPS_DllAPI virtual void on_requested_deadline_missed(DataReaderImpl* reader,
                                                          const evbs::ertps::RequestedDeadlineMissedStatus& status) {
        (void)reader;
        (void)status;
    }

    /**
     * @brief Method called when the liveliness status associated to a subscriber changes
     *
     * @param reader The DataReader
     * @param status The liveliness changed status
     */
    RTPS_DllAPI virtual void on_liveliness_changed(DataReaderImpl* reader,
                                                   const evbs::ertps::LivelinessChangedStatus& status) {
        (void)reader;
        (void)status;
    }

    /**
     * @brief Method called when a sample was rejected.
     *
     * @param reader The DataReader
     * @param status The rejected status
     */
    RTPS_DllAPI virtual void on_sample_rejected(DataReaderImpl* reader,
                                                const evbs::ertps::SampleRejectedStatus& status) {
        (void)reader;
        (void)status;
    }

    /**
     * @brief Method called an incompatible QoS was requested.
     *
     * @param reader The DataReader
     * @param status The requested incompatible QoS status
     */
    RTPS_DllAPI virtual void on_requested_incompatible_qos(
        DataReaderImpl* reader, const evbs::edds::dds::RequestedIncompatibleQosStatus& status) {
        (void)reader;
        (void)status;
    }

    /**
     * @brief Method called when a sample was lost.
     *
     * @param reader The DataReader
     * @param status The sample lost status
     */
    RTPS_DllAPI virtual void on_sample_lost(DataReaderImpl* reader, const evbs::edds::dds::SampleLostStatus& status) {
        (void)reader;
        logWarning(SAMPLE_LOST,
                   "Subscriber sample lost " << status.total_count << " Last state:" << status.last_status << ".");
    }

    /**
     * @brief Method called when a e2e exception was found.
     * @param status The sample lost status
     */
    RTPS_DllAPI virtual void on_e2e_exception(const evbs::edds::dds::E2EExceptionStatus& status) { (void)status; }

    RTPS_DllAPI virtual void on_endpoint_crash(const vbsutil::xmlparser::GUID_t& writer_guid) {
        static_cast<void>(writer_guid);
    }
};

} /* namespace vbs */

#endif  // INCLUDE_EDDS_DDS_SUBSCRIBER_DATAREADERLISTENER_HPP_
