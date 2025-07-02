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

#include "vbs/DataWriterListener.hpp"

namespace vbs {

DataWriterListener::DataWriterListener() {}

DataWriterListener::~DataWriterListener() {}

void DataWriterListener::on_publication_matched(DataWriter* const writer, const PublicationMatchedStatus& info) {
    (void)writer;
    (void)info;
}

void DataWriterListener::on_offered_deadline_missed(DataWriter* const writer,
                                                    const OfferedDeadlineMissedStatus& status) {
    (void)writer;
    (void)status;
}

void DataWriterListener::on_liveliness_lost(DataWriter* const writer, const LivelinessLostStatus& status) {
    (void)writer;
    (void)status;
}

void DataWriterListener::on_endpoint_crash(DataWriter* const writer, const EndpointCrashStatus& status) {
    (void)writer;
    (void)status;
}

}  // namespace vbs
