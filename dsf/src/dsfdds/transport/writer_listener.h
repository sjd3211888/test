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

#ifndef DSFDDS_TRANSPORT_WRITER_LISTENER_H_
#define DSFDDS_TRANSPORT_WRITER_LISTENER_H_

#include "dsfdds/transport/shm_writer.h"

namespace li {
namespace dsfdds {

/**
* Class ReaderListener, to be used by the user to override some of is virtual method to program actions to
* certain events.
* @ingroup READER_MODULE
*/
class WriterListener {
 public:
    WriterListener() = default;

    virtual ~WriterListener() = default;
};

}  // namespace dsfdds
}  // namespace li

#endif  // DSFDDS_TRANSPORT_WRITER_LISTENER_H_
