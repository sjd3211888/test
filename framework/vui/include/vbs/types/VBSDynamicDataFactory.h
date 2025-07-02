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

#ifndef VBS_TYPES_DYNAMIC_DATA_FACTORY_H_
#define VBS_TYPES_DYNAMIC_DATA_FACTORY_H_

#include "vbs/types/VBSDynamicTypeBuilder.h"

namespace vbs {

class VBSDynamicData;

class VBSDynamicDataFactory {
 public:
    /**
     * VBSDynamicDataFactory constructor
     * 
     * @note @li Thread-Safe: No
     *
     */
    VBSDynamicDataFactory();

    /**
     * VBSDynamicDataFactory deconstructor
     * 
     * @note @li Thread-Safe: No
     *
     */
    ~VBSDynamicDataFactory();

    /**
     * Get the instance of VBSDynamicDataFactory.
     *
     * This static method returns the singleton instance of the VBSDynamicDataFactory.
     * It uses lazy initialization, creating the instance upon first request if it is not already created.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization or initialization checks should be managed
     *    if used in a multi-threaded environment.
     *
     * @return VBSDynamicDataFactory* A pointer to the singleton instance of VBSDynamicDataFactory.
     */
    static VBSDynamicDataFactory* get_instance();

    /**
     * Delete the instance of VBSDynamicDataFactory.
     *
     * This static method cleans up the singleton instance of VBSDynamicDataFactory
     * by deleting the previously obtained instance from `get_instance`.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     *
     * @return ReturnCode_t A status code indicating the success or failure of the deletion operation.
     *
     * @warning Ensure that no other operations are being performed on the instance
     *          before calling this function to prevent undefined behavior.
     */
    static ReturnCode_t delete_instance();

    /**
     * Create dynamic data from a specified type.
     *
     * This method creates a new instance of VBSDynamicData based on the provided VBSDynamicType.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @param pType The type specification of the VBSDynamicData to be created.
     * @return VBSDynamicData* A pointer to the newly created VBSDynamicData instance.
     *         If the creation fails, the function may return a null pointer.
     *
     * @warning The caller is responsible for managing the memory of the returned pointer,
     *          ensuring it is properly freed or deleted when no longer needed.
     */
    VBSDynamicData* create_data(VBSDynamicType pType);

    /**
     * Delete dynamic data.
     *
     * This method cleans up and deletes the specified VBSDynamicData instance.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     *
     * @param pData A pointer to the VBSDynamicData instance to be deleted.
     * @return ReturnCode_t A status code indicating the success or failure of the deletion operation.
     *
     * @warning Ensure that the pointer is valid and not being used elsewhere before calling this function
     *          to prevent undefined behavior. After deletion, the pointer should not be used.
     */
    ReturnCode_t delete_data(VBSDynamicData* pData);

    /**
     * Create a copy of dynamic data.
     * 
     * This method creates a deep copy of the specified VBSDynamicData instance.
     * 
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization should be managed if used
     *    in a multi-threaded environment.
     *
     * @param pData A pointer to the VBSDynamicData instance to be copied.
     * @return VBSDynamicData* A pointer to the newly created deep copy of the VBSDynamicData instance.
     *         If the copy operation fails, the function may return a null pointer.
     *
     * @warning Ensure that pData is a valid pointer. The caller is responsible for managing 
     *          the memory of the returned pointer, ensuring it is properly freed or deleted 
     *          when no longer needed.
     */
    VBSDynamicData* create_copy(const VBSDynamicData* pData);

 private:
    EvbsDynamicDataFactory* dynamic_data_factory_;
};

}  // namespace vbs

#endif  // VBS_TYPES_DYNAMIC_DATA_FACTORY_H_
