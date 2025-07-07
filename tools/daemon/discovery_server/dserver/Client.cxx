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

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace {
char dummy;
}  // namespace
#endif  // _WIN32

#include "Client.hpp"
#include <ecdr/Cdr.h>

#include <ecdr/exceptions/BadParamException.h>
using namespace vbsutil::ecdr::exception;

#include <utility>

Client::Client() {
    // m_index com.evbs.idl.parser.typecode.PrimitiveTypeCode@627551fb
    m_index = 0;
    // m_message com.evbs.idl.parser.typecode.StringTypeCode@2758fe70
    m_message = "";
}

Client::~Client() {}

Client::Client(const Client& x) {
    m_index = x.m_index;
    m_message = x.m_message;
}

Client::Client(Client&& x) {
    m_index = x.m_index;
    m_message = std::move(x.m_message);
}

Client& Client::operator=(const Client& x) {
    m_index = x.m_index;
    m_message = x.m_message;

    return *this;
}

Client& Client::operator=(Client&& x) {
    m_index = x.m_index;
    m_message = std::move(x.m_message);

    return *this;
}

bool Client::operator==(const Client& x) const {
    return (m_index == x.m_index && m_message == x.m_message);
}

bool Client::operator!=(const Client& x) const {
    return !(*this == x);
}

size_t Client::getMaxCdrSerializedSize(size_t current_alignment) {
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4) + 255 + 1;

    return current_alignment - initial_alignment;
}

size_t Client::getCdrSerializedSize(const Client& data, size_t current_alignment) {
    (void)data;
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4) + data.message().size() + 1;

    return current_alignment - initial_alignment;
}

void Client::serialize(vbsutil::ecdr::Cdr& scdr) const {
    scdr << m_index;
    scdr << m_message.c_str();
}

void Client::deserialize(vbsutil::ecdr::Cdr& dcdr) {
    dcdr >> m_index;
    dcdr >> m_message;
}

/*!
 * @brief This function sets a value in member index
 * @param _index New value for member index
 */
void Client::index(uint32_t _index) {
    m_index = _index;
}

/*!
 * @brief This function returns the value of member index
 * @return Value of member index
 */
uint32_t Client::index() const {
    return m_index;
}

/*!
 * @brief This function returns a reference to member index
 * @return Reference to member index
 */
uint32_t& Client::index() {
    return m_index;
}

/*!
 * @brief This function copies the value in member message
 * @param _message New value to be copied in member message
 */
void Client::message(const std::string& _message) {
    m_message = _message;
}

/*!
 * @brief This function moves the value in member message
 * @param _message New value to be moved in member message
 */
void Client::message(std::string&& _message) {
    m_message = std::move(_message);
}

/*!
 * @brief This function returns a constant reference to member message
 * @return Constant reference to member message
 */
const std::string& Client::message() const {
    return m_message;
}

/*!
 * @brief This function returns a reference to member message
 * @return Reference to member message
 */
std::string& Client::message() {
    return m_message;
}

size_t Client::getKeyMaxCdrSerializedSize(size_t current_alignment) {
    size_t current_align = current_alignment;

    return current_align;
}

bool Client::isKeyDefined() {
    return false;
}

void Client::serializeKey(vbsutil::ecdr::Cdr& scdr) const {
    (void)scdr;
}
