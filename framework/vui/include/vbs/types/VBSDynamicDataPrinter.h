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

#ifndef VBS_VBSDYNAMICDATAPRINTER_H_
#define VBS_VBSDYNAMICDATAPRINTER_H_

#include <iostream>
#include <sstream>

#include "deps/type/DynamicDataPrinter.hpp"
#include "deps/type/DynamicDataHelper.hpp"

namespace vbs {
class VBSDynamicData;
using vbs::common::types::DynamicDataHelper;
using vbs::common::types::DynamicDataPrinter;
class VBSDynamicDataPrinter {
 public:
    /**
     * VBSDynamicDataPrinter constructor
     *
     * @note @li Thread-Safe: Yes
     *
     */
    VBSDynamicDataPrinter();

    /**
     * VBSDynamicDataPrinter deconstructor
     *
     * @note @li Thread-Safe: Yes
     */
    ~VBSDynamicDataPrinter();

    /**
     * Print dynamic data to an output string stream.
     *
     * This method outputs all members and their values of the specified VBSDynamicData
     * instance to an `ostringstream`.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     * @li Lock-Free: No. This operation may involve internal locking mechanisms.
     *
     * @param out An `ostringstream` where the dynamic data will be printed.
     * @param data A pointer to the VBSDynamicData instance containing the data to print.
     * @param enum_mode Specifies the mode of enumeration. Default is 0. Include additional
     *                      descriptions about what different enum_mode values mean, if applicable.
     *
     * @warning Ensure that the `data` pointer is valid. The function assumes the `ostringstream`
     *          is properly initialized and ready for input.
     */
    void print_to_sstream(std::ostringstream& out, const VBSDynamicData* data, int enum_mode = 0);

    /**
     * Use DynamicDataHelper to print dynamic data to standard output.
     *
     * This static method prints a simplified view of all members and their values
     * of the specified VBSDynamicData instance to `cout`.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     * @li Lock-Free: No. This operation may involve internal locking mechanisms.
     *
     * @param data A pointer to the VBSDynamicData instance containing the data to print.
     *
     * @warning Ensure that the `data` pointer is valid before calling this function.
     *          The function outputs to `cout`, so consider redirecting if needed.
     */
    static void print_with_helper(const VBSDynamicData* data);

    /**
     * Get the instance of DynamicDataPrinter.
     *
     * This method returns a pointer to the initialized DynamicDataPrinter object.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     * @li Lock-Free: No. This operation may involve internal locking mechanisms.
     *
     * @return DynamicDataPrinter* A pointer to the initialized DynamicDataPrinter instance.
     *
     * @warning Ensure to handle the returned pointer responsibly and avoid concurrent
     *          access unless proper synchronization is provided.
     */
    DynamicDataPrinter* get_instance();

 private:
    DynamicDataPrinter* instance;
    static DynamicDataHelper helper;
};

}  // namespace vbs

#endif
