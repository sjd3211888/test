// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef DSFDDS_BASE_STATUS_HPP_
#define DSFDDS_BASE_STATUS_HPP_

#include <cstdint>
#include <ostream>
#include "deps/core/status/BaseStatus.hpp"

namespace li {
namespace dsfdds {

//! @brief A struct storing the base status
struct BaseStatus : public evbs::edds::dds::BaseStatus {
    /** @see shmmq::queue_ret_t
   * error code reason
   * SUCCESS = 0,                 Operation success
   * ERR_BAD_ADDRESS = -1,        Bad address
   * ERR_INVALID_ARGUMENT = -2,   Invalid argument
   * ERR_FILE_EXIST = -3,         File exists
   * ERR_INVALID_OPERATION = -4,  Invalid operation
   * ERR_TIME_OUT = -5,           time out
   * ERR_QUEUE_FULL = -6,   enqueue will return this error in no rewrite mode
   * ERR_QUEUE_EMPTY = -7,  dequeue will return this error in no block mode,
   *                        detected an attempt to re-initialise the object ,
   *                        a previously initialised, but not yet destroyed
   * ERR_BUSY = -8,
   * ERR_NOMEM = -9, Insufficient memory exists
   *                 The system lacked the necessary resources (other than memory) to initialise
   * ERR_AGAIN = -10,
   * ERR_INVALID_READER_INDEX = -11, reader index invalid
   * ERR_RUNTIME_EXCEPTION = -12,    run time exception
   * ERR_MUTEX = -13,                mutex error
   * ERR_UNKNOWN = -14               Error unknown
   * BEST_EFFORT_OVERWRITE = -15     best effort mode over write
   */
    int32_t error_code = 0;
    friend std::ostream& operator<<(std::ostream& os, const BaseStatus& status) {
        os << "total_count: " << status.total_count << " total_count_change: " << status.total_count_change
           << " error_code: " << status.error_code;
        return os;
    }
};

// !Alias of BaseStatus
using SampleLostStatus = BaseStatus;

enum SampleLostStatusErrorCode { BEST_EFFORT_OVERWRITE = -15 };

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_BASE_STATUS_HPP_
