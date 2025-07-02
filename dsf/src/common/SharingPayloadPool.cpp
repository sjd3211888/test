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

#include "common/SharingPayloadPool.hpp"

#include "history/CacheChange.h"
#include "common/log/DsfLog.hpp"
#include "BufferPoolOwnerManager.h"
#include "dsfdds/discovery/discovery_endpoint_vbs.h"
#include "dsfdds/transport/crash_listener.h"
#include "common/types/TypesBase.h"

namespace li {
namespace dsfdds {

SharingPayloadPool::SharingPayloadPool(vbsutil::xmlparser::GUID_t guid,
                                       std::unique_ptr<mbuf::BufferPoolOwnerManager> pool_owner_manager)
    : guid_(guid), pool_owner_manager_(std::move(pool_owner_manager)) {}

SharingPayloadPool::SharingPayloadPool(vbsutil::xmlparser::GUID_t guid) : guid_(guid), pool_owner_manager_ {} {}

SharingPayloadPool::~SharingPayloadPool() {
    DSF_LOG_DEBUG(SharingPayloadPool, "dtor guid: " << guid_);
}

bool SharingPayloadPool::get_payload(uint32_t size, vbs::common::CacheChange_t& cache_change) {
    if (!pool_owner_manager_) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_POOL_OWNER_MANAGER_IS_NULL,
                      "pool_owner_manager_ is nullptr, guid " << guid_);
        cache_change.serializedPayload.data = nullptr;
        cache_change.serializedPayload.max_size = 0U;
        cache_change.payload_owner(nullptr);
        return false;
    }
    int64_t poolId = -1;
    auto mbuf = pool_owner_manager_->AllocBuffer(size, poolId);
    if (mbuf == nullptr) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_ALLOC_BUFFER_FAILED,
                      "alloc buffer failed, guid " << guid_);
        cache_change.serializedPayload.data = nullptr;
        cache_change.serializedPayload.max_size = 0U;
        cache_change.payload_owner(nullptr);
        return false;
    }
    if (poolId > 0) {
        // place old pool id into free pool list
        CrashListener::GetInstance(VbsDiscoveryEndpoint::GetInstance())
            ->poolCrashStrategy.UpdateWriterRecycleMbufPools(poolId);
    }
    cache_change.serializedPayload.data = static_cast<vbsutil::xmlparser::octet*>(mbuf->data());
    cache_change.serializedPayload.max_size = mbuf->size();
    cache_change.serializedPayload.mbuf = std::move(mbuf);
    cache_change.payload_owner(this);
    return true;
}

bool SharingPayloadPool::get_payload(vbsutil::xmlparser::SerializedPayload_t& data,
                                     vbs::common::IPayloadPool*& data_owner, vbs::common::CacheChange_t& cache_change) {
    assert(cache_change.writerGUID != GUID_t::unknown());
    assert(cache_change.sequenceNumber != vbs::common::SequenceNumber_t::unknown());

    if (!pool_owner_manager_) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_POOL_OWNER_MANAGER_IS_NULL,
                      "pool_owner_manager_ is nullptr, guid " << guid_);
        return false;
    }

    if (data_owner == this) {
        cache_change.serializedPayload.data = data.data;
        cache_change.serializedPayload.length = data.length;
        cache_change.serializedPayload.max_size = data.mbuf->size();
        cache_change.serializedPayload.mbuf = data.mbuf;
        cache_change.payload_owner(this);
        return true;
    } else {
        if (get_payload(data.length, cache_change)) {
            if (!cache_change.serializedPayload.copy(&data, true)) {
                (void)release_payload(cache_change);
                return false;
            }

            if (data_owner == nullptr) {
                data_owner = this;
                data.data = cache_change.serializedPayload.data;
                data.mbuf = cache_change.serializedPayload.mbuf;
            }

            return true;
        }
    }

    return false;
}

bool SharingPayloadPool::get_payload(vbsutil::xmlparser::SerializedPayload_t& data,
                                     vbs::common::CacheChange_t& cache_change) {
    assert(cache_change.writerGUID != GUID_t::unknown());
    assert(cache_change.sequenceNumber != vbs::common::SequenceNumber_t::unknown());

    if (get_payload(data.length, cache_change)) {
        if (!cache_change.serializedPayload.copy(&data, true)) {
            (void)release_payload(cache_change);
            return false;
        }
        return true;
    }

    return false;
}

bool SharingPayloadPool::release_payload(vbs::common::CacheChange_t& cache_change) {
    cache_change.serializedPayload.length = 0U;
    cache_change.serializedPayload.pos = 0U;
    cache_change.serializedPayload.max_size = 0U;
    cache_change.serializedPayload.data = nullptr;
    cache_change.serializedPayload.origin_data = nullptr;
    cache_change.payload_owner(nullptr);
    cache_change.serializedPayload.mbuf = nullptr;
    cache_change.nonSerializedPayload.length = 0U;
    cache_change.nonSerializedPayload.pos = 0U;
    cache_change.nonSerializedPayload.max_size = 0U;
    cache_change.nonSerializedPayload.data = nullptr;
    cache_change.nonSerializedPayload.mbuf = nullptr;
    return true;
}

bool SharingPayloadPool::release_history(const vbs::common::PoolConfig& config, bool is_reader) {
    static_cast<void>(config);
    static_cast<void>(is_reader);
    return true;
}

bool SharingPayloadPool::free_payload(uint32_t mark) {
    static_cast<void>(mark);
    return true;
}

void SharingPayloadPool::reference_payload(vbsutil::xmlparser::SerializedPayload_t& payload) {
    static_cast<void>(payload);
}

}  // namespace dsfdds
}  // namespace li