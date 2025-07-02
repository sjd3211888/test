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

#ifndef VBS_DYNAMICPUBSUBTYPE_HPP_
#define VBS_DYNAMICPUBSUBTYPE_HPP_

#include "core/Global.hpp"
#include "ertps/types/DynamicPubSubType.h"

namespace vbs {
class VbsDynamicPubSubTypeImpl : public evbs::ertps::types::DynamicPubSubType {
 public:
    VbsDynamicPubSubTypeImpl();

    explicit VbsDynamicPubSubTypeImpl(vbs::DynamicType_ptr pType);
};

class VbsFlatDataPubSubType : public vbs::TopicDataType {
 public:
    explicit VbsFlatDataPubSubType(const std::string& type_name);

    virtual bool is_bounded() const override;

    virtual bool is_plain() const override;

    bool serialize(void* data, vbs::SerializedPayload_t* payload) override;

    bool deserialize(vbs::SerializedPayload_t* payload, void* data) override;

    std::function<uint32_t()> getSerializedSizeProvider(void* data) override;

    bool getKey(void* data, vbs::InstanceHandle_t* ihandle, bool force_md5 = false) override;

    void* createData() override;

    void deleteData(void* data) override;
};

}  // namespace vbs
#endif  // VBS_DYNAMICPUBSUBTYPE_HPP_
