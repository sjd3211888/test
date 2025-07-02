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

#ifndef DSF_PARTICIPANT_H_
#define DSF_PARTICIPANT_H_

#include <string>
#include <memory>
#include "history/SubstrateParticipant.h"
#include "history/SubstrateReader.h"
#include "history/SubstrateWriter.h"

namespace li {
namespace dsfdds {

class DiscoveryEndpoint;

/**
 * Class DSFParticipant used to create DSFReader and DSFWriter
 */
class DSFParticipant : public vbs::dispatcher::SubstrateParticipant {
 public:
    /**
   * Create a DSFParticipant for vbs2.0 dispatcher use
   * @param did Domain Id.
   */
    static DSFParticipant* create_DSFParticipant(uint32_t did);

    /**
   * Remove a DSFParticipant
   * @param part Pointer to the DSFParticipant.
   * @return true or false
   */
    static bool delete_DSFParticipant(const vbs::dispatcher::SubstrateParticipant* part);

 public:
    DSFParticipant(uint32_t did);
    virtual ~DSFParticipant();

    vbs::dispatcher::SubstrateWriter* create_writer(GUID_t guid, vbs::BaseWriterAttributes& watt,
                                                    std::string topic_name, std::string topic_type,
                                                    std::shared_ptr<vbs::common::IPayloadPool>& payload_pool,
                                                    std::shared_ptr<vbs::common::IChangePool>& change_pool,
                                                    WriterHistory* history, vbs::common::WriterListener* listener,
                                                    vbsutil::xmlparser::Locator_t* local_loc = nullptr) override;

    vbs::dispatcher::SubstrateReader* create_reader(GUID_t guid, vbs::BaseReaderAttributes& ratt,
                                                    std::string topic_name, std::string topic_type,
                                                    std::shared_ptr<vbs::common::IChangePool>& change_pool,
                                                    ReaderHistory* history, vbs::common::ReaderListener* listener,
                                                    vbsutil::xmlparser::Locator_t* local_loc = nullptr) override;

    bool delete_writer(vbs::dispatcher::SubstrateWriter* writer) override;

    bool delete_reader(vbs::dispatcher::SubstrateReader* reader) override;

 private:
    uint32_t domainID;
    std::shared_ptr<DiscoveryEndpoint>
        discovery_endpoint_;  // hold instance to guarantee its lifespan not less than our participant
};

}  // namespace dsfdds
}  // namespace li

#endif /*DSF_PARTICIPANT_H_ */
