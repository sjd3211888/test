// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef INCLUDE_CORE_GUID_H_
#define INCLUDE_CORE_GUID_H_

#include <random>
#include <ctime>

#include "core/Global.hpp"

namespace vbs {
class VBSGuid {
 public:
    VBSGuid();

    uint16_t generate();

    uint16_t guid();

    VBSGuid operator=(const VBSGuid* id);

 private:
    int vbs_get_pid_info();

    char value[16];
};

}  // namespace vbs
#endif  // INCLUDE_CORE_GUID_H_
