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

#ifndef INCLUDE_QOS_READERRESOURCELIMITSQOS_HPP_
#define INCLUDE_QOS_READERRESOURCELIMITSQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class ResourceLimitedContainerConfig;
class ReaderResourceLimitsQos;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using ResourceLimitedContainerConfig = vbsutil::xmlparser::ResourceLimitedContainerConfig;
using EvbsReaderResourceLimitsQos = vbsutil::xmlparser::ReaderResourceLimitsQos;

class VbsReaderResourceLimitsQos {
 public:
    VbsReaderResourceLimitsQos();

    VbsReaderResourceLimitsQos(const VbsReaderResourceLimitsQos& qos);

    VbsReaderResourceLimitsQos(VbsReaderResourceLimitsQos&& qos);

    VbsReaderResourceLimitsQos& operator=(const VbsReaderResourceLimitsQos& b);

    VbsReaderResourceLimitsQos& operator=(VbsReaderResourceLimitsQos&& b);

    bool operator==(const VbsReaderResourceLimitsQos& qos) const;

    void clear();

    const ResourceLimitedContainerConfig& matched_publisher_allocation() const;

    const ResourceLimitedContainerConfig& sample_infos_allocation() const;

    const ResourceLimitedContainerConfig& outstanding_reads_allocation() const;

    int32_t max_samples_per_read() const;

    void max_samples_per_read(const int32_t& new_sample);

    void set_dds_reader_resource_limits_qos(const EvbsReaderResourceLimitsQos& new_dds_reader_resource_limits_qos);

    void get_dds_reader_resource_limits_qos(EvbsReaderResourceLimitsQos& new_dds_reader_resource_limits_qos) const;

 private:
    std::unique_ptr<EvbsReaderResourceLimitsQos> dds_reader_resource_limits_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_READERRESOURCELIMITSQOS_HPP_
