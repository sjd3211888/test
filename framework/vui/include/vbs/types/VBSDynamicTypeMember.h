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

#ifndef VBS_TYPES_VBS_DYNAMIC_TYPE_MEMBER_H_
#define VBS_TYPES_VBS_DYNAMIC_TYPE_MEMBER_H_

#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class VBSDynamicTypeMember {
    friend class VBSDynamicType;

 public:
    /**
     * VBSDynamicTypeMember constructor
     * 
     * @note @li Thread-Safe: Yes
     *
     */
    VBSDynamicTypeMember();

    /**
     * VBSDynamicTypeMember constructor
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param other VBSDynamicTypeMember
     */
    VBSDynamicTypeMember(const VBSDynamicTypeMember& other);

    /**
     * operator assign
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param other VBSDynamicTypeMember
     * @return VBSDynamicTypeMember
     */
    VBSDynamicTypeMember& operator=(VBSDynamicTypeMember&& other);

    /**
     * VBSDynamicTypeMember deconstructor
     * 
     * @note @li Thread-Safe: Yes
     *
     */
    ~VBSDynamicTypeMember();

    /**
     * get element's name
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return name of element
     */
    std::string get_name() const;

    /**
     * get element's member id
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return member id
     */
    uint32_t get_id() const;

    /**
     * get type kind
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return type kind
     */
    TypeKind get_kind() const;

    /**
     * get index
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return index
     */
    uint32_t get_index() const;

    /**
     * get dynamic type
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return dynamic type
     */
    VBSDynamicType get_type() const;

    /**
     * get union labels
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return uint64_t vector
     */
    std::vector<uint64_t> get_union_labels() const;

    /**
     * annotation get bit bound
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return bit bound of annotation
     */
    uint16_t annotation_get_bit_bound() const;

    /**
     * annotation get position
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return position of annotation
     */
    uint16_t annotation_get_position() const;

    /**
     * get_default_value
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return default value
     */
    std::string get_default_value() const;

    /**
     * get element's cdr id
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return member id
     */
    uint32_t get_cdr_id() const;

    /**
     * is optional
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true is optional, false not
     */
    bool annotation_is_optional() const;

    /**
     * is key
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true is key, false not
     */
    bool annotation_is_key() const;

 private:
    EvbsDynamicTypeMember* dynamic_type_member_;
};

}  // namespace vbs

#endif  // VBS_TYPES_VBS_DYNAMIC_TYPE_MEMBER_H_
