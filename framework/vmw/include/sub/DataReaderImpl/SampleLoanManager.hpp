// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: support loan_sample
// feature: use SharingPayloadPool while transport_type is SHM
// ------------------------------------------------------------------

#ifndef _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLELOANMANAGER_HPP_
#define _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLELOANMANAGER_HPP_

#include <algorithm>
#include <cassert>

#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "deps/common/TypeSupport.hpp"
#include "history/CacheChange.h"
#include "edds/rtps/common/SerializedPayload.h"
#include "history/IPayloadPool.h"
#include "ertps/types/TypesBase.h"
#include "ertps/utils/collections/ResourceLimitedContainerConfig.hpp"
#include "ertps/utils/collections/ResourceLimitedVector.hpp"
#include "history/TopicPayloadPool.hpp"

namespace vbs {
namespace detail {

using TypeSupport = vbs::TypeSupport;

struct SampleLoanManager {
    using CacheChange_t = vbs::common::CacheChange_t;
    using IPayloadPool = vbs::common::IPayloadPool;
    using PoolConfig = vbs::common::PoolConfig;
    using SampleIdentity = evbs::ertps::rtps::SampleIdentity;
    using SerializedPayload_t = vbs::SerializedPayload_t;

    SampleLoanManager(const PoolConfig& pool_config, const TypeSupport& type)
        : limits_(pool_config.initial_size,
                  pool_config.maximum_size ? pool_config.maximum_size : std::numeric_limits<size_t>::max(), 1U),
          free_loans_(limits_),
          used_loans_(limits_),
          type_(type) {
        for (size_t n = 0U; n < limits_.initial; ++n) {
            OutstandingLoanItem item;
            if (!type_->is_plain()) {
                item.sampledata = type_->createData();
            }
            (void)free_loans_.push_back(item);
        }
    }

    ~SampleLoanManager() {
        for (const OutstandingLoanItem& item : free_loans_) {
            if (item.sampledata != nullptr) {
                type_->deleteData(item.sampledata);
            }
        }
    }

    int32_t num_allocated() const {
        assert(used_loans_.size() <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));
        return static_cast<int32_t>(used_loans_.size());
    }

    bool get_loan(CacheChange_t* change, void*& sample, bool raw_buffer) {
        bool ret = false;
        OutstandingLoanItem* item = nullptr;
        // Early return an already loaned item, only useful for change which has been readed.
        if (change->isRead) {
            item = find_by_change(change);
            if (nullptr != item) {
                item->num_refs += 1U;
                sample = item->sample;
                return true;
            }
        }

        // Get an item from the pool
        if (free_loans_.empty()) {
            // Try to create a new entry
            item = used_loans_.push_back({});
            if (nullptr != item) {
                // Create sample if necessary
                if (!(raw_buffer || (change->serializedPayload.data[2] == 1))) {
                    item->sampledata = type_->createData();
                }
            }
        } else {
            // Reuse a free entry
            item = used_loans_.push_back(free_loans_.back());
            assert(nullptr != item);
            if (item->sampledata == nullptr && !(raw_buffer || (change->serializedPayload.data[2] == 1))) {
                item->sampledata = type_->createData();
            }
            free_loans_.pop_back();
        }

        // Should always find an entry, as resource limits are checked before calling this method
        assert(nullptr != item);

        if (nullptr != item) {
            // Should be the first time we loan this item
            assert(item->num_refs == 0);

            // Increment references of input payload
            change->payload_owner()->reference_payload(change->serializedPayload);
            item->owner = change->payload_owner();
            item->payload = change->serializedPayload;
            if (change->nonSerializedPayload.data) {
                change->payload_owner()->reference_payload(change->nonSerializedPayload);
                item->nonSerializedPayload = change->nonSerializedPayload;
            }

            // Perform deserialization
            if (raw_buffer) {
                item->sample = item->payload.data;
                ret = true;
            } else if (change->serializedPayload.data[2] == 1) {
                auto ptr = item->payload.data;
                ptr += item->payload.representation_header_size;
                item->sample = ptr;
                ret = true;
            } else if (type_->is_big()) {
                /*域内收包大buffer零拷贝是2个payload，域间收包是1个payload */
                if (item->nonSerializedPayload.length > 0) {
                    ret = type_->deserialize(&item->payload, &item->nonSerializedPayload, item->sampledata);
                } else {
                    ret = type_->deserialize(&item->payload, item->sampledata);
                }
                item->sample = item->sampledata;
            } else {
                ret = type_->deserialize(&item->payload, item->sampledata);
                item->sample = item->sampledata;
            }

            // Increment reference counter and return sample
            item->num_refs += 1U;
            sample = item->sample;
        }
        return ret;
    }

    void return_loan(void* sample) {
        OutstandingLoanItem* item = find_by_sample(sample);
        assert(nullptr != item);

        item->num_refs -= 1U;
        if (item->num_refs == 0U) {
            CacheChange_t tmp;
            tmp.payload_owner(item->owner);
            tmp.serializedPayload = item->payload;
            tmp.nonSerializedPayload = item->nonSerializedPayload;
            (void)item->owner->release_payload(tmp);
            item->payload.data = nullptr;
            item->nonSerializedPayload.data = nullptr;
            item->owner = nullptr;
            item->sample = nullptr;
#if !defined(_WIN32)
            item->payload.mbuf = nullptr;
            item->nonSerializedPayload.mbuf = nullptr;
#endif
            item = free_loans_.push_back(*item);
            assert(nullptr != item);
            (void)used_loans_.remove(*item);
        }
    }

 private:
    struct OutstandingLoanItem {
        void* sample = nullptr;
        void* sampledata = nullptr;
        SampleIdentity identity;
        SerializedPayload_t payload;
        SerializedPayload_t nonSerializedPayload;
        IPayloadPool* owner = nullptr;
        uint32_t num_refs = 0U;

        ~OutstandingLoanItem() { payload.data = nullptr; }

        OutstandingLoanItem() = default;
        OutstandingLoanItem(const OutstandingLoanItem&) = default;
        OutstandingLoanItem& operator=(const OutstandingLoanItem&) = default;
        OutstandingLoanItem(OutstandingLoanItem&&) = default;
        OutstandingLoanItem& operator=(OutstandingLoanItem&&) = default;

        bool operator==(const OutstandingLoanItem& other) const {
            return (other.sample == sample) && (other.payload.data == payload.data);
        }
    };

    using collection_type = evbs::ertps::ResourceLimitedVector<OutstandingLoanItem>;

    evbs::ertps::ResourceLimitedContainerConfig limits_;
    collection_type free_loans_;
    collection_type used_loans_;
    TypeSupport type_;

    OutstandingLoanItem* find_by_change(CacheChange_t* change) {
        SampleIdentity id;
        (void)id.writer_guid(change->writerGUID);
        (void)id.sequence_number(change->sequenceNumber);

        auto comp = [id](const OutstandingLoanItem& item) {
            return id == item.identity;
        };
        auto it = std::find_if(used_loans_.begin(), used_loans_.end(), comp);
        if (it != used_loans_.end()) {
            return &(*it);
        }
        return nullptr;
    }

    OutstandingLoanItem* find_by_sample(void* sample) {
        auto comp = [sample](const OutstandingLoanItem& item) {
            return sample == item.sample;
        };
        auto it = std::find_if(used_loans_.begin(), used_loans_.end(), comp);
        assert(it != used_loans_.end());
        return &(*it);
    }
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_SAMPLELOANMANAGER_HPP_
