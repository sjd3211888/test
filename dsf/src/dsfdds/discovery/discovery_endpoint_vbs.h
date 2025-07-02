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

#ifndef DSFDDS_DISCOVERY_VBS_DISCOVERYENDPOINT_H
#define DSFDDS_DISCOVERY_VBS_DISCOVERYENDPOINT_H

#include "dsfdds/discovery/discovery_endpoint.h"

namespace li {
namespace dsfdds {
class VbsDiscoveryEndpoint : public DiscoveryEndpoint {
 public:
    VbsDiscoveryEndpoint(VbsDiscoveryEndpoint&&) = delete;
    VbsDiscoveryEndpoint(VbsDiscoveryEndpoint const&) = delete;
    VbsDiscoveryEndpoint& operator=(VbsDiscoveryEndpoint&&) = delete;
    VbsDiscoveryEndpoint& operator=(VbsDiscoveryEndpoint const&) = delete;
    VbsDiscoveryEndpoint();
    virtual ~VbsDiscoveryEndpoint();

    static std::shared_ptr<VbsDiscoveryEndpoint> GetInstance();

    bool RegisterLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) override;
    bool RemoveLocalReaderInfo(ShmReader* shm_reader, const DataReaderQos& qos) override;

    bool RegisterLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) override;
    bool RemoveLocalWriterInfo(ShmWriter* shm_writer, const DataWriterQos& qos) override;

    bool ProcessWriterReaderMatch(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid,
                                  const vbs::RemoteEndpointInfo& rdata);

    bool ProcessWriterReaderUnMatch(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid,
                                    const vbs::RemoteEndpointInfo& rdata);

    void ReaderAddPeerId(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& reader_guid,
                         const vbsutil::xmlparser::GUID_t& writer_guid);

    bool ProcessReaderWriterMatch(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid,
                                  const vbs::RemoteEndpointInfo& wdata);
    bool ProcessReaderWriterUnMatch(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid,
                                    const vbs::RemoteEndpointInfo& wdata);

 private:
    ShmWriter* FindLocalWriter(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid);
    ShmReader* FindLocalReader(const uint32_t domain_id, const vbsutil::xmlparser::GUID_t& local_guid);
};
}  // namespace dsfdds
}  // namespace li
#endif