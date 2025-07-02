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
// feature: change name NewusContext to NexusContext
// feature: Reduce level of DomainParticipantListener
// feature: vbs2.0 contentfilteredtopic adaptation
// feature: implement ContentFilteredTopic
// feature: private constructor
// feature: separating interface and implementation about DomainParticipant
// feature: use Listeners and NexusUnits as singleton
// feature: VBS framework add performance test
// feature: add interface to get domain-id
// feature: fwk adds a typename-less initialization topic interface
// ------------------------------------------------------------------

#ifndef VBS_DOMAINPARTICIPANT_HPP_
#define VBS_DOMAINPARTICIPANT_HPP_

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <tuple>

#include "vbs/DataWriter.hpp"
#include "vbs/Topic.hpp"
#include "vbs/DataWriterQos.hpp"
#include "vbs/DataReaderQos.hpp"
#include "vbs/DomainParticipantQos.hpp"
#include "vbs/TopicQos.hpp"
#include "vbs/Global.hpp"
#include "vbs/DataReaderListener.hpp"
#include "vbs/DomainParticipantListener.hpp"
#include "vbs/DomainParticipantQos.hpp"

namespace vbs {
class Topic;
class DataWriter;
class DataReader;
class DataWriterListener;
class DataReaderListener;
class ReturnCode_t;
class DomainParticipantImpl;
class NexusUnits;
class NexusContext;
class VBSDynamicType;
class DomainParticipantListener;
class ContentFilteredTopic;

class DomainParticipant {
    friend class DomainParticipantFactory;

 public:
    /**
     * Participant deconstructor.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     */
    virtual ~DomainParticipant();

    /**
     * Creates and returns a pointer to a new DataWriter instance.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     * @note @li len(profile_name) != 0
     *
     * @param local_topic Topic on which data will be published.
     * @param profile_name Configuration profile name for the DataWriter.
     * @param listener Pointer to the listener for the DataWriter; default is nullptr.
     * mask Status mask for the DataWriter; default is all statuses.
     * @return Pointer to the newly created DataWriter. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataWriter* create_data_writer(Topic* const local_topic, const std::string& profile_name,
                                   DataWriterListener* const listener = nullptr);

    /**
     * Creates and returns a pointer to a new DataWriter instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     *
     * @param local_topic Topic on which data will be published.
     * @param profile_name Configuration profile name for the DataWriter.
     * @param listener Pointer to the listener for the DataWriter; default is nullptr.
     * @param mask Status mask for the DataWriter; default is all statuses.
     * @return Pointer to the newly created DataWriter. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataWriter* create_data_writer(Topic* const local_topic, const std::string& profile_name,
                                   DataWriterListener* const listener, const StatusMask& mask);

    /**
     * Creates and returns a pointer to a new DataWriter instance.
     *
     * @note @li Thread-Safe: Yes
     * @note @li Lock-Free: Yes
     * @note @li len(profile_name) != 0
     *
     * @param local_topic Topic on which data will be published.
     * @param writer_qos Qos for the DataWriter.
     * @param listener Pointer to the listener for the DataWriter; default is nullptr.
     * mask Status mask for the DataWriter; default is all statuses.
     * @return Pointer to the newly created DataWriter. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataWriter* create_data_writer(Topic* const local_topic, const DataWriterQos& writer_qos,
                                   DataWriterListener* const listener = nullptr);

    /**
     * Creates and returns a pointer to a new DataWriter instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     *
     * @param local_topic Topic on which data will be published.
     * @param writer_qos Qos for the DataWriter.
     * @param listener Pointer to the listener for the DataWriter; default is nullptr.
     * @param mask Status mask for the DataWriter; default is all statuses.
     * @return Pointer to the newly created DataWriter. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataWriter* create_data_writer(Topic* const local_topic, const DataWriterQos& writer_qos,
                                   DataWriterListener* const listener, const StatusMask& mask);

    /**
     * Deletes a given DataWriter instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param writer Pointer to the DataWriter to delete.
     * @return Return code indicating the result of the deletion.
     */
    ReturnCode_t delete_data_writer(const DataWriter* writer);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param profile_name Configuration profile name for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(Topic* const local_topic, const std::string& profile_name,
                                   DataReaderListener* const listener);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param profile_name Configuration profile name for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * @param mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(Topic* const local_topic, const std::string& profile_name,
                                   DataReaderListener* const listener, const StatusMask& mask);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param profile_name Configuration profile name for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(ContentFilteredTopic* const local_topic, const std::string& profile_name,
                                   DataReaderListener* const listener);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param profile_name Configuration profile name for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * @param mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(ContentFilteredTopic* const local_topic, const std::string& profile_name,
                                   DataReaderListener* const listener, const StatusMask& mask);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param reader_qos Qos for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(Topic* const local_topic, const DataReaderQos& reader_qos,
                                   DataReaderListener* const listener);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param reader_qos Qos for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * @param mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(Topic* const local_topic, const DataReaderQos& reader_qos,
                                   DataReaderListener* const listener, const StatusMask& mask);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param reader_qos Qos for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(ContentFilteredTopic* const local_topic, const DataReaderQos& reader_qos,
                                   DataReaderListener* const listener);

    /**
     * Creates and returns a pointer to a new DataReader instance.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(profile_name) != 0
     * @note @li listener != nullptr
     *
     * @param local_topic Topic on which data will be subscribed to.
     * @param reader_qos Qos for the DataReader.
     * @param listener Pointer to the listener for the DataReader.
     * @param mask Status mask for the DataReader; default is all statuses.
     * @return Pointer to the newly created DataReader. If the creation fails or any other error occurs, nullptr is returned.
     */
    DataReader* create_data_reader(ContentFilteredTopic* const local_topic, const DataReaderQos& reader_qos,
                                   DataReaderListener* const listener, const StatusMask& mask);

    /**
     * Deletes a given DataReader.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param reader Pointer to the DataReader to be deleted.
     * @return Return code indicating success or failure of the operation.
     */
    ReturnCode_t delete_data_reader(const DataReader* reader);

    /**
     * Creates a topic with a given name and type.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(type_name) != 0 && len(profile_name) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param type_name Name of the data type for the topic.
     * @param local_type TypeSupport object representing the data type.
     * @param profile_name Name of the QoS profile for the topic.
     * @return Pointer to the created Topic. If the creation fails or any other error occurs, nullptr is returned.
     */
    Topic* create_topic(const std::string& topic_name, const std::string& type_name, TypeSupport const& local_type,
                        const std::string& profile_name = "");

    /**
     * Creates a topic with a given name and type.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(profile_name) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param local_type TypeSupport object representing the data type.
     * @param profile_name Name of the QoS profile for the topic.
     * @return Pointer to the created Topic. If the creation fails or any other error occurs, nullptr is returned.
     */
    Topic* create_topic(const std::string& topic_name, TypeSupport const& local_type,
                        const std::string& profile_name = "");

    /**
     * Creates a topic with a given name and type.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(type_name) != 0 && len(profile_name) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param type_name Name of the data type for the topic.
     * @param local_type TypeSupport object representing the data type.
     * @param topic_qos Qos for the Topic.
     * @return Pointer to the created Topic. If the creation fails or any other error occurs, nullptr is returned.
     */
    Topic* create_topic(const std::string& topic_name, const std::string& type_name, TypeSupport const& local_type,
                        const TopicQos& topic_qos);

    /**
     * Creates a topic with a given name and type.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(profile_name) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param local_type TypeSupport object representing the data type.
     * @param topic_qos Qos for the Topic.
     * @return Pointer to the created Topic. If the creation fails or any other error occurs, nullptr is returned.
     */
    Topic* create_topic(const std::string& topic_name, TypeSupport const& local_type, const TopicQos& topic_qos);

    /**
     * Creates a contentfilteredtopic with a given name and related_topic.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(name) != 0 && related_topic != nullptr
     *
     * @param name The name of the contentfilteredtopic to be created.
     * @param related_topic The related_topic of the created topic.
     * @param filter_expression The filter_expression is the rules to filter
     * @param expression_parameters The expression_parameters is the content to filter
     * @return Pointer to the created ContentfilteredTopic. If the creation fails or any other error occurs, nullptr is returned.
     */
    ContentFilteredTopic* create_contentfilteredtopic(const std::string& name, Topic* related_topic,
                                                      const std::string& filter_expression,
                                                      const std::vector<std::string>& expression_parameters);

    /**
     * Creates a topic with a given name and dynamic type.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(type_name) != 0 && len(profile_name) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param type_name Name of the data type for the topic.
     * @param dyn_type Pointer to the DynamicType representing the data type.
     * @param profile_name Name of the QoS profile for the topic.
     * @return Pointer to the created Topic. If the creation fails or any other error occurs, nullptr is returned.
     */
    Topic* create_dynamic_type_topic(const std::string& topic_name, const std::string& type_name,
                                     VBSDynamicType& dyn_type, const std::string& profile_name = "");

    /**
     * Deletes a given topic.
     *
     * @note @li Thread-Safe: Yes
     * @note @li topic != nullptr
     *
     * @param topic Pointer to the Topic to be deleted.
     * @return Return code indicating success or failure of the operation.
     */
    ReturnCode_t delete_topic(const Topic* topic);

    /**
     * Gets the instance of the DomainParticipant.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return Pointer to the DomainParticipant instance.
     */
    vbs::DomainParticipantImpl* get_instance();

    /**
     * @brief Check if initialization was successful.
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * This function verifies whether the previous initialization process
     * completed successfully. It should be called after the initialization
     * routine to ensure that the system or object is ready for use.
     *
     * @return true if the initialization was successful, false otherwise.
     */
    bool init_verify();

    /**
     * Find a topic* by DomainParticipant*, topicname and topictype.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(topic_type) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param topic_type Name of the data type for the topic.
     * @return topic_ptr.
     */
    Topic* find_topic(const std::string& topic_name, const std::string& topic_type);

    /**
     * Remove a piece of data by topic*.
     *
     * @note @li Thread-Safe: Yes
     *
     * @param topic_ptr Topic pointer .
     */
    void remove_topic(const Topic* topic_ptr);

    /**
     * Add a DomainParticipant*, topicname , topictypetopic* and Topic*  in map.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(topic_type) != 0
     *
     * @param topic_name Name of the topic to be created.
     * @param topic_type Name of the data type for the topic.
     * @param topic_ptr
     */
    void add_topic(const std::string& topic_name, const std::string& topic_type, Topic* topic_ptr);

    /**
     * clear the map.
     *
     * @note @li Thread-Safe: Yes
     *
     */
    void del_participant_infos();

    /**
     * Deletes a given topic.
     *
     * @note @li Thread-Safe: Yes
     * @note @li topic != nullptr
     *
     * @param topic Pointer to the Topic to be deleted.
     * @return Return code indicating success or failure of the operation.
     */
    ReturnCode_t delete_topic(ContentFilteredTopic* topic);

    /**
     * Add a ContentFilteredTopic in set.
     *
     * @note @li Thread-Safe: Yes
     * @note @li len(topic_name) != 0 && len(topic_type) != 0
     *
     * @param topic_ptr A pointer to a ContentFilteredTopic object that defines
     *          the topic and its content filter rules. The caller must ensure
     *          that this pointer is valid for the duration of its use.
     */
    void add_topic(ContentFilteredTopic* topic_ptr);

    /**
     * clear the set.
     *
     * @note @li Thread-Safe: Yes
     *
     */
    void del_content_filtered_topics();

    /**
     * @brief register_remote_topic
     * @note @li len(topic_name) != 0
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param topic_name Name of the remote topic to be registered.
     */
    void register_remote_topic(const std::string& topic_name);

    /**
     * @brief register remote topic name
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    void register_remote_topic();

    /**
     * @brief unregister remote topic name
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    void unregister_remote_topic();

    /**
     * @brief Get dynamic_type
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li len(topic_name) != 0
     *
     * @param type_name Data type name bound to given Topics
     *
     * @return DynamicType_Ptr instance describe Data type struct info
     */
    VBSDynamicType get_dynamic_type(const std::string& type_name);

    /**
     * @brief delete all xml info
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    static void delete_xml_instance();

    /**
     * @brief set listener
     *
     * @note @li Thread-Safe: Yes
     * @note @li listener != nullptr
     *
     * @param listener DomainParticipantListener
     *
     */
    void set_listener(DomainParticipantListener* listener);

    /**
     * get domain id
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return domain_id
     */
    DomainId_t get_domain_id();

 private:
    DomainParticipant() = default;

    void set_participant_lisener(DomainParticipantListener* listener);
    DataReader* create_data_reader_helper(std::tuple<Topic*, ContentFilteredTopic*> local_topic,
                                          std::tuple<const std::string*, const DataReaderQos*> qos_info,
                                          DataReaderListener* const listener, const StatusMask& mask);

    vbs::DomainParticipantImpl* domain_participant_ptr_ = nullptr;
};

}  // namespace vbs

#endif  // VBS_DOMAINPARTICIPANT_HPP_
