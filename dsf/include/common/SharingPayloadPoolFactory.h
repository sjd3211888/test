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

#ifndef DSFDDS_SHARING_PAYLAODPOOL_FACTORY_
#define DSFDDS_SHARING_PAYLAODPOOL_FACTORY_

#include <deps/common/Guid.h>
#include <deps/attributes/BaseWriterAttributes.h>
#include <string>
#include <memory>
#include "history/IPayloadPool.h"

namespace li {
namespace dsfdds {

class SharingPayloadFactory {
 public:
    /**
   * @brief added in vbs2.0. create payloadpool based on mbuf, used on the writer side
   * 
   * @param domain_id 
   * @param guid 
   * @param watt 
   * @param topic_name 
   * @return std::shared_ptr<vbs::common::IPayloadPool>. nullptr if create mbuf pool failed
   */
    static std::shared_ptr<vbs::common::IPayloadPool> create(uint32_t domain_id, vbsutil::xmlparser::GUID_t guid,
                                                             vbs::BaseWriterAttributes& watt,
                                                             const std::string& topic_name);

    /**
   * @brief add in vbs2.0. Create a payloadpool only used for release of mbuf, used on the reader side.
   * 
   * @return std::shared_ptr<vbs::common::IPayloadPool> 
   */
    static std::shared_ptr<vbs::common::IPayloadPool> create_empty(vbsutil::xmlparser::GUID_t guid);
};

}  // namespace dsfdds
}  // namespace li

#endif /*DSFDDS_SHARING_PAYLAODPOOL_FACTORY_ */
