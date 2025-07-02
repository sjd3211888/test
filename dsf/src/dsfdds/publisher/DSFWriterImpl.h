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

#ifndef DSFWRITERIMPL_H_
#define DSFWRITERIMPL_H_

#include <memory>
#include <string>
#include <vector>
#include "dsfdds/dds/publisher/qos/DataWriterQos.hpp"
#include "history/WriterListener.h"
#include "common/types/TypesBase.h"
#include "membuf/BaseType.h"

#ifdef DSFDDS_STATISTICS
#include "statistics/domain/DomainParticipantImpl.hpp"
#endif
#include "common/all_common.h"

#include <dsfdds/dds/publisher/DSFWriter.h>

namespace li {
namespace dsfdds {
class ShmWriter;
class EndpointProxy;
class CrashListener;

/**
 * Class DSFWriterImpl, contains the actual implementation of the behaviour of the DsfWriter.
 * for vbs 2.0
 */
class DSFWriterImpl {
    friend DSFWriter;

 protected:
    /**
   * Create a data writer, assigning its pointer to the associated writer.
   * Don't use directly, create Publisher using DomainRTPSParticipant static function.
   */
    DSFWriterImpl(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                  std::string& topic_type, std::shared_ptr<vbs::common::IPayloadPool>& payload_pool,
                  const DataWriterQos& qos, vbs::common::WriterHistory* history,
                  vbsutil::xmlparser::Locator_t* local_loc, vbs::common::WriterListener* listener = nullptr);

 public:
    virtual ~DSFWriterImpl();

    /**
   * Enable this object.
   * The required lower layer entities will be created.
   *
   * @pre This method has not previously returned ReturnCode_t::RETCODE_PUBSUB_OK
   *
   * @return ReturnCode_t::RETCODE_PUBSUB_OK if all the lower layer entities have been correctly created.
   * @return Other standard return codes on error.
   */
    ReturnCode_t enable();

    /**
   * Get a pointer to the internal pool where the user could directly write.
   *
   * @param [in]  size  buf size.
   *
   * @return std::shared_ptr<Buffer>
   */
    std::shared_ptr<mbuf::Buffer> loan_sample(uint32_t size);

    /**
   * This is the entry of actual sending the message
   *
   * @param change cache change to be sent
   * @param max_blocking_time Maximum time the operation should be waiting. (TODO Unused for now)
   *
   * @return true if data is correctly delivered to the lower layers, false otherwise.
   */
    bool unsent_change_added_to_history(vbs::common::CacheChange_t* change,
                                        const std::chrono::time_point<std::chrono::steady_clock>& max_blocking_time);

    /**
   * Write data to the topic.
   *
   * @param mbuf Pointer to the data.
   * @param length actual length of data, reader need to knonw the real length of data
   * before mem align. Passed by BufferDescriptor
   * @param timestamp timestamp of change added to history, reader need to know.
   * Passed by BufferDescriptor
   * @param guids guids of content filter out of reader
   *
   * @return true if mbuf is correctly delivered to the lower layers, false otherwise.
   */
    bool write(const std::shared_ptr<mbuf::Buffer>& mbuf, uint32_t length, MultiModeChannel auto_shm_bigbuffer,
               uint64_t timestamp, const std::vector<vbsutil::xmlparser::GUID_t>& guids,
               vbs::common::SequenceNumber_t seqNum);

    bool write_meta(const std::shared_ptr<mbuf::Buffer>& mbuf, uint32_t serLength, MultiModeChannel auto_shm_bigbuffer,
                    uint64_t timestamp, const std::vector<vbsutil::xmlparser::GUID_t>& guids,
                    vbs::common::SequenceNumber_t seqNum);

    //! Remove all listeners in the hierarchy to allow a quiet destruction (TODO do nothing for now)
    void disable();

    vbsutil::xmlparser::GUID_t guid() const;

    uint32_t get_domain_id() const;

    std::vector<mbuf::PoolID> get_pool_ids() const;

    bool is_reissue_required(int lp_durability, int rp_durability);

    std::shared_ptr<mbuf::Buffer> prepare_shm_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                                                          vbs::common::CacheChange_t* change);
    std::shared_ptr<mbuf::Buffer> prepare_auto_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                                                           vbs::common::CacheChange_t* change);

    bool write_shm_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                               MultiModeChannel auto_shm_bigbuffer, vbs::common::CacheChange_t* change);

    bool write_auto_meta_buffer(uint32_t serDataRealLength, uint32_t nonSerDataRealLength,
                                MultiModeChannel auto_shm_bigbuffer, vbs::common::CacheChange_t* change);

 protected:
    //! Pointer to the associated Data Writer.
    ShmWriter* shmWriterPtr = nullptr;
    bool is_shm_only_;  // 是否是 shm only 链路
    std::string topicName;
    std::string topicType;
    std::shared_ptr<vbs::common::IPayloadPool> payload_pool_;
    DataWriterQos dsfWriterQos;
    vbs::common::WriterListener* listener;
    vbs::common::WriterHistory* history_;
    vbsutil::xmlparser::Locator_t* localLocatorPtr;
#ifdef DSFDDS_STATISTICS
    std::shared_ptr<li::dsfdds::statistics::DomainParticipantStatisticsListener> statistics_listener_;
#endif

 private:
    bool EdpRegisterInfo();
    void CrashCallBack(const EndpointProxy& ep);
    void LogSendResult(bool success, const std::string& changeType, vbs::common::SequenceNumber_t seqNum) const;
    vbsutil::xmlparser::GUID_t wGuid;
    uint32_t domainId;
    std::shared_ptr<CrashListener> crash_listener_;
};

} /* namespace dsfdds */
} /* namespace li */

#endif  // DSFWRITERIMPL_H_
