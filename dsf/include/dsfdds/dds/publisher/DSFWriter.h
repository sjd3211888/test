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

#ifndef DSFWRITER_H_
#define DSFWRITER_H_

#include <string>
#include <memory>
#include "history/SubstrateWriter.h"
#include "edds/rtps/attributes/WriterAttributes.h"
#include "common/all_common.h"
#include "common/types/TypesBase.h"
#include "history/IPayloadPool.h"
#include "history/IChangePool.h"
#include "history/WriterHistory.h"
#include "history/WriterListener.h"
#include "deps/common/Guid.h"
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "deps/attributes/BaseWriterAttributes.h"

namespace li {
namespace dsfdds {
class DSFWriterImpl;
struct BuildMbufContext;

/**
 * Class DSFWriter for vbs2.0
 */
class DSFWriter : public vbs::dispatcher::SubstrateWriter {
 public:
    DSFWriter(uint32_t did, vbsutil::xmlparser::GUID_t guid, vbs::BaseWriterAttributes& watt, std::string topic_name,
              std::string topic_type, std::shared_ptr<vbs::common::IPayloadPool>& payload_pool,
              std::shared_ptr<vbs::common::IChangePool>& change_pool, vbs::common::WriterHistory* history,
              vbs::common::WriterListener* listener, vbsutil::xmlparser::Locator_t* local_loc);
    // for dsf
    bool add_matched_reader(const vbs::RemoteEndpointInfo& rdata) override;
    bool update_matched_reader(const vbs::RemoteEndpointInfo& rdata) override;
    bool delete_matched_reader(const vbs::RemoteEndpointInfo& rdata) override;

    // for all
    bool write(vbs::common::CacheChange_t* data,
               const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time, bool in_history) override;
    virtual ~DSFWriter();

    /**
   * Get the domain id of this DSFWriter.
   * @return domain id
   */
    uint32_t get_domain_id() const;

    /**
   * Get the qos of this DSFWriter.
   * @return DataWriterQos
   */
    const DataWriterQos& get_qos() const;

    /**
   * Get the guid of this DSFWriter.
   * @return guid
   */
    vbsutil::xmlparser::GUID_t guid() const;
    ReturnCode_t enable();

 private:
    uint32_t domainID;
    vbsutil::xmlparser::GUID_t evbsGuid;
    vbs::BaseWriterAttributes& writerAttributes;
    vbs::common::WriterHistory* historyPtr;
    DSFWriterImpl* dsfWriterImpl;
    vbs::common::WriterListener* listener_;
};

} /* namespace dsfdds */
} /* namespace li */

#endif  // DSFWRITER_H_
