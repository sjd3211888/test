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

#ifndef DSFDDS_COMMON_PROPERTY_PROPERTPARSE_H_
#define DSFDDS_COMMON_PROPERTY_PROPERTPARSE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <DsfLog.hpp>
#include "common/all_common.h"
#include "common/QosProperties.hpp"
#include "common/types/TypesBase.h"

namespace li {
namespace dsfdds {

struct MbufPropertyField {
    static inline const std::string MaxAllocation = "dsf.mbuf.max_allocation";
    static inline const std::string PoolSize = "dsf.mbuf.pool_size";
    static inline const std::string AlignAddress = "dsf.mbuf.align_address";
    static inline const std::string AllocateMode = "dsf.mbuf.allocate_mode";
    static inline const std::string PoolSizeUpLimit = "dsf.mbuf.pool_size_up_limit";
    static inline const std::string AllocSizeLimit = "dsf.mbuf.alloc_size_limit";
    static inline const std::string BufCountUpLimit = "dsf.mbuf.buf_count_up_limit";
    static inline const std::string PoolSizeUsageScaleDownThld = "dsf.mbuf.pool_size_usage_scale_down_thld";
    static inline const std::string LowUsageDurationForScaleDown = "dsf.mbuf.low_usage_duration_for_scale_down";
    static inline const std::string AllocSizeSteadyTol = "dsf.mbuf.alloc_size_steady_tol";
    static inline const std::string AllocSteadyDurationForScaleDown = "dsf.mbuf.alloc_steady_duration_for_scale_down";
    static inline const std::string ScaleDownTargetUsage = "dsf.mbuf.scale_down_target_usage";
    static inline const std::string QueryStatusInterval = "dsf.mbuf.query_status_interval";
    static inline const uint32_t DefaultMaxAllocation = 100;
    static inline const uint32_t DefaultPoolSize = 10240;
    static inline const size_t DefaultAlignAddress = 64;
    static inline const MemAllocateMode DefaultAllocateMode = DYNAMIC;
    static inline const uint32_t DefaultPoolSizeUpLimit = 128 * 1024 * 1024;  // 128MB
    static inline const uint32_t DefaultAllocSizeLimit = 64 * 1024 * 1024;    // 64MB
    static inline const uint32_t DefaultBufCountUpLimit = 200;
    static inline const double DefaultPoolSizeUsageScaleDownThld = 0.3;
    static inline const uint32_t DefaultLowUsageDurationForScaleDown = 50;
    static inline const double DefaultAllocSizeSteadyTol = 0.2;
    static inline const uint32_t DefaultAllocSteadyDurationForScaleDown = 50;
    static inline const double DefaultScaleDownTargetUsage = 0.5;
    static inline const uint32_t DefaultQueryStatusInterval = 10;
};

static std::unordered_map<std::string, uint32_t> MbufPropertyDefaultValue {
    {MbufPropertyField::MaxAllocation, MbufPropertyField::DefaultMaxAllocation},
    {MbufPropertyField::PoolSize, MbufPropertyField::DefaultPoolSize},
    {MbufPropertyField::AlignAddress, MbufPropertyField::DefaultAlignAddress},
    {MbufPropertyField::AllocateMode, MbufPropertyField::DefaultAllocateMode},
    {MbufPropertyField::PoolSizeUpLimit, MbufPropertyField::DefaultPoolSizeUpLimit},
    {MbufPropertyField::AllocSizeLimit, MbufPropertyField::DefaultAllocSizeLimit},
    {MbufPropertyField::BufCountUpLimit, MbufPropertyField::DefaultBufCountUpLimit},
    {MbufPropertyField::LowUsageDurationForScaleDown, MbufPropertyField::DefaultLowUsageDurationForScaleDown},
    {MbufPropertyField::AllocSteadyDurationForScaleDown, MbufPropertyField::DefaultAllocSteadyDurationForScaleDown},
    {MbufPropertyField::QueryStatusInterval, MbufPropertyField::DefaultQueryStatusInterval},
};

static std::unordered_map<std::string, double> MbufPropertyDefaultDoubleValue {
    {MbufPropertyField::PoolSizeUsageScaleDownThld, MbufPropertyField::DefaultPoolSizeUsageScaleDownThld},
    {MbufPropertyField::AllocSizeSteadyTol, MbufPropertyField::DefaultAllocSizeSteadyTol},
    {MbufPropertyField::ScaleDownTargetUsage, MbufPropertyField::DefaultScaleDownTargetUsage},
};

struct QueuePropertyField {
    static inline const std::string WakeUpMode = "dsf.queue.wake_up_mode";
    static inline const std::string QueueSize = "dsf.queue.size";
    static inline const QueueWakeUp DefaultWakeUpMode = PTHREAD_CONDITION_WAKE_UP;
    static inline const uint32_t DefaultQueueSize = 64;
};

static std::unordered_map<std::string, uint32_t> QueuePropertyDefaultValue {
    {QueuePropertyField::WakeUpMode, QueuePropertyField::DefaultWakeUpMode},
    {QueuePropertyField::QueueSize, QueuePropertyField::DefaultQueueSize}};

struct ReaderPropertyFiled {
    static inline const std::string FilterExpression = "dsf.reader.filter_expression";
    static inline const std::string FilterParameters = "dsf.reader.filter_parameters";
};

#ifdef ENABLE_DES
struct DesPropertyField {
    static inline const std::string ResGroupID = "dsf.des.resource_group_id";
    static inline const std::string DeadlineTime = "dsf.des.deadline_time";
    static inline const std::string RunTime = "dsf.des.run_time";
    static inline const std::string SchedulePeriod = "dsf.des.schedule_period";
    static inline const uint32_t DefaultResGroupID = 0U;
    static inline const uint32_t DefaultDeadlineTime = 0U;
    static inline const uint32_t DefaultRunTime = 0U;
    static inline const uint32_t DefaultSchedulePeriod = 0U;
};

static std::unordered_map<std::string, uint32_t> DesPropertyDefaultValue {
    {DesPropertyField::ResGroupID, DesPropertyField::DefaultResGroupID},
    {DesPropertyField::DeadlineTime, DesPropertyField::DefaultDeadlineTime},
    {DesPropertyField::RunTime, DesPropertyField::DefaultRunTime},
    {DesPropertyField::SchedulePeriod, DesPropertyField::DefaultSchedulePeriod}};
#endif
class PropertyParse {
 private:
    static uint32_t GetValueFromStr(const std::string* p_value,
                                    const std::unordered_map<std::string, uint32_t>& default_map,
                                    const std::string& field);

    static double GetValueFromStr(const std::string* p_value,
                                  const std::unordered_map<std::string, double>& default_map, const std::string& field);

 public:
    inline static QueueWakeUp GetQueueWakeUp(const std::string* p_value) {
        if (p_value == nullptr) {
            DSF_LOG_DEBUG(PropertyParse, QueuePropertyField::WakeUpMode
                                             << " p_value is nullptr,use default wake up mode:"
                                             << QueuePropertyField::DefaultWakeUpMode);
            return QueuePropertyField::DefaultWakeUpMode;
        }
        if (*p_value == "PTHREAD_CONDITION_WAKE_UP") {
            return PTHREAD_CONDITION_WAKE_UP;
        }
        if (*p_value == "DTS_WAKE_UP") {
            return QueueWakeUp::DTS_WAKE_UP;
        }
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
            QueuePropertyField::WakeUpMode << " can not parse property name:" << *p_value << ",configuration error");
        return INVALID_WAKE_UP;
    }

    inline static uint32_t GetQueueSize(const std::string* p_value) {
        return GetValueFromStr(p_value, QueuePropertyDefaultValue, QueuePropertyField::QueueSize);
    }

    inline static uint32_t GetMaxAllocation(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::MaxAllocation);
    }

    inline static uint32_t GetPoolSize(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::PoolSize);
    }

    inline static MemAllocateMode GetAllocateMode(const std::string* p_value) {
        if (p_value == nullptr) {
            DSF_LOG_DEBUG(PropertyParse, MbufPropertyField::AllocateMode
                                             << " p_value is nullptr,use default allocate mode："
                                             << MbufPropertyField::DefaultAllocateMode);
            return MbufPropertyField::DefaultAllocateMode;
        }
        if (*p_value == "FIXED") {
            return MemAllocateMode::FIXED;
        }
        if (*p_value == "DYNAMIC") {
            return MemAllocateMode::DYNAMIC;
        }
        if (*p_value == "MULTIREADER") {
            return MemAllocateMode::MULTIREADER;
        }
        DSF_LOG_ERROR(
            DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_QOS_CHECK_ERR,
            MbufPropertyField::AllocateMode << " can not parse property name:" << *p_value << ",configuration error");
        return MemAllocateMode::INVALID_ALLOCATE_MODE;
    }

    inline static uint32_t GetAlignAddress(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::AlignAddress);
    }

    inline static uint32_t GetPoolSizeUpLimit(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::PoolSizeUpLimit);
    }

    inline static uint32_t GetAllocSizeLimit(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::AllocSizeLimit);
    }

    inline static uint32_t GetBufCountUpLimit(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::BufCountUpLimit);
    }

    inline static double GetPoolSizeUsageScaleDownThld(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultDoubleValue, MbufPropertyField::PoolSizeUsageScaleDownThld);
    }

    inline static uint32_t GetLowUsageDurationForScaleDown(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::LowUsageDurationForScaleDown);
    }

    inline static double GetAllocSizeSteadyTol(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultDoubleValue, MbufPropertyField::AllocSizeSteadyTol);
    }

    inline static uint32_t GetAllocSteadyDurationForScaleDown(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::AllocSteadyDurationForScaleDown);
    }

    inline static double GetScaleDownTargetUsage(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultDoubleValue, MbufPropertyField::ScaleDownTargetUsage);
    }

    inline static double GetQueryStatusInterval(const std::string* p_value) {
        return GetValueFromStr(p_value, MbufPropertyDefaultValue, MbufPropertyField::QueryStatusInterval);
    }

#ifdef ENABLE_DES
    // Des property
    inline static uint32_t GetResGroupID(const std::string* p_value) {
        return GetValueFromStr(p_value, DesPropertyDefaultValue, DesPropertyField::ResGroupID);
    }
    inline static uint32_t GetDeadlineTime(const std::string* p_value) {
        return GetValueFromStr(p_value, DesPropertyDefaultValue, DesPropertyField::DeadlineTime);
    }
    inline static uint32_t GetRunTime(const std::string* p_value) {
        return GetValueFromStr(p_value, DesPropertyDefaultValue, DesPropertyField::RunTime);
    }
    inline static uint32_t GetSchedulePeriod(const std::string* p_value) {
        return GetValueFromStr(p_value, DesPropertyDefaultValue, DesPropertyField::SchedulePeriod);
    }
#endif
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_COMMON_PROPERTY_PROPERTPARSE_H_
