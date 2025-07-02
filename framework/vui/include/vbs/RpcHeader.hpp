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

#ifndef VBS_RPCHEADER_HPP_
#define VBS_RPCHEADER_HPP_

#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <set>

#include "erpc/server/RpcServer.h"
#include "erpc/server/RpcServerImpl.h"
#include "erpc/server/RpcServerFactory.h"
#include "erpc/client/RpcClient.h"
#include "erpc/client/RpcClientFactory.h"
#include "erpc/utils/Utilities.h"
#include "erpc/utils/RpcDataType.hpp"
#include "ecdr/Cdr.h"
#include "ecdr/exceptions/BadParamException.h"
#include "ecdr/CdrSizeCalculator.hpp"
#include "ecdr/cdr/fixed_size_string.hpp"
#include "vbs/Utils.hpp"

namespace vbs {
using Cdr = vbsutil::ecdr::Cdr;
using NotEnoughMemoryException = vbsutil::ecdr::exception::NotEnoughMemoryException;
using BadParamException = vbsutil::ecdr::exception::BadParamException;
using FastBuffer = vbsutil::ecdr::FastBuffer;

template <size_t MAX_CHARS>
using fixed_string = vbsutil::ecdr::fixed_string<MAX_CHARS>;
}  // namespace vbs
#endif  // VBS_RPCHEADER_HPP_
