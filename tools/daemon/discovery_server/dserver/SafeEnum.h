// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef _EDDS_GENERATED_SafeEnumHeader_H_
#define _EDDS_GENERATED_SafeEnumHeader_H_

#include <iostream>
#include <type_traits>
#include <cstdint>

namespace vbs {
template <typename def, typename inner = typename def::type>
class safe_enum : public def {
 public:
    using inner_enum = inner;

 private:
    inner_enum val;

 public:
    safe_enum() : val(static_cast<inner_enum>(0)) {}

    safe_enum(inner_enum v) : val(v) {}

    safe_enum(uint32_t v) : val(static_cast<inner_enum>(v)) {}

    inner_enum underlying() const { return val; }

    inner_enum& underlying_ref() { return val; }

    bool operator==(const safe_enum& s) const { return val == s.val; }
    bool operator!=(const safe_enum& s) const { return val != s.val; }
    bool operator<(const safe_enum& s) const { return val < s.val; }
    bool operator<=(const safe_enum& s) const { return val <= s.val; }
    bool operator>(const safe_enum& s) const { return val > s.val; }
    bool operator>=(const safe_enum& s) const { return val >= s.val; }

    friend void swap(safe_enum& left, safe_enum& right) noexcept { std::swap(left.val, right.val); }

    explicit operator uint32_t() const { return static_cast<uint32_t>(val); }
};

template <typename def, typename inner>
std::ostream& operator<<(std::ostream& out, const safe_enum<def, inner>& the_enum) {
    out << the_enum.underlying();
    return out;
}
}  // namespace vbs

#endif  // _EDDS_GENERATED_SafeEnumHeader_H_
