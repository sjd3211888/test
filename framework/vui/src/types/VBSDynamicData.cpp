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

#include "ertps/xmlparser/XMLProfileManager.h"
#include "topic/DynamicPubSubTypeImpl.hpp"
#include "vbs/types/VBSDynamicData.h"
#include "vbs/types/LoanedDynamicData.hpp"
#include "core/Global.hpp"
#include "vbs/Utils.hpp"

namespace vbs {

VBSDynamicData::VBSDynamicData() : vbs_dynamic_data_(nullptr), is_complex_data_(true), is_value_(true) {
    load_idl("");
    parse_idl_to_xml("");
}

VBSDynamicData::VBSDynamicData(const std::string& type_name)
    : type_name_(type_name), vbs_dynamic_data_(nullptr), is_complex_data_(false), is_value_(true) {
    generate_type();
    create_data();
}

VBSDynamicData::VBSDynamicData(const std::string& type_name, void* data, const uint32_t& len)
    : type_name_(type_name), vbs_dynamic_data_(nullptr), is_complex_data_(false), is_value_(true) {
    generate_type();
    create_data();
    deserialize(data, len);
}

VBSDynamicData::VBSDynamicData(const VBSDynamicData& data)
    : type_name_(data.type_name_),
      vbs_dynamic_data_(data.vbs_dynamic_data_),
      is_complex_data_(true),
      is_value_(data.is_value_) {
    if (data.vbs_dynamic_data_ == nullptr) {
        return;
    }
    type_ = data.type_;
    is_value_ = true;
    vbs_dynamic_data_ = EvbsDynamicDataFactory::get_instance()->create_copy(data.vbs_dynamic_data_);
}

VBSDynamicData& VBSDynamicData::operator=(const VBSDynamicData& data) {
    if (is_value_) {
        delete_data();
    }

    type_name_ = data.type_name_;
    vbs_dynamic_data_ = data.vbs_dynamic_data_;
    is_complex_data_ = true;
    is_value_ = data.is_value_;
    if (data.vbs_dynamic_data_ == nullptr) {
        return *this;
    }
    type_ = data.type_;
    is_value_ = true;
    vbs_dynamic_data_ = EvbsDynamicDataFactory::get_instance()->create_copy(data.vbs_dynamic_data_);
    return *this;
}

VBSDynamicData::VBSDynamicData(VBSDynamicData&& data)
    : type_name_(data.type_name_),
      vbs_dynamic_data_(data.vbs_dynamic_data_),
      is_complex_data_(true),
      is_value_(data.is_value_) {
    if (data.vbs_dynamic_data_ == nullptr) {
        return;
    }
    type_ = data.type_;
    is_value_ = true;
    vbs_dynamic_data_ = EvbsDynamicDataFactory::get_instance()->create_copy(data.vbs_dynamic_data_);
}

VBSDynamicData& VBSDynamicData::operator=(VBSDynamicData&& data) {
    if (is_value_) {
        delete_data();
    }

    type_name_ = data.type_name_;
    vbs_dynamic_data_ = data.vbs_dynamic_data_;
    is_complex_data_ = true;
    is_value_ = data.is_value_;
    if (data.vbs_dynamic_data_ == nullptr) {
        return *this;
    }
    type_ = data.type_;
    is_value_ = true;
    vbs_dynamic_data_ = EvbsDynamicDataFactory::get_instance()->create_copy(data.vbs_dynamic_data_);
    return *this;
}

VBSDynamicData::~VBSDynamicData() {
    if (is_value_) {
        delete_data();
    }
}

bool VBSDynamicData::is_valid() const {
    return (vbs_dynamic_data_ != nullptr);
}

void VBSDynamicData::deserialize(const void* src, const uint32_t& len) {
    vbs::SerializedPayload_t payload;
    payload.const_data(reinterpret_cast<const vbs::octet*>(src));
    payload.length = len;
    payload.max_size = len;
    type_->deserialize(&payload, vbs_dynamic_data_);
}

void VBSDynamicData::serialize(void* dst, const uint32_t& len) const {
    vbs::SerializedPayload_t payload;
    payload.data = reinterpret_cast<vbs::octet*>(dst);
    payload.length = len;
    payload.max_size = len;
    type_->serialize(vbs_dynamic_data_, &payload);
    payload.data = nullptr;
}

uint32_t VBSDynamicData::get_size() const {
    return type_->getSerializedSizeProvider(vbs_dynamic_data_)();
}

void VBSDynamicData::create_data() {
    if (is_complex_data_)
        return;
    vbs_dynamic_data_ = reinterpret_cast<vbs::DynamicData*>(type_.create_data());
}

void VBSDynamicData::delete_data() {
    if (is_complex_data_) {
        EvbsDynamicDataFactory::get_instance()->delete_data(vbs_dynamic_data_);
    } else {
        type_.delete_data(vbs_dynamic_data_);
    }
}

bool VBSDynamicData::load_xml(const std::string& xml_path) {
    evbs::ertps::xmlparser::XMLP_ret ret = evbs::ertps::xmlparser::XMLProfileManager::loadXMLFile(xml_path);
    if (ret == evbs::ertps::xmlparser::XMLP_ret::XML_OK) {
        return true;
    }
    return false;
}

bool VBSDynamicData::load_xml(const char* data, size_t length) {
    evbs::ertps::xmlparser::XMLP_ret ret = evbs::ertps::xmlparser::XMLProfileManager::loadXMLString(data, length);
    if (ret == evbs::ertps::xmlparser::XMLP_ret::XML_OK) {
        return true;
    }
    return false;
}

void VBSDynamicData::load_idl(const std::string& idl_path) {
    (void)idl_path;
}

std::string VBSDynamicData::parse_idl_to_xml(const std::string& idl_path) {
    (void)idl_path;
    return "";
}

void VBSDynamicData::generate_type() {
    if (is_complex_data_)
        return;
    auto const dyn_builder = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name_);
    if (dyn_builder == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                << "generate dynamic type builder failed. type name: " << type_name_);
        return;
    }
    EvbsDynamicType_ptr const dyn_type = dyn_builder->build();
    if (dyn_type == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                << "generate dynamic type failed. type name: " << type_name_);
        return;
    }
    type_ = EvbsTypeSupport(new vbs::VbsDynamicPubSubTypeImpl(dyn_type));
}

VBSDynamicType VBSDynamicData::get_type() const {
    VBSDynamicType type;
    if (vbs_dynamic_data_ != nullptr) {
        type.dynamic_type_ = std::make_shared<DynamicType_ptr>(vbs_dynamic_data_->get_type());
    }
    return type;
}

VBSDynamicType VBSDynamicData::get_dynamic_type(const std::string& type_name) {
    auto dyn_builder = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name);
    VBSDynamicType type;
    if (dyn_builder != nullptr) {
        EvbsDynamicType_ptr dyn_type = dyn_builder->build();
        type.dynamic_type_ = std::make_shared<DynamicType_ptr>(dyn_type);
    }
    return type;
}

std::string VBSDynamicData::get_type_name() const {
    return type_name_;
}

EvbsTypeSupport VBSDynamicData::get_type(const std::string& type_name) {
    auto dyn_builder = evbs::ertps::xmlparser::XMLProfileManager::getDynamicTypeByName(type_name);
    EvbsDynamicType_ptr dyn_type = dyn_builder->build();
    return EvbsTypeSupport(new vbs::VbsDynamicPubSubTypeImpl(std::move(dyn_type)));
    ;
}

ReturnCode_t VBSDynamicData::clear_all_values() {
    return ReturnCode_t(vbs_dynamic_data_->clear_all_values()());
}

ReturnCode_t VBSDynamicData::clear_nonkey_values() {
    return ReturnCode_t(vbs_dynamic_data_->clear_nonkey_values()());
}

ReturnCode_t VBSDynamicData::clear_value(uint32_t id) {
    return ReturnCode_t(vbs_dynamic_data_->clear_value(id)());
}

bool VBSDynamicData::equals(const VBSDynamicData* other) const {
    return vbs_dynamic_data_->equals(other->vbs_dynamic_data_);
}

TypeKind VBSDynamicData::get_kind() const {
    return VBSDynamicType::ToVbsTypeKind(vbs_dynamic_data_->get_kind());
}

uint32_t VBSDynamicData::get_item_count() const {
    return vbs_dynamic_data_->get_item_count();
}

uint32_t VBSDynamicData::get_member_id_by_name(const std::string& name) const {
    return vbs_dynamic_data_->get_member_id_by_name(name);
}

uint32_t VBSDynamicData::get_member_id_at_index(const uint32_t& index) const {
    return vbs_dynamic_data_->get_member_id_at_index(index);
}

VBSDynamicData* VBSDynamicData::loan_value(uint32_t id) const {
    VBSDynamicData* data = new VBSDynamicData();
    data->vbs_dynamic_data_ = vbs_dynamic_data_->loan_value(id);
    data->is_value_ = false;
    if (data->vbs_dynamic_data_ != nullptr) {
        data->type_name_ = data->vbs_dynamic_data_->get_type()->get_name();
    }
    return data;
}

VBSDynamicData* VBSDynamicData::loan_value(const std::string& name) const {
    VBSDynamicData* data = new VBSDynamicData();
    uint32_t member_id = vbs_dynamic_data_->get_member_id_by_name(name);
    data->vbs_dynamic_data_ = vbs_dynamic_data_->loan_value(member_id);
    data->is_value_ = false;
    if (data->vbs_dynamic_data_ != nullptr) {
        data->type_name_ = data->vbs_dynamic_data_->get_type()->get_name();
    }
    return data;
}

LoanedDynamicData VBSDynamicData::loan_value_obj(uint32_t id) const {
    return LoanedDynamicData(loan_value(id), (VBSDynamicData*)this);
}

LoanedDynamicData VBSDynamicData::loan_value_obj(const std::string& name) const {
    uint32_t member_id = vbs_dynamic_data_->get_member_id_by_name(name);
    return LoanedDynamicData(loan_value(member_id), (VBSDynamicData*)this);
}

ReturnCode_t VBSDynamicData::return_loaned_value(VBSDynamicData* value) {
    vbs_dynamic_data_->return_loaned_value(value->vbs_dynamic_data_);
    delete value;
    value = nullptr;
    return ReturnCode_t(ReturnCode_t::RETCODE_OK);
}

uint32_t VBSDynamicData::get_array_index(const std::vector<uint32_t>& position) {
    return vbs_dynamic_data_->get_array_index(position);
}

ReturnCode_t VBSDynamicData::insert_sequence_data(uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_sequence_data(outId)());
}

ReturnCode_t VBSDynamicData::insert_int32_value(int32_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_int32_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_uint32_value(uint32_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_uint32_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_int16_value(int16_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_int16_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_int8_value(int8_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_int8_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_uint16_value(uint16_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_uint16_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_int64_value(int64_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_int64_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_uint64_value(uint64_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_uint64_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_float32_value(float value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_float32_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_float64_value(double value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_float64_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_float128_value(long double value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_float128_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_char8_value(char value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_char8_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_char16_value(wchar_t value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_char16_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_byte_value(octet value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_byte_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_bool_value(bool value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_bool_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_string_value(const std::string& value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_string_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_wstring_value(const std::wstring& value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_wstring_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_enum_value(const std::string& value, uint32_t& outId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_enum_value(value, outId)());
}

ReturnCode_t VBSDynamicData::insert_complex_value(const VBSDynamicData* value, uint32_t& outId) {
    uint32_t res = vbs_dynamic_data_->insert_complex_value(value->vbs_dynamic_data_, outId)();
    if (res == 0) {
        value->is_value_ = false;
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(res);
}

ReturnCode_t VBSDynamicData::insert_complex_value(VBSDynamicData* value, uint32_t& outId) {
    uint32_t res = vbs_dynamic_data_->insert_complex_value(value->vbs_dynamic_data_, outId)();
    if (res == 0) {
        value->is_value_ = false;
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(res);
}

ReturnCode_t VBSDynamicData::remove_sequence_data(uint32_t id) {
    return ReturnCode_t(vbs_dynamic_data_->remove_sequence_data(id)());
}

ReturnCode_t VBSDynamicData::clear_data() {
    return ReturnCode_t(vbs_dynamic_data_->clear_data()());
}

ReturnCode_t VBSDynamicData::clear_array_data(uint32_t indexId) {
    return ReturnCode_t(vbs_dynamic_data_->clear_array_data(indexId)());
}

ReturnCode_t VBSDynamicData::insert_map_data(const VBSDynamicData* key, uint32_t& outKeyId, uint32_t& outValueId) {
    return ReturnCode_t(vbs_dynamic_data_->insert_map_data(key->vbs_dynamic_data_, outKeyId, outValueId)());
}

ReturnCode_t VBSDynamicData::insert_map_data(const VBSDynamicData* key, VBSDynamicData* value, uint32_t& outKey,
                                             uint32_t& outValue) {
    uint32_t res =
        vbs_dynamic_data_->insert_map_data(key->vbs_dynamic_data_, value->vbs_dynamic_data_, outKey, outValue)();
    if (res == 0) {
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(res);
}

ReturnCode_t VBSDynamicData::insert_map_data(const VBSDynamicData* key, const VBSDynamicData* value, uint32_t& outKey,
                                             uint32_t& outValue) {
    uint32_t res =
        vbs_dynamic_data_->insert_map_data(key->vbs_dynamic_data_, value->vbs_dynamic_data_, outKey, outValue)();
    if (res == 0) {
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(res);
}

ReturnCode_t VBSDynamicData::remove_map_data(uint32_t keyId) {
    return ReturnCode_t(vbs_dynamic_data_->remove_map_data(keyId)());
}

ReturnCode_t VBSDynamicData::get_int32_value(int32_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_int32_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_int32_value(int32_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_int32_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_uint32_value(uint32_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_uint32_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_uint32_value(uint32_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_uint32_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_int8_value(int8_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_int8_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_int16_value(int16_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_int16_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_int16_value(int16_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_int16_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_uint16_value(uint16_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_uint16_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_uint16_value(uint16_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_uint16_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_int64_value(int64_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_int64_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_int64_value(int64_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_int64_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_uint64_value(uint64_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_uint64_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_uint64_value(uint64_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_uint64_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_float32_value(float& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_float32_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_float32_value(float value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_float32_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_float64_value(double& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_float64_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_float64_value(double value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_float64_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_float128_value(long double& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_float128_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_float128_value(long double value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_float128_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_char_value(char& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_char8_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_char_value(char value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_char8_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_char8_value(char& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_char8_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_char8_value(char value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_char8_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_char16_value(wchar_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_char16_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_char16_value(wchar_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_char16_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_byte_value(octet& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_byte_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_byte_value(octet value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_byte_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_int8_value(int8_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_int8_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_uint8_value(uint8_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_uint8_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_uint8_value(uint8_t value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_uint8_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_bool_value(bool& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_bool_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_bool_value(bool value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_bool_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_string_value(std::string& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_string_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_string_value(const std::string& value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_string_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_wstring_value(std::wstring& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_wstring_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_wstring_value(const std::wstring& value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_wstring_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_enum_value(std::string& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_enum_value(const std::string& value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_enum_value(uint32_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_enum_value(const uint32_t& value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_enum_value(int32_t& value, const uint32_t& id) const {
    return ReturnCode_t(vbs_dynamic_data_->get_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::set_enum_value(const int32_t& value, const uint32_t& id) {
    return ReturnCode_t(vbs_dynamic_data_->set_enum_value(value, id)());
}

ReturnCode_t VBSDynamicData::get_bitmask_value(uint64_t& value) const {
    return ReturnCode_t(vbs_dynamic_data_->get_bitmask_value(value)());
}

ReturnCode_t VBSDynamicData::set_bitmask_value(uint64_t value) {
    return ReturnCode_t(vbs_dynamic_data_->set_bitmask_value(value)());
}

std::shared_ptr<VBSDynamicData> VBSDynamicData::get_complex_value(const std::string& name) const {
    uint32_t member_id = vbs_dynamic_data_->get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return nullptr;
    }
    EvbsDynamicData* complex_dymamic;
    vbs_dynamic_data_->get_complex_value(&complex_dymamic, member_id);
    std::shared_ptr<VBSDynamicData> res_data = std::make_shared<VBSDynamicData>();
    res_data->vbs_dynamic_data_ = complex_dymamic;
    return res_data;
}

std::shared_ptr<VBSDynamicData> VBSDynamicData::get_complex_value(const uint32_t& id) const {
    EvbsDynamicData* complex_dymamic = nullptr;
    vbs_dynamic_data_->get_complex_value(&complex_dymamic, id);
    if (complex_dymamic == nullptr) {
        return nullptr;
    }
    std::shared_ptr<VBSDynamicData> res_data = std::make_shared<VBSDynamicData>();
    res_data->vbs_dynamic_data_ = complex_dymamic;
    return res_data;
}

ReturnCode_t VBSDynamicData::set_complex_value(VBSDynamicData* value, const uint32_t& id) {
    uint32_t res = vbs_dynamic_data_->set_complex_value(value->vbs_dynamic_data_, id)();
    if (res == 0) {
        value->is_value_ = false;
        return ReturnCode_t(ReturnCode_t::RETCODE_OK);
    }
    return ReturnCode_t(res);
}

ReturnCode_t VBSDynamicData::get_union_label(uint64_t& value) const {
    return ReturnCode_t(vbs_dynamic_data_->get_union_label(value)());
}

void VBSDynamicData::split_string_with_specified_char(const std::string& src, char c,
                                                      std::vector<std::string>& nameVec) const {
    int len = src.length();
    std::string tmp = "";
    for (int i = 0; i < len; i++) {
        if (src[i] == c) {
            nameVec.emplace_back(tmp);
            tmp = "";
        } else {
            tmp += src[i];
        }
        if (tmp != "" && i == len - 1) {
            nameVec.emplace_back(tmp);
        }
    }
}

void VBSDynamicData::back_tracing_return_loaned_value(EvbsDynamicData* target,
                                                      std::stack<EvbsDynamicData*>& btStack) const {
    if (btStack.empty()) {
        return;
    }
    if (target == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " target unexpected nullptr and btStack is not empty");
        return;
    }
    EvbsDynamicData* parent = btStack.top();
    parent->return_loaned_value(target);
    btStack.pop();
    return back_tracing_return_loaned_value(parent, btStack);
}

bool VBSDynamicData::back_tracing_loan_value(EvbsDynamicData* parent, std::vector<std::string>& nameVec, uint32_t index,
                                             std::stack<EvbsDynamicData*>& btStack) const {
    if (index == nameVec.size() - 1) {
        return true;
    }
    if (parent == nullptr) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                << " parent unexpected nullptr and index is not out of bound");
        return true;
    }
    uint32_t id = parent->get_member_id_by_name(nameVec[index]);
    if (id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_NOT_FIND)
                << "Not found member who's name is \'" << nameVec[index] << "\'");
        return false;
    }
    btStack.push(parent->loan_value(parent->get_member_id_by_name(nameVec[index])));
    return back_tracing_loan_value(btStack.top(), nameVec, index + 1, btStack);
}

template <>
bool VBSDynamicData::value<bool>(const std::string& name) const {
    bool val = false;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_bool_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's bool value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_bool_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's bool value failed");
        }
    }
    return val;
}

template <>
octet VBSDynamicData::value<octet>(const std::string& name) const {
    octet val = 0;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_byte_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's octet value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_byte_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's octet value failed");
        }
    }
    return val;
}

template <>
char VBSDynamicData::value<char>(const std::string& name) const {
    char val = '\0';
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_char8_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's char value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_char_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's char value failed");
        }
    }
    return val;
}

template <>
wchar_t VBSDynamicData::value<wchar_t>(const std::string& name) const {
    wchar_t val = L'\0';
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_char16_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's wchar_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_char16_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's wchar_t value failed");
        }
    }
    return val;
}

template <>
int8_t VBSDynamicData::value<int8_t>(const std::string& name) const {
    int8_t val = -1;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_int8_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's int8_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_int8_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's int8_t value failed");
        }
    }
    return val;
}

template <>
short VBSDynamicData::value<short>(const std::string& name) const {
    short val = -1;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_int16_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's short value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_int16_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's short value failed");
        }
    }
    return val;
}

template <>
unsigned short VBSDynamicData::value<unsigned short>(const std::string& name) const {
    unsigned short val = -1;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_uint16_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's unsigned short value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_uint16_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's unsigned short value failed");
        }
    }
    return val;
}

template <>
int VBSDynamicData::value<int>(const std::string& name) const {
    int val = -1;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_int32_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                ret = ReturnCode_t(target->get_enum_value(val, id)());
                if (ret() != ReturnCode_t::RETCODE_OK) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "get member \'" << name << "\' 's int value failed");
                }
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_int32_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            uint32_t enum_val = 0;
            ret = get_enum_value(enum_val, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's int value failed");
            }
            val = enum_val;
        }
    }
    return val;
}

template <>
unsigned int VBSDynamicData::value<unsigned int>(const std::string& name) const {
    unsigned int val = 0xFFFFFFFF;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_uint32_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                ret = ReturnCode_t(target->get_enum_value(val, id)());
                if (ret() != ReturnCode_t::RETCODE_OK) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "get member \'" << name << "\' 's unsigned int value failed");
                }
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_uint32_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = get_enum_value(val, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's unsigned int value failed");
            }
        }
    }
    return val;
}

template <>
int64_t VBSDynamicData::value<int64_t>(const std::string& name) const {
    int64_t val = -1;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_int64_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's int64_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_int64_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's int64_t value failed");
        }
    }
    return val;
}

template <>
unsigned long long VBSDynamicData::value<unsigned long long>(const std::string& name) const {
    uint64_t val = 0xFFFFFFFFFFFFFFFF;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_uint64_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's uint64_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_uint64_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's uint64_t value failed");
        }
    }
    return static_cast<unsigned long long>(val);
}

template <>
unsigned long int VBSDynamicData::value<unsigned long int>(const std::string& name) const {
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
    return static_cast<unsigned long int>(value<unsigned long long>(name));
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
    return static_cast<unsigned long int>(value<uint32_t>(name));
#else
#error "Unsupported platform"
#endif
}

template <>
std::string VBSDynamicData::value<std::string>(const std::string& name) const {
    std::string val = "";
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_string_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                ret = ReturnCode_t(target->get_enum_value(val, id)());
                if (ret() != ReturnCode_t::RETCODE_OK) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "get member \'" << name << "\' 's string value failed");
                }
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_string_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = get_enum_value(val, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's string value failed");
            }
        }
    }
    return val;
}

template <>
float VBSDynamicData::value<float>(const std::string& name) const {
    float val = 0.0;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_float32_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's float value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_float32_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's float value failed");
        }
    }
    return val;
}

template <>
double VBSDynamicData::value<double>(const std::string& name) const {
    double val = 0.0;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_float64_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's double value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_float64_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's double value failed");
        }
    }
    return val;
}

template <>
long double VBSDynamicData::value<long double>(const std::string& name) const {
    long double val = 0.0;
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_float128_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's long double value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_float128_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's with memeber id:" << id << " long double value failed");
        }
    }
    return val;
}

template <>
std::wstring VBSDynamicData::value<std::wstring>(const std::string& name) const {
    std::wstring val = L"";
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);
        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->get_wstring_value(val, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's wstring value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            target = btStack.top();
        }

        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = get_wstring_value(val, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's wstring value failed");
        }
    }
    return val;
}

// set value functions
template <>
int VBSDynamicData::value<bool>(const std::string& name, const bool& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_bool_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's bool value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_bool_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's bool value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<octet>(const std::string& name, const octet& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_byte_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's octet value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_byte_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's octet value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<char>(const std::string& name, const char& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_char8_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's char value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_char_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's char value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<wchar_t>(const std::string& name, const wchar_t& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_char16_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's wchar_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_char16_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's wchar_t value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int8_t>(const std::string& name, const int8_t& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_int8_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's int8_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_int8_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's int8_t value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<short>(const std::string& name, const short& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_int16_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's short value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_int16_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's short value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned short>(const std::string& name, const unsigned short& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_uint16_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's unsigned short value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_uint16_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's unsigned short value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int>(const std::string& name, const int& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_int32_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                ret = ReturnCode_t(target->set_enum_value(value, id)());
                if (ret() != ReturnCode_t::RETCODE_OK) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "set member \'" << name << "\' 's int value failed");
                }
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_int32_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's int value failed");
            }
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned int>(const std::string& name, const unsigned int& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_uint32_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                ret = ReturnCode_t(target->set_enum_value(value, id)());
                if (ret() != ReturnCode_t::RETCODE_OK) {
                    VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                            << "set member \'" << name << "\' 's unsigned int value failed");
                }
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_uint32_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << name << "\' 's unsigned int value failed");
            }
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int64_t>(const std::string& name, const int64_t& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_int64_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's int64_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_int64_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's int64_t value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned long long>(const std::string& name, const unsigned long long& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_uint64_value(static_cast<uint64_t>(value), id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's uint64_t value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_uint64_value(static_cast<uint64_t>(value), id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's uint64_t value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned long int>(const std::string& name, const unsigned long int& value) {
    int ret = static_cast<int>(ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER)());
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
    ret = this->template value<unsigned long long>(name, static_cast<unsigned long long>(value));
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
    ret = this->template value<uint32_t>(name, static_cast<uint32_t>(value));
#else
#error "Unsupported platform"
#endif
    return ret;
}

template <>
int VBSDynamicData::value<float>(const std::string& name, const float& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_float32_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's float value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_float32_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's float value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<double>(const std::string& name, const double& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_float64_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's double value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_float64_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "get member \'" << name << "\' 's double value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<long double>(const std::string& name, const long double& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_float128_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's long double value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_float128_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member \'" << name << "\' 's with memeber id:" << id << " long double value failed");
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<std::string>(const std::string& name, const std::string& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_string_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's string value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_string_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's string value failed");
            }
        }
    }

    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<std::wstring>(const std::string& name, const std::wstring& value) {
    ReturnCode_t ret;
    uint32_t index = 0;
    std::vector<std::string> nameVec;
    split_string_with_specified_char(name, '.', nameVec);
    if (nameVec.size() > 1) {
        std::stack<EvbsDynamicData*> btStack;
        btStack.push(vbs_dynamic_data_);

        EvbsDynamicData* target = nullptr;
        if (back_tracing_loan_value(vbs_dynamic_data_, nameVec, index, btStack)) {
            target = btStack.top();
            std::string member_name = nameVec.back();
            uint32_t id = target->get_member_id_by_name(member_name);
            ret = ReturnCode_t(target->set_wstring_value(value, id)());
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member \'" << name << "\' 's wstring value failed");
            }
        } else {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "back_tracing_loan_value for memebr \'" << name << "\' failed");
            ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
            target = btStack.top();
        }
        btStack.pop();
        back_tracing_return_loaned_value(target, btStack);
    } else {
        uint32_t id = get_member_id_by_name(name);
        ret = set_wstring_value(value, id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "get member \'" << name << "\' 's wstring value failed");
        }
    }

    return static_cast<int>(ret());
}

// set value functions by index
template <>
int VBSDynamicData::value<bool>(uint32_t index, const bool& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's bool value failed");
    } else {
        ret = set_bool_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's bool value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<octet>(uint32_t index, const octet& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's octet value failed");
    } else {
        ret = set_byte_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's octet value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<char>(uint32_t index, const char& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's char value failed");
    } else {
        ret = set_char_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's char value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<wchar_t>(uint32_t index, const wchar_t& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's wchar_t value failed");
    } else {
        ret = set_char16_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's wchar_t value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int8_t>(uint32_t index, const int8_t& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's int8_t value failed");
    } else {
        ret = set_int8_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's int8_t value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<short>(uint32_t index, const short& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's short value failed");
    } else {
        ret = set_int16_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's short value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned short>(uint32_t index, const unsigned short& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's unsigned short value failed");
    } else {
        ret = set_uint16_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's unsigned short value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int>(uint32_t index, const int& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR) << "set member index \'" << index << "\' 's int value failed");
    } else {
        ret = set_int32_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, member_id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "set member index \'" << index << "\' 's int value failed");
            }
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned int>(uint32_t index, const unsigned int& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's unsigned int value failed");
    } else {
        ret = set_uint32_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, member_id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << index << "\' 's unsigned int value failed");
            }
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<int64_t>(uint32_t index, const int64_t& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's int64_t value failed");
    } else {
        ret = set_int64_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's int64_t value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned long long>(uint32_t index, const unsigned long long& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's uint64_t value failed");
    } else {
        ret = set_uint64_value(static_cast<uint64_t>(value), member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's uint64_t value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<unsigned long int>(uint32_t index, const unsigned long int& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
    ret = this->template value<unsigned long long>(index, static_cast<unsigned long long>(value));
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
    ret = this->template value<uint32_t>(index, static_cast<uint32_t>(value));
#else
#error "Unsupported platform"
#endif
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<float>(uint32_t index, const float& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's float value failed");
    } else {
        ret = set_float32_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's float value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<double>(uint32_t index, const double& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's double value failed");
    } else {
        ret = set_float64_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's double value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<long double>(uint32_t index, const long double& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's long double value failed");
    } else {
        ret = set_float128_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's long double value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<std::string>(uint32_t index, const std::string& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's string value failed");
    } else {
        ret = set_string_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            ret = set_enum_value(value, member_id);
            if (ret() != ReturnCode_t::RETCODE_OK) {
                VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                        << "get member \'" << index << "\' 's string value failed");
            }
        }
    }
    return static_cast<int>(ret());
}

template <>
int VBSDynamicData::value<std::wstring>(uint32_t index, const std::wstring& value) {
    ReturnCode_t ret = ReturnCode_t(ReturnCode_t::RETCODE_BAD_PARAMETER);

    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            break;
    }

    if (member_id == VBS_MEMBER_ID_INVALID) {
        VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                << "set member index \'" << index << "\' 's wstring value failed");
    } else {
        ret = set_wstring_value(value, member_id);
        if (ret() != ReturnCode_t::RETCODE_OK) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_ERROR)
                    << "set member index \'" << index << "\' 's wstring value failed");
        }
    }
    return static_cast<int>(ret());
}

template <>
bool VBSDynamicData::value<bool>(uint32_t index) const {
    bool val = false;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_bool_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return 0;
}

template <>
char VBSDynamicData::value<char>(uint32_t index) const {
    char val = '\0';
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_char_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
wchar_t VBSDynamicData::value<wchar_t>(uint32_t index) const {
    wchar_t val = L'\0';
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_char16_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
int8_t VBSDynamicData::value<int8_t>(uint32_t index) const {
    int8_t val = -1;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_int8_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
uint8_t VBSDynamicData::value<uint8_t>(uint32_t index) const {
    uint8_t val = 0xFF;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_uint8_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
int16_t VBSDynamicData::value<int16_t>(uint32_t index) const {
    int16_t val = -1;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_int16_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
uint16_t VBSDynamicData::value<uint16_t>(uint32_t index) const {
    uint16_t val = 0xFFFF;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_uint16_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
int32_t VBSDynamicData::value<int32_t>(uint32_t index) const {
    int32_t val = 0;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_int32_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    ret = get_enum_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    std::cerr << "ret is: " << ret() << std::endl;
    return -1;
}

template <>
uint32_t VBSDynamicData::value<uint32_t>(uint32_t index) const {
    uint32_t val = 0xFFFFFFFF;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_uint32_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    ret = get_enum_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
int64_t VBSDynamicData::value<int64_t>(uint32_t index) const {
    int64_t val = -1;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_int64_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return static_cast<long>(val);
    }
    return -1;
}

template <>
unsigned long long VBSDynamicData::value<unsigned long long>(uint32_t index) const {
    uint64_t val = 0xFFFFFFFFFFFFFFFF;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_uint64_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return static_cast<unsigned long long>(val);
    }
    return -1;
}

template <>
unsigned long int VBSDynamicData::value<unsigned long int>(uint32_t index) const {
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
    return static_cast<unsigned long int>(value<unsigned long long>(index));
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
    return static_cast<unsigned long int>(value<uint32_t>(index));
#else
#error "Unsupported platform"
#endif
}

template <>
std::string VBSDynamicData::value<std::string>(uint32_t index) const {
    std::string val = "";
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_string_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    ret = get_enum_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return std::string("");
}

template <>
float VBSDynamicData::value<float>(uint32_t index) const {
    float val = 0.0;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_float32_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
double VBSDynamicData::value<double>(uint32_t index) const {
    double val = 0.0;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_float64_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
long double VBSDynamicData::value<long double>(uint32_t index) const {
    long double val = 0.0;
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_float128_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

template <>
std::wstring VBSDynamicData::value<std::wstring>(uint32_t index) const {
    std::wstring val = L"";
    uint32_t member_id = VBS_MEMBER_ID_INVALID;
    switch (get_kind()) {
        case vbs::TypeKind::TK_ARRAY:
        case vbs::TypeKind::TK_SEQUENCE:
            member_id = index;
            break;
        default:
            member_id = get_member_id_at_index(index);
            if (member_id == VBS_MEMBER_ID_INVALID) {
                return val;
            }
            break;
    }
    ReturnCode_t ret = get_wstring_value(val, member_id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return L"";
}

template <>
VBSDynamicData VBSDynamicData::value<VBSDynamicData>(uint32_t index) const {
    uint32_t member_id = get_member_id_at_index(index);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return VBSDynamicData();
    }
    EvbsDynamicData* complex_dymamic = nullptr;
    vbs_dynamic_data_->get_complex_value(&complex_dymamic, member_id);
    if (complex_dymamic == nullptr) {
        return VBSDynamicData();
    }
    VBSDynamicData res_data;
    res_data.vbs_dynamic_data_ = complex_dymamic;
    return res_data;
}

template <>
std::vector<bool> VBSDynamicData::get_values<bool>(const uint32_t& id) {
    std::vector<bool> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            bool data;
            dy->get_bool_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<char> VBSDynamicData::get_values<char>(const uint32_t& id) {
    std::vector<char> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            char data;
            dy->get_char_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<wchar_t> VBSDynamicData::get_values<wchar_t>(const uint32_t& id) {
    std::vector<wchar_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            wchar_t data;
            dy->get_char16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int8_t> VBSDynamicData::get_values<int8_t>(const uint32_t& id) {
    std::vector<int8_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int8_t data;
            dy->get_int8_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint8_t> VBSDynamicData::get_values<uint8_t>(const uint32_t& id) {
    std::vector<uint8_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint8_t data;
            dy->get_uint8_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int16_t> VBSDynamicData::get_values<int16_t>(const uint32_t& id) {
    std::vector<int16_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int16_t data;
            dy->get_int16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint16_t> VBSDynamicData::get_values<uint16_t>(const uint32_t& id) {
    std::vector<uint16_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint16_t data;
            dy->get_uint16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int32_t> VBSDynamicData::get_values<int32_t>(const uint32_t& id) {
    std::vector<int32_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int32_t data;
            dy->get_int32_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint32_t> VBSDynamicData::get_values<uint32_t>(const uint32_t& id) {
    std::vector<uint32_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        if (dy->get_type().get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                uint32_t data;
                dy->get_enum_value(data, i);
                v.push_back(data);
            }
        } else {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                uint32_t data;
                dy->get_uint32_value(data, i);
                v.push_back(data);
            }
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int64_t> VBSDynamicData::get_values<int64_t>(const uint32_t& id) {
    std::vector<int64_t> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int64_t data;
            dy->get_int64_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<unsigned long long> VBSDynamicData::get_values<unsigned long long>(const uint32_t& id) {
    std::vector<unsigned long long> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint64_t data;
            dy->get_uint64_value(data, i);
            v.push_back(static_cast<unsigned long long>(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<unsigned long int> VBSDynamicData::get_values<unsigned long int>(const uint32_t& id) {
    std::vector<unsigned long int> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
            uint64_t data;
            dy->get_uint64_value(data, i);
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
            uint32_t data;
            dy->get_uint32_value(data, i);
#else
#error "Unsupported platform"
            break;
#endif
            v.push_back(static_cast<unsigned long int>(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::string> VBSDynamicData::get_values<std::string>(const uint32_t& id) {
    std::vector<std::string> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        if (dy->get_type().get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                std::string data;
                dy->get_enum_value(data, i);
                v.push_back(std::move(data));
            }
        } else {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                std::string data;
                dy->get_string_value(data, i);
                v.push_back(std::move(data));
            }
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::wstring> VBSDynamicData::get_values<std::wstring>(const uint32_t& id) {
    std::vector<std::wstring> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            std::wstring data;
            dy->get_wstring_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<float> VBSDynamicData::get_values<float>(const uint32_t& id) {
    std::vector<float> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            float data;
            dy->get_float32_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<double> VBSDynamicData::get_values<double>(const uint32_t& id) {
    std::vector<double> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            double data;
            dy->get_float64_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<long double> VBSDynamicData::get_values<long double>(const uint32_t& id) {
    std::vector<long double> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            long double data;
            dy->get_float128_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::shared_ptr<VBSDynamicData>> VBSDynamicData::get_values<std::shared_ptr<VBSDynamicData>>(
    const uint32_t& id) {
    std::vector<std::shared_ptr<VBSDynamicData>> v;
    VBSDynamicData* dy = loan_value(id);
    if (dy && (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE)) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            std::shared_ptr<VBSDynamicData> data = dy->get_complex_value(i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<bool> VBSDynamicData::get_values<bool>(const std::string& name) {
    std::vector<bool> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            bool data;
            dy->get_bool_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<char> VBSDynamicData::get_values<char>(const std::string& name) {
    std::vector<char> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            char data;
            dy->get_char_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<wchar_t> VBSDynamicData::get_values<wchar_t>(const std::string& name) {
    std::vector<wchar_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            wchar_t data;
            dy->get_char16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int8_t> VBSDynamicData::get_values<int8_t>(const std::string& name) {
    std::vector<int8_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int8_t data;
            dy->get_int8_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint8_t> VBSDynamicData::get_values<uint8_t>(const std::string& name) {
    std::vector<uint8_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint8_t data;
            dy->get_uint8_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int16_t> VBSDynamicData::get_values<int16_t>(const std::string& name) {
    std::vector<int16_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int16_t data;
            dy->get_int16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint16_t> VBSDynamicData::get_values<uint16_t>(const std::string& name) {
    std::vector<uint16_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint16_t data;
            dy->get_uint16_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int32_t> VBSDynamicData::get_values<int32_t>(const std::string& name) {
    std::vector<int32_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int32_t data;
            dy->get_int32_value(data, i);
            v.push_back(data);
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<uint32_t> VBSDynamicData::get_values<uint32_t>(const std::string& name) {
    std::vector<uint32_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        if (dy->get_type().get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                uint32_t data;
                dy->get_enum_value(data, i);
                v.push_back(data);
            }
        } else {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                uint32_t data;
                dy->get_uint32_value(data, i);
                v.push_back(data);
            }
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<int64_t> VBSDynamicData::get_values<int64_t>(const std::string& name) {
    std::vector<int64_t> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            int64_t data;
            dy->get_int64_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<unsigned long long> VBSDynamicData::get_values<unsigned long long>(const std::string& name) {
    std::vector<unsigned long long> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            uint64_t data;
            dy->get_uint64_value(data, i);
            v.push_back(static_cast<unsigned long long>(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<unsigned long int> VBSDynamicData::get_values<unsigned long int>(const std::string& name) {
    std::vector<unsigned long int> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
#if defined(__x86_64__) || defined(_WIN64) || defined(__aarch64__)
            uint64_t data;
            dy->get_uint64_value(data, i);
#elif defined(__i386__) || defined(_WIN32) || defined(__arm__)
            uint32_t data;
            dy->get_uint32_value(data, i);
#else
#error "Unsupported platform"
            break;
#endif
            v.push_back(static_cast<unsigned long int>(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::string> VBSDynamicData::get_values<std::string>(const std::string& name) {
    std::vector<std::string> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        if (dy->get_type().get_element_type().get_kind() == vbs::TypeKind::TK_ENUM) {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                std::string data;
                dy->get_enum_value(data, i);
                v.push_back(std::move(data));
            }
        } else {
            for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
                std::string data;
                dy->get_string_value(data, i);
                v.push_back(std::move(data));
            }
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::wstring> VBSDynamicData::get_values<std::wstring>(const std::string& name) {
    std::vector<std::wstring> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            std::wstring data;
            dy->get_wstring_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<float> VBSDynamicData::get_values<float>(const std::string& name) {
    std::vector<float> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            float data;
            dy->get_float32_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<double> VBSDynamicData::get_values<double>(const std::string& name) {
    std::vector<double> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            double data;
            dy->get_float64_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<long double> VBSDynamicData::get_values<long double>(const std::string& name) {
    std::vector<long double> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            long double data;
            dy->get_float128_value(data, i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

template <>
std::vector<std::shared_ptr<VBSDynamicData>> VBSDynamicData::get_values<std::shared_ptr<VBSDynamicData>>(
    const std::string& name) {
    std::vector<std::shared_ptr<VBSDynamicData>> v;
    uint32_t member_id = get_member_id_by_name(name);
    if (member_id == VBS_MEMBER_ID_INVALID) {
        return v;
    }
    VBSDynamicData* dy = loan_value(member_id);
    if (dy && (dy->get_kind() == vbs::TypeKind::TK_ARRAY || dy->get_kind() == vbs::TypeKind::TK_SEQUENCE)) {
        for (uint32_t i = 0; i < dy->get_item_count(); ++i) {
            std::shared_ptr<VBSDynamicData> data = dy->get_complex_value(i);
            v.push_back(std::move(data));
        }
    }
    return_loaned_value(dy);
    return v;
}

char VBSDynamicData::get_char8_value(const std::string& name) const {
    char val = '\0';
    uint32_t id = get_member_id_by_name(name);
    ReturnCode_t ret = get_char8_value(val, id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

int VBSDynamicData::set_char8_value(const std::string& name, const char& value) {
    uint32_t id = get_member_id_by_name(name);
    return (int)set_char8_value(value, id)();
}

int8_t VBSDynamicData::get_int8_value(const std::string& name) const {
    int8_t val = -1;
    uint32_t id = get_member_id_by_name(name);
    ReturnCode_t ret = get_int8_value(val, id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

int VBSDynamicData::set_int8_value(const std::string& name, const int8_t& value) {
    uint32_t id = get_member_id_by_name(name);
    return (int)set_int8_value(value, id)();
}

std::string VBSDynamicData::get_enum_string(const std::string& name) const {
    std::string val = "";
    uint32_t id = get_member_id_by_name(name);
    ReturnCode_t ret = get_enum_value(val, id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return "";
}

int VBSDynamicData::set_enum_string(const std::string& name, const std::string& value) {
    uint32_t id = get_member_id_by_name(name);
    return (int)set_enum_value(value, id)();
}

unsigned int VBSDynamicData::get_enum_int(const std::string& name) const {
    unsigned int val = 0xFFFFFFFF;
    uint32_t id = get_member_id_by_name(name);
    ReturnCode_t ret = get_enum_value(val, id);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

int VBSDynamicData::set_enum_int(const std::string& name, const uint32_t& value) {
    uint32_t id = get_member_id_by_name(name);
    return (int)set_enum_value(value, id)();
}

int VBSDynamicData::set_enum_int(const std::string& name, const int32_t& value) {
    uint32_t id = get_member_id_by_name(name);
    return (int)set_enum_value(value, id)();
}

uint64_t VBSDynamicData::get_bitmask_value() const {
    uint64_t val = 0xFFFFFFFFFFFFFFFF;
    ReturnCode_t ret = get_bitmask_value(val);
    if (ret() == ReturnCode_t::RETCODE_OK) {
        return val;
    }
    return -1;
}

int VBSDynamicData::set_bitmask(uint64_t value) {
    return (int)set_bitmask_value(value)();
}

}  // namespace vbs
