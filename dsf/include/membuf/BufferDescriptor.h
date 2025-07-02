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

#ifndef MEMBUF_INC_BUFFER_DESCRIPTOR_H
#define MEMBUF_INC_BUFFER_DESCRIPTOR_H
#include <ostream>
#include "common/types/TypesBase.h"
#include "membuf/BufferDesc.h"
namespace mbuf {
class BufferDescriptor : public BufferDesc {
 public:
    BufferDescriptor(const PoolID id, const UserID& userId, const BufferNodeOffset offset, const uint64_t count,
                     const uint64_t timestamp, const int priority, const uint64_t deadline, const uint64_t pid,
                     const uint32_t writer_id, const uint32_t writer_participant_id, const uint32_t reader_id,
                     const int32_t sequence_number_high, const uint32_t sequence_number_low)
        : BufferDesc(id, userId, offset, count),
          timestamp_(timestamp),
          priority_(priority),
          deadline_(deadline),
          pid_(pid),
          writer_id_(writer_id),
          writer_participant_id_(writer_participant_id),
          reader_id_(reader_id),
          sequence_number_high_(sequence_number_high),
          sequence_number_low_(sequence_number_low),
          data_real_length_(0U) {}
    BufferDescriptor(const uint64_t timestamp, const int priority, const uint64_t deadline, const uint64_t pid,
                     const uint32_t writer_id, const uint32_t writer_participant_id, const uint32_t reader_id,
                     const int32_t sequence_number_high, const uint32_t sequence_number_low)
        : timestamp_(timestamp),
          priority_(priority),
          deadline_(deadline),
          pid_(pid),
          writer_id_(writer_id),
          writer_participant_id_(writer_participant_id),
          reader_id_(reader_id),
          sequence_number_high_(sequence_number_high),
          sequence_number_low_(sequence_number_low),
          data_real_length_(0U) {}
    BufferDescriptor(const PoolID id, const UserID& userId, const BufferNodeOffset offset, const uint64_t count,
                     const uint64_t timestamp, const int priority, const uint64_t deadline, const uint64_t pid,
                     const uint32_t writer_id, const uint32_t writer_participant_id, const uint32_t reader_id,
                     const int32_t sequence_number_high, const uint32_t sequence_number_low, uint32_t ser_length,
                     li::dsfdds::MultiModeChannel auto_shm_bigbuffer = li::dsfdds::MultiModeChannel::NONE)
        : BufferDesc(id, userId, offset, count),
          timestamp_(timestamp),
          priority_(priority),
          deadline_(deadline),
          pid_(pid),
          writer_id_(writer_id),
          writer_participant_id_(writer_participant_id),
          reader_id_(reader_id),
          sequence_number_high_(sequence_number_high),
          sequence_number_low_(sequence_number_low),
          data_real_length_(ser_length),
          auto_shm_bigbuffer_(auto_shm_bigbuffer) {}

    BufferDescriptor() = default;
    uint64_t timestamp() const { return timestamp_; }
    uint64_t deadline() const { return deadline_; }
    uint64_t pid() const { return pid_; }
    uint32_t writer_id() const { return writer_id_; }
    uint32_t writer_participant_id() const { return writer_participant_id_; }

    uint64_t timestamp_;
    int priority_;
    uint64_t deadline_;
    uint64_t pid_;
    uint32_t writer_id_;
    uint32_t writer_participant_id_;
    uint32_t reader_id_;
    int32_t sequence_number_high_;
    uint32_t sequence_number_low_;
    uint32_t data_real_length_;
    li::dsfdds::MultiModeChannel auto_shm_bigbuffer_ = li::dsfdds::MultiModeChannel::NONE;
    friend std::ostream& operator<<(std::ostream& os, const BufferDescriptor& obj) {
        return os << static_cast<const BufferDesc&>(obj) << ",timestamp:" << obj.timestamp_
                  << ",priority:" << obj.priority_ << ",deadline:" << obj.deadline_ << ",pid:" << obj.pid_
                  << ",writer_id:" << obj.writer_id_ << ",writer_participant_id:" << obj.writer_participant_id_
                  << ",reader_id:" << obj.reader_id_ << ",sequence_number_high:" << obj.sequence_number_high_
                  << ",sequence_number_low:" << obj.sequence_number_low_
                  << ",data_real_length: " << obj.data_real_length_
                  << ",auto_shm_bigbuffer_: " << obj.auto_shm_bigbuffer_;
    }
};
}  // namespace mbuf
#endif  // MEMBUF_INC_BUFFER_DESCRIPTOR_H
