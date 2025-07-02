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

#include "reader_proxy.h"

namespace li {
namespace dsfdds {

bool ReaderProxy::CheckSetQueueParam(uint32_t queue_id) const {
    if (msg_queue_ != nullptr) {
        DSF_LOG_INFO(READERPROXY, "msg_queue_ already have queue name : " << msg_queue_->get_name());
        return true;
    }
    if (topic_name_.empty()) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER, "topic name is empty!!!");
        return false;
    }
    if ((!is_static_discovery_) && (pid_ == 0)) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER, "pid is 0!!!");
        return false;
    }
    if (queue_id == 0U) {
        DSF_LOG_ERROR(DSF_PUBSUB, ReturnCode_t::RETCODE_PUBSUB_BAD_PARAMETER,
                      "remote queue id:" << queue_id << " is 0");
        return false;
    }
    return true;
}

bool ReaderProxy::is_last_send_failed() const {
    return last_send_failed_;
}
}  // namespace dsfdds
}  // namespace li
