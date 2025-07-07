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

#ifndef _EDDS_GENERATED_855318312_RPCDISCOVERY_PUBSUBTYPES_H_
#define _EDDS_GENERATED_855318312_RPCDISCOVERY_PUBSUBTYPES_H_

#include <deps/common/TopicDataType.hpp>
#include <deps/common/md5.h>

#include "RpcDiscovery.h"

#if !defined(GEN_API_VER) || (GEN_API_VER != 1)
#error Generated RpcDiscovery is not compatible with current installed Fast DDS. Please, regenerate it with fastddsgen.
#endif  // GEN_API_VER

/*!
 * @brief This class represents the TopicDataType of the type RpcDiscovery defined by the user in the IDL file.
 * @ingroup RPCDISCOVERY
 */
class RpcDiscoveryPubSubType : public vbs::TopicDataType {
 public:
    typedef RpcDiscovery type;

    vbs_user_DllExport RpcDiscoveryPubSubType();

    vbs_user_DllExport virtual ~RpcDiscoveryPubSubType() override;

    vbs_user_DllExport virtual bool serialize(void* data, vbs::SerializedPayload_t* payload) override;

    vbs_user_DllExport virtual bool deserialize(vbs::SerializedPayload_t* payload, void* data) override;

    vbs_user_DllExport virtual std::function<uint32_t()> getSerializedSizeProvider(void* data) override;
    vbs_user_DllExport virtual bool getKey(void* data, vbs::InstanceHandle_t* ihandle, bool force_md5 = false) override;

    vbs_user_DllExport virtual void* createData() override;

    vbs_user_DllExport virtual void deleteData(void* data) override;

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
    vbs_user_DllExport inline bool is_bounded() const override { return false; }

#endif  // TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED

#ifdef TOPIC_DATA_TYPE_API_HAS_IS_PLAIN
    vbs_user_DllExport inline bool is_plain() const override {
        return false;
        //return false;
    }

#endif  // TOPIC_DATA_TYPE_API_HAS_IS_PLAIN

#ifdef TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE
    vbs_user_DllExport inline bool construct_sample(void* memory) const override {
        static_cast<void>(memory);
        return false;
    }

#endif  // TOPIC_DATA_TYPE_API_HAS_CONSTRUCT_SAMPLE

    vbsutil::MD5 m_md5;
    unsigned char* m_keyBuffer;
};

#endif
