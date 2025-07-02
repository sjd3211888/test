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

#ifndef DSFDDS_SHARING_PAYLOADPOOL_
#define DSFDDS_SHARING_PAYLOADPOOL_

#include "history/IPayloadPool.h"
#include "deps/common/Guid.h"

namespace mbuf {
class BufferPoolOwnerManager;
}  // namespace mbuf

namespace li {
namespace dsfdds {

class SharingPayloadPool : public vbs::common::IPayloadPool {
 public:
    SharingPayloadPool(vbsutil::xmlparser::GUID_t guid, std::unique_ptr<mbuf::BufferPoolOwnerManager> pool_owner);
    SharingPayloadPool(vbsutil::xmlparser::GUID_t guid);
    virtual ~SharingPayloadPool();

    bool get_payload(uint32_t size, vbs::common::CacheChange_t& cache_change) override;

    bool get_payload(vbsutil::xmlparser::SerializedPayload_t& data, vbs::common::IPayloadPool*& data_owner,
                     vbs::common::CacheChange_t& cache_change) override;

    bool get_payload(vbsutil::xmlparser::SerializedPayload_t& data, vbs::common::CacheChange_t& cache_change) override;

    bool release_payload(vbs::common::CacheChange_t& cache_change) override;

    bool release_history(const vbs::common::PoolConfig& config, bool is_reader) override;

    bool free_payload(uint32_t mark) override;

    uint32_t get_payload_type() override { return PAYLOAD_TYPE_DSF; }

    void reference_payload(vbsutil::xmlparser::SerializedPayload_t& payload) override;

    std::shared_ptr<mbuf::BufferPoolOwnerManager> get_pool_owner() { return pool_owner_manager_; }

    void reset_pool_owner() { pool_owner_manager_.reset(); }

 private:
    vbsutil::xmlparser::GUID_t guid_;
    std::shared_ptr<mbuf::BufferPoolOwnerManager> pool_owner_manager_;
};

}  // namespace dsfdds
}  // namespace li

#endif /*DSFDDS_SHARING_PAYLOADPOOL_ */