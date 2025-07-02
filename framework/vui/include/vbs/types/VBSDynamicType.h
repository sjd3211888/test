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

#ifndef VBS_TYPES_DYNAMIC_TYPE_H_
#define VBS_TYPES_DYNAMIC_TYPE_H_

#include <memory>

#include "deps/common/TypeSupport.hpp"
#include "vbs/Utils.hpp"

namespace vbs {
namespace common {
namespace types {
class DynamicData;
class DynamicData_ptr;
class DynamicType_ptr;
class DynamicDataFactory;
class DynamicTypeBuilderFactory;
class DynamicTypeBuilder;
class DynamicTypeMember;
}  // namespace types
}  // namespace common
}  // namespace vbs

namespace vbs {

using EvbsTypeKind = unsigned char;
using EvbsTypeSupport = vbs::TypeSupport;
using EvbsDynamicData = vbs::common::types::DynamicData;
using EvbsDynamicData_ptr = vbs::common::types::DynamicData_ptr;
using EvbsDynamicType_ptr = vbs::common::types::DynamicType_ptr;
using EvbsDynamicDataFactory = vbs::common::types::DynamicDataFactory;
using EvbsDynamicTypeBuilderFactory = vbs::common::types::DynamicTypeBuilderFactory;
using EvbsDynamicTypeBuilder = vbs::common::types::DynamicTypeBuilder;
using EvbsDynamicTypeMember = vbs::common::types::DynamicTypeMember;

class ReturnCode_t;
class VBSDynamicTypeMember;

class VBSDynamicType {
    friend class VBSDynamicTypeBuilder;
    friend class VBSDynamicTypeBuilderFactory;
    friend class VBSDynamicDataFactory;
    friend class VBSDynamicPubSubType;
    friend class VBSDynamicData;
    friend class VBSDynamicTypeMember;
    friend class Topic;
    friend class DomainParticipant;
    friend class DomainParticipantImpl;
    friend class TopicImpl;

 public:
    /**
     * VBSDynamicDataFactory constructor
     * 
     * @note @li Thread-Safe: Yes
     */
    VBSDynamicType();

    /**
     * VBSDynamicDataFactory copy constructor
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param type dynamic type
     */
    VBSDynamicType(const VBSDynamicType& type);

    /**
     * operator assign
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param data dynamic type
     * @return dynamic type
     */
    VBSDynamicType& operator=(const VBSDynamicType& data);

    /**
     * VBSDynamicDataFactory deconstructor
     * 
     * @note @li Thread-Safe: Yes
     */
    ~VBSDynamicType();

    /**
     * evbs type kind convert to vbs type kind
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param kind evbs type kind
     * @return vbs type kind
     */
    static TypeKind ToVbsTypeKind(EvbsTypeKind kind);

    /**
     * vbs type kind convert to evbs type kind
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param kind vbs type kind
     * @return evbs type kind
     */
    static EvbsTypeKind ToEvbsTypeKind(TypeKind kind);

    /**
     * generate TypeSupport
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return TypeSupport
     */
    EvbsTypeSupport type() const;

    /**
    * Get the VBSDynamicType of a member by member ID.
    *
    * This method retrieves the VBSDynamicType object for a specified member within
    * a VBSDynamicType, based on the member ID.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be managed if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @param memberId The member ID for which to retrieve the VBSDynamicType.
    * @return VBSDynamicType The dynamic type object of the specified member.
    *
    * @warning Ensure that the memberId is valid within the context of the VBSDynamicType
    *          to prevent undefined behavior.
    */
    VBSDynamicType member_type(uint32_t memberId) const;

    /**
    * Retrieve all elements of the dynamic data type.
    *
    * This method populates a provided map with information about all members of the dynamic data type.
    * Each entry in the map uses the member ID as the key and a shared pointer to the VBSDynamicTypeMember as the value.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be managed if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @param members An empty map that will be filled with all elements' information.
    *        The map's key is the member ID, and the value is a shared pointer to a VBSDynamicTypeMember object.
    *
    * @warning Ensure that the `members` map is empty before calling this function to avoid overwriting existing data.
    */
    void get_all_members(std::map<uint32_t, std::shared_ptr<VBSDynamicTypeMember>>& members) const;

    /**
    * Retrieve all elements of the dynamic data type by name.
    *
    * This method populates a provided map with information about all members of the dynamic data type.
    * Each entry in the map uses the member name as the key and a shared pointer to the VBSDynamicTypeMember as the value.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be managed if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @param members An empty map that will be filled with all elements' information.
    *        The map's key is the member name, and the value is a shared pointer to a VBSDynamicTypeMember object.
    *
    * @warning Ensure that the `members` map is empty before calling this function to avoid overwriting existing data.
    */
    void get_all_members_by_name(std::map<std::string, std::shared_ptr<VBSDynamicTypeMember>>& members) const;

    /**
    * Get the kind of the dynamic type.
    *
    * This method returns the TypeKind corresponding to the dynamic type,
    * indicating the general category or form of the type.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads
    *    concurrently.
    *
    * @return TypeKind The kind of the dynamic type.
    */
    TypeKind get_kind() const;

    /**
    * Get the name of the dynamic type.
    *
    * This method returns the name of the VBSDynamicType as a string.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from 
    *    multiple threads. External synchronization should be handled in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @return std::string The name of the VBSDynamicType.
    */
    std::string get_name() const;

    /**
    * Get the descriptor name of the dynamic type.
    *
    * This method returns the descriptor name of the VBSDynamicType as a string.
    * It serves the same purpose as get_name().
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return std::string The descriptor name of the VBSDynamicType.
    */
    std::string get_descriptor_name() const;

    /**
    * Get the count of members in the dynamic type.
    *
    * This method returns the number of members in the VBSDynamicType.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return uint32_t The count of members in the VBSDynamicType.
    */
    uint32_t get_members_count() const;

    /**
    * Check if the dynamic type has members.
    *
    * This method determines if the VBSDynamicType contains any members.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the type has members, `false` otherwise.
   */
    bool has_children() const;

    /**
    * Check if the dynamic type is consistent.
    *
    * This method determines whether the VBSDynamicType is complete and valid.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the type is consistent and complete, `false` otherwise.
    */
    bool is_consistent() const;

    /**
    * Check if the dynamic type is of a complex kind.
    *
    * This method determines if the VBSDynamicType is one of the complex types,
    * such as array, bitmask, map, sequence, struct, union, bitset, or annotation.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the type is complex, `false` otherwise.
    */
    bool is_complex_kind() const;

    /**
    * Get the total bounds of the dynamic type.
    *
    * This method returns the total upper bound value of the VBSDynamicType.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return uint32_t The total upper bound of the VBSDynamicType.
    */
    uint32_t get_total_bounds() const;

    /**
    * Get the bounds for a member by index.
    *
    * This method retrieves the bounds of a sub-member within the VBSDynamicType,
    * specified by the member index.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @param index The index of the member whose bounds are to be retrieved. Defaults to 0.
    * @return uint32_t The bounds of the specified member index.
    *
    * @warning Ensure that the index is valid for the specific VBSDynamicType to avoid undefined behavior.
    */
    uint32_t get_bounds(uint32_t index = 0U) const;

    /**
    * Get the size of bounds.
    *
    * This method returns the number of dimensions for the bounds of the VBSDynamicType.
    * If the dynamic type has multiple dimensions, it returns the count of these dimensions.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return uint32_t The number of dimensions in the bounds of the VBSDynamicType.
    */
    uint32_t get_bounds_size() const;

    /**
    * Get the base type of the dynamic type.
    *
    * This method retrieves the base VBSDynamicType, either as the fundamental type
    * of a derived class or as the real type of an alias.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return VBSDynamicType The base or real type of the VBSDynamicType.
    */
    VBSDynamicType get_base_type() const;

    /**
    * Get the element type of the dynamic type.
    *
    * This method returns the VBSDynamicType representing the type of elements contained
    * within the dynamic type, such as the type of elements in an array or collection.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be handled if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @return VBSDynamicType The type of elements within the dynamic type.
    */
    VBSDynamicType get_element_type() const;

    /**
    * Get the key element type of a map.
    *
    * This method retrieves the type of the keys used in a map-type dynamic data.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be handled if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @return VBSDynamicType The type of the key elements in the map.
    */
    VBSDynamicType get_key_element_type() const;

    /**
    * Get the discriminator type of a union.
    *
    * This method retrieves the type of the discriminator used in a union-type dynamic data.
    *
    * @note
    * @li Thread-Safe: No. This function is not safe for concurrent use from
    *    multiple threads. External synchronization should be handled if used
    *    in a multi-threaded environment.
    * @li Lock-Free: No. This operation may involve internal locking mechanisms.
    *
    * @return VBSDynamicType The type of the discriminator in the union.
    */
    VBSDynamicType get_discriminator_type() const;

    /**
     * get bound of bitmask_bit
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return bound of bitmask_bit
     */
    uint16_t bitmask_bit_bound() const;

    /**
     * whether this type has base type
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true has, false not
     */
    bool has_base_type() const;

    /**
     * generate xml
     *
     * @param file_name xml name you want to save
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     */
    void generate_xml(const std::string& file_name) const;

    /**
     * generate xml
     *
     * @return xml content
     */
    std::string generate_xml() const;

    /**
     * whether this type is valid
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true is, flase not
     */
    bool is_valid() const;

    /**
     * @brief print VBSDynamicType as idl format
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type VBSDynamicType object
     * @param idl  VBSDynamicType's idl format string
     */
    static void print_idl(vbs::VBSDynamicType& type, std::string& idl);

    /**
     * @brief get VBSDynamicType from xml
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param xml_file xml file name
     * @param type_name name of dynamic type
     */
    static VBSDynamicType getDynamicTypeFromXml(const std::string& xml_file, const std::string& type_name);

    /**
     * @brief get member count of a type
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return count value
     */
    size_t member_count() const;

    /**
     * @brief compare two VBSDynamicType is same
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return compare result
     */
    bool operator==(const VBSDynamicType& data) const;

    /**
     * @brief compare two VBSDynamicType is diff
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return compare result
     */
    bool operator!=(const VBSDynamicType& data) const;

    /**
    * @brief Check if the annotation is mutable.
    *
    * This method determines if the annotation of the dynamic type is mutable.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the annotation is mutable, `false` otherwise.
    */
    bool annotation_is_mutable() const;

    /**
    * @brief Check if the annotation is appendable.
    *
    * This method determines if the annotation of the dynamic type is appendable.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the annotation is appendable, `false` otherwise.
    */
    bool annotation_is_appendable() const;

    /**
    * @brief Check if the annotation is final.
    *
    * This method determines if the annotation of the dynamic type is final.
    *
    * @note
    * @li Thread-Safe: Yes. This function can be safely called from multiple threads concurrently.
    *
    * @return bool `true` if the annotation is final, `false` otherwise.
    */
    bool annotation_is_final() const;

 private:
    std::shared_ptr<EvbsDynamicType_ptr> dynamic_type_;
};

}  // namespace vbs

#endif  // VBS_TYPES_DYNAMIC_TYPE_H_
