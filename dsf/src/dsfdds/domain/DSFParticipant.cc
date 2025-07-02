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

#include "dsfdds/dds/domain/DSFParticipant.h"
#include "dsfdds/dds/publisher/DSFWriter.h"
#include "dsfdds/dds/subscriber/DSFReader.h"
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "DsfLog.hpp"

namespace li {
namespace dsfdds {

DSFParticipant* DSFParticipant::create_DSFParticipant(uint32_t did) {
    return new DSFParticipant(did);
}

bool DSFParticipant::delete_DSFParticipant(const vbs::dispatcher::SubstrateParticipant* part) {
    delete (DSFParticipant*)part;
    return true;
}

DSFParticipant::DSFParticipant(uint32_t did)
    : domainID(did), discovery_endpoint_(VbsDiscoveryEndpoint::GetInstance()) {}
DSFParticipant::~DSFParticipant() = default;

vbs::dispatcher::SubstrateWriter* DSFParticipant::create_writer(
    GUID_t guid, vbs::BaseWriterAttributes& watt, std::string topic_name, std::string topic_type,
    std::shared_ptr<IPayloadPool>& payload_pool, std::shared_ptr<IChangePool>& change_pool, WriterHistory* history,
    vbs::common::WriterListener* listener, vbsutil::xmlparser::Locator_t* local_loc) {
    DSFWriter* writer;

    writer = new DSFWriter(domainID, guid, watt, topic_name, topic_type, payload_pool, change_pool, history, listener,
                           local_loc);
    if (writer) {
        if (writer->enable() != ReturnCode_t::RETCODE_PUBSUB_OK) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "DSFWriter enable failed! guid:" << guid << ", topic_name:" << topic_name << ",topic_type: "
                                                           << topic_type << ", domainID: " << domainID);
        } else {
            DSF_LOG_DEBUG(DSFParticipant, "create and enable DSFWriter success! guid:"
                                              << guid << ", topic_name:" << topic_name << ",topic_type: " << topic_type
                                              << ", domainID: " << domainID);
        }
        return dynamic_cast<vbs::dispatcher::SubstrateWriter*>(writer);
    } else {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                      "create DSFWriter failed! guid:" << guid << ", topic_name:" << topic_name
                                                       << ",topic_type: " << topic_type << ", domainID: " << domainID);
    }
    return nullptr;
}

vbs::dispatcher::SubstrateReader* DSFParticipant::create_reader(
    GUID_t guid, vbs::BaseReaderAttributes& ratt, std::string topic_name, std::string topic_type,
    std::shared_ptr<IChangePool>& change_pool, ReaderHistory* history, vbs::common::ReaderListener* listener,
    vbsutil::xmlparser::Locator_t* local_loc) {
    DSFReader* reader;

    reader = new DSFReader(domainID, guid, ratt, topic_name, topic_type, change_pool, history, listener, local_loc);
    if (reader) {
        if (reader->enable() != ReturnCode_t::RETCODE_PUBSUB_OK) {
            DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                          "DSFReader enable failed! , guid:" << guid << ", topic_name:" << topic_name
                                                             << ",topic_type: " << topic_type);
        } else {
            DSF_LOG_DEBUG(DSFParticipant, "create and enable DSFReader success! guid:"
                                              << guid << ", topic_name:" << topic_name << ",topic_type: " << topic_type
                                              << ", domainID: " << domainID << ",locator: " << local_loc);
        }
        return dynamic_cast<vbs::dispatcher::SubstrateReader*>(reader);
    } else {
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
            "create DSFReader failed! guid:" << guid << ", topic_name:" << topic_name << ",topic_type: " << topic_type);
    }
    return nullptr;
}

bool DSFParticipant::delete_writer(vbs::dispatcher::SubstrateWriter* writer) {
    delete (DSFWriter*)writer;
    return true;
}

bool DSFParticipant::delete_reader(vbs::dispatcher::SubstrateReader* reader) {
    delete (DSFReader*)reader;
    return true;
}

}  // namespace dsfdds
}  // namespace li
