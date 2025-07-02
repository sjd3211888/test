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

#ifndef VBS_GLOBAL_HPP_
#define VBS_GLOBAL_HPP_

#include <cstddef>

#include "ecdr/Cdr.h"
#include "ecdr/CdrSizeCalculator.hpp"
#include "ecdr/exceptions/BadParamException.h"
#include "deps/common/TopicDataType.hpp"
#include "deps/common/TypeSupport.hpp"
#include "deps/common/md5.h"
#include "deps/type/TypesBase.h"
#include "vbs/Utils.hpp"

namespace vbs {
class VbsDurabilityQos;
class VbsDeadLineQos;
class VbsLiveLinessQos;
class VbsReliabilityQos;
class VbsHistoryQos;
class VbsResourceLimitsQos;
class VbsLifespanQos;
class VbsOwnershipQos;
class VbsOwnershipStrengthQos;
class VbsPublishModeQos;
class VbsE2EProtectionQos;
class VbsTransportConfigQos;
class VbsWireProtocolConfigQos;
}  // namespace vbs

namespace vbsutil {
namespace ecdr {
template <size_t MAX_CHARS>
struct fixed_string;
class Cdr;
namespace exception {
class NotEnoughMemoryException;
class BadParamException;
}  // namespace exception
class FastBuffer;
}  // namespace ecdr
namespace xmlparser {
struct InstanceHandle_t;
}
}  // namespace vbsutil

namespace vbs {
using SerializedPayload_t = vbsutil::xmlparser::SerializedPayload_t;
using InstanceHandle_t = vbsutil::xmlparser::InstanceHandle_t;

using MD5 = vbsutil::MD5;
using Cdr = vbsutil::ecdr::Cdr;
using NotEnoughMemoryException = vbsutil::ecdr::exception::NotEnoughMemoryException;
using BadParamException = vbsutil::ecdr::exception::BadParamException;
using FastBuffer = vbsutil::ecdr::FastBuffer;
using vbsutil::ecdr::convertToStringVector;

template <size_t MAX_CHARS>
using fixed_string = vbsutil::ecdr::fixed_string<MAX_CHARS>;

using MemberId = vbs::common::types::MemberId;
using DomainId_t = uint32_t;
using octet = unsigned char;

constexpr DataRepresentationId_t DEFAULT_DATA_REPRESENTATION {DataRepresentationId_t::XCDR_DATA_REPRESENTATION};
}  // namespace vbs

#endif  // VBS_GLOBAL_HPP_
