// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef _EDDS_ENTITYTOPICPROXYFACTORY_HPP_
#define _EDDS_ENTITYTOPICPROXYFACTORY_HPP_

#include <algorithm>
#include <list>
#include <memory>
#include <string>

#include "deps/core/status/StatusMask.hpp"
#include "topic/TopicListener.hpp"
#include "deps/common/TypeSupport.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"
#include "topic/TopicCore.hpp"
#include "topic/TopicProxy.hpp"

namespace vbs {
class DomainParticipantImpl;

/**
 * A factory of TopicProxy objects for a specific topic.
 */
class TopicProxyFactory {
 public:
    /**
     * Construct a TopicProxyFactory.
     *
     * @param participant   Pointer to the DomainParticipant creating this object.
     * @param topic_name    Name of the topic managed by this factory.
     * @param status_mask   Initial StatusMask of the topic managed by this factory.
     * @param type_support  TypeSupport to use for the topics created by this factory.
     * @param qos           TopicQos to use on the creation of the implementation object.
     * @param listener      TopicListener to use on the creation of the implementation object.
     */
    TopicProxyFactory(DomainParticipantImpl* participant, const std::string& topic_name, const std::string& type_name,
                      const evbs::edds::dds::StatusMask& status_mask, vbs::TypeSupport type_support,
                      const vbsutil::xmlparser::TopicQos& qos, TopicListener* listener)
        : topic_name_(topic_name),
          type_name_(type_name),
          status_mask_(status_mask),
          topic_core_(this, participant, std::move(type_support), qos, listener) {}

    /**
     * Create a new proxy object for the topic managed by the factory.
     *
     * @return Pointer to the created TopicProxy
     */
    TopicProxy* create_topic();

    /**
     * Delete a proxy object for the topic managed by the factory.
     *
     * @param proxy Pointer to the TopicProxy object to be deleted.
     *
     * @return PRECONDITION_NOT_MET if the @c proxy was not created by this factory, or has already
     * being deleted.
     * @return PRECONDITION_NOT_MET if the @c proxy is still referenced.
     * @return OK if the @c proxy is correctly deleted.
     */
    vbsutil::elog::ReturnCode_t delete_topic(TopicProxy* proxy);

    /**
     * Get one of the TopicProxy objects created by the factory.
     *
     * @return nullptr if the factory owns no proxy objects.
     * @return Pointer to one of the proxies owned by the factory.
     */
    TopicProxy* get_topic();

    /**
     * Return whether this factory can be deleted.
     * Will disallow deletion if it still owns some proxy objects.
     *
     * @return true if the factory owns no proxy objects
     */
    bool can_be_deleted();

    /**
     * Enable the topic managed by the factory.
     */
    void enable_topic();

    /**
     * Apply the given function to all the TopicProxy objects owned by the factory.
     */
    template <class UnaryFunction>
    void for_each(UnaryFunction f) const {
        (void)std::for_each(proxies_.begin(), proxies_.end(), f);
    }

 private:
    //! Name of the topic managed by the factory.
    std::string topic_name_;
    //! Name of the topic data type
    std::string type_name_;
    //! StatusMask of the topic managed by the factory.
    evbs::edds::dds::StatusMask status_mask_;
    //! Implementation object for the topic managed by the factory.
    vbs::TopicCore topic_core_;
    //! List of TopicProxy objects created by this factory.
    std::list<std::unique_ptr<TopicProxy>> proxies_;
};

}  // namespace vbs

#endif /* _EDDS_ENTITYTOPICPROXYFACTORY_HPP_ */
