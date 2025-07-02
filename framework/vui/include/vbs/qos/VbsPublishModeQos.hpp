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

#ifndef INCLUDE_QOS_PUBLISHMODEQOS_HPP_
#define INCLUDE_QOS_PUBLISHMODEQOS_HPP_

#include <utility>
#include <memory>
#include <string>

namespace vbsutil {
namespace xmlparser {
class PublishModeQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsPublishModeQosPolicy = vbsutil::xmlparser::PublishModeQosPolicy;
class VbsPublishModeQos {
 public:
    VbsPublishModeQos();

    VbsPublishModeQos(const VbsPublishModeQos& qos);

    VbsPublishModeQos(VbsPublishModeQos&& qos);

    VbsPublishModeQos& operator=(const VbsPublishModeQos& b);

    VbsPublishModeQos& operator=(VbsPublishModeQos&& b);

    bool operator==(const VbsPublishModeQos& qos) const;

    void clear();

    std::string flowname() const;

    void flowname(const std::string& new_flowname);

    void set_dds_publishmode_qos(const EvbsPublishModeQosPolicy& new_dds_publishmode_qos);

    void get_dds_publishmode_qos(EvbsPublishModeQosPolicy& new_dds_publishmode_qos) const;

 private:
    std::unique_ptr<EvbsPublishModeQosPolicy> dds_publishmode_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_PUBLISHMODEQOS_HPP_
