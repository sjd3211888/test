// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: change name NewusContext to NexusContext
// feature: add QosManager 1. load xml unified management of qos and attribute 2. create topic、reader and writer config map 3. Provide external interfaces for get and set running qos 4. add UT for QosManager
// feature: Remove redundant code 1. Remove MuxType(the source of data) 2. Remove unused member variables in DataWriterImpl/DataReadImpl/DomainParticipantListener 3. Remove member variables about shm type discovery 4. Delete some commented code
// feature: implement ContentFilteredTopic
// feature: separating interface and implementation about Topic
// feature: Support dump topic traffic to disk
// feature: VBS framework add performance test
// feature: shm dynamic type discovery
// ------------------------------------------------------------------

#ifndef INCLUDE_TOPIC_TOPIC_HPP_
#define INCLUDE_TOPIC_TOPIC_HPP_

#include <map>
#include <string>
#include <deps/common/TypeSupport.hpp>
#include "qos/TopicQosImpl.hpp"
#include "topic/DynamicPubSubTypeImpl.hpp"
#include "context/NexusContext.hpp"

namespace vbs {
class TopicProxy;
class DomainParticipantImpl;
class VBSDynamicDataPrinter;
class VBSDynamicData;

enum TRAFFIC_TYPE : uint8_t {
    UNKNOWN_TRAFFIC = 0,
    SEND_TRAFFIC = 1,
    RECV_TRAFFIC = 2,
};

struct NetTrafficInfoImpl {
    NetTrafficInfoImpl() : pkg(0), throughput(0), timestamp(0), direction(TRAFFIC_TYPE::UNKNOWN_TRAFFIC) {}
    uint32_t pkg;            // total VBS package number
    uint64_t throughput;     // total VBS throughput number
    uint64_t timestamp;      // time when this data collected
    TRAFFIC_TYPE direction;  // 1 is send traffic, 2 is receive traffic, 0 is unknwon
    std::string to_string() {
        std::string directionStr = "";
        if (direction == TRAFFIC_TYPE::SEND_TRAFFIC) {
            directionStr = "send traffic:";
        } else if (direction == TRAFFIC_TYPE::RECV_TRAFFIC) {
            directionStr = "recv traffic:";
        } else {
            directionStr = "unknown traffic:";
        }
        std::string content = "[" + std::to_string(timestamp) + "] " + directionStr +
                              " pkg_number:" + std::to_string(pkg) + ", total_traffic: " + std::to_string(throughput) +
                              " bytes";
        return content;
    }
};

class TopicImpl : public DomainEntity, public vbs::TopicDescription {
    friend class DomainParticipantImpl;
    friend class DataWriterImpl;
    friend class DataReaderImpl;
    friend class ContentFilteredTopicImpl;

 private:
    // make sure that NexusContext is released after all entities release.
    std::shared_ptr<NexusContext> context_ = NexusContext::get_context_shared();

    bool dds_is_initialized_ = false;
    TypeSupport type_;
    TypeSupport dymanic_type_;
    TopicQosImpl qos_;
    DomainParticipantImpl* participant_;
    std::string topic_name_;
    std::string data_type_name_;
    std::string xml_path_ = "vbs_tmp_xml";
    std::atomic<uint64_t> framework_seq_id_;
    std::string shm_xml_path_ = "";

    static VBSDynamicDataPrinter printer_;

    TopicImpl(const TopicImpl& local_topic)
        : TopicDescription(local_topic.name_, local_topic.type_name_),
          type_(local_topic.type_),
          qos_(local_topic.qos_),
          participant_(local_topic.participant_),
          topic_proxy_(local_topic.topic_proxy_) {}

    TopicImpl& operator=(const TopicImpl& local_topic) {
        topic_proxy_ = local_topic.topic_proxy_;
        type_ = local_topic.type_;
        qos_ = local_topic.qos_;
        participant_ = local_topic.participant_;
        return *this;
    }

 public:
    /**
     * TopicImpl static constructor
     *
     * @param topic_name TopicImpl name.
     * @param type_name type name.
     * @param p TopicProxy.
     * @param mask StatusMask.
     */
    TopicImpl(DomainParticipantImpl* const local_participant, const std::string& topic_name,
              const std::string& type_name, const TypeSupport& local_type, TopicProxy* p,
              const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

    /**
     * TopicImpl constructor.
     */
    virtual ~TopicImpl();

    /**
     * Allows accessing the TopicImpl Qos.
     *
     * @return reference to TopicQos
     */
    bool GetQos(TopicQosImpl& qos);
    bool SetQos(const TopicQosImpl& qos);
    const TopicQosImpl& get_qos_impl() const;

    /**
     * create data.
     *
     * @return Pointer to the data
     */
    void* create_data();

    /**
     * delete data.
     *
     * @param data pointer to the data to delete
     */
    void delete_data(void* data);

    std::string get_topic_name_() { return topic_name_; }

    std::string get_data_type_name() { return data_type_name_; }

    TypeSupport* get_type() { return &type_; }

    /**
     * @brief Check if initialization was successful.
     *
     * This function verifies whether the previous initialization process
     * completed successfully. It should be called after the initialization
     * routine to ensure that the system or object is ready for use.
     *
     * @return true if the initialization was successful, false otherwise.
     */
    bool init_verify();

    bool get_topic_throughput(NetTrafficInfoImpl& send, NetTrafficInfoImpl& recv, int interval = 10);
    void dump_network_traffic(vbs::NetTrafficInfoImpl (&traffic)[2]);
    void update_network_traffic(int pgk_size, int pkg_count, TRAFFIC_TYPE type);

    static std::string to_string(const VBSDynamicData* data, int enum_mode = 0);
    static std::ostream& to_string(std::ostream& out, const VBSDynamicData* data, int enum_mode = 0);

    DomainParticipantImpl* get_participant() const override;

    vbs::TopicDescriptionImpl* get_impl() const override;

    RTPS_DllAPI const vbsutil::xmlparser::TopicQos& get_qos() const;
    RTPS_DllAPI ReturnCode_t get_qos(vbsutil::xmlparser::TopicQos& qos) const;
    RTPS_DllAPI ReturnCode_t set_qos(const vbsutil::xmlparser::TopicQos& qos);
    RTPS_DllAPI const TopicListener* get_listener() const;
    RTPS_DllAPI ReturnCode_t set_listener(TopicListener* listener,
                                          const evbs::edds::dds::StatusMask& mask = evbs::edds::dds::StatusMask::all());

 private:
    std::mutex send_traffic_mtx_;
    std::mutex recv_traffic_mtx_;
    NetTrafficInfoImpl history_recv_throughput_;
    NetTrafficInfoImpl history_send_throughput_;

 protected:
    TopicProxy* topic_proxy_;
};

}  // namespace vbs
#endif  // INCLUDE_TOPIC_TOPIC_HPP_
