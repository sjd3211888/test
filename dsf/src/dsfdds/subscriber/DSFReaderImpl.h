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

#ifndef DSFDDS_DSFREADERIMPL_H_
#define DSFDDS_DSFREADERIMPL_H_

#include <cstdint>
#include "dsfdds/transport/reader_listener.h"
#include <mutex>
#include <memory>
#include <vector>
#include <string>
#include "dsfdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "history/ReaderListener.h"
#ifdef DSFDDS_STATISTICS
#include "statistics/data/data.hpp"
#endif
#include "common/types/TypesBase.h"
#include "common/all_common.h"
#include "dsfdds/dds/subscriber/DSFReader.h"
#include "history/ReaderListener.h"
#include "membuf/BufferDescriptor.h"

namespace mbuf {
struct BuildMbufContext;
class BufferPoolUserManager;
}  // namespace mbuf

namespace li {
namespace dsfdds {

class ShmReader;
class EndpointProxy;
class EntityProxy;
class ChannelResource;
class SampleInfo;

namespace statistics {
class DomainParticipantStatisticsListener;
}
class CrashListener;

/**
 * Class DataReader, contains the actual implementation of the behaviour of the Subscriber.
 * for vbs2.0
 *  @ingroup FASTDDS_MODULE
 */
class DSFReaderImpl {
    friend DSFReader;

 protected:
    /**
   * Creates a DataReader. Don't use it directly, but through Subscriber.
   */
    DSFReaderImpl(uint32_t domainID, vbsutil::xmlparser::GUID_t guid, const std::string& topic_name,
                  std::string& topic_type, const DataReaderQos& qos,
                  std::shared_ptr<vbs::common::IChangePool>& changePool, vbs::common::ReaderHistory* historyPtr,
                  vbsutil::xmlparser::Locator_t* local_loc, vbs::common::ReaderListener* listener);

 public:
    virtual ~DSFReaderImpl();
    class DSFReaderListener : public li::dsfdds::ReaderListener {
     public:
        DSFReaderListener(DSFReaderImpl* dsf_reader, vbs::common::ReaderListener* evbs_listener)
            : dsf_reader_(dsf_reader), evbsListener(evbs_listener) {}

        ~DSFReaderListener() {}

        /**
     * @brief callback for receiving message
     * 
     * @param reader 
     */
        void on_data_available(ShmReader* reader) override;

        /**
     * @brief callback for sample lost
     * 
     * @param reader 
     * @param sample_lost_since_last_update 
     * @param error_code 
     */
        void on_sample_lost(ShmReader* reader, int32_t sample_lost_since_last_update, int32_t error_code) override;

        DSFReaderImpl* dsf_reader_;
        vbs::common::ReaderListener* evbsListener;
    } dsfListener;

    /**
   * @brief param check, create ShmReader, start receiving thread
   * 
   * @return ReturnCode_t RETCODE_PUBSUB_OK if all ok. 
   *                      ENTITY_TYPE_UNKNOW if entity unknown. 
   *                      RETCODE_PUBSUB_ERROR if ShmReader created fail. 
   */
    ReturnCode_t enable();

    /**
   * @brief Get the qos object
   * 
   * @return const DataReaderQos& 
   */
    const DataReaderQos& get_qos() const;

    /**
   * @brief stop receiving thread
   * 
   */
    void disable();

    uint16_t get_domain_id() const;

    vbsutil::xmlparser::GUID_t guid() const;

    mbuf::BufferPoolUserManager* GetPool() const;

 protected:
    // !Pointer to associated ShmReader
    ShmReader* shmReaderPtr = nullptr;
    uint32_t domainId;
    vbsutil::xmlparser::GUID_t rGuid;
    std::string topicName;
    std::string topicType;
    DataReaderQos dataReaderQos;
    vbsutil::xmlparser::Locator_t* localLocatorPtr;

#ifdef DSFDDS_STATISTICS
    std::shared_ptr<li::dsfdds::statistics::DomainParticipantStatisticsListener> statistics_listener_;
#endif

 private:
    bool EdpRegisterInfo();
#ifdef DSFDDS_STATISTICS
    void StatisticsDataCallBackRegister();
#endif
    void update_subscription_matched_status(const EntityProxy& ep, const int32_t& count_change);

    void CrashCallBack(const EndpointProxy& ep);

    /**
   * @brief entry point for receiving message
   * 1. take mbuf from ShmReader
   * 2. move content to cache change, including data length, timestamp added to writer history,
   * writer guid
   * 3. add cache change to reader history
   * 
   * @param cr 
   */
    void DataReceiveCallBack(const ChannelResource* cr);
#ifdef ENABLE_DES
    void DataReceiveCallBack(ReaderListener* readerListener);
#endif
    void HandleDataReceive(const ChannelResource* cr);

    void move_mbuf_into_change(std::shared_ptr<mbuf::Buffer>& mbuf, vbs::common::CacheChange_t* change, uint32_t length,
                               uint64_t timestamp, vbs::common::SequenceNumber_t current_sequence);

    void move_meta_shm_mbuf_into_change(std::shared_ptr<mbuf::Buffer>* mbufs, vbs::common::CacheChange_t* change,
                                        uint32_t ser_length, uint32_t non_ser_length, uint64_t timestamp,
                                        vbs::common::SequenceNumber_t current_sequence);
    void move_meta_auto_mbuf_into_change(const std::shared_ptr<mbuf::Buffer>& mbuf, vbs::common::CacheChange_t* change,
                                         uint32_t ser_length, uint32_t non_ser_length, uint64_t timestamp,
                                         vbs::common::SequenceNumber_t current_sequence, uint64_t head_count);

    std::shared_ptr<vbs::common::IPayloadPool> payloadPool;
    std::shared_ptr<vbs::common::IChangePool> changePool;
    vbs::common::ReaderHistory* historyPtr;

    bool change_received(vbs::common::CacheChange_t* change, vbsutil::xmlparser::GUID_t w_guid);
    void parser_meta_buf(const mbuf::BufferDescriptor& descriptor, const std::shared_ptr<mbuf::Buffer>& mbuf,
                         vbs::common::CacheChange_t* change, uint64_t timestamp,
                         vbs::common::SequenceNumber_t sequence);

    std::shared_ptr<CrashListener> crash_listener_;
};

} /* namespace dsfdds */
} /* namespace li */

#endif /* DSFDDS_DSFREADERIMPL_H_*/
