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

#ifndef INCLUDE_CORE_MACROS_H_
#define INCLUDE_CORE_MACROS_H_

#include <iostream>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#undef DISALLOW_COPY_AND_ASSIGN

#define DISALLOW_COPY_AND_ASSIGN(classname) \
    classname(const classname&) = delete;   \
    classname& operator=(const classname&) = delete;

#define DECLARE_SINGLETON(classname) \
 public:                             \
    static classname* Instance() {   \
        static classname instance;   \
        return &instance;            \
    }                                \
                                     \
 private:                            \
    classname();                     \
    DISALLOW_COPY_AND_ASSIGN(classname)

#define DEFINE_TYPE_TRAIT(name, func)                                    \
    template <typename T>                                                \
    class name {                                                         \
     private:                                                            \
        template <typename Class>                                        \
        static std::true_type Test(decltype(&Class::func)*);             \
        template <typename>                                              \
        static std::false_type Test(...);                                \
                                                                         \
     public:                                                             \
        static constexpr bool value = decltype(Test<T>(nullptr))::value; \
    };                                                                   \
    template <typename T>                                                \
    constexpr bool name<T>::value;

#endif  // INCLUDE_CORE_MACROS_H_
