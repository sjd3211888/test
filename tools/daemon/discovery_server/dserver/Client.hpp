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
// ------------------------------------------------------------------
// Modification history:
// feature: Code style modification
// ------------------------------------------------------------------

#ifndef _VBS_DDS_GENERATED_Client_H_
#define _VBS_DDS_GENERATED_Client_H_

#include <ertps/utils/fixed_size_string.hpp>

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>

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
#if defined(Client_SOURCE)
#define Client_DllAPI __declspec(dllexport)
#else
#define Client_DllAPI __declspec(dllimport)
#endif  // Client_SOURCE
#else
#define Client_DllAPI
#endif  // VBS_USER_DLL_EXPORT
#else
#define Client_DllAPI
#endif  // _WIN32

namespace vbsutil {
namespace ecdr {
class Cdr;
}  // namespace ecdr
}  // namespace vbsutil

/*!
 * @brief This class represents the structure Client defined by the user in the IDL file.
 * @ingroup Client
 */
class Client {
 public:
    /*!
     * @brief Default constructor.
     */
    vbs_user_DllExport Client();

    /*!
     * @brief Default destructor.
     */
    vbs_user_DllExport ~Client();

    /*!
     * @brief Copy constructor.
     * @param x Reference to the object Client that will be copied.
     */
    vbs_user_DllExport Client(const Client& x);

    /*!
     * @brief Move constructor.
     * @param x Reference to the object Client that will be copied.
     */
    vbs_user_DllExport Client(Client&& x);

    /*!
     * @brief Copy assignment.
     * @param x Reference to the object Client that will be copied.
     */
    vbs_user_DllExport Client& operator=(const Client& x);

    /*!
     * @brief Move assignment.
     * @param x Reference to the object Client that will be copied.
     */
    vbs_user_DllExport Client& operator=(Client&& x);

    /*!
     * @brief Comparison operator.
     * @param x Client object to compare.
     */
    vbs_user_DllExport bool operator==(const Client& x) const;

    /*!
     * @brief Comparison operator.
     * @param x Client object to compare.
     */
    vbs_user_DllExport bool operator!=(const Client& x) const;

    /*!
     * @brief This function sets a value in member index
     * @param _index New value for member index
     */
    vbs_user_DllExport void index(uint32_t _index);

    /*!
     * @brief This function returns the value of member index
     * @return Value of member index
     */
    vbs_user_DllExport uint32_t index() const;

    /*!
     * @brief This function returns a reference to member index
     * @return Reference to member index
     */
    vbs_user_DllExport uint32_t& index();

    /*!
     * @brief This function copies the value in member message
     * @param _message New value to be copied in member message
     */
    vbs_user_DllExport void message(const std::string& _message);

    /*!
     * @brief This function moves the value in member message
     * @param _message New value to be moved in member message
     */
    vbs_user_DllExport void message(std::string&& _message);

    /*!
     * @brief This function returns a constant reference to member message
     * @return Constant reference to member message
     */
    vbs_user_DllExport const std::string& message() const;

    /*!
     * @brief This function returns a reference to member message
     * @return Reference to member message
     */
    vbs_user_DllExport std::string& message();

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
    vbs_user_DllExport static size_t getCdrSerializedSize(const Client& data, size_t current_alignment = 0);

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

 private:
    uint32_t m_index;
    std::string m_message;
};

#endif  // _VBS_DDS_GENERATED_Client_H_
