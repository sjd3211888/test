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

#ifndef VBS_VBSDYNAMICDATA_H_
#define VBS_VBSDYNAMICDATA_H_

#include <vector>
#include <stack>
#include <string>

#include "vbs/Global.hpp"
#include "vbs/types/VBSDynamicType.h"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/VBSDynamicDataPrinter.h"
#include "vbs/types/VBSDynamicDataFactory.h"

namespace vbs {

class LoanedDynamicData;

class VBSDynamicData {
    friend class DataReaderImpl;
    friend class DataWriterImpl;
    friend class VBSDynamicDataFactory;
    friend class Topic;
    friend class VBSDynamicDataPrinter;

 public:
    /**
     * VBSDynamicData constructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    VBSDynamicData();

    /**
     * VBSDynamicData constructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type_name the name of data type
     */
    VBSDynamicData(const std::string& type_name);

    /**
     * VBSDynamicData constructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type_name the name of data type
     * @param data the data has been serialized
     * @param len the length of data
     */
    VBSDynamicData(const std::string& type_name, void* data, const uint32_t& len);

    /**
     * VBSDynamicData copy constructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param data the data you want to copy
     */
    VBSDynamicData(const VBSDynamicData& data);

    /**
     * VBSDynamicData copy constructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param data the right value data you want to copy
     */
    VBSDynamicData(VBSDynamicData&& data);

    /**
     * Overload the assignment operator of VBSDynamicData
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param data the data you want to copy
     */
    VBSDynamicData& operator=(const VBSDynamicData& data);

    /**
     * Overload the assignment operator of VBSDynamicData
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param data the right value data you want to copy
     */
    VBSDynamicData& operator=(VBSDynamicData&& data);

    /**
     * VBSDynamicData deconstructor
     *
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    ~VBSDynamicData();

    /**
     * dynamic data is valid
     *
     * @note @li Thread-Safe: Yes
     *
     * @return true is valid, else is false
     */
    bool is_valid() const;

    /**
     * @brief Clears all data in the dynamic data object.
     *
     * This function removes all members from the dynamic data object,
     * effectively resetting it to its initial state.
     *
     * @note Thread-Safe: Yes. This operation can be safely called from multiple threads.
     *
     * @return Returns a status code indicating success or failure.
     */
    ReturnCode_t clear_all_values();

    /**
     * @brief Clears all non-key values in a map-like dynamic data structure.
     *
     * This function removes or resets all values associated with keys in the
     * dynamic data structure, while preserving the keys themselves.
     *
     * @note Thread-Safety: No. Ensure that this function is not called concurrently from multiple threads
     *       as it may lead to undefined behavior.
     * @note Lock-Free: No. This function may involve operations that are not lock-free.
     *
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     */
    ReturnCode_t clear_nonkey_values();

    /**
     * @brief Clears the data associated with the specified member ID in the dynamic data object.
     *
     * This function removes or resets the value of a specific member identified by the given ID
     * within the dynamic data structure.
     *
     * @note Thread-Safety: No. Ensure that this function is not called concurrently from multiple threads
     *       as it may lead to undefined behavior.
     * @note Lock-Free: No. This function may involve operations that are not lock-free.
     *
     * @param id The ID of the member whose data is to be cleared.
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     */
    ReturnCode_t clear_value(uint32_t id);

    /**
     * @brief Checks whether the current dynamic data is equal to another dynamic data object.
     *
     * This function compares the contents of the current dynamic data object with another
     * specified by the given pointer.
     *
     * @note Thread-Safety: Yes. This operation is safe to call from multiple threads concurrently.
     *
     * @param other A pointer to the VBSDynamicData object to compare with.
     * @return True if both dynamic data objects are equal, false otherwise.
     */
    bool equals(const VBSDynamicData* other) const;

    /**
     * @brief Retrieves the type kind of the current dynamic data object.
     *
     * This function returns the TypeKind, which represents the type classification
     * of the dynamic data object (e.g., int, float, struct, etc.).
     *
     * @note Thread-Safety: Yes. This operation is safe to call concurrently from multiple threads.
     *
     * @return TypeKind The kind of type associated with the dynamic data object.
     */
    TypeKind get_kind() const;

    /**
     * Retrieve the total number of elements.
     *
     * This method is used to determine the count of elements in a dynamic data structure.
     *
     * Example Usage:
     * - For complex types such as sequences or maps, it returns the actual number of member elements.
     * - For basic data types (such as integers, floats, etc.), it returns 1, as the count reflects 
     *   the object itself.
     *
     * @note
     * @li Thread-Safe: Yes. This function is designed to be safely called from multiple threads 
     *    without causing data races or inconsistencies.
     *
     * @return uint32_t The number of elements contained within the object. For collections, 
     *         this represents the count of items. For singular data items, it returns 1.
     */
    uint32_t get_item_count() const;

    /**
     * Retrieve the member ID using the member's name.
     *
     * This method fetches the unique identifier (ID) of a data member
     * by providing its name.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe to call simultaneously from
     *    multiple threads. Synchronization must be managed externally if used in
     *    a multi-threaded environment.
     * @li Lock-Free: No. The operation may involve locking mechanisms internally;
     *    hence, it might be blocking.
     *
     * @param name The name of the element for which the ID is to be retrieved.
     * @return uint32_t The ID of the member corresponding to the provided name.
     *         If the name does not exist, the behavior is dependent on implementation
     *         (e.g., may return a special value or throw an exception).
     */
    uint32_t get_member_id_by_name(const std::string& name) const;

    /**
     * Retrieve the member ID by the index.
     *
     * This method fetches the unique identifier (ID) of a data member
     * using its positional index within a collection.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe to call simultaneously from
     *    multiple threads. Synchronization must be managed externally if used in
     *    a multi-threaded environment.
     * @li Lock-Free: No. The operation may involve locking mechanisms internally,
     *    hence it might be blocking.
     * @li The index can be obtained from methods like @link get_array_index @endlink
     *    or @link get_member_id_by_name @endlink.
     *
     * @param index The positional index of the element.
     * @return uint32_t The ID of the member at the specified index. If the index is
     *         out of range, the behavior is dependent on implementation (e.g., may
     *         return a special value or throw an exception).
     */
    uint32_t get_member_id_at_index(const uint32_t& index) const;

    /**
     * Loan dynamic data by the member ID.
     *
     * This method provides access to dynamic data associated with a specified member ID.
     * The lifecycle of the returned pointer is managed via the `return_loaned_value` function.
     *
     * @note
     * @li Thread-Safe: No. This function should not be called simultaneously from
     *    multiple threads. Synchronization must be handled externally in a
     *    multi-threaded environment.
     * @li Lock-Free: No. This operation may involve internal locking mechanisms,
     *    which can be blocking.
     *
     * @param id The member ID for which to retrieve the dynamic data.
     * @return VBSDynamicData* A pointer to the dynamic data associated with the given ID.
     *         If the ID does not exist, the behavior is implementation-defined
     *         (e.g., may return a null pointer).
     *
     * @warning Use `ReturnCode_t return_loaned_value(VBSDynamicData* value)` to manage the lifecycle
     *          of the returned pointer.
     */
    VBSDynamicData* loan_value(uint32_t id) const;

    /**
     * Loan a LoanedDynamicData object by the member ID.
     *
     * This method retrieves a `LoanedDynamicData` object associated with the specified member ID.
     * The `LoanedDynamicData` class ensures that the lifecycle of the loaned data is managed correctly.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe to call simultaneously from
     *    multiple threads. Synchronization must be managed externally if used in
     *    a multi-threaded environment.
     * @li Lock-Free: No. The operation may involve locking mechanisms internally;
     *    hence it might be blocking.
     *
     * @param id The member ID for which to obtain the `LoanedDynamicData` object.
     * @return LoanedDynamicData An object encapsulating the loaned dynamic data.
     *         If the ID does not exist, the behavior is dependent on implementation
     *         (e.g., may throw an exception or handle internally).
     *
     * @details
     * The returned `LoanedDynamicData` object provides methods to access, manipulate,
     * and eventually return the underlying `VBSDynamicData`. The destructor of `LoanedDynamicData`
     * will automatically return the loan if `return_loan()` has not been explicitly called.
     *
     * Usage:
     * - Use `get()` to access the mutable dynamic data.
     * - Use `return_loan()` to manually manage the loan lifecycle.
     * - Leverage move semantics to efficiently transfer ownership.
     */
    LoanedDynamicData loan_value_obj(uint32_t id) const;

    /**
     * Loan dynamic data by the member name.
     *
     * This method provides access to dynamic data associated with a specified member name.
     * The lifecycle of the returned pointer is managed via the `return_loaned_value` function.
     *
     * @note
     * @li Thread-Safe: No. This function should not be called simultaneously from
     *    multiple threads. Synchronization must be handled externally in a
     *    multi-threaded environment.
     * @li Lock-Free: No. This operation may involve internal locking mechanisms,
     *    which can be blocking.
     *
     * @param name The member name for which to retrieve the dynamic data.
     * @return VBSDynamicData* A pointer to the dynamic data associated with the given name.
     *         If the name does not exist, the behavior is implementation-defined
     *         (e.g., may return a null pointer).
     *
     * @warning Use `ReturnCode_t return_loaned_value(VBSDynamicData* value)` to manage the lifecycle
     *          of the returned pointer.
     */
    VBSDynamicData* loan_value(const std::string& name) const;

    /**
     * Loan a LoanedDynamicData object by the member name.
     *
     * This method retrieves a `LoanedDynamicData` object associated with the specified member name.
     * The `LoanedDynamicData` class ensures that the lifecycle of the loaned data is managed correctly.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe to call simultaneously from
     *    multiple threads. Synchronization must be managed externally if used in
     *    a multi-threaded environment.
     * @li Lock-Free: No. The operation may involve locking mechanisms internally;
     *    hence it might be blocking.
     *
     * @param name The member name for which to obtain the `LoanedDynamicData` object.
     * @return LoanedDynamicData An object encapsulating the loaned dynamic data.
     *         If the name does not exist, the behavior is dependent on implementation
     *         (e.g., may throw an exception or handle internally).
     *
     * @details
     * The returned `LoanedDynamicData` object provides methods to access, manipulate,
     * and eventually return the underlying `VBSDynamicData`. The destructor of `LoanedDynamicData`
     * will automatically return the loan if `return_loan()` has not been explicitly called.
     *
     * Usage:
     * - Use `get()` to access the mutable dynamic data.
     * - Use `return_loan()` to manually manage the loan lifecycle.
     * - Leverage move semantics to efficiently transfer ownership.
     */
    LoanedDynamicData loan_value_obj(const std::string& name) const;

    /**
     * Return the loaned dynamic data.
     * 
     * This method is used to return a dynamic data pointer that was previously loaned
     * via `loan_value`. It is essential to call this function to release resources.
     *
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from multiple threads.
     *    Synchronization must be managed externally if used in a multi-threaded environment.
     * @li The `value` parameter must be a pointer obtained from the `loan_value` method.
     *
     * @param value A pointer to the dynamic data to be returned.
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     *
     * @warning Ensure that only pointers obtained through `loan_value` are passed to this
     *          function to avoid undefined behavior.
     */
    ReturnCode_t return_loaned_value(VBSDynamicData* value);

    /**
     * Get the index within an array using a position vector.
     *
     * This method calculates the index of a dynamic data element within an array
     * based on a specified position vector. It is intended for use solely with
     * array-type dynamic data.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     * @li The returned index can be used with `set_xxxx_value` functions for setting values.
     *
     * @param position A vector representing the position within the array. For example,
     *        in a two-dimensional array, the vector should have a size of 2, representing
     *        the row and column.
     * @return uint32_t The calculated index corresponding to the specified position.
     *
     * @warning This function should only be used with array-type dynamic data.
     */
    uint32_t get_array_index(const std::vector<uint32_t>& position);

    /**
     * Insert an element into a sequence.
     *
     * This method inserts a new element into a sequence-type dynamic data and
     * returns the ID of the newly added element.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     * @li This function is intended exclusively for sequence-type dynamic data.
     *
     * @param outId A reference to a variable where the new element's ID will be stored.
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     *
     * @warning Ensure that the dynamic data is of sequence type before using this function.
     */
    ReturnCode_t insert_sequence_data(uint32_t& outId);

    /**
     * insert a int32 data into dyanmic-data
     *
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value int32_t data
     * @return return status code
     */
    ReturnCode_t insert_int32_value(int32_t value, uint32_t& outId);

    /**
     * insert a uint32 data into dyanmic-data
     *
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value uint32_t data
     * @return return status code
     */
    ReturnCode_t insert_uint32_value(uint32_t value, uint32_t& outId);

    /**
     * insert a int16 data into dyanmic-data
     *
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value int16_t data
     * @return return status code
     */
    ReturnCode_t insert_int16_value(int16_t value, uint32_t& outId);

    /**
     * insert a int8 data into dyanmic-data
     *
     * @param outId elements's member-id
     * @param value int8_t data
     * @return return status code
     */
    ReturnCode_t insert_int8_value(int8_t value, uint32_t& outId);

    /**
     * insert a uint16 data into dyanmic-data
     *
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value uint16_t data
     * @return return status code
     */
    ReturnCode_t insert_uint16_value(uint16_t value, uint32_t& outId);

    /**
     * insert a int64 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value int64_t data
     * @return return status code
     */
    ReturnCode_t insert_int64_value(int64_t value, uint32_t& outId);

    /**
     * insert a uint64 data into dyanmic-data
     * @note @li only use for sequence dynamic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param outId elements's member-id
     * @param value uint64_t data
     * @return return status code
     */
    ReturnCode_t insert_uint64_value(uint64_t value, uint32_t& outId);

    /**
     * insert a float32/float data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value float data
     * @return return status code
     */
    ReturnCode_t insert_float32_value(float value, uint32_t& outId);

    /**
     * insert a float64/double data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value double data
     * @return return status code
     */
    ReturnCode_t insert_float64_value(double value, uint32_t& outId);

    /**
     * insert a float128/long-double data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value long double data
     * @return return status code
     */
    ReturnCode_t insert_float128_value(long double value, uint32_t& outId);

    /**
     * insert a char8/char data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value char data
     * @return return status code
     */
    ReturnCode_t insert_char8_value(char value, uint32_t& outId);

    /**
     * insert a char16/wchar_t data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value wchar_t data
     * @return return status code
     */
    ReturnCode_t insert_char16_value(wchar_t value, uint32_t& outId);

    /**
     * insert a byte/uint8/vbs::octet data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value vbs::octet data
     * @return return status code
     */
    ReturnCode_t insert_byte_value(vbs::octet value, uint32_t& outId);

    /**
     * insert a bool data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value bool data
     * @return return status code
     */
    ReturnCode_t insert_bool_value(bool value, uint32_t& outId);

    /**
     * insert a string into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value string data
     * @return return status code
     */
    ReturnCode_t insert_string_value(const std::string& value, uint32_t& outId);

    /**
     * insert a wstring into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value wstring data
     * @return return status code
     */
    ReturnCode_t insert_wstring_value(const std::wstring& value, uint32_t& outId);

    /**
     * insert a enum data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value data of enum
     * @return return status code
     */
    ReturnCode_t insert_enum_value(const std::string& value, uint32_t& outId);

    /**
     * insert a complex data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value dynamic-data
     * @return return status code
     */
    ReturnCode_t insert_complex_value(const VBSDynamicData* value, uint32_t& outId);

    /**
     * insert a complex data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     * @note @li only use for sequence dynamic-data
     *
     * @param outId elements's member-id
     * @param value dynamic-data
     * @return return status code
     */
    ReturnCode_t insert_complex_value(VBSDynamicData* value, uint32_t& outId);

    /**
     * remove data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param id elements's member-id
     * @return return status code
     */
    ReturnCode_t remove_sequence_data(uint32_t id);

    /**
     * Clear all elements in the dynamic data.
     *
     * This method clears all elements within sequence, map, or array-type dynamic data,
     * effectively resetting the data structure to be empty.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     *
     * @warning This function is intended for use only with sequence, map, or array-type
     *          dynamic data. Ensure the correct data type before calling.
     */
    ReturnCode_t clear_data();

    /**
     * clear array data
     * 
     * @note @li Thread-Safe: No
     *
     * @param indexId elements's index
     * @return return status code
     */
    ReturnCode_t clear_array_data(uint32_t indexId);

    /**
     * insert a map data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param key dynamic data that is key
     * @param outKeyId key elements's member-id
     * @param outValueId value elements's member-id
     * @return return status code
     */
    ReturnCode_t insert_map_data(const VBSDynamicData* key, uint32_t& outKeyId, uint32_t& outValueId);

    /**
     * insert a map data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param key key of map
     * @param value val of map
     * @param outKey key member-id
     * @param outValue val member-id
     * @return return status code
     */
    ReturnCode_t insert_map_data(const VBSDynamicData* key, VBSDynamicData* value, uint32_t& outKey,
                                 uint32_t& outValue);

    /**
     * insert a map data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param key key of map
     * @param value val of map
     * @param outKey key member-id
     * @param outValue val member-id
     * @return return status code
     */
    ReturnCode_t insert_map_data(const VBSDynamicData* key, const VBSDynamicData* value, uint32_t& outKey,
                                 uint32_t& outValue);

    /**
     * insert a int32 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param keyId key member-id
     * @return return status code
     */
    ReturnCode_t remove_map_data(uint32_t keyId);

    /**
     * get a int32 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_int32_value(int32_t& value, const uint32_t& id) const;

    /**
     * set a int32 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_int32_value(int32_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a uint32 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_uint32_value(uint32_t& value, const uint32_t& id) const;

    /**
     * set a uint32 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_uint32_value(uint32_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a int16 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_int16_value(int16_t& value, const uint32_t& id) const;

    /**
     * set a int16 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_int16_value(int16_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a uint16 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_uint16_value(uint16_t& value, const uint32_t& id) const;

    /**
     * set a uint16 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_uint16_value(uint16_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a int64 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_int64_value(int64_t& value, const uint32_t& id) const;

    /**
     * set a int64 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_int64_value(int64_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a uint64 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_uint64_value(uint64_t& value, const uint32_t& id) const;

    /**
     * set a uint64 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_uint64_value(uint64_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a float data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_float32_value(float& value, const uint32_t& id) const;

    /**
     * set a float data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_float32_value(float value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a double data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_float64_value(double& value, const uint32_t& id) const;

    /**
     * set a double data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_float64_value(double value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a long double data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_float128_value(long double& value, const uint32_t& id) const;

    /**
     * set a long double data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_float128_value(long double value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a char data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_char8_value(char& value, const uint32_t& id) const;

    /**
     * set a char data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_char8_value(char value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a wchar_t data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_char16_value(wchar_t& value, const uint32_t& id) const;

    /**
     * set a wchar_t data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_char16_value(wchar_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a byte data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_byte_value(vbs::octet& value, const uint32_t& id) const;

    /**
     * set a byte data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_byte_value(vbs::octet value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a int8_t data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_int8_value(int8_t& value, const uint32_t& id) const;

    /**
     * set a int8_t data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_int8_value(int8_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a uint8 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_uint8_value(uint8_t& value, const uint32_t& id) const;

    /**
     * set a uint8 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_uint8_value(uint8_t value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a bool data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_bool_value(bool& value, const uint32_t& id) const;

    /**
     * set a bool data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_bool_value(bool value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a string data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_string_value(std::string& value, const uint32_t& id) const;

    /**
     * set a string data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_string_value(const std::string& value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a wstring data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_wstring_value(std::wstring& value, const uint32_t& id) const;

    /**
     * set a wstring data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_wstring_value(const std::wstring& value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a enum data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_enum_value(std::string& value, const uint32_t& id) const;

    /**
     * set a enum data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_enum_value(const std::string& value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a enum data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_enum_value(uint32_t& value, const uint32_t& id) const;

    /**
     * set a enum data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_enum_value(const uint32_t& value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a enum data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_enum_value(int32_t& value, const uint32_t& id) const;

    /**
     * set a enum data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_enum_value(const int32_t& value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a bitmask data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @return return status code
     */
    ReturnCode_t get_bitmask_value(uint64_t& value) const;

    /**
     * set a bitmask data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @return return status code
     */
    ReturnCode_t set_bitmask_value(uint64_t value);

    /**
     * Get a complex data element from dynamic data.
     *
     * This method retrieves a complex data element (such as struct, union, sequence, array, map, or bitset)
     * from the dynamic data by the specified element name.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @param name The name of the element to retrieve.
     * @return std::shared_ptr<VBSDynamicData> A shared pointer to the complex dynamic data element.
     *         If the element is not of a complex type, the function will return a nullptr and
     *         set an error code of RETCODE_BAD_PARAMETER.
     *
     * @warning This function is intended only for complex types like struct, union, sequence, array,
     *          map, or bitset. Calling this function with an incompatible type will result in
     *          RETCODE_BAD_PARAMETER.
     */
    std::shared_ptr<VBSDynamicData> get_complex_value(const std::string& name) const;

    /**
     * Get a complex data element from dynamic data.
     *
     * This method retrieves a complex data element (such as struct, union, sequence, array, map, or bitset)
     * from the dynamic data by the specified member ID.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @param id The member ID of the element to retrieve.
     * @return std::shared_ptr<VBSDynamicData> A shared pointer to the complex dynamic data element.
     *         If the element is not of a complex type, the function will return a nullptr and
     *         set an error code of RETCODE_BAD_PARAMETER.
     *
     * @warning This function is intended only for complex types like struct, union, sequence, array,
     *          map, or bitset. Calling this function with an incompatible type will result in
     *          RETCODE_BAD_PARAMETER.
     */
    std::shared_ptr<VBSDynamicData> get_complex_value(const uint32_t& id) const;

    /**
     * Set a complex data element into dynamic data.
     * 
     * This method sets a complex data element (such as struct, union, sequence, array, map, or bitset)
     * into the dynamic data at the specified member ID.
     * 
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @param value A pointer to the complex data to be set.
     * @param id The member ID where the complex data will be set. Defaults to `VBS_MEMBER_ID_INVALID` (0x0FFFFFFFU).
     * @return ReturnCode_t A status code indicating the success or failure of the operation.
     *
     * @warning This function is intended only for complex types like struct, union, sequence, array, 
     *          map, or bitset. Ensure that the `value` is compatible before calling this function.
     */
    ReturnCode_t set_complex_value(VBSDynamicData* value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * get a union data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @return return status code
     */
    ReturnCode_t get_union_label(uint64_t& value) const;

    /**
     * get a char data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t get_char_value(char& value, const uint32_t& id) const;

    /**
     * set a char data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @param id member-id
     * @return return status code
     */
    ReturnCode_t set_char_value(char value, const uint32_t& id = VBS_MEMBER_ID_INVALID);

    /**
     * deserialize data into dynamic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param src the data has been serialized
     * @param len length of src
     */
    void deserialize(const void* src, const uint32_t& len);

    /**
     * serialize dynamic-data into dst
     * 
     * @note @li Thread-Safe: No
     *
     * @param dst destination
     * @param len size of dynamic-data
     */
    void serialize(void* dst, const uint32_t& len) const;

    /**
     * return size of dynamic-data 
     * 
     * @note @li Thread-Safe: No
     *
     * @return size of dynamic-data 
     */
    uint32_t get_size() const;

    /**
     * load_xml - Load XML content from a specified file path.
     * 
     * @note @li Thread-Safe: No
     *
     * @param xml_path xml path
     * 
     * @return A boolean value indicating the success of the XML loading process. 
     */
    static bool load_xml(const std::string& xml_path);

    /**
     * load_xml - This function loads XML content from a given string pointer and its length.
     * 
     * @note @li Thread-Safe: No
     *
     * @param data xml content
     * @param length length of xml content
     * 
     * @return A boolean value indicating the success of the XML loading process. 
     */
    static bool load_xml(const char* data, size_t length);

    /**
     * load idl. not support now
     * 
     * @note @li Thread-Safe: No
     *
     * @param idl_path idl path
     */
    void load_idl(const std::string& idl_path);

    /**
     * return type name
     * 
     * @note @li Thread-Safe: No
     *
     * @return  type_name_
     */
    std::string get_type_name() const;

    /**
     * get VBSDynamicType from dynamic data
     * 
     * @note @li Thread-Safe: No
     *
     * @return return VBSDynamicType
     */
    VBSDynamicType get_type() const;

    /**
     * Get a VBSDynamicType by type name.
     * 
     * This static method retrieves a dynamic type based on the specified type name.
     * It assumes that the type information has been previously loaded via XML.
     * 
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * @param type_name The name of the type to retrieve.
     * @return VBSDynamicType The corresponding dynamic type. If the type name does not exist,
     *         the behavior depends on implementation (e.g., may throw an exception or return a default type).
     *
     * @remark Ensure that type information has been loaded via XML before calling this function.
     */
    static VBSDynamicType get_dynamic_type(const std::string& type_name);

    /**
     * check if dynamic data can convert to static data
     * 
     * @note @li Thread-Safe: No
     *
     * @param sample dynamic data
     * @param instance static data
     * @return return bool
     */
    template <typename T>
    static bool can_convert(const VBSDynamicData* sample, T* instance) {
        return sample->get_type_name() == instance->get_type_name();
    }

    /**
     * convert from dynamic data to static data, if convert failed, return nullptr
     * 
     * @note @li Thread-Safe: No
     *
     * @param sample dynamic data
     * @return return T*
     */
    template <typename T>
    static T* convert(const VBSDynamicData* sample) {
        T* instance = new T();
        if (!can_convert(sample, instance)) {
            delete instance;
            return nullptr;
        }

        std::vector<char> buffer;
        buffer.resize(sample->get_size());
        sample->serialize(buffer.data(), sample->get_size());
        vbs::topic_type_support<T>::FromBuffer(*instance, buffer);

        return instance;
    }

    /**
     * convert from static data to dynamic data
     * 
     * @note @li Thread-Safe: No
     *
     * @param instance static data
     * @return return VBSDynamicData*
     */
    template <typename T>
    static VBSDynamicData* convert(const T& instance) {
        T::register_dynamic_type();
        VBSDynamicType dyn_type = T::get_vbs_dynamic_type();
        vbs::VBSDynamicData* sample = vbs::VBSDynamicDataFactory::get_instance()->create_data(dyn_type);
        std::vector<char> buffer;
        vbs::topic_type_support<T>::ToBuffer(instance, buffer);
        sample->deserialize(buffer.data(), buffer.size());

        return sample;
    }

    /**
     * get TypeSupport by type-name
     * 
     * @note @li Thread-Safe: No
     *
     * @param type_name type name
     * @return return TypeSupport
     */
    static EvbsTypeSupport get_type(const std::string& type_name);

    /**
     * Get a value of type T from dynamic data by name.
     *
     * This template method retrieves the value of a specified type T from dynamic data using the element's name.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * Supported types and corresponding method names:
     * - bool -> get_bool_value
     * - octet -> get_byte_value
     * - char -> get_char_value
     * - wchar_t -> get_char16_value
     * - short -> get_int16_value
     * - unsigned short -> get_uint16_value
     * - int -> get_int32_value
     * - unsigned int -> get_uint32_value
     * - int64_t -> get_int64_value
     * - uint64_t -> get_uint64_value
     * - std::string -> get_string_value
     * - float -> get_float32_value
     * - double -> get_float64_value
     * - long double -> get_float128_value
     * - std::wstring -> get_wstring_value
     *
     * @param name The name of the element whose value is to be retrieved.
     * @return T The value of the element cast to the specified type.
     *
     * @warning Ensure that the type T matches the actual type of the element to prevent
     *          casting errors or undefined behavior.
     */
    template <typename T>
    T value(const std::string& name) const;

    /**
     * Get a value of type T from dynamic data by index.
     *
     * This template method retrieves the value of a specified type T from dynamic data using the member's index.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * Supported types and corresponding method names:
     * - bool -> get_bool_value
     * - octet -> get_byte_value
     * - char -> get_char_value
     * - wchar_t -> get_char16_value
     * - short -> get_int16_value
     * - unsigned short -> get_uint16_value
     * - int -> get_int32_value
     * - unsigned int -> get_uint32_value
     * - int64_t -> get_int64_value
     * - uint64_t -> get_uint64_value
     * - std::string -> get_string_value
     * - float -> get_float32_value
     * - double -> get_float64_value
     * - long double -> get_float128_value
     * - std::wstring -> get_wstring_value
     *
     * @param index The index of the element whose value is to be retrieved.
     * @return T The value of the element cast to the specified type.
     *
     * @warning Ensure that the type T matches the actual type of the element to prevent
     *          casting errors or undefined behavior.
     */
    template <typename T>
    T value(uint32_t index) const;

    /**
     * get a char8 data from dyanmic-data
     *
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @return return elements's value
     */
    char get_char8_value(const std::string& name) const;

    /**
     * get a int8 data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @return return elements's value
     */
    int8_t get_int8_value(const std::string& name) const;

    /**
     * get a enum int data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @return return enum's int value
     */
    unsigned int get_enum_int(const std::string& name) const;

    /**
     * get a enum string data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @return return enum's string value
     */
    std::string get_enum_string(const std::string& name) const;

    /**
     * get a bitmask data from dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @return return elements's value
     */
    uint64_t get_bitmask_value() const;

    /**
     * Set a value of type T in dynamic data by name.
     *
     * This template method sets the value of a specified type T in dynamic data using the element's name.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * Supported types and corresponding method names:
     * - bool -> set_bool_value
     * - octet -> set_byte_value
     * - char -> set_char_value
     * - wchar_t -> set_char16_value
     * - short -> set_int16_value
     * - unsigned short -> set_uint16_value
     * - int -> set_int32_value
     * - unsigned int -> set_uint32_value
     * - int64_t -> set_int64_value
     * - uint64_t -> set_uint64_value
     * - std::string -> set_string_value
     * - float -> set_float32_value
     * - double -> set_float64_value
     * - long double -> set_float128_value
     * - std::wstring -> set_wstring_value
     *
     * @param name The name of the element whose value is to be set.
     * @param value The value to set for the element.
     * @return int A status code indicating success or failure of the operation.
     *
     * @warning Ensure that the type T matches the actual type of the element to prevent 
     *          casting errors or undefined behavior.
     */
    template <typename T>
    int value(const std::string& name, const T& value);

    /**
     * Set a value of type T in dynamic data by index.
     *
     * This template method sets the value of a specified type T in dynamic data using the element's index.
     *
     * @note
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     *
     * Supported types and corresponding method names:
     * - bool -> set_bool_value
     * - octet -> set_byte_value
     * - char -> set_char_value
     * - wchar_t -> set_char16_value
     * - short -> set_int16_value
     * - unsigned short -> set_uint16_value
     * - int -> set_int32_value
     * - unsigned int -> set_uint32_value
     * - int64_t -> set_int64_value
     * - uint64_t -> set_uint64_value
     * - std::string -> set_string_value
     * - float -> set_float32_value
     * - double -> set_float64_value
     * - long double -> set_float128_value
     * - std::wstring -> set_wstring_value
     *
     * @param index The index of the element whose value is to be set.
     * @param value The value to set for the element.
     * @return int A status code indicating success or failure of the operation.
     *
     * @warning Ensure that the type T matches the actual type of the element to prevent 
     *          casting errors or undefined behavior.
     */
    template <typename T>
    int value(uint32_t index, const T& value);

    /**
     * Get a vector of type T values from dynamic data by member ID.
     * 
     * This template method retrieves all elements of a specified type T from an array 
     * or sequence-type dynamic data, given a member ID, and returns them in a vector.
     * 
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     * 
     * Supported types and corresponding method names:
     * - bool -> get_bool_value
     * - uint8 -> get_byte_value/get_uint8_value
     * - char -> get_char_value
     * - wchar_t -> get_char16_value
     * - int16 -> get_int16_value
     * - uint16 -> get_uint16_value
     * - int32_t -> get_int32_value
     * - uint32_t -> get_uint32_value
     * - int64_t -> get_int64_value
     * - uint64_t -> get_uint64_value
     * - std::string -> get_string_value
     * - std::wstring -> get_wstring_value
     * - float -> get_float32_value
     * - double -> get_float64_value
     * - long double -> get_float128_value
     *
     * @param id The member ID of the array or sequence element to retrieve.
     * @return std::vector<T> A vector containing all values of the specified type from the element.
     *
     * @warning This function is intended for use with array and sequence types only. 
     *          Ensure that type T matches the actual type of the elements to prevent casting errors.
     */
    template <typename T>
    std::vector<T> get_values(const uint32_t& id);

    /**
     * Get a vector of type T values from dynamic data by element name.
     * 
     * This template method retrieves all elements of a specified type T from an array 
     * or sequence-type dynamic data, given an element name, and returns them in a vector.
     * 
     * @note 
     * @li Thread-Safe: No. This function is not safe for concurrent use from 
     *    multiple threads. External synchronization is required in a multi-threaded environment.
     * 
     * Supported types and corresponding method names:
     * - bool -> get_bool_value
     * - uint8 -> get_byte_value/get_uint8_value
     * - char -> get_char_value
     * - wchar_t -> get_char16_value
     * - int16 -> get_int16_value
     * - uint16 -> get_uint16_value
     * - int32_t -> get_int32_value
     * - uint32_t -> get_uint32_value
     * - int64_t -> get_int64_value
     * - uint64_t -> get_uint64_value
     * - std::string -> get_string_value
     * - std::wstring -> get_wstring_value
     * - float -> get_float32_value
     * - double -> get_float64_value
     * - long double -> get_float128_value
     *
     * @param name The name of the element from which to retrieve values.
     * @return std::vector<T> A vector containing all values of the specified type from the element.
     *
     * @warning This function is intended for use with array and sequence types only. 
     *          Ensure that type T matches the actual type of the elements to prevent casting errors.
     */
    template <typename T>
    std::vector<T> get_values(const std::string& name);

    /**
     * set a char8 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @param value elements's value
     * @return return status code, !0 is error
     */
    int set_char8_value(const std::string& name, const char& value);

    /**
     * set a int8 data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @param value elements's value
     * @return return status code, !0 is error
     */
    int set_int8_value(const std::string& name, const int8_t& value);

    /**
     * set a enum string value into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @param value enum's string value
     * @return return status code, !0 is error
     */
    int set_enum_string(const std::string& name, const std::string& value);

    /**
     * set a enum uint32_t int value into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @param value enum's int value
     * @return return status code, !0 is error
     */
    int set_enum_int(const std::string& name, const uint32_t& value);

    /**
     * set a enum int value into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param name member-name
     * @param value enum's int value
     * @return return status code, !0 is error
     */
    int set_enum_int(const std::string& name, const int32_t& value);

    /**
     * set a bitmask data into dyanmic-data
     * 
     * @note @li Thread-Safe: No
     *
     * @param value elements's value
     * @return return status code, !0 is error
     */
    int set_bitmask(uint64_t value);

 private:
    static std::string parse_idl_to_xml(const std::string& idl_path);

    void split_string_with_specified_char(const std::string& src, char c, std::vector<std::string>& nameVec) const;

    bool back_tracing_loan_value(EvbsDynamicData* parent, std::vector<std::string>& nameVec, uint32_t index,
                                 std::stack<EvbsDynamicData*>& btStack) const;

    void back_tracing_return_loaned_value(EvbsDynamicData* target, std::stack<EvbsDynamicData*>& btStack) const;

    void generate_type();

    void create_data();

    void delete_data();

    std::string type_name_;
    EvbsTypeSupport type_;
    EvbsDynamicData* vbs_dynamic_data_;
    bool is_complex_data_;
    mutable bool is_value_;
};

}  // namespace vbs

#endif  // VBS_VBSDYNAMICDATA_H_
