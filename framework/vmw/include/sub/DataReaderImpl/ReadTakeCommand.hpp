// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: listener code decoupling
// feature: evbs compilation does not depend on framework header files
// feature: take_next_sample adapt loan_sample.
// feature: support agent for vbs
// ------------------------------------------------------------------

#ifndef _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_
#define _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_

#include <cassert>
#include <cstdint>
#include <memory>

#include "deps/base/LoanableCollection.hpp"
#include "deps/base/LoanableTypedCollection.hpp"
#include "deps/common/TypeSupport.hpp"
#include "sub/SampleInfoInner.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/DataReaderImpl/DataReaderLoanManager.hpp"
#include "sub/DataReaderImpl/StateFilter.hpp"
#include "sub/DataReaderImpl/SampleInfoPool.hpp"
#include "sub/DataReaderImpl/SampleLoanManager.hpp"
#include "sub/DataReaderHistory.hpp"
#include "elog/log/Log.h"

namespace vbs {
namespace detail {

struct ReadTakeCommand {
    using history_type = vbs::detail::DataReaderHistory;
    using CacheChange_t = vbs::common::CacheChange_t;
    using WriterProxy = evbs::ertps::rtps::WriterProxy;
    using SampleInfoSeq = evbs::edds::dds::LoanableTypedCollection<vbs::SampleInfoInner>;

    ReadTakeCommand(vbs::DataReaderImpl& reader, evbs::edds::dds::LoanableCollection& data_values,
                    SampleInfoSeq& sample_infos, int32_t max_samples, const vbs::detail::StateFilter& states,
                    const history_type::instance_info& instance, bool single_instance = false)
        : type_(reader.type_),
          loan_manager_(*(reader.loan_manager_)),
          history_(*(reader.history_)),
          reader_(reader.reader_),
          info_pool_(*(reader.sample_info_pool_)),
          sample_pool_(reader.sample_pool_),
          data_values_(data_values),
          sample_infos_(sample_infos),
          remaining_samples_(max_samples),
          states_(states),
          instance_(instance),
          handle_(instance->first),
          single_instance_(single_instance) {
        assert(0 <= remaining_samples_);
        current_slot_ = data_values_.length();
        finished_ = false;
    }

    ~ReadTakeCommand() {
        if ((!data_values_.has_ownership()) && (vbsutil::elog::ReturnCode_t::RETCODE_NO_DATA == return_value_)) {
            (void)loan_manager_.return_loan(data_values_, sample_infos_);
            (void)data_values_.unloan();
            (void)sample_infos_.unloan();
        }
    }

    bool add_instance(bool take_samples, bool raw_buffer) {
        // Advance to the first instance with a valid state
        if ((!go_to_first_valid_instance())) {
            return false;
        }

        // Traverse changes on current instance
        bool ret_val = false;
        evbs::edds::dds::LoanableCollection::size_type first_slot = current_slot_;
        auto it = instance_->second->cache_changes.begin();
        while ((!finished_) && (it != instance_->second->cache_changes.end())) {
            CacheChange_t* change = *it;

            vbs::SampleStateKind check;
            check =
                change->isRead ? vbs::SampleStateKind::READ_SAMPLE_STATE : vbs::SampleStateKind::NOT_READ_SAMPLE_STATE;
            if (change->bCanRead && (check & states_.sample_states) != 0) {
                //WriterProxy* wp       = nullptr;
                //bool is_future_change = false;
                /*队列中是5，7,9，检查到7时认为是future，不能取走。那为何收包时不设置好状态呢？ */
                //(void)reader_->begin_sample_access_nts(change, /*wp,*/ is_future_change);

                // If the change is in the future we can skip the remaining changes in the history,
                // as they will be in the future also
                //if (!is_future_change)
                {
                    bool remove_change = false;
                    // Add sample and info to collections
                    bool added = add_sample(*it, remove_change, raw_buffer);
                    history_.change_was_processed_nts(change, added);
                    history_.end_sample_access_nts(change, /*wp,*/ added);
                    MessageLog(reader_->getTopicName(), change->writerGUID, MSG_TRACE_TYPE_TAKE,
                               change->sequenceNumber.to64long(), change->sourceTimestamp,
                               change->serializedPayload.length);

                    if (remove_change || (added && take_samples)) {
                        // Remove from history
                        (void)history_.remove_change_sub_nts(change, it);
                        // Current iterator will point to change next to the one removed. Avoid
                        // incrementing.
                        continue;
                    }
                }
            }

            // Go to next sample on instance
            ++it;
        }

        if (current_slot_ > first_slot) {
            history_.instance_viewed_nts(instance_->second);
            ret_val = true;

            // complete sample infos
            evbs::edds::dds::LoanableCollection::size_type slot = current_slot_;
            evbs::edds::dds::LoanableCollection::size_type n = 0;
            while (slot > first_slot) {
                --slot;
                sample_infos_[slot].sample_rank = n;
                ++n;
            }
        }

        (void)next_instance();
        return ret_val;
    }

    bool add_changes(bool take_samples, bool raw_buffer) {
        // Traverse changes on current instance
        bool ret_val = false;

        evbs::edds::dds::LoanableCollection::size_type first_slot = current_slot_;
        auto it = history_.changesBegin();
        while (it != history_.changesEnd()) {
            CacheChange_t* change = *it;
            vbs::SampleStateKind check;
            check =
                change->isRead ? vbs::SampleStateKind::READ_SAMPLE_STATE : vbs::SampleStateKind::NOT_READ_SAMPLE_STATE;
            if (change->bCanRead && (check & states_.sample_states) != 0) {
                //WriterProxy* wp       = nullptr;
                //bool is_future_change = false;
                //(void)reader_->begin_sample_access_nts(change, /*wp,*/ is_future_change);

                // If the change is in the future we can skip the remaining changes in the history,
                // as they will be in the future also
                //if (!is_future_change)
                {
                    bool remove_change = false;
                    // Add sample and info to collections
                    bool added = add_sample(change, remove_change, raw_buffer);
                    history_.change_was_processed_nts(change, added);
                    history_.end_sample_access_nts(change, /*wp,*/ added);
                    MessageLog(reader_->getTopicName(), change->writerGUID, MSG_TRACE_TYPE_TAKE,
                               change->sequenceNumber.to64long(), change->sourceTimestamp,
                               change->serializedPayload.length);

                    if (remove_change || (added && take_samples)) {
                        // Remove from history
                        (void)history_.remove_change_sub_no_key_nts(change, it);
                        // Current iterator will point to change next to the one removed. Avoid
                        // incrementing.
                        continue;
                    }
                }
            }
            ++it;
        }

        if (current_slot_ > first_slot) {
            history_.instance_viewed_nts(instance_->second);
            ret_val = true;

            // complete sample infos
            evbs::edds::dds::LoanableCollection::size_type slot = current_slot_;
            evbs::edds::dds::LoanableCollection::size_type n = 0;
            while (slot > first_slot) {
                --slot;
                sample_infos_[slot].sample_rank = n;
                ++n;
            }
        }
        finished_ = true;
        return ret_val;
    }

    inline bool is_finished() const { return finished_; }

    inline vbsutil::elog::ReturnCode_t return_value() const { return return_value_; }
    static void generate_info(vbs::SampleInfoInner& info, const vbs::detail::DataReaderInstance& instance,
                              const vbs::detail::DataReaderCacheChange& item) {
        info.sample_state = item->isRead ? vbs::READ_SAMPLE_STATE : vbs::NOT_READ_SAMPLE_STATE;
        info.instance_state = instance.instance_state;
        info.view_state = instance.view_state;
        info.disposed_generation_count = item->reader_info.disposed_generation_count;
        info.no_writers_generation_count = item->reader_info.no_writers_generation_count;
        info.sample_rank = 0;
        info.generation_rank = 0;
        info.absolute_generation_rank = 0;
        info.source_timestamp = item->sourceTimestamp;
        info.reception_timestamp = item->reader_info.receptionTimestamp;
        info.instance_handle = item->instanceHandle;
        info.publication_handle = InstanceHandle_t(item->writerGUID);
        (void)info.sample_identity.writer_guid(item->writerGUID);
        (void)info.sample_identity.sequence_number(item->sequenceNumber);
        info.related_sample_identity = item->write_params.sample_identity();
        info.valid_data = true;
        info.payload_len = item->serializedPayload.length;
        info.e2eStatus.counter = item->e2eCounter;
        info.e2eStatus.status = evbs::edds::dds::E2E_StatusType(item->e2eStatus);
        info.reception_sequence_number = item->reception_sequnce_number;
        info.topic_id = item->topic_id;

        switch (item->kind) {
            case vbs::common::NOT_ALIVE_DISPOSED:
            case vbs::common::NOT_ALIVE_DISPOSED_UNREGISTERED:
            case vbs::common::NOT_ALIVE_UNREGISTERED:
                info.valid_data = false;
                break;
            case vbs::common::ALIVE:
            default:
                break;
        }
    }

 private:
    const vbs::TypeSupport& type_;
    vbs::detail::DataReaderLoanManager& loan_manager_;
    history_type& history_;
    vbs::dispatcher::DispatcherReader* reader_;
    vbs::detail::SampleInfoPool& info_pool_;
    std::shared_ptr<vbs::detail::SampleLoanManager> sample_pool_;
    evbs::edds::dds::LoanableCollection& data_values_;
    SampleInfoSeq& sample_infos_;
    int32_t remaining_samples_;
    vbs::detail::StateFilter states_;
    history_type::instance_info instance_;
    InstanceHandle_t handle_;
    bool single_instance_;

    bool finished_ = false;
    vbsutil::elog::ReturnCode_t return_value_ = vbsutil::elog::ReturnCode_t::RETCODE_NO_DATA;

    evbs::edds::dds::LoanableCollection::size_type current_slot_ = 0;

    bool go_to_first_valid_instance() {
        while (!is_current_instance_valid()) {
            if (!next_instance()) {
                return false;
            }
        }
        return true;
    }

    bool is_current_instance_valid() {
        // Check instance_state against states_.instance_states and view_state against
        // states_.view_states
        auto instance_state = instance_->second->instance_state;
        auto view_state = instance_->second->view_state;
        return (0 != (states_.instance_states & instance_state)) && (0 != (states_.view_states & view_state));
    }

    bool next_instance() {
        history_.check_and_remove_instance(instance_);
        if (single_instance_) {
            finished_ = true;
            return false;
        }

        auto result = history_.next_available_instance_nts(handle_, instance_);
        if (!result.first) {
            finished_ = true;
            return false;
        }

        instance_ = result.second;
        handle_ = instance_->first;
        return true;
    }

    bool add_sample(const vbs::detail::DataReaderCacheChange& item, bool& deserialization_error, bool raw_buffer) {
        // coverity[misra_cpp_2008_rule_0_1_6_violation:SUPPRESS]
        bool ret_val = false;
        deserialization_error = false;

        if (remaining_samples_ > 0) {
            // Increment length of collections
            auto new_len = current_slot_ + 1;
            (void)data_values_.length(new_len);
            (void)sample_infos_.length(new_len);

            try {
                // Add information
                generate_info(item);
            } catch (std::exception& e) {
                std::cerr << "add_sample caught: " << e.what() << '\n';
                return false;
            }
            if (sample_infos_[current_slot_].valid_data) {
                if (!deserialize_sample(item, raw_buffer)) {
                    // Decrement length of collections
                    (void)data_values_.length(current_slot_);
                    (void)sample_infos_.length(current_slot_);
                    deserialization_error = true;
                    auto listener = reader_->get_uplistener();
                    if (listener) {
                        listener->on_sample_lost(1, LOST_BY_DESERIALIZATION_FAILURE);
                    }
                    elogErrorKeyT(DATA_READER, RetCode_t::RETCODE_ERROR, type_->getName(),
                                  "Topic " << type_->getName() << " data deserialize fail.");
                    return false;
                }
            }

            // Mark that some data is available
            return_value_ = vbsutil::elog::ReturnCode_t::RETCODE_OK;
            ++current_slot_;
            --remaining_samples_;
            ret_val = true;
        }

        // Finish when there are no remaining samples
        finished_ = (remaining_samples_ == 0);
        return ret_val;
    }

    bool deserialize_sample(CacheChange_t* change, bool raw_buffer) {
        auto payload = &(change->serializedPayload);
        if (data_values_.has_ownership()) {
            // perform deserialization
            if (raw_buffer) {
                //memcpy(data_values_.buffer()[current_slot_], payload->data, payload->length);
                logWarning(DATA_READER, "Topic " << type_->getName() << "incorrect use of take_buffer() api");
                return false;
            } else if (payload->data[2] == 1) {
                size_t real_size = payload->length;
                if (type_->is_plain()) {
                    real_size = type_->getNonSerializedSize();
                }
                memcpy(data_values_.buffer()[current_slot_], payload->data + payload->representation_header_size,
                       real_size - payload->representation_header_size);
                return true;
            } else {
                return type_->deserialize(payload, data_values_.buffer()[current_slot_]);
            }
        } else {
            // loan
            void* sample = nullptr;
            if (sample_pool_->get_loan(change, sample, raw_buffer)) {
                const_cast<void**>(data_values_.buffer())[current_slot_] = sample;
                return true;
            }

            if (sample) {
                sample_pool_->return_loan(sample);
            }
            elogErrorKeyT(DATA_READER, RetCode_t::RETCODE_ERROR, type_->getName(),
                          "Topic " << type_->getName() << " data deserialize get loan fail.");
            return false;
        }
    }

    void generate_info(const vbs::detail::DataReaderCacheChange& item) {
        // Loan when necessary
        if (!sample_infos_.has_ownership()) {
            vbs::SampleInfoInner* pool_item = info_pool_.get_item();
            assert(pool_item != nullptr);
            const_cast<void**>(sample_infos_.buffer())[current_slot_] = pool_item;
        }

        vbs::SampleInfoInner& info = sample_infos_[current_slot_];
        generate_info(info, *instance_->second, item);
    }
};

} /* namespace detail */
} /* namespace vbs */

#endif  // _EDDS_ENTITYSUBSCRIBER_DATAREADERIMPL_READTAKECOMMAND_HPP_
