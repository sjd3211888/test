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

#ifndef INCLUDE_CORE_WRITEPARAMS_H_
#define INCLUDE_CORE_WRITEPARAMS_H_

#include "core/Guid.hpp"

namespace vbs {
class VBSWriteParams {
 public:
    VBSWriteParams();

    VBSWriteParams(uint64_t num);

    VBSWriteParams(uint64_t num, const uint16_t& id);

    uint64_t sequence_num();

    void sequence_num(uint64_t num);

    uint16_t guid();

    void guid(const uint16_t& id);

 private:
    uint64_t sequence_num_ = 0;
    uint16_t guid_ = 0;
};

}  // namespace vbs
#endif  // INCLUDE_CORE_WRITEPARAMS_H_
