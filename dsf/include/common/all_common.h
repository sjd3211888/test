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

#ifndef DSFDDS_ALL_COMMON_H_
#define DSFDDS_ALL_COMMON_H_

#include "deps/common/Types.h"
#include "deps/common/Time_t.h"
#include "xmlparser/attributes/XMLRTPSParticipantAttributes.h"
#include "xmlparser/XMLEndpointParserPart.h"
#include "xmlparser/qos/XMLQosPolicies.hpp"
#include "xmlparser/XMLParserCommonPart.h"
#include "xmlparser/attributes/XMLReaderAttributes.h"
#include "xmlparser/attributes/XMLSubscriberAttributes.h"
#include "deps/common/fixed_size_string.hpp"
#include "deps/common/PropertyPolicy.h"
#include "xmlparser/transport_descriptors/shared_queue_transport_descriptor.h"
#include "xmlparser/XMLProfileManagerPart.h"
#include "xmlparser/attributes/XMLPublisherAttributes.h"
#include "xmlparser/attributes/XMLTopicAttributes.h"
#include "xmlparser/data/XMLReaderProxyData.h"
#include "xmlparser/data/XMLWriterProxyData.h"
#include "xmlparser/qos/XMLDataWriterQos.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"

using vbsutil::xmlparser::XMLProfileManagerPart;  // example中使用

namespace li {
namespace dsfdds {
const uint16_t MAX_DOMAIN_ID = 255U;

#ifdef DSFDDS_STATISTICS
extern bool enable_statistics;
#endif

using vbsutil::c_TimeInfinite;
using vbsutil::Duration_t;
using vbsutil::Time_t;
using vbsutil::xmlparser::AUTOMATIC_LIVELINESS_QOS;
using vbsutil::xmlparser::BEST_EFFORT;
using vbsutil::xmlparser::BEST_EFFORT_RELIABILITY_QOS;
using vbsutil::xmlparser::BuiltinAttributes;
using vbsutil::xmlparser::DeadlineQosPolicy;
using vbsutil::xmlparser::DTS_WAKE_UP;
using vbsutil::xmlparser::DurabilityKind_t;
using vbsutil::xmlparser::DurabilityQosPolicy;
using vbsutil::xmlparser::DurabilityQosPolicyKind;
using vbsutil::xmlparser::EndpointKind_t;
using vbsutil::xmlparser::INVALID_QOS_POLICY_ID;
using vbsutil::xmlparser::INVALID_WAKE_UP;
using vbsutil::xmlparser::LivelinessQosPolicyKind;
using vbsutil::xmlparser::NEXT_QOS_POLICY_ID;
using vbsutil::xmlparser::NO_KEY;
using vbsutil::xmlparser::ParticipantAttributes;
using vbsutil::xmlparser::PropertyPolicyHelper;
using vbsutil::xmlparser::PropertyPolicyQos;
using vbsutil::xmlparser::PTHREAD_CONDITION_WAKE_UP;
using vbsutil::xmlparser::PublisherAttributes;
using vbsutil::xmlparser::QosPolicyId_t;
using vbsutil::xmlparser::QueueWakeUp;
using vbsutil::xmlparser::READER;
using vbsutil::xmlparser::ReaderProxyDataPart;
using vbsutil::xmlparser::ReaderTimes;
using vbsutil::xmlparser::ReliabilityKind_t;
using vbsutil::xmlparser::ReliabilityQosPolicy;
using vbsutil::xmlparser::ReliabilityQosPolicyKind;
using vbsutil::xmlparser::RELIABLE_RELIABILITY_QOS;
using vbsutil::xmlparser::RTPSEndpointQos;
using vbsutil::xmlparser::RTPSParticipantAttributes;
using vbsutil::xmlparser::SharedQueueTransportDescriptorPart;
using vbsutil::xmlparser::StaticRTPSParticipantInfoPart;
using vbsutil::xmlparser::string_255;
using vbsutil::xmlparser::SubscriberAttributes;
using vbsutil::xmlparser::TopicAttributes;
using vbsutil::xmlparser::TopicKind_t;
using vbsutil::xmlparser::TransportDescriptorInterfacePart;
using vbsutil::xmlparser::TransportTypeKind;
using vbsutil::xmlparser::VOLATILE;
using vbsutil::xmlparser::WireProtocolConfigQos;
using vbsutil::xmlparser::WITH_KEY;
using vbsutil::xmlparser::WRITER;
using vbsutil::xmlparser::WriterProxyDataPart;
using vbsutil::xmlparser::XMLEndpointParserPart;
using vbsutil::xmlparser::XMLP_ret;
using vbsutil::xmlparser::XMLParserpart;
using vbsutil::xmlparser::XMLProfileManagerPart;
using vbsutil::xmlparser::xmlString;

namespace utils {
using vbsutil::xmlparser::PublisherAttributes;
using vbsutil::xmlparser::RTPSParticipantAttributes;
using vbsutil::xmlparser::SubscriberAttributes;
using vbsutil::xmlparser::TopicAttributes;
using vbsutil::xmlparser::TransportTypeKind;
} /* namespace utils */

namespace statistics {
using vbsutil::xmlparser::ReliabilityQosPolicy;
using vbsutil::xmlparser::RELIABLE_RELIABILITY_QOS;
using vbsutil::xmlparser::RTPSEndpointQos;
} /* namespace statistics */

} /* namespace dsfdds */
} /* namespace li */

#endif /*DSFDDS_ALL_COMMON_H_ */
