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

#ifndef INCLUDE_QOS_RESOURCELIMITSQOS_HPP_
#define INCLUDE_QOS_RESOURCELIMITSQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class ResourceLimitsQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsResourceLimitsQosPolicy = vbsutil::xmlparser::ResourceLimitsQosPolicy;
class VbsResourceLimitsQos {
 public:
    VbsResourceLimitsQos();

    VbsResourceLimitsQos(const VbsResourceLimitsQos& qos);

    VbsResourceLimitsQos(VbsResourceLimitsQos&& qos);

    VbsResourceLimitsQos& operator=(const VbsResourceLimitsQos& b);

    VbsResourceLimitsQos& operator=(VbsResourceLimitsQos&& b);

    bool operator==(const VbsResourceLimitsQos& qos) const;

    void clear();

    int32_t max_samples() const;

    void max_samples(const int32_t& new_max_samples);

    int32_t max_instances() const;

    void max_instances(const int32_t& new_max_instances);

    int32_t max_samples_per_instance() const;

    void max_samples_per_instance(const int32_t& new_mmax_samples_per_instance);

    int32_t allocated_samples() const;

    void allocated_samples(const int32_t& new_allocated_samples);

    int32_t extra_samples() const;

    void extra_samples(const int32_t& new_extra_samples);

    void set_dds_resource_limit_qos(const EvbsResourceLimitsQosPolicy& new_dds_resource_limit_qos);

    void get_dds_resource_limit_qos(EvbsResourceLimitsQosPolicy& new_dds_resource_limit_qos) const;

 private:
    std::unique_ptr<EvbsResourceLimitsQosPolicy> dds_resource_limit_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_RESOURCELIMITSQOS_HPP_
