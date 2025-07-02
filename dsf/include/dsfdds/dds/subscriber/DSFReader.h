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

#ifndef DSFREADER_H_
#define DSFREADER_H_

#include <memory>
#include <string>
#include "history/SubstrateReader.h"
#include "edds/rtps/attributes/ReaderAttributes.h"
#include "history/IChangePool.h"
#include "history/ReaderHistory.h"
#include "history/ReaderListener.h"
#include "common/types/TypesBase.h"
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "deps/attributes/BaseReaderAttributes.h"

namespace li {
namespace dsfdds {
class DSFReaderImpl;

/**
 * Class DSFReader. for vbs2.0
 */
class DSFReader : public vbs::dispatcher::SubstrateReader {
 public:
    DSFReader(uint32_t did, vbsutil::xmlparser::GUID_t guid, vbs::BaseReaderAttributes& ratt, std::string topic_name,
              std::string topic_type, std::shared_ptr<vbs::common::IChangePool>& change_pool,
              vbs::common::ReaderHistory* history, vbs::common::ReaderListener* listener,
              vbsutil::xmlparser::Locator_t* local_loc);
    virtual ~DSFReader();

    /**
   * @brief stop receiving thread
   * 
   */
    void disable() override;

    void set_trust_remote_endpoint(const GUID_t& wguid) override;
    bool add_matched_writer(const vbs::RemoteEndpointInfo& wdata) override;
    bool update_matched_writer(const vbs::RemoteEndpointInfo& wdata) override;
    bool delete_matched_writer(const vbs::RemoteEndpointInfo& wdata) override;

    /**
   * Get the domain id of this DSFReader.
   * @return domain id
   */
    uint32_t get_domain_id() const;

    /**
   * Get the qos of this DSFReader.
   * @return DataReaderQos
   */
    const DataReaderQos& get_qos() const;

    /**
   * Get the guid of this DSFReader.
   * @return guid
   */
    vbsutil::xmlparser::GUID_t guid() const;

    ReturnCode_t enable();

 private:
    uint32_t domainID;
    vbs::BaseReaderAttributes& readerAttributes;
    std::string topicName;
    DSFReaderImpl* dsfReaderImpl;
};

} /* namespace dsfdds */
} /* namespace li */

#endif /*DSFREADER_H_*/
