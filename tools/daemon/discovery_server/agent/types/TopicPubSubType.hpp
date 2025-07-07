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

#ifndef AGENT_TYPES_TOPICPUBSUBTYPES_HPP
#define AGENT_TYPES_TOPICPUBSUBTYPES_HPP

#include <deps/common/TopicDataType.hpp>
#include <vector>

namespace discovery_server {
namespace agent {

class TopicPubSubType : public vbs::TopicDataType {
 public:
    typedef std::vector<unsigned char> type;

    explicit TopicPubSubType(bool with_key);

    ~TopicPubSubType() override = default;

    bool serialize(void* data, vbs::SerializedPayload_t* payload) override;

    bool serialize(void* data, vbs::SerializedPayload_t* payload,
                   vbs::DataRepresentationId_t data_representation) override;
    bool deserialize(vbs::SerializedPayload_t* payload, void* data) override;

    std::function<uint32_t()> getSerializedSizeProvider(void* data) override;

    std::function<uint32_t()> getSerializedSizeProvider(void* data,
                                                        vbs::DataRepresentationId_t data_representation) override;

    void* createData() override;

    void deleteData(void* data) override;

    bool getKey(void* data, vbs::InstanceHandle_t* ihandle, bool force_md5) override;
};

}  // namespace agent
}  // namespace discovery_server

#endif  // AGENT_TYPES_TOPICPUBSUBTYPES_HPP
