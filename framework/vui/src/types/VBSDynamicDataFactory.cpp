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

#include "ertps/types/DynamicDataFactory.h"
#include "topic/DynamicPubSubTypeImpl.hpp"
#include "vbs/types/VBSDynamicDataFactory.h"
#include "core/Global.hpp"
#include "vbs/Utils.hpp"
#include "vbs/types/VBSDynamicData.h"

namespace vbs {

VBSDynamicDataFactory::VBSDynamicDataFactory() {
    dynamic_data_factory_ = EvbsDynamicDataFactory::get_instance();
}

VBSDynamicDataFactory::~VBSDynamicDataFactory() {
    EvbsDynamicDataFactory::delete_instance();
}

static VBSDynamicDataFactory* instance_ = nullptr;
VBSDynamicDataFactory* VBSDynamicDataFactory::get_instance() {
    if (instance_ == nullptr) {
        instance_ = new VBSDynamicDataFactory();
    }
    return instance_;
}

ReturnCode_t VBSDynamicDataFactory::delete_instance() {
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(ReturnCode_t::RETCODE_ERROR);
}

VBSDynamicData* VBSDynamicDataFactory::create_data(VBSDynamicType pType) {
    EvbsDynamicData* data = dynamic_data_factory_->create_data(*(pType.dynamic_type_));
    VBSDynamicData* vbs_data = new VBSDynamicData();
    vbs_data->vbs_dynamic_data_ = data;
    vbs_data->type_ = EvbsTypeSupport(new vbs::VbsDynamicPubSubTypeImpl(*(pType.dynamic_type_)));
    vbs_data->type_name_ = pType.get_name();
    return vbs_data;
}

ReturnCode_t VBSDynamicDataFactory::delete_data(VBSDynamicData* pData) {
    delete pData;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

VBSDynamicData* VBSDynamicDataFactory::create_copy(const VBSDynamicData* pData) {
    VBSDynamicData* vbs_data = new VBSDynamicData();
    EvbsDynamicData* data = dynamic_data_factory_->create_copy(pData->vbs_dynamic_data_);
    vbs_data->vbs_dynamic_data_ = data;
    return vbs_data;
}

}  // namespace vbs
