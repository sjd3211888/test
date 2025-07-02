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

#ifndef INCLUDE_QOS_LIFESPANQOS_HPP_
#define INCLUDE_QOS_LIFESPANQOS_HPP_

#include <utility>
#include <memory>

namespace vbsutil {
namespace xmlparser {
class LifespanQosPolicy;
}  // namespace xmlparser
}  // namespace vbsutil

namespace vbs {
using EvbsLifespanQosPolicy = vbsutil::xmlparser::LifespanQosPolicy;
class VbsLifespanQos {
 public:
    VbsLifespanQos();

    VbsLifespanQos(const VbsLifespanQos& qos);

    VbsLifespanQos(VbsLifespanQos&& qos);

    VbsLifespanQos& operator=(const VbsLifespanQos& b);

    VbsLifespanQos& operator=(VbsLifespanQos&& b);

    bool operator==(const VbsLifespanQos& qos) const;

    void clear();

    int64_t duration() const;

    void duration(const int64_t& new_duration);

    void set_dds_lifespan_qos(const EvbsLifespanQosPolicy& new_dds_life_span_qos);

    void get_dds_lifespan_qos(EvbsLifespanQosPolicy& new_dds_life_span_qos) const;

 private:
    std::unique_ptr<EvbsLifespanQosPolicy> dds_life_span_qos_;
};

}  // namespace vbs
#endif  // INCLUDE_QOS_LIFESPANQOS_HPP_
