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

#ifndef INCLUDE_QOS_WRITERRESOURCELIMITSQOS_HPP_
#define INCLUDE_QOS_WRITERRESOURCELIMITSQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class ResourceLimitedContainerConfig;
class WriterResourceLimitsQos;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsResourceLimitedContainerConfig = vbsutil::xmlparser::ResourceLimitedContainerConfig;
using EvbsWriterResourceLimitsQos = vbsutil::xmlparser::WriterResourceLimitsQos;

class VbsWriterResourceLimitsQos {
 public:
    VbsWriterResourceLimitsQos();

    VbsWriterResourceLimitsQos(const VbsWriterResourceLimitsQos& qos);

    VbsWriterResourceLimitsQos(VbsWriterResourceLimitsQos&& qos);

    VbsWriterResourceLimitsQos& operator=(const VbsWriterResourceLimitsQos& b);

    VbsWriterResourceLimitsQos& operator=(VbsWriterResourceLimitsQos&& b);

    bool operator==(const VbsWriterResourceLimitsQos& qos) const;

    void clear();

    const EvbsResourceLimitedContainerConfig& matched_subscriber_allocation() const;

    const EvbsResourceLimitedContainerConfig& reader_filters_allocation() const;

    void set_dds_writer_resource_limits_qos(const EvbsWriterResourceLimitsQos& new_dds_writer_resource_limits_qos);

    void get_dds_writer_resource_limits_qos(EvbsWriterResourceLimitsQos& new_dds_writer_resource_limits_qos) const;

 private:
    std::unique_ptr<EvbsWriterResourceLimitsQos> dds_writer_resource_limits_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_WRITERRESOURCELIMITSQOS_HPP_
