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

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace {
char dummy;
}  // namespace
#endif  // _WIN32

#include "RpcDiscovery.h"
#include <sstream>
#include <ecdr/Cdr.h>

#include <ecdr/exceptions/BadParamException.h>
using namespace vbsutil::ecdr::exception;
#include <utility>

RpcDiscovery::RpcDiscovery() {
    // m_service_name std::string
    m_service_name = "";
    // m_interface_name std::string
    m_interface_name = "";
    // m_ip_address_port std::string
    m_ip_address_port = "";
    // m_transport_type uint32_t
    m_transport_type = 0;
    // m_hostid uint32_t
    m_hostid = 0;
    // m_operating_mode uint16_t
    m_operating_mode = 0;
}

RpcDiscovery::~RpcDiscovery() {}

RpcDiscovery::RpcDiscovery(const RpcDiscovery& x) {
    m_service_name = x.m_service_name;
    m_interface_name = x.m_interface_name;
    m_ip_address_port = x.m_ip_address_port;
    m_transport_type = x.m_transport_type;
    m_hostid = x.m_hostid;
    m_operating_mode = x.m_operating_mode;
}

RpcDiscovery::RpcDiscovery(RpcDiscovery&& x) {
    m_service_name = std::move(x.m_service_name);
    m_interface_name = std::move(x.m_interface_name);
    m_ip_address_port = std::move(x.m_ip_address_port);
    m_transport_type = x.m_transport_type;
    m_hostid = x.m_hostid;
    m_operating_mode = x.m_operating_mode;
}

RpcDiscovery::RpcDiscovery(const std::string& service_name, const std::string& interface_name,
                           const std::string& ip_address_port, const uint32_t& transport_type, const uint32_t& hostid,
                           const uint16_t& operating_mode) {
    m_service_name = service_name;
    m_interface_name = interface_name;
    m_ip_address_port = ip_address_port;
    m_transport_type = transport_type;
    m_hostid = hostid;
    m_operating_mode = operating_mode;
}
RpcDiscovery& RpcDiscovery::operator=(const RpcDiscovery& x) {

    if (this != &x) {

        m_service_name = x.m_service_name;
        m_interface_name = x.m_interface_name;
        m_ip_address_port = x.m_ip_address_port;
        m_transport_type = x.m_transport_type;
        m_hostid = x.m_hostid;
        m_operating_mode = x.m_operating_mode;
    }
    return *this;
}

RpcDiscovery& RpcDiscovery::operator=(RpcDiscovery&& x) {

    m_service_name = std::move(x.m_service_name);
    m_interface_name = std::move(x.m_interface_name);
    m_ip_address_port = std::move(x.m_ip_address_port);
    m_transport_type = x.m_transport_type;
    m_hostid = x.m_hostid;
    m_operating_mode = x.m_operating_mode;

    return *this;
}

bool RpcDiscovery::operator==(const RpcDiscovery& x) const {

    return (m_service_name == x.m_service_name && m_interface_name == x.m_interface_name &&
            m_ip_address_port == x.m_ip_address_port && m_transport_type == x.m_transport_type &&
            m_hostid == x.m_hostid && m_operating_mode == x.m_operating_mode);
}

bool RpcDiscovery::operator!=(const RpcDiscovery& x) const {
    return !(*this == x);
}

void RpcDiscovery::swap(RpcDiscovery& other_) {
    std::swap(m_service_name, other_.m_service_name);
    std::swap(m_interface_name, other_.m_interface_name);
    std::swap(m_ip_address_port, other_.m_ip_address_port);
    std::swap(m_transport_type, other_.m_transport_type);
    std::swap(m_hostid, other_.m_hostid);
    std::swap(m_operating_mode, other_.m_operating_mode);
}

size_t RpcDiscovery::getMaxCdrSerializedSize(size_t current_alignment) {
    static_cast<void>(current_alignment);
    return RpcDiscovery_max_cdr_typesize;
}

size_t RpcDiscovery::getCdrSerializedSize(const RpcDiscovery& data, size_t current_alignment) {
    (void)data;
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4) + data.service_name().size() + 1;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4) + data.interface_name().size() + 1;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4) + data.ip_address_port().size() + 1;

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + vbsutil::ecdr::Cdr::alignment(current_alignment, 4);

    current_alignment += 2 + vbsutil::ecdr::Cdr::alignment(current_alignment, 2);

    return current_alignment - initial_alignment;
}

size_t RpcDiscovery::getObjectDataSize() {
    return getMaxCdrSerializedSize();
}

void RpcDiscovery::serialize(vbsutil::ecdr::Cdr& scdr) const {

    scdr << m_service_name.c_str();
    scdr << m_interface_name.c_str();
    scdr << m_ip_address_port.c_str();
    scdr << m_transport_type;
    scdr << m_hostid;
    scdr << m_operating_mode;
}

void RpcDiscovery::deserialize(vbsutil::ecdr::Cdr& dcdr) {

    dcdr >> m_service_name;
    dcdr >> m_interface_name;
    dcdr >> m_ip_address_port;
    dcdr >> m_transport_type;
    dcdr >> m_hostid;
    dcdr >> m_operating_mode;
}

/*!
 * @brief This function sets a value in member service_name
 * @param _service_name const reference for member service_name
 */
void RpcDiscovery::service_name(const std::string& _service_name) {
    m_service_name = _service_name;
}

/*!
 * @brief This function moves the value in member service_name
 * @param _service_name New value to be moved in member service_name
 */
void RpcDiscovery::service_name(std::string&& _service_name) {
    m_service_name = _service_name;
}

/*!
 * @brief This function returns a reference to member service_name
 * @return Reference to member service_name
 */
std::string& RpcDiscovery::service_name() {
    return m_service_name;
}

/*!
 * @brief This function returns a constant reference to member service_name
 * @return Constant reference to member service_name
 */
const std::string& RpcDiscovery::service_name() const {
    return m_service_name;
}
/*!
 * @brief This function sets a value in member interface_name
 * @param _interface_name const reference for member interface_name
 */
void RpcDiscovery::interface_name(const std::string& _interface_name) {
    m_interface_name = _interface_name;
}

/*!
 * @brief This function moves the value in member interface_name
 * @param _interface_name New value to be moved in member interface_name
 */
void RpcDiscovery::interface_name(std::string&& _interface_name) {
    m_interface_name = _interface_name;
}

/*!
 * @brief This function returns a reference to member interface_name
 * @return Reference to member interface_name
 */
std::string& RpcDiscovery::interface_name() {
    return m_interface_name;
}

/*!
 * @brief This function returns a constant reference to member interface_name
 * @return Constant reference to member interface_name
 */
const std::string& RpcDiscovery::interface_name() const {
    return m_interface_name;
}
/*!
 * @brief This function sets a value in member ip_address_port
 * @param _ip_address_port const reference for member ip_address_port
 */
void RpcDiscovery::ip_address_port(const std::string& _ip_address_port) {
    m_ip_address_port = _ip_address_port;
}

/*!
 * @brief This function moves the value in member ip_address_port
 * @param _ip_address_port New value to be moved in member ip_address_port
 */
void RpcDiscovery::ip_address_port(std::string&& _ip_address_port) {
    m_ip_address_port = _ip_address_port;
}

/*!
 * @brief This function returns a reference to member ip_address_port
 * @return Reference to member ip_address_port
 */
std::string& RpcDiscovery::ip_address_port() {
    return m_ip_address_port;
}

/*!
 * @brief This function returns a constant reference to member ip_address_port
 * @return Constant reference to member ip_address_port
 */
const std::string& RpcDiscovery::ip_address_port() const {
    return m_ip_address_port;
}
/*!
 * @brief This function sets a value in member transport_type
 * @param _transport_type const reference for member transport_type
 */
void RpcDiscovery::transport_type(const uint32_t& _transport_type) {
    m_transport_type = _transport_type;
}

/*!
 * @brief This function moves the value in member transport_type
 * @param _transport_type New value to be moved in member transport_type
 */
void RpcDiscovery::transport_type(uint32_t&& _transport_type) {
    m_transport_type = _transport_type;
}

/*!
 * @brief This function returns a reference to member transport_type
 * @return Reference to member transport_type
 */
uint32_t& RpcDiscovery::transport_type() {
    return m_transport_type;
}

/*!
 * @brief This function returns the value of member transport_type
 * @return Value of member transport_type
 */
uint32_t RpcDiscovery::transport_type() const {
    return m_transport_type;
}
/*!
 * @brief This function sets a value in member hostid
 * @param _hostid const reference for member hostid
 */
void RpcDiscovery::hostid(const uint32_t& _hostid) {
    m_hostid = _hostid;
}

/*!
 * @brief This function moves the value in member hostid
 * @param _hostid New value to be moved in member hostid
 */
void RpcDiscovery::hostid(uint32_t&& _hostid) {
    m_hostid = _hostid;
}

/*!
 * @brief This function returns a reference to member hostid
 * @return Reference to member hostid
 */
uint32_t& RpcDiscovery::hostid() {
    return m_hostid;
}

/*!
 * @brief This function returns the value of member hostid
 * @return Value of member hostid
 */
uint32_t RpcDiscovery::hostid() const {
    return m_hostid;
}
/*!
 * @brief This function sets a value in member operating_mode
 * @param _operating_mode const reference for member operating_mode
 */
void RpcDiscovery::operating_mode(const uint16_t& _operating_mode) {
    m_operating_mode = _operating_mode;
}

/*!
 * @brief This function moves the value in member operating_mode
 * @param _operating_mode New value to be moved in member operating_mode
 */
void RpcDiscovery::operating_mode(uint16_t&& _operating_mode) {
    m_operating_mode = _operating_mode;
}

/*!
 * @brief This function returns a reference to member operating_mode
 * @return Reference to member operating_mode
 */
uint16_t& RpcDiscovery::operating_mode() {
    return m_operating_mode;
}

/*!
 * @brief This function returns the value of member operating_mode
 * @return Value of member operating_mode
 */
uint16_t RpcDiscovery::operating_mode() const {
    return m_operating_mode;
}

size_t RpcDiscovery::getKeyMaxCdrSerializedSize(size_t current_alignment) {
    size_t current_align = current_alignment;

    return current_align;
}
bool RpcDiscovery::isKeyDefined() {
    return false;
}

void RpcDiscovery::serializeKey(vbsutil::ecdr::Cdr& scdr) const {
    (void)scdr;
}

std::ostream& operator<<(std::ostream& o, const RpcDiscovery& sample) {

    o << "{\n";

    o << "service_name: " << sample.service_name() << "\n";

    o << "interface_name: " << sample.interface_name() << "\n";

    o << "ip_address_port: " << sample.ip_address_port() << "\n";

    o << "transport_type: " << sample.transport_type() << "\n";

    o << "hostid: " << sample.hostid() << "\n";

    o << "operating_mode: " << sample.operating_mode() << "\n";

    o << "}\n";

    return o;
}

std::string RpcDiscovery::get_type_name() {
    return "RpcDiscovery";
}

std::string RpcDiscovery::to_idl_string(std::set<std::string>* processedTypesPtr, const bool is_top) const {
    if (!processedTypesPtr && is_top) {
        processedTypesPtr = new std::set<std::string>();
    }
    if (processedTypesPtr && (processedTypesPtr->find("RpcDiscovery") == processedTypesPtr->end())) {
        processedTypesPtr->insert("RpcDiscovery");
    } else {
        if (!is_top) {
            return "";
        }
    }
    std::ostringstream oss;

    oss << "struct RpcDiscovery {\n"

           "    string service_name;\n"

           "    string interface_name;\n"

           "    string ip_address_port;\n"

           "    unsigned long transport_type;\n"

           "    unsigned long hostid;\n"

           "    unsigned short operating_mode;\n"
           "};\n";

    if (is_top) {
        delete processedTypesPtr;
    }
    return oss.str();
}
