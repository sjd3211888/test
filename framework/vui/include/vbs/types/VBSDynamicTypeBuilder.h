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

#ifndef VBS_TYPES_DYNAMIC_TYPE_BUILDER_H_
#define VBS_TYPES_DYNAMIC_TYPE_BUILDER_H_

#include "vbs/types/VBSDynamicType.h"

namespace vbs {

class VBSDynamicType;

class VBSDynamicTypeBuilder {
    friend class VBSDynamicTypeBuilderFactory;
    friend class VBSDynamicType;

 public:
    /**
     * VBSDynamicTypeBuilder constructor
     * 
     * @note @li Thread-Safe: Yes
     *
     */
    VBSDynamicTypeBuilder();

    /**
     * VBSDynamicTypeBuilder constructor
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param builder VBSDynamicTypeBuilder
     */
    VBSDynamicTypeBuilder(const VBSDynamicTypeBuilder& builder);

    /**
     * VBSDynamicTypeBuilder deconstructor
     * 
     * @note @li Thread-Safe: Yes
     *
     */
    ~VBSDynamicTypeBuilder();

    /**
     * add empty member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li this method only for Enumrate Type
     *
     * @param id element's member-id
     * @param name element's name
     * @return return status code
     */
    ReturnCode_t add_empty_member(uint32_t id, const std::string& name);

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li only use for struct builder
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type = VBSDynamicTypeBuilder());

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @param defaultValue default value
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type,
                            const std::string& defaultValue);

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li only use for union type
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @param defaultValue default value
     * @param unionLabels union labels
     * @param isDefaultLabel is defaultLabel
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicTypeBuilder type,
                            const std::string& defaultValue, const std::vector<uint64_t>& unionLabels,
                            bool isDefaultLabel);

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicType type);

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @param defaultValue default value
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicType type, const std::string& defaultValue);

    /**
     * add member into this builder
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     * @note @li only use for union type
     *
     * @param id element's member-id
     * @param name element's name
     * @param type dynamic type builder
     * @param defaultValue default value
     * @param unionLabels union labels
     * @param isDefaultLabel is defaultLabel
     * @return return status code
     */
    ReturnCode_t add_member(uint32_t id, const std::string& name, VBSDynamicType type, const std::string& defaultValue,
                            const std::vector<uint64_t>& unionLabels, bool isDefaultLabel);

    /**
     * generate dynamic type
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return dynamic type
     */
    VBSDynamicType build();

    /**
     * copy VBSDynamicTypeBuilder from other
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @param other dynamic type builder
     * @return return status code
     */
    ReturnCode_t copy_from(const VBSDynamicTypeBuilder& other);

    /**
     * get type kind
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return type kind
     */
    TypeKind get_kind() const;

    /**
     * get type name
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return type name
     */
    std::string get_name() const;

    /**
     * get member id by name
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param name elements's name
     * @return member id
     */
    uint32_t get_member_id_by_name(const std::string& name) const;

    /**
     * whether this type is consistent
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true is, false not
     */
    bool is_consistent() const;

    /**
     * whether this type is discriminator type
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return true is, false not
     */
    bool is_discriminator_type() const;

    /**
     * set dynamic type's name
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param name dynamic type's name
     * @return return status code
     */
    ReturnCode_t set_name(const std::string& name);

 private:
    EvbsDynamicTypeBuilder* dynamic_type_builder_;
};

}  // namespace vbs

#endif  // VBS_TYPES_DYNAMIC_TYPE_BUILDER_H_
