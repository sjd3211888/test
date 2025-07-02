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

#include "vbs/DataReaderListener.hpp"

namespace vbs {

DataReaderListener::DataReaderListener() {}

DataReaderListener::~DataReaderListener() {}

void DataReaderListener::on_data_available(DataReader* const vbs_reader) {
    (void)vbs_reader;
}

void DataReaderListener::on_subscription_matched(DataReader* const vbs_reader, const SubscriptionMatchedStatus& info) {
    (void)vbs_reader;
    (void)info;
}

void DataReaderListener::on_requested_deadline_missed(DataReader* const vbs_reader,
                                                      const RequestedDeadlineMissedStatus& status) {
    (void)vbs_reader;
    (void)status;
}

void DataReaderListener::on_liveliness_changed(DataReader* const vbs_reader, const LivelinessChangedStatus& status) {
    (void)vbs_reader;
    (void)status;
}

void DataReaderListener::on_sample_lost(DataReader* const vbs_reader, const SampleLostStatus& status) {
    (void)vbs_reader;
    (void)status;
}

void DataReaderListener::on_endpoint_crash(DataReader* const vbs_reader, const EndpointCrashStatus& status) {
    (void)vbs_reader;
    (void)status;
}

void DataReaderListener::on_e2e_exception(const E2EExceptionStatus& status) {
    (void)status;
}

}  // namespace vbs
