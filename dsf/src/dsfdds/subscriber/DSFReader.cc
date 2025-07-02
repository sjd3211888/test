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

#include "dsfdds/dds/subscriber/DSFReader.h"
#include "dsfdds/subscriber/DSFReaderImpl.h"
#include "dsfdds/utils/QosConverters.hpp"
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "DsfLog.hpp"
namespace li {
namespace dsfdds {

DSFReader::DSFReader(uint32_t did, vbsutil::xmlparser::GUID_t guid, vbs::BaseReaderAttributes& ratt,
                     std::string topic_name, std::string topic_type,
                     std::shared_ptr<vbs::common::IChangePool>& change_pool, vbs::common::ReaderHistory* history,
                     vbs::common::ReaderListener* listener, vbsutil::xmlparser::Locator_t* local_loc)
    : domainID(did), readerAttributes(ratt), topicName(topic_name) {
    DataReaderQos qos;
    utils::set_qos_from_attributes(qos, ratt);
    dsfReaderImpl =
        new DSFReaderImpl(domainID, guid, topic_name, topic_type, qos, change_pool, history, local_loc, listener);
    if (dsfReaderImpl == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ERROR,
                      "create dsfReaderImpl failed! guid: " << guid << ", topic_name: " << topic_name
                                                            << ", topic_type: " << topic_type
                                                            << ", domainID: " << domainID);
    } else {
        DSF_LOG_DEBUG(DSFReader, "create dsfReaderImpl success! guid: " << guid << ", topic_name: " << topic_name
                                                                        << ", topic_type: " << topic_type
                                                                        << ", domainID: " << domainID);
    }
}

DSFReader::~DSFReader() {
    DSF_LOG_DEBUG(DSFReader, "dtor " << guid());
    delete dsfReaderImpl;
}

void DSFReader::disable() {
    dsfReaderImpl->disable();
}

void DSFReader::set_trust_remote_endpoint(const GUID_t& wguid) {
  return (VbsDiscoveryEndpoint::GetInstance()->ReaderAddPeerId(get_domain_id(), guid(), wguid));
}
bool DSFReader::add_matched_writer(const vbs::RemoteEndpointInfo& wdata) {
    return (VbsDiscoveryEndpoint::GetInstance()->ProcessReaderWriterMatch(get_domain_id(), guid(), wdata));
}
bool DSFReader::update_matched_writer(const vbs::RemoteEndpointInfo& wdata) {
    static_cast<void>(wdata);
    return false;
}
bool DSFReader::delete_matched_writer(const vbs::RemoteEndpointInfo& wdata) {
    return (VbsDiscoveryEndpoint::GetInstance()->ProcessReaderWriterUnMatch(get_domain_id(), guid(), wdata));
}
#if 0
bool DSFReader::change_removed_by_history(vbs::common::CacheChange_t* change) {
  static_cast<void>(change);
  return false;
}
#endif
/**
 * Get the domain id of this DSFReader.
 * @return domain id
 */
uint32_t DSFReader::get_domain_id() const {
    return domainID;
}

const DataReaderQos& DSFReader::get_qos() const {
    return dsfReaderImpl->dataReaderQos;
}

/**
 * Get the guid of this DSFReader.
 * @return guid
 */
vbsutil::xmlparser::GUID_t DSFReader::guid() const {
    return dsfReaderImpl->guid();
}

ReturnCode_t DSFReader::enable() {
    ReturnCode_t ret_code = dsfReaderImpl->enable();
    return ret_code;
}

} /* namespace dsfdds */
} /* namespace li */
