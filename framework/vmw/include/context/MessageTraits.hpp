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

#ifndef INCLUDE_CONTEXT_MESSAGE_TRAITS_H_
#define INCLUDE_CONTEXT_MESSAGE_TRAITS_H_

#include <memory>
#include <string>

#include "core/Macros.h"

namespace vbs {
DEFINE_TYPE_TRAIT(HasSetQos, SetQos)
DEFINE_TYPE_TRAIT(HasGetQos, GetQos)

template <typename T, typename Q>
typename std::enable_if<HasSetQos<T>::value, bool>::type SetQos(T& t, const Q& q) {
    return t.SetQos(q);
}

template <typename T, typename Q>
typename std::enable_if<!HasSetQos<T>::value, bool>::type SetQos(T& t, const Q& q) {
    return false;
}

template <typename T, typename Q>
typename std::enable_if<HasGetQos<T>::value, bool>::type GetQos(T& t, Q& q) {
    return t.GetQos(q);
}

template <typename T, typename Q>
typename std::enable_if<!HasGetQos<T>::value, bool>::type GetQos(T& t, Q& q) {
    return false;
}

}  // namespace vbs

#endif  // INCLUDE_CONTEXT_NEXUS_UNITS_H_
