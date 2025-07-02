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

#ifndef INCLUDE_CORE_HOST_H_
#define INCLUDE_CORE_HOST_H_

#include "core/Global.hpp"

namespace vbs {
class Host {
 public:
    Host();

    uint32_t id();

 private:
    uint32_t value;
};

}  // namespace vbs
#endif  // INCLUDE_CORE_HOST_H_
