// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _EDDS_PUBLISHER_FILTERING_READERFILTERCOLLECTIONIMPL_HPP_
#define _EDDS_PUBLISHER_FILTERING_READERFILTERCOLLECTIONIMPL_HPP_

#include "core/LoanableSequence.hpp"
#include "deps/common/Guid.h"
#include "edds/rtps/interfaces/IContentFilter.hpp"
#include "edds/rtps/interfaces/IContentFilterFactory.hpp"
#include "topic/ContentFilterInfo.hpp"
#include "edds/rtps/interfaces/ContentFilterUtils.hpp"
#include "ecdr/Cdr.h"
#include "ecdr/FastBuffer.h"
#include "ecdr/exceptions/Exception.h"
#include "deps/type/DynamicData.h"
#include "deps/common/shared_mutex.hpp"

namespace vbs {

/**
  * Class responsible for writer side filtering.
  * Contains a resource-limited map associating a reader GUID with its filtering information.
  * Performs the evaluation of filters when a change is added to the DataWriter's history.
  */
class ReaderFilterCollection {
    struct ReaderFilterInformation {
        evbs::ertps::rtps::IContentFilterFactory* filter_factory = nullptr;
        evbs::ertps::rtps::IContentFilter* filter = nullptr;
        std::array<uint8_t, 16> filter_signature {{0}};
    };

 public:
    /**
      * Construct a ReaderFilterCollection.
      *
      * @param allocation  Allocation configuration for reader filtering information.
      */
    ReaderFilterCollection(const std::string& type_name) : type_name_(type_name) {}

    ~ReaderFilterCollection() {
        is_destroyed_ = true;
        {
            std::lock_guard<vbs::shared_mutex> lock(filter_mutex_);
            for (auto& item : reader_filters_) {
                destroy_filter(item.second);
            }
        }
        data_ = nullptr;
    }

    /**
      * @return true when there are no reader filters registered.
      */
    bool empty() const { return reader_filters_.empty(); }

    /**
      * Performs filter evaluation on a CacheChange_t.
      *
      * @param [in,out] filtered_out_readers     This method updates two of its properties:
      *                                          - @c filtered_out_readers will contain the GUIDs of the readers to
      *                                            which the change should not be delivered.
      *                                            parameter informing about the applied filters.
      */
    void update_filter_info(vbs::common::CacheChange_t& change) {
        if (is_destroyed_) {
            return;
        }
        vbs::shared_lock<vbs::shared_mutex> lock(filter_mutex_);
        change.filtered_out_readers.clear();
        size_t num_filters = reader_filters_.size();
        if (0 < num_filters) {
            if (!data_) {
                return;
            }
            update_data(change.serializedPayload.data, change.serializedPayload.length);
            for (std::size_t i = 0; i < num_filters; ++i) {
                auto it = reader_filters_.cbegin();
                std::advance(it, i);
                if (it == reader_filters_.cend()) {
                    return;
                }
                const ReaderFilterInformation& entry = it->second;
                // Evaluate filter and update filtered_out_readers
                if (!entry.filter->evaluate(data_.get())) {
                    change.filtered_out_readers.emplace_back(it->first);
                }
            }
            if (!change.filtered_out_readers.empty()) {
                update_filter_signature(change);
            }
        }
    }

    void update_filter_signature(vbs::common::CacheChange_t& change) {
        if (is_destroyed_) {
            return;
        }
        vbs::shared_lock<vbs::shared_mutex> lock(filter_mutex_);
        size_t num_filters = reader_filters_.size();
        uint16_t cdr_size = 0;
        if ((0 < num_filters) && ContentFilterInfo::cdr_serialized_size(num_filters, cdr_size)) {
            change.inline_qos.reserve(cdr_size);
            change.inline_qos.pos = 0;
            change.inline_qos.length = 0;
            // Functor used from the serialization process to evaluate each filter and write its signature.
            auto filter_process = [this, &change](std::size_t i, uint8_t* signature) -> bool {
                // Point to the corresponding entry
                auto it = reader_filters_.cbegin();
                std::advance(it, i);
                if (it == reader_filters_.cend()) {
                    return false;
                }
                const ReaderFilterInformation& entry = it->second;

                // Copy the signature
                std::copy(entry.filter_signature.begin(), entry.filter_signature.end(), signature);

                auto& filtered_readers = change.filtered_out_readers;
                return std::find(filtered_readers.begin(), filtered_readers.end(), it->first) == filtered_readers.end();
            };
            ContentFilterInfo::cdr_serialize(change.inline_qos, num_filters, filter_process);
        }
    }

    /**
      * Unregister a reader from writer-side filtering.
      * Called when the reader is unmatched or when its filtering information is
      * updated to indicate it stopped filtering.
      *
      * @param [in] guid  GUID of the reader to remove.
      */
    void remove_reader_filter(const vbsutil::xmlparser::GUID_t& guid) {
        if (is_destroyed_) {
            return;
        }
        std::lock_guard<vbs::shared_mutex> lock(filter_mutex_);
        auto it = reader_filters_.find(guid);
        if (it != reader_filters_.end()) {
            destroy_filter(it->second);
            reader_filters_.erase(it);
        }
    }

    /**
      * Process filtering information about a reader.
      * Called whenever the discovery information about a reader changes.
      *
      * @param [in] guid         GUID of the reader for which the discovery information has changed.
      * @param [in] filter_info  Content filter discovery information.
      */
    void add_reader_filter(const vbsutil::xmlparser::GUID_t& guid,
                           const vbsutil::xmlparser::ContentFilterProperty& filter_info) {
        if (is_destroyed_) {
            return;
        }
        std::lock_guard<vbs::shared_mutex> lock(filter_mutex_);
        auto it = reader_filters_.find(guid);
        if (it == reader_filters_.end()) {
            if (filter_info.filter_expression.empty()) {
                return;
            }
            // Prepare and insert element
            ReaderFilterInformation entry;
            if (update_entry(entry, filter_info)) {
                reader_filters_.emplace(std::make_pair(guid, std::move(entry)));
            }
        } else {
            // Update entry
            if (!update_entry(it->second, filter_info)) {
                // If the entry could not be updated, it means we cannot use the filter information, so
                // we remove the old information
                destroy_filter(it->second);
                reader_filters_.erase(it);
            }
        }
    }

    bool has_reader_filter(const vbsutil::xmlparser::GUID_t& guid) {
        if (is_destroyed_) {
            return false;
        }
        std::lock_guard<vbs::shared_mutex> lock(filter_mutex_);
        return reader_filters_.find(guid) != reader_filters_.end();
    }

 private:
    void update_data(void* buffer, size_t length) {
        data_->clear_all_values();

        using namespace vbsutil::ecdr;
        try {
            FastBuffer fastbuffer(reinterpret_cast<char*>(buffer), length);
            Cdr deser(fastbuffer);
            deser.read_encapsulation();
            data_->deserialize(deser);
        } catch (exception::Exception& /*exception*/) {
            return;
        }
    }

    /**
      * Ensure a filter instance is removed before an information entry is removed.
      *
      * @param [in,out] entry  The ReaderFilterInformation entry being removed.
      */
    void destroy_filter(ReaderFilterInformation& entry) {
        if (nullptr != entry.filter) {
            evbs::ertps::rtps::IContentFilterFactory::get_default_filter_factory()->delete_content_filter("",
                                                                                                          entry.filter);
        }
    }

    /**
      * Update an information entry.
      *
      * @param [in,out] entry        The ReaderFilterInformation entry to update.
      * @param [in]     filter_info  Content filter discovery information to apply.
      * @param [in]     type         Type to use for the creation of the content filter.
      */
    bool update_entry(ReaderFilterInformation& entry, const vbsutil::xmlparser::ContentFilterProperty& filter_info) {
        const char* class_name = filter_info.filter_class_name.c_str();
        evbs::ertps::rtps::IContentFilterFactory* new_factory =
            evbs::ertps::rtps::IContentFilterFactory::get_default_filter_factory();
        if (nullptr == new_factory) {
            return false;
        }

        std::array<uint8_t, 16> new_signature = {0};
        ContentFilterUtils::compute_signature(filter_info, new_signature);
        if (new_signature == entry.filter_signature && new_factory == entry.filter_factory && nullptr != entry.filter) {
            return true;
        }

        LoanableSequence<const char*>::size_type n_params;
        n_params = static_cast<LoanableSequence<const char*>::size_type>(filter_info.expression_parameters.size());
        LoanableSequence<const char*> filter_parameters(n_params);
        filter_parameters.length(n_params);
        while (n_params > 0) {
            n_params--;
            filter_parameters[n_params] = filter_info.expression_parameters[n_params].c_str();
        }

        evbs::ertps::rtps::IContentFilter* new_filter = entry.filter;

        ReturnCode_t ret =
            new_factory->create_content_filter(class_name, type_name_.c_str(), filter_info.filter_expression.c_str(),
                                               filter_parameters, new_filter, data_);

        if (ReturnCode_t::RETCODE_OK != ret) {
            return false;
        }
        entry.filter = new_filter;
        entry.filter_signature = new_signature;
        entry.filter_factory = new_factory;
        return true;
    }

    std::atomic<bool> is_destroyed_ {false};
    vbs::shared_mutex filter_mutex_;
    std::map<vbsutil::xmlparser::GUID_t, ReaderFilterInformation> reader_filters_;
    std::string type_name_;
    evbs::ertps::types::DynamicData_ptr data_;
};

}  // namespace vbs

#endif  //_EDDS_PUBLISHER_FILTERING_READERFILTERCOLLECTIONIMPL_HPP_
