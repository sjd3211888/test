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

#ifndef VBS_Utils_HPP_
#define VBS_Utils_HPP_

#include <cstdint>
#include <iostream>

#include "elog/Log.hpp"
#include "elog/ReturnCode.hpp"

namespace vbs {

#define VBS_MEMBER_ID_INVALID 0X0FFFFFFFU

class ReturnCode_t {
 private:
    uint32_t value_;

 public:
    enum ReturnCodeValue : uint32_t {
        RETCODE_OK = 0,
        RETCODE_ERROR = FWK_ERRORCODE(vbsutil::elog::ExceptionCode_t::Submodule::VBS_FWK, 1),
        RETCODE_UNSUPPORTED,
        RETCODE_BAD_PARAMETER,
        RETCODE_PRECONDITION_NOT_MET,
        RETCODE_OUT_OF_RESOURCES,
        RETCODE_NOT_ENABLED,
        RETCODE_IMMUTABLE_POLICY,
        RETCODE_INCONSISTENT_POLICY,
        RETCODE_ALREADY_DELETED,
        RETCODE_TIMEOUT,
        RETCODE_NO_DATA,
        RETCODE_ILLEGAL_OPERATION,
        RETCODE_NOT_ALLOWED_BY_SECURITY,
        RETCODE_E2E_CHECK_FAILED,
        RETCODE_EXIST,
        RETCODE_NOT_FIND,
        RETCODE_DELIVERED,
        RETCODE_NOT_DELIVERED,
        RETCODE_EXCEEDED_LIMIT,
        RETCODE_TRACERMSG_TAG,
        RETCODE_PUBSUB_ERROR = FWK_ERRORCODE(vbsutil::elog::ExceptionCode_t::Submodule::VBS_FWK, 3000),
        RETCODE_UNKNOWN_CODE = FWK_ERRORCODE(vbsutil::elog::ExceptionCode_t::Submodule::VBS_FWK, 0xFFFF)
    };

    ReturnCode_t() : value_(RETCODE_OK) {}

    ReturnCode_t(RetCode_t e) : value_(e()) {}

    ReturnCode_t(const uint32_t& e) {
        if (e == 0u) {
            value_ = e;
        } else {
            value_ = FWK_ERRORCODE(vbsutil::elog::ExceptionCode_t::Submodule::VBS_FWK, e);
        }
    };

    bool operator!=(const ReturnCode_t& c) const { return value_ != c.value_; }

    bool operator==(const ReturnCode_t& c) const { return value_ == c.value_; }

    friend bool operator==(ReturnCodeValue lhs, const ReturnCode_t& rhs) {
        return static_cast<uint32_t>(lhs) == rhs.value_;
    }

    friend bool operator!=(ReturnCodeValue lhs, const ReturnCode_t& rhs) { return !(lhs == rhs); }

    explicit operator bool() = delete;

    uint32_t operator()() const { return value_; }

    bool operator!() const { return value_ != 0U; }
};

#ifndef _VBSTYPE_TEMP_
#define _VBSTYPE_TEMP_
template <typename T, typename = void>
struct isVBSType : std::false_type {};

template <typename T, typename = void>
struct isVBSRpcType : std::false_type {};

template <typename T, typename = void>
struct isVBSDdsType : std::false_type {};
#endif  // _VBSTYPE_TEMP_

template <typename T>
struct topic_type_support;

template <typename T>
struct rpc_type_support;

enum class TypeKind {
    TK_NONE = 0x00U,
    TK_BOOLEAN = 0x01U,
    TK_BYTE = 0x02U,
    TK_INT16 = 0x03U,
    TK_INT32 = 0x04U,
    TK_INT64 = 0x05U,
    TK_UINT16 = 0x06U,
    TK_UINT32 = 0x07U,
    TK_UINT64 = 0x08U,
    TK_FLOAT32 = 0x09U,
    TK_FLOAT64 = 0x0AU,
    TK_FLOAT128 = 0x0BU,
    TK_CHAR8 = 0x10U,
    TK_CHAR16 = 0x11U,
    TK_INT8 = 0x12U,
    TK_STRING8 = 0x20U,
    TK_STRING16 = 0x21U,
    TK_ALIAS = 0x30U,
    TK_ENUM = 0x40U,
    TK_BITMASK = 0x41U,
    TK_ANNOTATION = 0x50U,
    TK_STRUCTURE = 0x51U,
    TK_UNION = 0x52U,
    TK_BITSET = 0x53U,
    TK_SEQUENCE = 0x60U,
    TK_ARRAY = 0x61U,
    TK_MAP = 0x62U,
    TI_STRING8_SMALL = 0x70U,
    TI_STRING8_LARGE = 0x71U,
    TI_STRING16_SMALL = 0x72U,
    TI_STRING16_LARGE = 0x73U,
    TI_PLAIN_SEQUENCE_SMALL = 0x80U,
    TI_PLAIN_SEQUENCE_LARGE = 0x81U,
    TI_PLAIN_ARRAY_SMALL = 0x90U,
    TI_PLAIN_ARRAY_LARGE = 0x91U,
    TI_PLAIN_MAP_SMALL = 0xA0U,
    TI_PLAIN_MAP_LARGE = 0xA1U,
    TI_STRONGLY_CONNECTED_COMPONENT = 0xB0U
};

enum class ListenerMatchStatus {
    UnMatch,
    MatchNoCall,
    MatchCall,
};

enum class VbsVersionModule {
    FWK,
    EVBS,
    DSF,
};

class TraceEnableModule {
 private:
    uint32_t value_;

 public:
    enum TraceEnablKind {
        VBS_OBV_TRACE_CLOSE = 0x010000,
        VBS_OBV_TRACE_OPEN = 0x010100,
        VBS_OBV_TRACE_DUMP = 0x010200,
        VBS_OBV_LOG_ERROR = 0x020000,
        VBS_OBV_LOG_EVENT = 0x020001,
        VBS_OBV_LOG_WARN = 0x020002,
        VBS_OBV_LOG_INFO = 0x020004
    };

    TraceEnableModule() : value_(VBS_OBV_TRACE_OPEN) {}

    TraceEnableModule(uint32_t& e) { value_ = (e & 0xff) | ((e) & (0xff << 8)) | ((e) & (0xff << 16)); }

    TraceEnableModule(uint32_t&& e) { value_ = (e & 0xff) | ((e) & (0xff << 8)) | ((e) & (0xff << 16)); }

    TraceEnableModule& operator=(const uint32_t& e) {
        value_ = e;
        return *this;
    }

    bool operator!=(const TraceEnableModule& c) const { return value_ != c.value_; }

    bool operator==(const TraceEnableModule& c) const { return value_ == c.value_; }

    friend TraceEnableModule operator|(const TraceEnableModule& lhs, const TraceEnableModule& rhs) {
        return vbs::TraceEnableModule(static_cast<uint32_t>(lhs()) | static_cast<uint32_t>(rhs()));
    }

    explicit operator bool() = delete;

    uint32_t operator()() const { return value_; }

    bool operator!() const { return value_ != 0U; }
};

#define NET_TRAFFIC_HOSTORY_TIME 10000

/**
 * @brief Network traffic information.
 *
 * This struct contains information about the network traffic, including
 * the total number of packets, throughput, timestamp, and direction.
 *
 * @memberof pkg
 */
struct NetTrafficInfo {
    NetTrafficInfo() : pkg(0), throughput(0), timestamp(0), direction(0) {}
    uint32_t pkg;         // total VBS package number
    uint64_t throughput;  // total VBS throughput number
    uint64_t timestamp;   // time when this data collected
    uint8_t direction;    // 1 is send traffic, 2 is receive traffic, 0 is unknwon
};

#define VbsLogD(msg) logDebug_(VBS_FWK, msg)
#define VbsLogI(msg) logInfo_(VBS_FWK, msg)
#define VbsLogW(msg) logWarning_(VBS_FWK, msg)
#define VbsLogE(msg) logError_(VBS_FWK, msg)
#define VbsLogEvent(msg) logEvent_(VBS_FWK, msg)

inline std::string VbsDecToHex(uint32_t x) {
    std::string hex_str = "";
    int tmp = 0;
    char c = '\0';
    while (x) {
        tmp = x % 16;
        switch (tmp) {
            case 0:
                c = '0';
                break;
            case 1:
                c = '1';
                break;
            case 2:
                c = '2';
                break;
            case 3:
                c = '3';
                break;
            case 4:
                c = '4';
                break;
            case 5:
                c = '5';
                break;
            case 6:
                c = '6';
                break;
            case 7:
                c = '7';
                break;
            case 8:
                c = '8';
                break;
            case 9:
                c = '9';
                break;
            case 10:
                c = 'A';
                break;
            case 11:
                c = 'B';
                break;
            case 12:
                c = 'C';
                break;
            case 13:
                c = 'D';
                break;
            case 14:
                c = 'E';
                break;
            case 15:
                c = 'F';
                break;
        }
        hex_str += c;
        x = x / 16;
    }
    hex_str += 'x';
    hex_str += '0';
    std::reverse(hex_str.begin(), hex_str.end());
    return hex_str;
}

}  // namespace vbs

#endif  // VBS_Utils_HPP_
