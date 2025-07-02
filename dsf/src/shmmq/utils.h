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

#ifndef UTILS_H
#define UTILS_H
#include "shmmq/message_queue.h"

namespace shmmq {

class Utils {
 public:
    static int64_t GetTime_NS();

    static queue_ret_t ToShmmqErr(int error);
};

inline int64_t Utils::GetTime_NS() {
    std::chrono::nanoseconds nano_seconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
    return nano_seconds.count();
}

}  // namespace shmmq

#endif  // UTILS_H
