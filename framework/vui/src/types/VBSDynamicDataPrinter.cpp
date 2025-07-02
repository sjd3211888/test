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

#include <map>
#include <memory>
#include <new>
#include <locale>
#include <codecvt>

#include "elog/Log.hpp"
#include "vbs/Global.hpp"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/VBSDynamicDataPrinter.h"

using vbs::EvbsDynamicData;
vbs::common::types::DynamicDataHelper vbs::VBSDynamicDataPrinter::helper;

namespace vbs {
class VBSDynamicData;

VBSDynamicDataPrinter::VBSDynamicDataPrinter() {
    instance = new DynamicDataPrinter();
}

VBSDynamicDataPrinter::~VBSDynamicDataPrinter() {
    if (instance != nullptr)
        delete instance;
}

DynamicDataPrinter* VBSDynamicDataPrinter::get_instance() {
    return instance;
}

void VBSDynamicDataPrinter::print_to_sstream(std::ostringstream& out, const VBSDynamicData* data, int enum_mode) {
    EvbsDynamicData* dyn_data = data->vbs_dynamic_data_;
    instance->print_to_sstream(out, dyn_data, enum_mode);
}
void VBSDynamicDataPrinter::print_with_helper(const VBSDynamicData* data) {
    const EvbsDynamicData* dyn_data = data->vbs_dynamic_data_;
    helper.print(dyn_data);
}
}  // namespace vbs
