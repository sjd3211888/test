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

#ifndef INCLUDE_CORE_GLOBAL_HPP_
#define INCLUDE_CORE_GLOBAL_HPP_

#include <cstdint>
#include <cassert>
#include <map>
#include <string>

#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "deps/core/status/DeadlineMissedStatus.hpp"
#include "deps/core/status/LivelinessChangedStatus.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "domain/DomainParticipantFactoryInner.hpp"
#include "domain/qos/DomainParticipantQosInner.hpp"
#include "pub/Publisher.hpp"
#include "pub/DataWriterListenerInner.hpp"
#include "sub/DataReaderListenerInner.hpp"
#include "xmlparser/qos/XMLDataReaderQos.hpp"
#include "xmlparser/qos/XMLTopicQos.hpp"
#include "topic/ContentFilteredTopicInner.hpp"
#include "edds/rtps/common/Guid.h"
#include "ertps/types/DynamicDataHelper.hpp"
#include "ertps/utils/fixed_size_string.hpp"
#include "vbs/Utils.hpp"
#include "deps/type/DynamicDataFactory.h"
#include "deps/type/TypeObjectFactory.h"
#include "deps/type/TypeObject.h"
#include "deps/type/DynamicTypeBuilderFactory.h"
#include "deps/type/MemberDescriptor.h"
#include "deps/core/status/PublicationMatchedStatus.hpp"
#include "deps/core/status/SubscriptionMatchedStatus.hpp"
#include "deps/core/status/DeadlineMissedStatus.hpp"
#include "deps/core/status/LivelinessChangedStatus.hpp"
#include "deps/core/status/BaseStatus.hpp"
#include "domain/DomainParticipantListenerInner.hpp"
#include "domain/DomainParticipantFactoryInner.hpp"
#include "domain/qos/DomainParticipantQosInner.hpp"
#include "pub/Publisher.hpp"
#include "pub/DataWriterListenerInner.hpp"
#include "sub/DataReaderListenerInner.hpp"
#include "topic/ContentFilteredTopicInner.hpp"
#include "edds/rtps/common/Guid.h"
#include "ertps/types/DynamicDataHelper.hpp"
#include "ertps/utils/fixed_size_string.hpp"
#include "ecdr/Cdr.h"
#include "ecdr/exceptions/BadParamException.h"
#include "deps/common/TopicDataType.hpp"
#include "deps/common/TypeSupport.hpp"
#include "deps/common/md5.h"

namespace DDSNS = evbs::edds::dds;

#define VBS_STRACE_WRITE_FLAG 1
#define VBS_STRACE_READ_FLAG 0
#define VBS_STRACE_DDS_FLAG 1
#define VBS_STRACE_DSF_FLAG 0

// for DDS
using VbsDDSDomainId_t = DDSNS::DomainId_t;
using VbsDDSDataWriterQos = vbsutil::xmlparser::DataWriterQos;
using VbsDDSDataReaderQos = vbsutil::xmlparser::DataReaderQos;
using VbsDDSPublicationMatchedStatus = DDSNS::PublicationMatchedStatus;
using VbsDDSSubscriptionMatchedStatus = DDSNS::SubscriptionMatchedStatus;
using VbsDDSOfferedDeadlineMissedStatus = DDSNS::OfferedDeadlineMissedStatus;
using VbsDDSLivelinessLostStatus = DDSNS::LivelinessLostStatus;
using VbsDDSLivelinessChangedStatus = DDSNS::LivelinessChangedStatus;
using VbsDDSSampleLostStatus = DDSNS::SampleLostStatus;
using VbsDDSE2EExceptionStatus = DDSNS::E2EExceptionStatus;
using VbsDDSStatusMask = DDSNS::StatusMask;
using VbsDDSTopicQos = vbsutil::xmlparser::TopicQos;
using VbsDDSReaderDiscoveryInfo = evbs::ertps::rtps::ReaderDiscoveryInfo;
using VbsDDSWriterDiscoveryInfo = evbs::ertps::rtps::WriterDiscoveryInfo;

namespace vbs {
using VbsDynamicTypeBuilderFactory = vbs::common::types::DynamicTypeBuilderFactory;
using DynamicDataFactory = vbs::common::types::DynamicDataFactory;
using DynamicPubSubType = evbs::ertps::types::DynamicPubSubType;
using DynamicType_ptr = vbs::common::types::DynamicType_ptr;
using DynamicData_ptr = vbs::common::types::DynamicData_ptr;
using DynamicData = vbs::common::types::DynamicData;
using DynamicDataHelper = evbs::ertps::types::DynamicDataHelper;
using TypeInformation = vbs::common::types::TypeInformation;
using TypeObjectFactory = vbs::common::types::TypeObjectFactory;
using TypeIdentifier = vbs::common::types::TypeIdentifier;
using TypeObject = vbs::common::types::TypeObject;
using SerializedPayload_t = vbs::SerializedPayload_t;
using WriteParams = vbs::common::WriteParams;
using InstanceHandle_t = vbs::InstanceHandle_t;
using DomainId_t = uint32_t;
using MD5 = vbsutil::MD5;
using TopicDataType = vbs::TopicDataType;
using Cdr = vbsutil::ecdr::Cdr;
using octet = evbs::ertps::rtps::octet;
using BadParamException = vbsutil::ecdr::exception::BadParamException;
using NotEnoughMemoryException = vbsutil::ecdr::exception::NotEnoughMemoryException;
using FastBuffer = vbsutil::ecdr::FastBuffer;

template <size_t MAX_CHARS>
using fixed_string = typename vbsutil::ecdr::fixed_string<MAX_CHARS>;

}  // namespace vbs
#endif  // INCLUDE_CORE_GLOBAL_HPP_
