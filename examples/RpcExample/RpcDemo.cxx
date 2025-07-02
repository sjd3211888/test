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

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace {
char dummy;
}  // namespace
#endif  // _WIN32

#include "RpcDemo.h"

#include <utility>

#ifndef _FIRST_DEFINE_DJS
#define _FIRST_DEFINE_DJS

template <typename T>
void printres_type(std::ostream& os, const T& t);
template <>
void printres_type(std::ostream& os, const uint8_t& value);

template <std::size_t N>
void printres_type(std::ostream& os, const vbs::fixed_string<N>& str);

template <typename T>
void vbs_print_osres_type(std::ostream& os, const T& t, const bool is_json = false);

template <>
inline void vbs_print_osres_type(std::ostream& os, const std::wstring& str, const bool is_json);

template <std::size_t N>
void vbs_print_osres_type(std::ostream& os, const vbs::fixed_string<N>& str, const bool is_json = false);

template <typename K>
void print_json_key(std::ostream& os, const K& key);

template <typename T, std::size_t N>
std::ostream& vbs_print_osres_type(std::ostream& os, const std::array<T, N>& arr, bool is_json = false);

template <typename T, std::size_t N>
void printres_type(std::ostream& os, const std::array<T, N>& arr);

template <typename T>
std::ostream& vbs_print_osres_type(std::ostream& os, const std::vector<T>& vec, bool is_json = false);

template <typename T>
void printres_type(std::ostream& os, const std::vector<T>& vec);

template <typename K, typename V>
std::ostream& vbs_print_osres_type(std::ostream& os, const std::map<K, V>& m, const bool is_json = false);

template <typename K, typename V>
void printres_type(std::ostream& os, const std::map<K, V>& m);

template <typename T>
void printreq_type(std::ostream& os, const T& t);
template <>
void printreq_type(std::ostream& os, const uint8_t& value);

template <std::size_t N>
void printreq_type(std::ostream& os, const vbs::fixed_string<N>& str);

template <typename T>
void vbs_print_osreq_type(std::ostream& os, const T& t, const bool is_json = false);

template <>
inline void vbs_print_osreq_type(std::ostream& os, const std::wstring& str, const bool is_json);

template <std::size_t N>
void vbs_print_osreq_type(std::ostream& os, const vbs::fixed_string<N>& str, const bool is_json = false);

template <typename K>
void print_json_key(std::ostream& os, const K& key);

template <typename T, std::size_t N>
std::ostream& vbs_print_osreq_type(std::ostream& os, const std::array<T, N>& arr, bool is_json = false);

template <typename T, std::size_t N>
void printreq_type(std::ostream& os, const std::array<T, N>& arr);

template <typename T>
std::ostream& vbs_print_osreq_type(std::ostream& os, const std::vector<T>& vec, bool is_json = false);

template <typename T>
void printreq_type(std::ostream& os, const std::vector<T>& vec);

template <typename K, typename V>
std::ostream& vbs_print_osreq_type(std::ostream& os, const std::map<K, V>& m, const bool is_json = false);

template <typename K, typename V>
void printreq_type(std::ostream& os, const std::map<K, V>& m);

namespace vbs {

inline void data_to_json_string(const bool& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);
inline void data_to_json_string(const char& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);
inline void data_to_json_string(const wchar_t& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);
inline void data_to_json_string(const std::string& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);

template <std::size_t N>
inline void data_to_json_string(const vbs::fixed_string<N>& str, std::ostringstream* o_ptr = nullptr,
                                const bool is_json = false);

inline void data_to_json_string(const int8_t& sample, std::ostringstream* o_ptr = nullptr, const bool is_json = false);

inline void data_to_json_string(const uint8_t& sample, std::ostringstream* o_ptr = nullptr, const bool is_json = false);

inline void data_to_json_string(const float& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);

inline void data_to_json_string(const double& sample, std::ostringstream* o_ptr = nullptr,
                                const bool is_need_delete = false);

template <typename T>
void data_to_json_string(const T& sample, std::ostringstream* o_ptr = nullptr, const bool is_need_delete = true,
                         const bool include_braces = true);

inline void data_to_json_string(const bool& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    *o_ptr << (sample ? "true" : "false");
    return;
}

inline void data_to_json_string(const char& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    unsigned char uvalue = static_cast<unsigned char>(sample);
    *o_ptr << "\"\\u" << std::setw(4) << std::setfill('0') << std::hex << static_cast<int>(uvalue) << "\"";
    return;
}

inline void data_to_json_string(const wchar_t& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    unsigned char uvalue = static_cast<unsigned char>(sample);
    *o_ptr << "\"\\u" << std::setw(4) << std::setfill('0') << std::hex << static_cast<int>(uvalue) << "\"";
    return;
}

inline void data_to_json_string(const std::string& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    *o_ptr << "\"" << sample << "\"";
    return;
}

template <std::size_t N>
inline void data_to_json_string(const vbs::fixed_string<N>& str, std::ostringstream* o_ptr, const bool is_json) {
    (void)is_json;
    *o_ptr << "\"" << str.to_string() << "\"";
}

inline void data_to_json_string(const int8_t& sample, std::ostringstream* o_ptr, const bool is_json) {
    (void)is_json;
    *o_ptr << static_cast<int>(sample);
}

inline void data_to_json_string(const uint8_t& sample, std::ostringstream* o_ptr, const bool is_json) {
    (void)is_json;
    *o_ptr << static_cast<int>(sample);
}

inline void data_to_json_string(const float& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    *o_ptr << std::setprecision(16);

    if (std::isnan(sample)) {
        *o_ptr << "nan";
    } else if (std::isinf(sample)) {
        *o_ptr << "inf";
    } else {
        float abs_sample = std::abs(sample);
        if (abs_sample >= 1e16f || (sample != 0.0f && abs_sample < 1e-16f)) {
            *o_ptr << std::scientific << sample;
        } else {
            *o_ptr << sample;
        }
    }
}

inline void data_to_json_string(const double& sample, std::ostringstream* o_ptr, const bool is_need_delete) {
    (void)is_need_delete;
    *o_ptr << std::setprecision(16);

    if (std::isnan(sample)) {
        *o_ptr << "nan";
    } else if (std::isinf(sample)) {
        *o_ptr << "inf";
    } else {
        double abs_sample = std::abs(sample);
        if (abs_sample >= 1e16 || (sample != 0.0 && abs_sample < 1e-16)) {
            *o_ptr << std::scientific << sample;
        } else {
            *o_ptr << sample;
        }
    }
}

template <typename T>
void data_to_json_string(const T& sample, std::ostringstream* o_ptr, const bool is_need_delete,
                         const bool include_braces) {
    (void)is_need_delete;
    (void)include_braces;
    *o_ptr << sample;
    return;
}

}  // namespace vbs
#endif

req_type::req_type() {

    // m_add1 int32_t
    m_add1 = 0;
    // m_add2 int32_t
    m_add2 = 0;
}

req_type::~req_type() {}

req_type::req_type(const req_type& x) {
    (m_add1) = x.m_add1;
    (m_add2) = x.m_add2;
}
req_type::req_type(req_type&& x) {
    (m_add1) = x.m_add1;
    (m_add2) = x.m_add2;
}

req_type::req_type(const int32_t& add1, const int32_t& add2) {
    (m_add1) = add1;
    (m_add2) = add2;
}
req_type& req_type::operator=(const req_type& x) {

    if (this != &x) {
        (m_add1) = x.m_add1;
        (m_add2) = x.m_add2;
    }
    return *this;
}
req_type& req_type::operator=(req_type&& x) {

    (m_add1) = x.m_add1;
    (m_add2) = x.m_add2;

    return *this;
}

bool req_type::operator==(const req_type& x) const {
    return (this->add1() == x.add1() && this->add2() == x.add2());
}

bool req_type::operator!=(const req_type& x) const {
    return !(*this == x);
}

void req_type::swap(req_type& other_) {
    std::swap(m_add1, other_.m_add1);
    std::swap(m_add2, other_.m_add2);
}

size_t req_type::getMaxCdrSerializedSize(size_t current_alignment) {
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}

size_t req_type::getCdrSerializedSize(const req_type& data, size_t current_alignment) {
    (void)data;
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}

size_t req_type::getObjectDataSize() {
    return sizeof(req_type);
}

void req_type::serialize(vbs::Cdr& scdr) const {

    scdr << m_add1;
    scdr << m_add2;
}

void req_type::deserialize(vbs::Cdr& dcdr) {

    dcdr >> m_add1;
    dcdr >> m_add2;
}

/*!
 * @brief This function sets a value in member add1
 * @param _add1 const reference for member add1
 */
void req_type::add1(const int32_t& _add1) {
    (m_add1) = _add1;
}

/*!
 * @brief This function moves the value in member add1
 * @param _add1 New value to be moved in member add1
 */
void req_type::add1(int32_t&& _add1) {
    (m_add1) = _add1;
}

/*!
 * @brief This function returns a reference to member add1
 * @return Reference to member add1
 */
int32_t& req_type::add1() {
    return (m_add1);
}

/*!
 * @brief This function returns the value of member add1
 * @return Value of member add1
 */
int32_t req_type::add1() const {
    return (m_add1);
}
/*!
 * @brief This function sets a value in member add2
 * @param _add2 const reference for member add2
 */
void req_type::add2(const int32_t& _add2) {
    (m_add2) = _add2;
}

/*!
 * @brief This function moves the value in member add2
 * @param _add2 New value to be moved in member add2
 */
void req_type::add2(int32_t&& _add2) {
    (m_add2) = _add2;
}

/*!
 * @brief This function returns a reference to member add2
 * @return Reference to member add2
 */
int32_t& req_type::add2() {
    return (m_add2);
}

/*!
 * @brief This function returns the value of member add2
 * @return Value of member add2
 */
int32_t req_type::add2() const {
    return (m_add2);
}

void req_type::reset_all_member() {

    ((m_add1)) = 0;

    ((m_add2)) = 0;
}

template <typename T>
void printreq_type(std::ostream& os, const T& t) {
    os << t;
}

template <typename T>
void vbs_print_osreq_type(std::ostream& os, const T& t, const bool is_json) {
    if (is_json) {
        std::ostringstream oss;
        vbs::data_to_json_string(t, &oss, false);
        os << oss.str();
    } else {
        printreq_type(os, t);
    }
}

template <>
void printreq_type(std::ostream& os, const uint8_t& value) {
    os << static_cast<int>(value);
}

std::ostream& operator<<(std::ostream& o, const req_type& sample) {

    o << "{\n";

    o << "add1: ";
    printreq_type(o, sample.add1());
    o << "\n";

    o << "add2: ";
    printreq_type(o, sample.add2());
    o << "\n";

    o << "}\n";

    return o;
}

std::string req_type::get_type_name() {
    return "req_type";
}

std::string req_type::to_idl_string(std::set<std::string>* processedTypesPtr, const bool is_top) const {
    if (!processedTypesPtr && is_top) {
        processedTypesPtr = new std::set<std::string>();
    }
    if (processedTypesPtr && (processedTypesPtr->find("req_type") == processedTypesPtr->end())) {
        processedTypesPtr->insert("req_type");
    } else {
        if (!is_top) {
            return "";
        }
    }
    std::ostringstream oss;

    oss << "struct req_type {\n"

           "    long add1;\n"

           "    long add2;\n"
           "};\n";

    if (is_top) {
        delete processedTypesPtr;
    }
    return oss.str();
}

namespace vbs {

bool rpc_type_support<req_type>::ToBuffer(const req_type& sample, std::vector<char>& buffer) {
    try {
        uint32_t len = static_cast<uint32_t>(
                           sample.getCdrSerializedSize(*static_cast<req_type*>(const_cast<req_type*>(&sample)))) +
                       4u /*encapsulation*/;
        buffer.resize(len);
        vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(buffer.data()), len);
        vbsutil::ecdr::Cdr ser(vbsbuffer);
        ser.serialize_encapsulation();
        sample.serialize(ser);
    } catch (vbsutil::ecdr::exception::Exception& /*exception*/) {
        return false;
    }
    return true;
}

bool rpc_type_support<req_type>::FromBuffer(req_type& sample, const std::vector<char>& buffer) {
    try {
        uint32_t len = buffer.size();
        vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(const_cast<char*>(buffer.data())), len);
        vbsutil::ecdr::Cdr deser(vbsbuffer);
        deser.read_encapsulation();
        sample.deserialize(deser);
    } catch (vbsutil::ecdr::exception::Exception& /*exception*/) {
        return false;
    }
    return true;
}
std::string data_to_json_string(const req_type& sample, std::ostringstream* o_ptr, const bool is_need_delete,
                                const bool include_braces) {
    if (o_ptr == nullptr && is_need_delete) {
        o_ptr = new std::ostringstream();
    }
    std::ostringstream& o = *o_ptr;
    if (include_braces)
        o << "{"
          << "\n";

    o << "\"add1\": ";
    ::vbs_print_osreq_type(o, sample.add1(), true);
    o << "\n";

    o << ",";
    o << "\"add2\": ";
    ::vbs_print_osreq_type(o, sample.add2(), true);
    o << "\n";

    if (include_braces)
        o << "}\n";
    if (!is_need_delete)
        return "";
    std::string tmp_res_str = o.str();

    if (o_ptr && is_need_delete) {
        delete o_ptr;
        o_ptr = nullptr;
    }
    return tmp_res_str;
}
}  // namespace vbs

res_type::res_type() {

    // m_sum int32_t
    m_sum = 0;
}

res_type::~res_type() {}

res_type::res_type(const res_type& x) {
    (m_sum) = x.m_sum;
}
res_type::res_type(res_type&& x) {
    (m_sum) = x.m_sum;
}

res_type::res_type(const int32_t& sum) {
    (m_sum) = sum;
}
res_type& res_type::operator=(const res_type& x) {

    if (this != &x) {
        (m_sum) = x.m_sum;
    }
    return *this;
}
res_type& res_type::operator=(res_type&& x) {

    (m_sum) = x.m_sum;

    return *this;
}

bool res_type::operator==(const res_type& x) const {
    return (this->sum() == x.sum());
}

bool res_type::operator!=(const res_type& x) const {
    return !(*this == x);
}

void res_type::swap(res_type& other_) {
    std::swap(m_sum, other_.m_sum);
}

size_t res_type::getMaxCdrSerializedSize(size_t current_alignment) {
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}

size_t res_type::getCdrSerializedSize(const res_type& data, size_t current_alignment) {
    (void)data;
    size_t initial_alignment = current_alignment;

    current_alignment += 4 + vbs::Cdr::alignment(current_alignment, 4);

    return current_alignment - initial_alignment;
}

size_t res_type::getObjectDataSize() {
    return sizeof(res_type);
}

void res_type::serialize(vbs::Cdr& scdr) const {

    scdr << m_sum;
}

void res_type::deserialize(vbs::Cdr& dcdr) {

    dcdr >> m_sum;
}

/*!
 * @brief This function sets a value in member sum
 * @param _sum const reference for member sum
 */
void res_type::sum(const int32_t& _sum) {
    (m_sum) = _sum;
}

/*!
 * @brief This function moves the value in member sum
 * @param _sum New value to be moved in member sum
 */
void res_type::sum(int32_t&& _sum) {
    (m_sum) = _sum;
}

/*!
 * @brief This function returns a reference to member sum
 * @return Reference to member sum
 */
int32_t& res_type::sum() {
    return (m_sum);
}

/*!
 * @brief This function returns the value of member sum
 * @return Value of member sum
 */
int32_t res_type::sum() const {
    return (m_sum);
}

void res_type::reset_all_member() {

    ((m_sum)) = 0;
}

template <typename T>
void printres_type(std::ostream& os, const T& t) {
    os << t;
}

template <typename T>
void vbs_print_osres_type(std::ostream& os, const T& t, const bool is_json) {
    if (is_json) {
        std::ostringstream oss;
        vbs::data_to_json_string(t, &oss, false);
        os << oss.str();
    } else {
        printres_type(os, t);
    }
}

template <>
void printres_type(std::ostream& os, const uint8_t& value) {
    os << static_cast<int>(value);
}

std::ostream& operator<<(std::ostream& o, const res_type& sample) {

    o << "{\n";

    o << "sum: ";
    printres_type(o, sample.sum());
    o << "\n";

    o << "}\n";

    return o;
}

std::string res_type::get_type_name() {
    return "res_type";
}

std::string res_type::to_idl_string(std::set<std::string>* processedTypesPtr, const bool is_top) const {
    if (!processedTypesPtr && is_top) {
        processedTypesPtr = new std::set<std::string>();
    }
    if (processedTypesPtr && (processedTypesPtr->find("res_type") == processedTypesPtr->end())) {
        processedTypesPtr->insert("res_type");
    } else {
        if (!is_top) {
            return "";
        }
    }
    std::ostringstream oss;

    oss << "struct res_type {\n"

           "    long sum;\n"
           "};\n";

    if (is_top) {
        delete processedTypesPtr;
    }
    return oss.str();
}

namespace vbs {

bool rpc_type_support<res_type>::ToBuffer(const res_type& sample, std::vector<char>& buffer) {
    try {
        uint32_t len = static_cast<uint32_t>(
                           sample.getCdrSerializedSize(*static_cast<res_type*>(const_cast<res_type*>(&sample)))) +
                       4u /*encapsulation*/;
        buffer.resize(len);
        vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(buffer.data()), len);
        vbsutil::ecdr::Cdr ser(vbsbuffer);
        ser.serialize_encapsulation();
        sample.serialize(ser);
    } catch (vbsutil::ecdr::exception::Exception& /*exception*/) {
        return false;
    }
    return true;
}

bool rpc_type_support<res_type>::FromBuffer(res_type& sample, const std::vector<char>& buffer) {
    try {
        uint32_t len = buffer.size();
        vbs::FastBuffer vbsbuffer(reinterpret_cast<char*>(const_cast<char*>(buffer.data())), len);
        vbsutil::ecdr::Cdr deser(vbsbuffer);
        deser.read_encapsulation();
        sample.deserialize(deser);
    } catch (vbsutil::ecdr::exception::Exception& /*exception*/) {
        return false;
    }
    return true;
}
std::string data_to_json_string(const res_type& sample, std::ostringstream* o_ptr, const bool is_need_delete,
                                const bool include_braces) {
    if (o_ptr == nullptr && is_need_delete) {
        o_ptr = new std::ostringstream();
    }
    std::ostringstream& o = *o_ptr;
    if (include_braces)
        o << "{"
          << "\n";

    o << "\"sum\": ";
    ::vbs_print_osres_type(o, sample.sum(), true);
    o << "\n";

    if (include_braces)
        o << "}\n";
    if (!is_need_delete)
        return "";
    std::string tmp_res_str = o.str();

    if (o_ptr && is_need_delete) {
        delete o_ptr;
        o_ptr = nullptr;
    }
    return tmp_res_str;
}
}  // namespace vbs
