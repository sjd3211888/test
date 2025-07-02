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

#ifndef DSFDDS_POOL_CRASH_STRATEGY_H
#define DSFDDS_POOL_CRASH_STRATEGY_H
#include <string>
#include <memory>
#include "dsfdds/discovery/discovery_endpoint.h"
namespace li {
namespace dsfdds {
constexpr std::size_t MAX_MAPPED_MBUF_POOLS_SIZE = 1000U;
class PoolCrashStrategy {
 public:
    explicit PoolCrashStrategy(const std::shared_ptr<DiscoveryEndpoint>& discoveryEndpoint)
        : discovery_endpoint_(discoveryEndpoint) {}
    virtual ~PoolCrashStrategy() {}
    virtual void UpdateCrashedMbufPools(const std::string& name, bool can_destruct = false);
    virtual void UpdateCrashedMbufPools(const uint32_t uuid, const uint64_t pid);

 protected:
    std::shared_ptr<DiscoveryEndpoint> discovery_endpoint_;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_POOL_CRASH_STRATEGY_H
