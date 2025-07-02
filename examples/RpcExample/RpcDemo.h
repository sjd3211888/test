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

#ifndef _EDDS_GENERATED_168448424_RPCDEMO_H_
#define _EDDS_GENERATED_168448424_RPCDEMO_H_

////#include <erpc/exceptions/UserException.h>
//

#include "SafeEnum.h"
// #include "ContainerPrintHelpers.h"

#include <vbs/RpcHeader.hpp>

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <sstream>
#include <type_traits>
#include <set>

#if defined(_WIN32)
#if defined(VBS_USER_DLL_EXPORT)
#define vbs_user_DllExport __declspec(dllexport)
#else
#define vbs_user_DllExport
#endif  // VBS_USER_DLL_EXPORT
#else
#define vbs_user_DllExport
#endif  // _WIN32

#if defined(_WIN32)
#if defined(VBS_USER_DLL_EXPORT)
#if defined(RpcDemo_SOURCE)
#define RpcDemo_DllAPI __declspec(dllexport)
#else
#define RpcDemo_DllAPI __declspec(dllimport)
#endif  // RpcDemo_SOURCE
#else
#define RpcDemo_DllAPI
#endif  // VBS_USER_DLL_EXPORT
#else
#define RpcDemo_DllAPI
#endif  // _WIN32

namespace vbs {
#ifndef _VBSTYPE_GEN_TEMP_
#define _VBSTYPE_GEN_TEMP_

template <typename T>
struct isVBSType<T,
                 typename std::enable_if<std::is_member_function_pointer<decltype(&T::VBSVersionPrinter)>::value>::type>
    : std::true_type {};

#endif  // _VBSTYPE_GEN_TEMP_

#ifndef _VBSTYPE_RPC_GEN_TEMP_
#define _VBSTYPE_RPC_GEN_TEMP_

template <typename T>
struct isVBSRpcType<T,
                    typename std::enable_if<std::is_member_function_pointer<decltype(&T::VBSRpcPrinter)>::value>::type>
    : std::true_type {};

#endif  // _VBSTYPE_RPC_GEN_TEMP_

}  // namespace vbs

/*!
 * @brief This class represents the structure req_type defined by the user in the IDL file.
 * @ingroup RPCDEMO
 */

#pragma pack(push, 4)
class req_type : public vbs::rpc::RpcMessageType {
 public:
    /*!
     * @brief Default constructor.
     */
    vbs_user_DllExport req_type();

    /*!
     * @brief Default destructor.
     */
    vbs_user_DllExport ~req_type();

    /*!
     * @brief Copy constructor.
     * @param x Reference to the object req_type that will be copied.
     */
    vbs_user_DllExport req_type(const req_type& x);

    /*!
     * @brief Move constructor.
     * @param x Reference to the object req_type that will be copied.
     */
    vbs_user_DllExport req_type(req_type&& x);

    /*!
     * @brief Move constructor.
     * @param x All parameters to the object req_type that will be copied.
     */
    vbs_user_DllExport req_type(const int32_t& add1, const int32_t& add2);
    /*!
     * @brief Copy assignment.
     * @param x Reference to the object req_type that will be copied.
     */
    vbs_user_DllExport req_type& operator=(const req_type& x);

    /*!
     * @brief Move assignment.
     * @param x Reference to the object req_type that will be copied.
     */
    vbs_user_DllExport req_type& operator=(req_type&& x);

    /*!
     * @brief Comparison operator.
     * @param x req_type object to compare.
     */
    vbs_user_DllExport bool operator==(const req_type& x) const;

    /*!
     * @brief Comparison operator.
     * @param x req_type object to compare.
     */
    vbs_user_DllExport bool operator!=(const req_type& x) const;

    /*!
     * @brief This function returns a const reference to member add1
     * @param _add1 New value to be copied in member add1
     */
    vbs_user_DllExport void add1(const int32_t& _add1);
    /*!
     * @brief This function moves the value in member add1
     * @param _add1 New value to be moved in member add1
     */
    vbs_user_DllExport void add1(int32_t&& _add1);

    /*!
     * @brief This function returns a reference to member add1
     * @return Reference to member add1
     */
    vbs_user_DllExport int32_t& add1();

    /*!
    * @brief This function returns the value of member add1
    * @return Value of member add1
    */
    vbs_user_DllExport int32_t add1() const;

    /*!
     * @brief This function returns a const reference to member add2
     * @param _add2 New value to be copied in member add2
     */
    vbs_user_DllExport void add2(const int32_t& _add2);
    /*!
     * @brief This function moves the value in member add2
     * @param _add2 New value to be moved in member add2
     */
    vbs_user_DllExport void add2(int32_t&& _add2);

    /*!
     * @brief This function returns a reference to member add2
     * @return Reference to member add2
     */
    vbs_user_DllExport int32_t& add2();

    /*!
    * @brief This function returns the value of member add2
    * @return Value of member add2
    */
    vbs_user_DllExport int32_t add2() const;

    /*!
     * @brief This function returns the maximum serialized size of an object
     * depending on the buffer alignment.
     * @param current_alignment Buffer alignment.
     * @return Maximum serialized size.
     */
    vbs_user_DllExport static size_t getMaxCdrSerializedSize(size_t current_alignment = 0);

    /*!
     * @brief This function returns the serialized size of a data depending on the buffer alignment.
     * @param data Data which is calculated its serialized size.
     * @param current_alignment Buffer alignment.
     * @return Serialized size.
     */
    vbs_user_DllExport static size_t getCdrSerializedSize(const req_type& data, size_t current_alignment = 0);

    /*!
     * @brief This function returns the size of an object.
     * @param current_alignment Buffer alignment.
     * @return Maximum serialized size.
     */
    vbs_user_DllExport static size_t getObjectDataSize();

    /*!
     * @brief This function serializes an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void serialize(vbs::Cdr& cdr) const;

    /*!
     * @brief This function deserializes an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void deserialize(vbs::Cdr& cdr);

    void swap(req_type& other_);

    std::string to_idl_string(std::set<std::string>* processedTypes = nullptr, const bool is_top = true) const;

    static std::string get_type_name();

    void reset_all_member();
    friend vbs_user_DllExport std::ostream& operator<<(std::ostream& o, const req_type& sample);

    void VBSVersionPrinter() { std::cout << "VBS IDL Gen Version 1.3.0-20240522-001" << std::endl; }

    void VBSRpcPrinter() { std::cout << "This is Rpc code." << std::endl; }

 private:
    int32_t m_add1;
    int32_t m_add2;
};
#pragma pack(pop)

inline void swap(req_type& a, req_type& b) {
    a.swap(b);
}

namespace vbs {
template <>
struct rpc_type_support<req_type> {

    static bool ToBuffer(const req_type& sample, std::vector<char>& buffer);

    static bool FromBuffer(req_type& sample, const std::vector<char>& buffer);
};

std::string data_to_json_string(const req_type& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = true, const bool include_braces = true);
}  // namespace vbs

/*!
 * @brief This class represents the structure res_type defined by the user in the IDL file.
 * @ingroup RPCDEMO
 */

#pragma pack(push, 4)
class res_type : public vbs::rpc::RpcMessageType {
 public:
    /*!
     * @brief Default constructor.
     */
    vbs_user_DllExport res_type();

    /*!
     * @brief Default destructor.
     */
    vbs_user_DllExport ~res_type();

    /*!
     * @brief Copy constructor.
     * @param x Reference to the object res_type that will be copied.
     */
    vbs_user_DllExport res_type(const res_type& x);

    /*!
     * @brief Move constructor.
     * @param x Reference to the object res_type that will be copied.
     */
    vbs_user_DllExport res_type(res_type&& x);

    /*!
     * @brief Move constructor.
     * @param x All parameters to the object res_type that will be copied.
     */
    vbs_user_DllExport res_type(const int32_t& sum);
    /*!
     * @brief Copy assignment.
     * @param x Reference to the object res_type that will be copied.
     */
    vbs_user_DllExport res_type& operator=(const res_type& x);

    /*!
     * @brief Move assignment.
     * @param x Reference to the object res_type that will be copied.
     */
    vbs_user_DllExport res_type& operator=(res_type&& x);

    /*!
     * @brief Comparison operator.
     * @param x res_type object to compare.
     */
    vbs_user_DllExport bool operator==(const res_type& x) const;

    /*!
     * @brief Comparison operator.
     * @param x res_type object to compare.
     */
    vbs_user_DllExport bool operator!=(const res_type& x) const;

    /*!
     * @brief This function returns a const reference to member sum
     * @param _sum New value to be copied in member sum
     */
    vbs_user_DllExport void sum(const int32_t& _sum);
    /*!
     * @brief This function moves the value in member sum
     * @param _sum New value to be moved in member sum
     */
    vbs_user_DllExport void sum(int32_t&& _sum);

    /*!
     * @brief This function returns a reference to member sum
     * @return Reference to member sum
     */
    vbs_user_DllExport int32_t& sum();

    /*!
    * @brief This function returns the value of member sum
    * @return Value of member sum
    */
    vbs_user_DllExport int32_t sum() const;

    /*!
     * @brief This function returns the maximum serialized size of an object
     * depending on the buffer alignment.
     * @param current_alignment Buffer alignment.
     * @return Maximum serialized size.
     */
    vbs_user_DllExport static size_t getMaxCdrSerializedSize(size_t current_alignment = 0);

    /*!
     * @brief This function returns the serialized size of a data depending on the buffer alignment.
     * @param data Data which is calculated its serialized size.
     * @param current_alignment Buffer alignment.
     * @return Serialized size.
     */
    vbs_user_DllExport static size_t getCdrSerializedSize(const res_type& data, size_t current_alignment = 0);

    /*!
     * @brief This function returns the size of an object.
     * @param current_alignment Buffer alignment.
     * @return Maximum serialized size.
     */
    vbs_user_DllExport static size_t getObjectDataSize();

    /*!
     * @brief This function serializes an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void serialize(vbs::Cdr& cdr) const;

    /*!
     * @brief This function deserializes an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void deserialize(vbs::Cdr& cdr);

    void swap(res_type& other_);

    std::string to_idl_string(std::set<std::string>* processedTypes = nullptr, const bool is_top = true) const;

    static std::string get_type_name();

    void reset_all_member();
    friend vbs_user_DllExport std::ostream& operator<<(std::ostream& o, const res_type& sample);

    void VBSVersionPrinter() { std::cout << "VBS IDL Gen Version 1.3.0-20240522-001" << std::endl; }

    void VBSRpcPrinter() { std::cout << "This is Rpc code." << std::endl; }

 private:
    int32_t m_sum;
};
#pragma pack(pop)

inline void swap(res_type& a, res_type& b) {
    a.swap(b);
}

namespace vbs {
template <>
struct rpc_type_support<res_type> {

    static bool ToBuffer(const res_type& sample, std::vector<char>& buffer);

    static bool FromBuffer(res_type& sample, const std::vector<char>& buffer);
};

std::string data_to_json_string(const res_type& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = true, const bool include_braces = true);
}  // namespace vbs

#endif  // _EDDS_GENERATED_168448424_RPCDEMO_H_
