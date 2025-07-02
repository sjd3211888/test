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

#ifndef VBS_LOANEDDYNAMICDATA_H_
#define VBS_LOANEDDYNAMICDATA_H_

#include "vbs/types/VBSDynamicData.h"

namespace vbs {

class LoanedDynamicData {
    friend class VBSDynamicData;

 public:
    /**
     * @brief Destructor that returns the loan if not explicitly returned.
     * 
     * @note Thread-Safety: This destructor is not thread-safe. Ensure that no other threads
     *       are accessing the object when it is being destroyed.
     * @note Lock-Free: This destructor does not operate in a lock-free manner.
     */
    ~LoanedDynamicData();

    /**
     * @brief Explicitly returns the loan to the VBSDynamicData.
     * 
     * This function should be called to return the loan when the data is no longer needed.
     */
    void return_loan();

    /**
     * @brief Obtains the mutable loaned VBSDynamicData object.
     * 
     * @return A reference to the loaned VBSDynamicData object.
     */
    VBSDynamicData& get();

    /**
     * @brief Obtains the immutable loaned VBSDynamicData object.
     * 
     * @return A const reference to the loaned VBSDynamicData object.
     */
    const VBSDynamicData& get() const;

    /**
     * @brief Conversion to VBSDynamicData for convenience.
     * 
     * @return A reference to the loaned VBSDynamicData.
     */
    operator VBSDynamicData&();

    /**
     * @brief Conversion to VBSDynamicData for convenience.
     * 
     * @return A const reference to the loaned VBSDynamicData.
     */
    operator const VBSDynamicData&() const;

    /**
     * @brief Move constructor for LoanedDynamicData.
     * 
     * Allows transferring the management of resources from one instance to another.
     * 
     * @param other Another LoanedDynamicData instance to be moved from.
     */
    LoanedDynamicData(LoanedDynamicData&& other);

    /**
     * @brief Move assignment operator for LoanedDynamicData.
     * 
     * Allows reusing a LoanedDynamicData instance by transferring resources safely.
     * 
     * @param other Another LoanedDynamicData instance to assign from.
     * @return A reference to the updated LoanedDynamicData instance.
     */
    LoanedDynamicData& operator=(LoanedDynamicData&& other);

 private:
    /**
     * @brief Constructs a LoanedDynamicData with the given data pointers.
     * 
     * This is a private constructor used by the VBSDynamicData class to manage data loans.
     * 
     * @param data A pointer to the loaned VBSDynamicData object.
     * @param parent_data A pointer to the parent VBSDynamicData object.
     */
    LoanedDynamicData(VBSDynamicData* data, VBSDynamicData* parent_data);
    VBSDynamicData* data_ = nullptr;
    VBSDynamicData* parent_data_ = nullptr;
};

}  // namespace vbs

#endif  // VBS_LOANEDDYNAMICDATA_H_
