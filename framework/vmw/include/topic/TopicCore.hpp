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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef _EDDS_ENTITYTOPICIMPL_HPP_
#define _EDDS_ENTITYTOPICIMPL_HPP_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include "ertps/types/TypesBase.h"
#include "deps/core/status/StatusMask.hpp"
#include "topic/TopicListener.hpp"
#include "deps/common/TypeSupport.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"

namespace vbs {

class DomainParticipantImpl;
class TopicImpl;
class TopicProxyFactory;
class TopicListener;
class TypeSupport;

class TopicCore {
 public:
    TopicCore(vbs::TopicProxyFactory* factory, vbs::DomainParticipantImpl* p, TypeSupport type_support,
              const vbsutil::xmlparser::TopicQos& qos, vbs::TopicListener* listen);

    /**
     * Extends the check_qos() call, including the check for
     * resource limits policy.
     * @param qos Pointer to the qos to be checked.
     * @param type Pointer to the associated TypeSupport object.
     * @return True if correct.
     */
    static vbsutil::elog::ReturnCode_t check_qos_including_resource_limits(const vbsutil::xmlparser::TopicQos& qos,
                                                                           const TypeSupport& type);

    /**
     * Checks the consistency of the qos configuration.
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static vbsutil::elog::ReturnCode_t check_qos(const vbsutil::xmlparser::TopicQos& qos);

    /**
     * Checks resource limits policy: Instance allocation consistency
     * @param qos Pointer to the qos to be checked.
     * @return True if correct.
     */
    static vbsutil::elog::ReturnCode_t check_allocation_consistency(const vbsutil::xmlparser::TopicQos& qos);

    static bool can_qos_be_updated(const vbsutil::xmlparser::TopicQos& to, const vbsutil::xmlparser::TopicQos& from);

    static void set_qos(vbsutil::xmlparser::TopicQos& to, const vbsutil::xmlparser::TopicQos& from, bool first_time);

    virtual ~TopicCore();

    const vbsutil::xmlparser::TopicQos& get_qos() const;

    vbsutil::elog::ReturnCode_t set_qos(const vbsutil::xmlparser::TopicQos& qos);

    const vbs::TopicListener* get_listener() const;

    void set_listener(vbs::TopicListener* listener);

    void set_listener(vbs::TopicListener* listener, const evbs::edds::dds::StatusMask& status);

    vbs::DomainParticipantImpl* get_participant() const;

    const TypeSupport& get_type() const;

    /**
     * Returns the most appropriate listener to handle the callback for the given status,
     * or nullptr if there is no appropriate listener.
     */
    vbs::TopicListener* get_listener_for(const evbs::edds::dds::StatusMask& status, const vbs::TopicImpl* topic);

 protected:
    vbs::TopicProxyFactory* factory_;
    vbs::DomainParticipantImpl* participant_;
    TypeSupport type_support_;
    vbsutil::xmlparser::TopicQos qos_;
    vbs::TopicListener* listener_;
};

}  // namespace vbs

#endif  // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif  /* _EDDS_ENTITYTOPICIMPL_HPP_ */
