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

#include "common/SharingPayloadPoolFactory.h"

#include "common/log/DsfLog.hpp"
#include "common/SharingPayloadPool.hpp"
#include "dsfdds/utils/QosConverters.hpp"
#include "dsfdds/transport/shm_creator.h"

namespace li {
namespace dsfdds {

std::shared_ptr<vbs::common::IPayloadPool> SharingPayloadFactory::create(uint32_t domain_id,
                                                                         vbsutil::xmlparser::GUID_t guid,
                                                                         vbs::BaseWriterAttributes& watt,
                                                                         const std::string& topic_name) {
    DataWriterQos qos {};
    utils::set_qos_from_attributes(qos, watt);
    uint16_t participant_id = guid.get_participant_id();
    auto entity_id = guid.entityId.to_uint32();
    auto pool_owner_manager = ShmCreator::CreateBufferPool(topic_name, domain_id, participant_id, entity_id, qos, true);
    if (!pool_owner_manager) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_CREATE_MEMBUF_POOL_ERR,
                      "writer ids:" << domain_id << " guid: " << guid << ",membuf pool_owner_manager create failed");
        return nullptr;
    }
    return std::make_shared<SharingPayloadPool>(guid, std::move(pool_owner_manager));
}

std::shared_ptr<vbs::common::IPayloadPool> SharingPayloadFactory::create_empty(vbsutil::xmlparser::GUID_t guid) {
    return std::make_shared<SharingPayloadPool>(guid);
}

}  // namespace dsfdds
}  // namespace li