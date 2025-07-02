// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _EDDS_DDS_BUILTINQOS_QOSPOLICYUTILS_HPP_
#define _EDDS_DDS_BUILTINQOS_QOSPOLICYUTILS_HPP_

#include <stdint.h>

namespace vbs {
namespace utils {

// Compute the default DataSharing domain ID
uint64_t default_domain_id();

}  // namespace utils
}  // namespace vbs

#endif  // _EDDS_DDS_BUILTINQOS_QOSPOLICYUTILS_HPP_
