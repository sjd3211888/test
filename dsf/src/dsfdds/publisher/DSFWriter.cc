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

#include "dsfdds/dds/publisher/DSFWriter.h"
#include "dsfdds/publisher/DSFWriterImpl.h"
#include "dsfdds/utils/QosConverters.hpp"
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "DsfLog.hpp"
namespace li {
namespace dsfdds {

/**
 * Class DSFWriter for vbs2.0
 */
DSFWriter::DSFWriter(uint32_t did, vbsutil::xmlparser::GUID_t guid, vbs::BaseWriterAttributes& watt,
                     std::string topic_name, std::string topic_type,
                     std::shared_ptr<vbs::common::IPayloadPool>& payload_pool,
                     std::shared_ptr<vbs::common::IChangePool>& change_pool, vbs::common::WriterHistory* history,
                     vbs::common::WriterListener* listener, vbsutil::xmlparser::Locator_t* local_loc)
    : domainID(did), evbsGuid(guid), writerAttributes(watt), historyPtr(history) {
    static_cast<void>(change_pool);
    listener_ = listener;
    DataWriterQos qos;
    utils::set_qos_from_attributes(qos, watt);
    dsfWriterImpl =
        new DSFWriterImpl(did, guid, topic_name, topic_type, payload_pool, qos, history, local_loc, listener);
    if (dsfWriterImpl == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CREATE_OBJECT_ERR,
                      "create dsfWriterImpl failed!, guid: " << evbsGuid << ", topic_name: " << topic_name
                                                             << ", topic_type: " << topic_type
                                                             << ", domainID: " << domainID);
    } else {
        DSF_LOG_DEBUG(DSFWriter, "create dsfWriterImpl success!, guid: " << evbsGuid << ", topic_name: " << topic_name
                                                                         << ", topic_type: " << topic_type
                                                                         << ", domainID: " << domainID);
    }
}

// for dsf
bool DSFWriter::add_matched_reader(const vbs::RemoteEndpointInfo& rdata) {
    return (VbsDiscoveryEndpoint::GetInstance()->ProcessWriterReaderMatch(get_domain_id(), guid(), rdata));
}

bool DSFWriter::update_matched_reader(const vbs::RemoteEndpointInfo& rdata) {
    static_cast<void>(rdata);
    return false;
}

bool DSFWriter::delete_matched_reader(const vbs::RemoteEndpointInfo& rdata) {
    return (VbsDiscoveryEndpoint::GetInstance()->ProcessWriterReaderUnMatch(get_domain_id(), guid(), rdata));
}

//for all
bool DSFWriter::write(vbs::common::CacheChange_t* data,
                      const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time, bool in_history) {

    if (dsfWriterImpl->unsent_change_added_to_history(data, max_blocking_time)) {
        DSF_LOG_DEBUG(DSFWriter, "write change success sequenceNumber: "
                                     << data->sequenceNumber << " payload length: " << data->serializedPayload.length
                                     << " sourceTimestamp: " << data->sourceTimestamp);
        if (in_history && listener_) {
            listener_->onWriterChangeReceivedByAll(this, data);
        }
        return true;
    }
    return false;
}

DSFWriter::~DSFWriter() {
    delete dsfWriterImpl;
}

/**
 * Get the domain id of this DSFWriter.
 * @return domain id
 */
uint32_t DSFWriter::get_domain_id() const {
    return domainID;
}

const DataWriterQos& DSFWriter::get_qos() const {
    return dsfWriterImpl->dsfWriterQos;
}

/**
 * Get the guid of this DSFWriter.
 * @return guid
 */
vbsutil::xmlparser::GUID_t DSFWriter::guid() const {
    return dsfWriterImpl->guid();
}

ReturnCode_t DSFWriter::enable() {
    ReturnCode_t ret_code = dsfWriterImpl->enable();
    return ret_code;
}

} /* namespace dsfdds */
} /* namespace li */
