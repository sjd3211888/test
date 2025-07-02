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

#ifndef VBS_TYPES_DYNAMIC_TYPE_BUILDER_FACTORY_H_
#define VBS_TYPES_DYNAMIC_TYPE_BUILDER_FACTORY_H_

#include "vbs/types/VBSDynamicTypeBuilder.h"
#include "vbs/types/VBSDynamicType.h"

namespace vbs {

const uint32_t VBS_DYNAMIC_MAX_BITMASK_LENGTH = 64U;
const uint32_t VBS_DYNAMIC_MAX_ELEMENTS_COUNT = 100U;
const uint32_t VBS_DYNAMIC_MAX_STRING_LENGTH = 255U;

class VBSDynamicTypeBuilderFactory {
 public:
    /**
     * VBSDynamicTypeBuilderFactory constructor
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    VBSDynamicTypeBuilderFactory();

    /**
     * VBSDynamicTypeBuilderFactory deconstructor
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     */
    ~VBSDynamicTypeBuilderFactory();

    /**
     * get instance
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return VBSDynamicTypeBuilderFactory pointer
     */
    static VBSDynamicTypeBuilderFactory* get_instance();

    /**
     * delete instance
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return return status code
     */
    static ReturnCode_t delete_instance();

    /**
     * get primitive type by kind
     *
     * @return dynamic type
     */
    VBSDynamicType get_primitive_type(TypeKind kind);

    /**
     * delete builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param builder the builder you want to delete
     * @return return status code
     */
    ReturnCode_t delete_builder(VBSDynamicTypeBuilder builder);

    /**
     * delete type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type the type you want to delete
     * @return return status code
     */
    ReturnCode_t delete_type(VBSDynamicType type);

    /**
     * copy builder from builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type the builder you want to copy
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_builder_copy(const VBSDynamicTypeBuilder& type);

    /**
     * create int32 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_int32_builder();

    /**
     * create uint32 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_uint32_builder();

    /**
     * create int16 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_int16_builder();

    /**
     * create uint16 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_uint16_builder();

    /**
     * create int64 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_int64_builder();

    /**
     * create uint64 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_uint64_builder();

    /**
     * create float32 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_float32_builder();

    /**
     * create float64 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_float64_builder();

    /**
     * create float128 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_float128_builder();

    /**
     * create char8 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_char8_builder();

    /**
     * create char16 builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_char16_builder();

    /**
     * create bool builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_bool_builder();

    /**
     * create byte builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_byte_builder();

    /**
     * create string builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of string
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_string_builder(uint32_t bound = VBS_DYNAMIC_MAX_STRING_LENGTH);

    /**
     * create wstring builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of wstring
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_wstring_builder(uint32_t bound = VBS_DYNAMIC_MAX_STRING_LENGTH);

    /**
     * create sequence builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param element_type element's builder
     * @param bound bound of sequence
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_sequence_builder(const VBSDynamicTypeBuilder& element_type,
                                                  uint32_t bound = VBS_DYNAMIC_MAX_ELEMENTS_COUNT);

    /**
     * create sequence builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type element's type
     * @param bound bound of sequence
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_sequence_builder(const VBSDynamicType& type,
                                                  uint32_t bound = VBS_DYNAMIC_MAX_ELEMENTS_COUNT);

    /**
     * create array builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param element_type element's builder
     * @param bounds bound of array
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_array_builder(const VBSDynamicTypeBuilder& element_type,
                                               const std::vector<uint32_t>& bounds);

    /**
     * create array builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param type element's type
     * @param bounds bound of array
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_array_builder(const VBSDynamicType& type, const std::vector<uint32_t>& bounds);

    /**
     * create map builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param key_element_type key element's type
     * @param element_type val element's type
     * @param bound bound of map
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_map_builder(VBSDynamicTypeBuilder key_element_type, VBSDynamicTypeBuilder element_type,
                                             uint32_t bound = VBS_DYNAMIC_MAX_ELEMENTS_COUNT);

    /**
     * create map builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param key_type key type
     * @param value_type val type
     * @param bound bound of map
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_map_builder(VBSDynamicType key_type, VBSDynamicType value_type,
                                             uint32_t bound = VBS_DYNAMIC_MAX_ELEMENTS_COUNT);

    /**
     * create bitmask builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of sequence
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_bitmask_builder(uint32_t bound);

    /**
     * create bitset builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_bitset_builder();

    /**
     * create alias builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param base_type base type
     * @param sName name of alias
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_alias_builder(VBSDynamicTypeBuilder base_type, const std::string& sName);

    /**
     * create alias builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param base_type base type
     * @param sName name of alias
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_alias_builder(VBSDynamicType base_type, const std::string& sName);

    /**
     * create enum builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_enum_builder();

    /**
     * create struct builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_struct_builder();

    /**
     * create child struct builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param parent_type parent type
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_child_struct_builder(VBSDynamicTypeBuilder parent_type);

    /**
     * create union builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param discriminator_type discriminator type
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_union_builder(VBSDynamicTypeBuilder discriminator_type);

    /**
     * create union builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param discriminator_type discriminator type
     * @return dynamic type builder
     */
    VBSDynamicTypeBuilder create_union_builder(VBSDynamicType discriminator_type);

    /**
     * create annotation type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param name type name
     * @return dynamic type
     */
    VBSDynamicType create_annotation_primitive(const std::string& name);

    /**
     * create dynamic type by builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param other dynamic type builder
     * @return dynamic type
     */
    VBSDynamicType create_type(const VBSDynamicTypeBuilder& other);

    /**
     * create alias type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param base_type base type
     * @param sName name of type
     * @return dynamic type
     */
    VBSDynamicType create_alias_type(VBSDynamicTypeBuilder base_type, const std::string& sName);

    /**
     * create alias type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param base_type base type
     * @param sName name of type
     * @return dynamic type
     */
    VBSDynamicType create_alias_type(VBSDynamicType base_type, const std::string& sName);

    /**
     * create int32 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_int32_type();

    /**
     * create uint32 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_uint32_type();

    /**
     * create int16 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */

    VBSDynamicType create_int16_type();

    /**
     * create uint16 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_uint16_type();

    /**
     * create int64 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_int64_type();

    /**
     * create uint64 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_uint64_type();

    /**
     * create float32 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_float32_type();

    /**
     * create float64 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_float64_type();

    /**
     * create float128 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_float128_type();

    /**
     * create char8 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_char8_type();

    /**
     * create char16 type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_char16_type();

    /**
     * create bool type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_bool_type();

    /**
     * create byte type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType create_byte_type();

    /**
     * create string type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of string
     * @return dynamic type
     */
    VBSDynamicType create_string_type(uint32_t bound = VBS_DYNAMIC_MAX_STRING_LENGTH);

    /**
     * create wstring type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of wstring
     * @return dynamic type
     */
    VBSDynamicType create_wstring_type(uint32_t bound = VBS_DYNAMIC_MAX_STRING_LENGTH);

    /**
     * create bitset type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param bound bound of bitset
     * @return dynamic type
     */
    VBSDynamicType create_bitset_type(uint32_t bound);

 private:
    EvbsDynamicTypeBuilderFactory* dynamic_type_builder_factory_;
};

}  // namespace vbs

#endif  // VBS_TYPES_DYNAMIC_TYPE_BUILDER_FACTORY_H_
