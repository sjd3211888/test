// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: adapt dispatcher for vbs2.0
// ------------------------------------------------------------------

#ifndef DSFDDS_UTILS_QOS_CONVERTERS_HPP_
#define DSFDDS_UTILS_QOS_CONVERTERS_HPP_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC

#include <dsfdds/dds/publisher/qos/DataWriterQos.hpp>
#include <dsfdds/dds/subscriber/qos/DataReaderQos.hpp>
#include "common/all_common.h"
#include "edds/rtps/attributes/WriterAttributes.h"
#include "edds/rtps/attributes/ReaderAttributes.h"

namespace li {
namespace dsfdds {
namespace utils {

/**
 * Obtains the DataWriterQos from the PublisherAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(DataWriterQos& qos, const vbs::BaseWriterAttributes& attr);

/**
 * Obtains the DataWriterQos from the PublisherAttributes provided.
 *
 * @param[out] qos Pointer to the QoS to write on
 * @param[in] attr Pointer to the attributes from which to obtain data
 */
void set_qos_from_attributes(DataReaderQos& qos, const vbs::BaseReaderAttributes& attr);

} /* namespace utils */
} /* namespace dsfdds */
} /* namespace li */
#endif  // ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#endif  /* DSFDDS_UTILS_QOS_CONVERTERS_HPP_ */
