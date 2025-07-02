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

#ifndef _EDDS_ENTITYUTILS_QOS_CONVERTERS_HPP_
#define _EDDS_ENTITYUTILS_QOS_CONVERTERS_HPP_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include "edds/rtps/attributes/RTPSParticipantAttributes.h"
#include "ertps/attributes/PublisherAttributes.h"
#include "ertps/attributes/SubscriberAttributes.h"
#include "ertps/attributes/TopicAttributes.h"
#include "domain/DomainParticipantImpl.hpp"
#include "xmlparser/qos/XMLDataWriterQos.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

namespace vbs {
namespace utils {

using evbs::ertps::PublisherAttributes;
using evbs::ertps::SubscriberAttributes;

/**
 * Obtains the DataWriterQos from the PublisherAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(vbsutil::xmlparser::DataWriterQos& qos, const PublisherAttributes& attr);

/**
 * Obtains the DataReaderQos from the SubscriberAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(vbsutil::xmlparser::DataReaderQos& qos, const SubscriberAttributes& attr);

/**
 * @brief Fill DomainParticipantQosInner from a given attributes RTPSParticipantAttributes object
 *
 * For the case of the non-binary properties, instead of the RTPSParticipantAttributes overriding
 * the property list in the DomainParticipantQosInner, a merge is performed in the following manner:
 *
 * - If any property from the RTPSParticipantAttributes is not in the DomainParticipantQosInner, then it
 * is appended to the DomainParticipantQosInner.
 * - If any property from the RTPSParticipantAttributes property is also in the
 * DomainParticipantQosInner, then the value in the DomainParticipantQosInner is overridden with that of the
 * RTPSParticipantAttributes.
 *
 * @param[in, out] qos The DomainParticipantQosInner to setQosConverters.hpp
 * @param[in] attr The RTPSParticipantAttributes from which the @c qos is set.
 */
void set_qos_from_attributes(vbs::DomainParticipantQosInner& qos,
                             const evbs::ertps::rtps::RTPSParticipantAttributes& attr);

/**
 * Obtains the RTPSParticipantAttributes from the DomainParticipantQosInner provided.
 *
 * @param[out] attr Pointer to the attributes from which to obtain data
 * @param[in] qos Pointer to the QoS to write on
 */
void set_attributes_from_qos(evbs::ertps::rtps::RTPSParticipantAttributes& attr,
                             const vbs::DomainParticipantQosInner& qos);

/**
 * Obtains the TopicQos from the vbsutil::xmlparser::TopicAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(vbsutil::xmlparser::TopicQos& qos, const vbsutil::xmlparser::TopicAttributes& attr);

/**
 * Obtains the SubscriberQos from the SubscriberAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(vbsutil::xmlparser::SubscriberQos& qos, const SubscriberAttributes& attr);

/**
 * Obtains the PublisherQos from the PublisherAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(vbsutil::xmlparser::PublisherQos& qos, const PublisherAttributes& attr);

} /* namespace utils */
} /* namespace vbs */
#endif  // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif  /* _EDDS_ENTITYUTILS_QOS_CONVERTERS_HPP_ */
