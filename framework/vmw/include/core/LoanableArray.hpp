// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef INCLUDE_EDDS_DDS_CORE_LOANABLEARRAY_HPP_
#define INCLUDE_EDDS_DDS_CORE_LOANABLEARRAY_HPP_

#include <cstdint>
#include <array>

namespace vbs {

/**
 * A type-safe, ordered collection of elements allocated on the stack, which can be loaned to a
 * @ref LoanableCollection.
 */
template <typename T, std::size_t num_items>
struct LoanableArray : public std::array<T, num_items> {
    LoanableArray() {
        for (std::size_t n = 0U; n < num_items; ++n) {
            buffer_[n] = &((*this)[n]);
        }
    }

    // Non-copyable
    LoanableArray(const LoanableArray&) = delete;
    LoanableArray& operator=(const LoanableArray&) = delete;

    // Non-moveable
    LoanableArray(LoanableArray&&) = delete;
    LoanableArray& operator=(LoanableArray&&) = delete;

    /**
     * Get a buffer pointer that could be used on @ref LoanableCollection::loan.
     *
     * @return buffer pointer for loans.
     */
    void** buffer_for_loans() const { return (void**)buffer_; }

 private:
    void* buffer_[num_items];
};

}  // namespace vbs

#endif  // INCLUDE_EDDS_DDS_CORE_LOANABLEARRAY_HPP_
