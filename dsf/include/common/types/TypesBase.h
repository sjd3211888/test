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

#ifndef DSFDDS_TYPES_BASE_H
#define DSFDDS_TYPES_BASE_H

#include <algorithm>
#include <bitset>
#include <type_traits>
#include <vector>
#include <utility>
#include <cstdint>
#include <elog/ExceptionCode.hpp>

namespace li {
namespace dsfdds {

#define AUTHENTICATION_FAILED 29

enum class MultiModeChannel : uint32_t { NONE = 0, AUTO = 1 << 0, SHM = 1 << 1, BIGBUFFER = 1 << 2 };

constexpr MultiModeChannel operator|(MultiModeChannel a, MultiModeChannel b) {
    return static_cast<MultiModeChannel>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr MultiModeChannel operator&(MultiModeChannel a, MultiModeChannel b) {
    return static_cast<MultiModeChannel>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline std::ostream& operator<<(std::ostream& os, MultiModeChannel mode) {
    switch (mode) {
        case MultiModeChannel::NONE:
            os << "NONE";
            break;
        case MultiModeChannel::AUTO:
            os << "AUTO";
            break;
        case MultiModeChannel::SHM:
            os << "SHM";
            break;
        case MultiModeChannel::BIGBUFFER:
            os << "BIGBUFFER";
            break;
        default:
            os << "UNKNOWN(" << static_cast<uint32_t>(mode) << ")";
    }
    return os;
}

using DomainId_t = uint16_t;
/*!
 * @brief This class represents the enumeration ReturnCode_t.
 */

class ReturnCode_t {
    uint32_t value_;

 public:
    enum ReturnCodeValue : uint32_t {
        // pubsub code
        RETCODE_PUBSUB_OK = 0,                                           // 0
        RETCODE_PUBSUB_ERROR = 0x3001,                                   // 0x03643001
        RETCODE_PUBSUB_UNSUPPORTED,                                      // 0x03643002
        RETCODE_PUBSUB_BAD_PARAMETER,                                    // 0x03643003
        RETCODE_PUBSUB_PRECONDITION_NOT_MET,                             // 0x03643004
        RETCODE_PUBSUB_OUT_OF_RESOURCES,                                 // 0x03643005
        RETCODE_PUBSUB_NOT_ENABLED,                                      // 0x03643006
        RETCODE_PUBSUB_ALREADY_DELETED,                                  // 0x03643007
        RETCODE_PUBSUB_TIMEOUT,                                          // 0x03643008
        RETCODE_PUBSUB_NO_DATA,                                          // 0x03643009
        RETCODE_PUBSUB_ILLEGAL_OPERATION,                                // 0x0364300a
        RETCODE_PUBSUB_QOS_CHECK_ERR,                                    // 0x0364300b
        RETCODE_PUBSUB_OPEN_QUEUE_ERR,                                   // 0x0364300c
        RETCODE_PUBSUB_LOCAL_QOS_CHECK_ERR,                              // 0x0364300d
        RETCODE_PUBSUB_CONFIG_CHECK_ERR,                                 // 0x0364300e
        RETCODE_PUBSUB_EDPREGISTER_ERR,                                  // 0x0364300f
        RETCODE_PUBSUB_CREATE_MEMBUF_POOL_ERR,                           // 0x03643010
        RETCODE_PUBSUB_TOPIC_NAME_EMPTY,                                 // 0x03643011
        RETCODE_PUBSUB_GET_XML_ATTRIBUTES_ERR,                           // 0x03643012
        RETCODE_PUBSUB_TOPIC_CHECK_ERR,                                  // 0x03643013
        RETCODE_PUBSUB_ENTITY_EMPLACE_ERR,                               // 0x03643014
        RETCODE_PUBSUB_ENTITY_REMOVE_ERR,                                // 0x03643015
        RETCODE_PUBSUB_LOAD_XML_ERR,                                     // 0x03643016
        RETCODE_PUBSUB_DISCOVERYINFO_REGISTER_ERR,                       // 0x03643017
        RETCODE_PUBSUB_TOPIC_NOT_REGISTERED_ERR,                         // 0x03643018
        RETCODE_PUBSUB_REGISTER_LOCAL_INFO_ERR,                          // 0x03643019
        RETCODE_PUBSUB_SEM_ERR,                                          // 0x0364301a
        RETCODE_PUBSUB_ENTITY_ID_LESS_THAN_0_ERR,                        // 0x0364301b
        RETCODE_PUBSUB_GET_MQ_ERR,                                       // 0x0364301c
        RETCODE_PUBSUB_CREATE_OBJECT_ERR,                                // 0x0364301d
        RETCODE_PUBSUB_ENTITY_ID_REPEATED_ERR,                           // 0x0364301e
        RETCODE_PUBSUB_PARTICIPANT_ID_REPEATED_ERR,                      // 0x0364301f
        RETCODE_PUBSUB_PID_CHECK_ERR,                                    // 0x03643020
        RETCODE_PUBSUB_REMOVE_LOCAL_ENTITY_ERR,                          // 0x03643021
        RETCODE_PUBSUB_GUID_GENERATE_ERR,                                // 0x03643022
        RETCODE_PUBSUB_SHARED_QUEUE_READER_NOT_MATCH,                    // 0x03643023
        RETCODE_PUBSUB_SHARED_QUEUE_READER_OFFLINE,                      // 0x03643024
        RETCODE_PUBSUB_SHARED_QUEUE_READER_STATUS_ERR,                   // 0x03643025
        RETCODE_PUBSUB_SHARED_QUEUE_READER_NOT_ENABLE_OR_LISTENER_NULL,  // 0x03643026
        RETCODE_PUBSUB_SHARED_QUEUE_READER_ID_NOT_MY_OWN,                // 0x03643027
        RETCODE_PUBSUB_POOL_OWNER_MANAGER_IS_NULL,                       // 0x03643028
        RETCODE_PUBSUB_ALLOC_BUFFER_FAILED,                              // 0x03643029
        RETCODE_PUBSUB_LOAN_SAMPLE_FAILED,                               // 0x0364302a
        RETCODE_PUBSUB_CHANGE_MBUF_IS_EMPTY,                             // 0x0364302b
        RETCODE_PUBSUB_CHANGE_SER_MBUF_IS_EMPTY,                         // 0x0364302c
        RETCODE_PUBSUB_CHANGE_NON_SER_MBUF_IS_EMPTY,                     // 0x0364302d
        RETCODE_PUBSUB_TAKE_SAMPLE_FAILED,                               // 0x0364302e
        RETCODE_PUBSUB_GET_REBUILD_BUFFER_FAILED,                        // 0x0364302f
        RETCODE_PUBSUB_SEND_MBUF_FAILED,                                 // 0x03643030

        // discovery code
        RETCODE_DISCOVERY_OK = RETCODE_PUBSUB_OK,              // 0
        RETCODE_DISCOVERY_ERROR = 0x3101,                      // 0x03653101
        RETCODE_DISCOVERY_REGISTER_USERID_ERR,                 // 0x03653102
        RETCODE_DISCOVERY_UNREGISTER_USERID_ERR,               // 0x03653103
        RETCODE_DISCOVERY_NOT_FIND_LOCAL_WRITER,               // 0x03653104
        RETCODE_DISCOVERY_NOT_FIND_LOCAL_READER,               // 0x03653105
        RETCODE_DISCOVERY_NOT_FIND_EP_MATCHED_WRITER,          // 0x03653106
        RETCODE_DISCOVERY_NOT_FIND_EP_MATCHED_READER,          // 0x03653107
        RETCODE_DISCOVERY_WRITER_OPEN_QUEUE_ERR,               // 0x03653108
        RETCODE_DISCOVERY_SEND_TO_SERVICE_ERR,                 // 0x03653109
        RETCODE_DISCOVERY_UPDATE_VBS_CALLBACK_NOT_REGISTERED,  // 0x0365310a
        RETCODE_DISCOVERY_UPDATE_VBS_CALLBACK_EXISTED,         // 0x0365310b
        RETCODE_DISCOVERY_UPDATE_VBS_CALLBACK_NULL,            // 0x0365310c
        RETCODE_DISCOVERY_ENDPOINT_PROXY_NOT_FOUND,            // 0x0365310d
        RETCODE_DISCOVERY_INVALID_STATUS,                      // 0x0365310e
        RETCODE_DISCOVERY_NOT_IN_MATCHED_MAP,                  // 0x0365310f
        RETCODE_DISCOVERY_LOCAL_READER_NULL,                   // 0x03653110
        RETCODE_DISCOVERY_LOCAL_WRITER_NULL,                   // 0x03653111
        RETCODE_DISCOVERY_QOS_CHECK_ERR,                       // 0x03653112
        RETCODE_DISCOVERY_ENTITY_STATUS_UNKNOWN,               // 0x03653113
        RETCODE_DISCOVERY_OPEN_DS_RECVQUEUE_ERR,               // 0x03653114
        RETCODE_DISCOVERY_OPEN_PROCESS_RECVQUEUE_ERR,          // 0x03653115
        RETCODE_DISCOVERY_INVALID_STATUS_MATCH_TO_ONLINE,      // 0x03653116
        RETCODE_DISCOVERY_INVALID_STATUS_NOT_ONLINE_TO_MATCH,  // 0x03653117

        // shmmq code
        RETCODE_SHMMQ_SUCCESS = RETCODE_PUBSUB_OK, /*Operation success 0*/
        RETCODE_SHMMQ_BAD_ADDRESS = 0x3201,        /* Bad address 0x03663201*/
        RETCODE_SHMMQ_INVALID_ARGUMENT,            /* Invalid argument 0x03663202*/
        RETCODE_SHMMQ_FILE_EXIST,                  /* File exists 0x03663203*/
        RETCODE_SHMMQ_INVALID_OPERATION,           /* Invalid operation 0x03663204*/
        RETCODE_SHMMQ_TIME_OUT,                    /* time out 0x03663205*/
        RETCODE_SHMMQ_QUEUE_FULL,                  /* enqueue will return this error in no rewrite mode 0x03663206*/
        RETCODE_SHMMQ_QUEUE_EMPTY,                 /* dequeue will return this error in no block mode 0x03663207*/
        /*detected an attempt to re-initialise the object ,a previously*/
        /*initialised, but not yet destroyed*/
        RETCODE_SHMMQ_BUSY,  // 0x03663208
        RETCODE_SHMMQ_NOMEM, /*Insufficient memory exists 0x03663209*/
        /*The system lacked the necessary resources (other than memory) to initialise*/
        RETCODE_SHMMQ_AGAIN,                     /*ref to error num EAGAIN 0x0366320a*/
        RETCODE_SHMMQ_INVALID_READER_INDEX,      /*reader index invalid 0x0366320b*/
        RETCODE_SHMMQ_RUNTIME_EXCEPTION,         /*run time exception 0x0366320c*/
        RETCODE_SHMMQ_MUTEX_EXCEPTION,           /*mutex error 0x0366320d*/
        RETCODE_SHMMQ_INVALID_MBUF,              /*mbuf invalid 0x0366320e*/
        RETCODE_SHMMQ_UNKNOWN,                   /*Error unknown 0x0366320f*/
        RETCODE_SHMMQ_CHECK_ATTRIBUTE_FAILED,    // 0x03663210
        RETCODE_SHMMQ_ERROR,                     // 0x03663211
        RETCODE_SHMMQ_MUTEX_LOCK_ERR,            // 0x03663212
        RETCODE_SHMMQ_SEM_OPERATION_ERR,         // 0x03663213
        RETCODE_SHMMQ_SHM_OPEN_ERR,              // 0x03663214
        RETCODE_SHMMQ_SHM_CREATE_ERR,            // 0x03663215
        RETCODE_SHMMQ_SHM_MMAP_ERR,              // 0x03663216
        RETCODE_SHMMQ_SHM_UNMAP_ERR,             // 0x03663217
        RETCODE_SHMMQ_SHM_CLOSE_ERR,             // 0x03663218
        RETCODE_SHMMQ_SHM_FSTAT_ERR,             // 0x03663219
        RETCODE_SHMMQ_EPOLL_ERR,                 // 0x0366321a
        RETCODE_SHMMQ_HEAP_FD_ERR,               // 0x0366321b
        RETCODE_SHMMQ_HEAP_QUERY_ERR,            // 0x0366321c
        RETCODE_SHMMQ_NOT_FIND_QUEUE_NAME_ERR,   // 0x0366321d
        RETCODE_SHMMQ_PROFILE_TIMESTAMP_ERR,     // 0x0366321e
        RETCODE_SHMMQ_QUEUE_FULL_INDEX_ERR,      // 0x0366321f
        RETCODE_SHMMQ_QUEUE_EMPTY_INDEX_ERR,     // 0x03663220
        RETCODE_SHMMQ_EVENT_INFO_IS_UNEXPECTED,  // 0x03663221

        // membuf code
        RETCODE_MEMBUF_OK = RETCODE_PUBSUB_OK,              // 0
        RETCODE_MEMBUF_ERROR = 0x3301,                      // 0x03673301
        RETCODE_MEMBUF_INPUT_MBUF_NULL,                     // 0x03673302
        RETCODE_MEMBUF_SUBBUF_RECYCLE_NUM_INVALID,          // 0x03673303
        RETCODE_MEMBUF_SUBBUF_ON_PENDING_FAILED,            // 0x03673304
        RETCODE_MEMBUF_SUBBUF_ON_FREE_FAILED,               // 0x03673305
        RETCODE_MEMBUF_SUBBUF_UNKNOWN_OPERATE,              // 0x03673306
        RETCODE_MEMBUF_REBUILD_BUFFER_RECYCLE_NUM_INVALID,  // 0x03673307
        RETCODE_MEMBUF_REBUILD_BUFFER_ON_USING_ERR,         // 0x03673308
        RETCODE_MEMBUF_BUILD_DESC_ON_PENDING_ERR,           // 0x03673309
        RETCODE_MEMBUF_REBUILD_BUFFER_ERR,                  // 0x0367330a
        RETCODE_MEMBUF_USERID_INVALID,                      // 0x0367330b
        RETCODE_MEMBUF_BAD_PARAMETER,                       // 0x0367330c
        RETCODE_MEMBUF_OUT_OF_RANGE,                        // 0x0367330d
        RETCODE_MEMBUF_SHM_OPEN_ERR,                        // 0x0367330e
        RETCODE_MEMBUF_SHM_GET_UUID_ERR,                    // 0x0367330f
        RETCODE_MEMBUF_SHM_UNMAP_ERR,                       // 0x03673310
        RETCODE_MEMBUF_SHM_MMAP_ERR,                        // 0x03673311
        RETCODE_MEMBUF_OPEN_POOL_ERR,                       // 0x03673312
        RETCODE_MEMBUF_EMPLACE_POOL_ERR,                    // 0x03673313
        RETCODE_MEMBUF_RESET_TO_FREE_ON_EXCEPT_ERR,         // 0x03673314
        RETCODE_MEMBUF_STREAM_READ_ERR,                     // 0x03673315
        RETCODE_MEMBUF_CAS_INVALID_TO_USING_FAILED,         // 0x03673316
        RETCODE_MEMBUF_CAS_ADD_PID_FAILED,                  // 0x03673317
        RETCODE_MEMBUF_CAS_USING_TO_CRASH_FAILED,           // 0x03673318
        RETCODE_MEMBUF_CAS_USING_TO_INVALID_FAILED,         // 0x03673319
        RETCODE_MEMBUF_CAS_ANY_TO_FREE_FAILED,              // 0x0367331a
        RETCODE_MEMBUF_MUTEX_EXCEPTION,                     // 0x0367331b
        RETCODE_MEMBUF_HOLDMUTEX_CRASH,                     // 0x0367331c
        RETCODE_MEMBUF_CONFIG_ERR,                          // 0x0367331d
        RETCODE_MEMBUF_SCALE_UP_FAILED,                     // 0x0367331e
        RETCODE_MEMBUF_SCALE_DOWN_FAILED,                   // 0x0367331f
        RETCODE_MEMBUF_CREATE_POOL_FAILED,                  // 0x03673320
        RETCODE_MEMBUF_SHM_BUFFER_CLOSE_FAILED,             // 0x03673321
        RETCODE_MEMBUF_PTHREAD_MUTEX_INIT_FAILED,           // 0x03673322
        RETCODE_MEMBUF_ELEM_IS_NULL,                        // 0x03673323

        // discovery service code
        RETCODE_DS_OK = RETCODE_PUBSUB_OK,      // 0
        RETCODE_DS_ERROR = 0x3401,              // 0x03683401
        RETCODE_DS_RECEIVE_INVALID_ARGS,        // 0x03683402
        RETCODE_DS_RECEIVE_QUEUE_EMPTY,         // 0x03683403
        RETCODE_DS_ONLINE_EARLIER_CRASH,        // 0x03683404
        RETCODE_DS_VERSION_EMPTY,               // 0x03683405
        RETCODE_DS_ANOTHER_INSTANCE_RUNNING,    // 0x03683406
        RETCODE_DS_DATA_TYPE_NOT_MATCH,         // 0x03683407
        RETCODE_DS_DISCOVERY_TYPE_NOT_MATCH,    // 0x03683408
        RETCODE_DS_KIND_QOS_NOT_MATCH,          // 0x03683409
        RETCODE_DS_SEND_FAILED,                 // 0x0368340a
        RETCODE_DS_STATIC_ENTITY_ID_REPEATED,   // 0x0368340b
        RETCODE_DS_ENTITY_REPEATED,             // 0x0368340c
        RETCODE_DS_VERSION_NOT_MATCH,           // 0x0368340d
        RETCODE_DS_MQ_NULLPTR,                  // 0x0368340e
        RETCODE_DS_NOT_FIND_WIDEDOMAIN,         // 0x0368340f
        RETCODE_DS_OPEN_LOCK_FILE_ERR,          // 0x03683410
        RETCODE_DS_SETTING_SIGTERM_ERR,         // 0x03683411
        RETCODE_DS_MESSAGE_PAYLOAD_TYPE_ERR,    // 0x03683412
        RETCODE_DS_GET_ENTITY_RETURN_NULLPTR,   // 0x03683413
        RETCODE_DS_VERSION_ERROR,               // 0x03683414
        RETCODE_DS_RECEIVE_PAYLOAD_TYPE_ERROR,  // 0x03683415
    };

    ReturnCode_t() : value_(RETCODE_PUBSUB_OK) {}

    ReturnCode_t(uint32_t e) { value_ = e; }  // NOLINT

    bool operator==(const ReturnCode_t& c) const { return value_ == c.value_; }

    bool operator!=(const ReturnCode_t& c) const { return value_ != c.value_; }

    explicit operator bool() = delete;

    uint32_t operator()() const { return value_; }

    bool operator!() const { return value_ != 0; }
};

template <class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type operator==(
    T a, const ReturnCode_t& b) {
    return b.operator==(a);
}

template <class T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, bool>::type operator!=(
    T a, const ReturnCode_t& b) {
    return b.operator!=(a);
}

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TYPES_BASE_H
