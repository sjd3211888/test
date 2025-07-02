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

#ifndef INCLUDE_QOS_DEADLINEQOS_HPP_
#define INCLUDE_QOS_DEADLINEQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class DeadlineQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {

using EvbsDeadlineQosPolicy = vbsutil::xmlparser::DeadlineQosPolicy;
class VbsDeadLineQos {
 public:
    VbsDeadLineQos();

    VbsDeadLineQos(const VbsDeadLineQos& qos);

    VbsDeadLineQos(VbsDeadLineQos&& qos);

    virtual ~VbsDeadLineQos();

    VbsDeadLineQos& operator=(const VbsDeadLineQos& b);

    VbsDeadLineQos& operator=(VbsDeadLineQos&& b);

    bool operator==(const VbsDeadLineQos& qos) const;

    void clear();

    void period(uint64_t nsec);

    void period(int32_t sec, uint32_t nsec);

    uint64_t period() const;

    void set_dds_deadline_qos(const EvbsDeadlineQosPolicy& new_dds_deadline_qos);

    void get_dds_deadline_qos(EvbsDeadlineQosPolicy& new_dds_deadline_qos) const;

 private:
    std::unique_ptr<EvbsDeadlineQosPolicy> dds_deadline_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_DEADLINEQOS_HPP_
