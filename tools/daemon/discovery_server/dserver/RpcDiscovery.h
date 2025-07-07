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

#ifndef _EDDS_GENERATED_1647164140_RPCDISCOVERY_H_
#define _EDDS_GENERATED_1647164140_RPCDISCOVERY_H_

//

#include "SafeEnum.h"

#include <ertps/utils/fixed_size_string.hpp>

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <iostream>
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
#if defined(RpcDiscovery_SOURCE)
#define RpcDiscovery_DllAPI __declspec(dllexport)
#else
#define RpcDiscovery_DllAPI __declspec(dllimport)
#endif  // RpcDiscovery_SOURCE
#else
#define RpcDiscovery_DllAPI
#endif  // VBS_USER_DLL_EXPORT
#else
#define RpcDiscovery_DllAPI
#endif  // _WIN32

namespace vbsutil {
namespace ecdr {
class Cdr;
}  // namespace ecdr
}  // namespace vbsutil

namespace vbs {}

constexpr int64_t RpcDiscovery_max_cdr_typesize {794ULL};
constexpr int64_t RpcDiscovery_max_key_cdr_typesize {0ULL};

/*!
 * @brief This class represents the structure RpcDiscovery defined by the user in the IDL file.
 * @ingroup RPCDISCOVERY
 */

class RpcDiscovery {
 public:
    /*!
     * @brief Default constructor.
     */
    vbs_user_DllExport RpcDiscovery();

    /*!
     * @brief Default destructor.
     */
    vbs_user_DllExport ~RpcDiscovery();

    /*!
     * @brief Copy constructor.
     * @param x Reference to the object RpcDiscovery that will be copied.
     */
    vbs_user_DllExport RpcDiscovery(const RpcDiscovery& x);

    /*!
     * @brief Move constructor.
     * @param x Reference to the object RpcDiscovery that will be copied.
     */
    vbs_user_DllExport RpcDiscovery(RpcDiscovery&& x);

    /*!
     * @brief Move constructor.
     * @param x All parameters to the object RpcDiscovery that will be copied.
     */
    vbs_user_DllExport RpcDiscovery(const std::string& service_name, const std::string& interface_name,
                                    const std::string& ip_address_port, const uint32_t& transport_type,
                                    const uint32_t& hostid, const uint16_t& operating_mode);
    /*!
     * @brief Copy assignment.
     * @param x Reference to the object RpcDiscovery that will be copied.
     */
    vbs_user_DllExport RpcDiscovery& operator=(const RpcDiscovery& x);

    /*!
     * @brief Move assignment.
     * @param x Reference to the object RpcDiscovery that will be copied.
     */
    vbs_user_DllExport RpcDiscovery& operator=(RpcDiscovery&& x);

    /*!
     * @brief Comparison operator.
     * @param x RpcDiscovery object to compare.
     */
    vbs_user_DllExport bool operator==(const RpcDiscovery& x) const;

    /*!
     * @brief Comparison operator.
     * @param x RpcDiscovery object to compare.
     */
    vbs_user_DllExport bool operator!=(const RpcDiscovery& x) const;

    /*!
     * @brief This function returns a const reference to member service_name
     * @param _service_name New value to be copied in member service_name
     */
    vbs_user_DllExport void service_name(const std::string& _service_name);
    /*!
     * @brief This function moves the value in member service_name
     * @param _service_name New value to be moved in member service_name
     */
    vbs_user_DllExport void service_name(std::string&& _service_name);

    /*!
     * @brief This function returns a reference to member service_name
     * @return Reference to member service_name
     */
    vbs_user_DllExport std::string& service_name();

    /*!
     * @brief This function returns a constant reference to member service_name
     * @return Constant reference to member service_name
     */
    vbs_user_DllExport const std::string& service_name() const;

    /*!
     * @brief This function returns a const reference to member interface_name
     * @param _interface_name New value to be copied in member interface_name
     */
    vbs_user_DllExport void interface_name(const std::string& _interface_name);
    /*!
     * @brief This function moves the value in member interface_name
     * @param _interface_name New value to be moved in member interface_name
     */
    vbs_user_DllExport void interface_name(std::string&& _interface_name);

    /*!
     * @brief This function returns a reference to member interface_name
     * @return Reference to member interface_name
     */
    vbs_user_DllExport std::string& interface_name();

    /*!
     * @brief This function returns a constant reference to member interface_name
     * @return Constant reference to member interface_name
     */
    vbs_user_DllExport const std::string& interface_name() const;

    /*!
     * @brief This function returns a const reference to member ip_address_port
     * @param _ip_address_port New value to be copied in member ip_address_port
     */
    vbs_user_DllExport void ip_address_port(const std::string& _ip_address_port);
    /*!
     * @brief This function moves the value in member ip_address_port
     * @param _ip_address_port New value to be moved in member ip_address_port
     */
    vbs_user_DllExport void ip_address_port(std::string&& _ip_address_port);

    /*!
     * @brief This function returns a reference to member ip_address_port
     * @return Reference to member ip_address_port
     */
    vbs_user_DllExport std::string& ip_address_port();

    /*!
     * @brief This function returns a constant reference to member ip_address_port
     * @return Constant reference to member ip_address_port
     */
    vbs_user_DllExport const std::string& ip_address_port() const;

    /*!
     * @brief This function returns a const reference to member transport_type
     * @param _transport_type New value to be copied in member transport_type
     */
    vbs_user_DllExport void transport_type(const uint32_t& _transport_type);
    /*!
     * @brief This function moves the value in member transport_type
     * @param _transport_type New value to be moved in member transport_type
     */
    vbs_user_DllExport void transport_type(uint32_t&& _transport_type);

    /*!
     * @brief This function returns a reference to member transport_type
     * @return Reference to member transport_type
     */
    vbs_user_DllExport uint32_t& transport_type();

    /*!
    * @brief This function returns the value of member transport_type
    * @return Value of member transport_type
    */
    vbs_user_DllExport uint32_t transport_type() const;

    /*!
     * @brief This function returns a const reference to member hostid
     * @param _hostid New value to be copied in member hostid
     */
    vbs_user_DllExport void hostid(const uint32_t& _hostid);
    /*!
     * @brief This function moves the value in member hostid
     * @param _hostid New value to be moved in member hostid
     */
    vbs_user_DllExport void hostid(uint32_t&& _hostid);

    /*!
     * @brief This function returns a reference to member hostid
     * @return Reference to member hostid
     */
    vbs_user_DllExport uint32_t& hostid();

    /*!
    * @brief This function returns the value of member hostid
    * @return Value of member hostid
    */
    vbs_user_DllExport uint32_t hostid() const;

    /*!
     * @brief This function returns a const reference to member operating_mode
     * @param _operating_mode New value to be copied in member operating_mode
     */
    vbs_user_DllExport void operating_mode(const uint16_t& _operating_mode);
    /*!
     * @brief This function moves the value in member operating_mode
     * @param _operating_mode New value to be moved in member operating_mode
     */
    vbs_user_DllExport void operating_mode(uint16_t&& _operating_mode);

    /*!
     * @brief This function returns a reference to member operating_mode
     * @return Reference to member operating_mode
     */
    vbs_user_DllExport uint16_t& operating_mode();

    /*!
    * @brief This function returns the value of member operating_mode
    * @return Value of member operating_mode
    */
    vbs_user_DllExport uint16_t operating_mode() const;

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
    vbs_user_DllExport static size_t getCdrSerializedSize(const RpcDiscovery& data, size_t current_alignment = 0);

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
    vbs_user_DllExport void serialize(vbsutil::ecdr::Cdr& cdr) const;

    /*!
     * @brief This function deserializes an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void deserialize(vbsutil::ecdr::Cdr& cdr);

    /*!
     * @brief This function returns the maximum serialized size of the Key of an object
     * depending on the buffer alignment.
     * @param current_alignment Buffer alignment.
     * @return Maximum serialized size.
     */
    vbs_user_DllExport static size_t getKeyMaxCdrSerializedSize(size_t current_alignment = 0);
    /*!
     * @brief This function tells you if the Key has been defined for this type
     */
    vbs_user_DllExport static bool isKeyDefined();

    /*!
     * @brief This function serializes the key members of an object using CDR serialization.
     * @param cdr CDR serialization object.
     */
    vbs_user_DllExport void serializeKey(vbsutil::ecdr::Cdr& cdr) const;

    void swap(RpcDiscovery& other_);

    std::string to_idl_string(std::set<std::string>* processedTypes = nullptr, const bool is_top = true) const;

    static std::string get_type_name();

    friend vbs_user_DllExport std::ostream& operator<<(std::ostream& o, const RpcDiscovery& sample);

    void VBSVersionPrinter() { std::cout << "VBS IDL Gen Version 1.3.0-20240522-001" << std::endl; }

    void VBSDdsPrinter() { std::cout << "This is Dds code." << std::endl; }

 private:
    std::string m_service_name;
    std::string m_interface_name;
    std::string m_ip_address_port;
    uint32_t m_transport_type;
    uint32_t m_hostid;
    uint16_t m_operating_mode;
};

inline void swap(RpcDiscovery& a, RpcDiscovery& b) {
    a.swap(b);
}

#endif  // _EDDS_GENERATED_1647164140_RPCDISCOVERY_H_
