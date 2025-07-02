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

#ifndef INCLUDE_CORE_LOANABLECOLLECTIONIMPL_HPP_
#define INCLUDE_CORE_LOANABLECOLLECTIONIMPL_HPP_

#include <cstdint>
#include <vector>
#include <memory>

#include "core/Global.hpp"
#include "sub/DataReaderImpl.hpp"
#include "sub/SampleInfoImpl.hpp"

namespace vbs {
class DataReaderImpl;
template <typename T, typename N = std::true_type>
class LoanableCollectionImpl {
    friend class DataReaderImpl;

 public:
    /**
     * LoanableCollectionImpl constructor.
     */
    LoanableCollectionImpl() {}

    /**
     * LoanableCollectionImpl deconstructor.
     */
    ~LoanableCollectionImpl() { clear(); }

    template <typename X, typename Y = std::true_type>
    class Iterator {
     public:
        /**
         * Iterator constructor
         *
         * @param data LoanableCollectionImpl
         * @param index the index of data.
         */
        Iterator(LoanableCollectionImpl<X, Y>* data, const size_t& index) : index_(index), data_(data) {}

        /**
         * Iterator operator++
         *
         * @return Iterator
         */
        Iterator<X, N>& operator++() {
            index_++;
            return *this;
        }

        /**
         * Iterator operator==
         *
         * @return bool
         */
        bool operator==(const Iterator<X, Y>& iter) { return data_ == iter.data_ && index_ == iter.index_; }

        /**
         * Iterator operator!=
         *
         * @return bool
         */
        bool operator!=(const Iterator<X, Y>& iter) { return !(*this == iter); }

        /**
         * Iterator operator*
         *
         * @return shared_ptr
         */
        std::shared_ptr<X> operator*() { return data_->get_ptr(index_); }

     private:
        size_t index_;
        LoanableCollectionImpl<X, Y>* data_;
    };

    /**
     * get the length of data vector.
     *
     * @return length.
     */
    size_t length() { return dds_data_.size(); }

    /**
     * get data. T is the type of data
     *
     * @param i the index of data.
     * @return data.
     */
    const T& operator[](size_t i) {
        size_t len = length();
        if (i >= len)
            return data_;
        return *dds_data_[i];
    }

    /**
     * get_ptr. T is the type of data
     *
     * @param i the index of data.
     * @return shared_ptr.
     */
    std::shared_ptr<T> get_ptr(size_t i) {
        size_t len = length();
        if (i >= len)
            return nullptr;
        return dds_data_[i];
    }

    /**
     * begin Iterator
     *
     * @return Iterator.
     */
    Iterator<T, N> begin() { return Iterator<T, N>(this, 0); }

    /**
     * end Iterator
     *
     * @return Iterator.
     */
    Iterator<T, N> end() { return Iterator<T, N>(this, length()); }

    /**
     * clear data vector
     */
    void clear() { dds_data_.clear(); }

 private:
    std::vector<std::shared_ptr<T>> dds_data_;
    T data_;
};

template <typename T, typename N = std::true_type>
class LoanableCollectionPtrImpl {
    friend class DataReaderImpl;

 public:
    /**
     * LoanableCollectionPtrImpl constructor.
     */
    LoanableCollectionPtrImpl() {}

    /**
     * LoanableCollectionPtrImpl constructor.
     */
    ~LoanableCollectionPtrImpl() {}

    /**
     * get the length of data vector.
     *
     * @return length.
     */
    size_t length() const { return data_.size(); }

    template <typename Enabler = N>
    typename std::enable_if<Enabler::value, T>::type& operator[](const size_t& i) {
        const size_t len = length();
        if (i >= len)
            return vail_data_;
        return data_[i];
    }

    /**
     * get data. T is the type of data
     *
     * @param i the index of data.
     * @return data.
     */
    const T operator[](const size_t& i) const {
        const size_t len = length();
        if (i >= len)
            return vail_data_;
        return data_[i];
    }

    /**
     * clear data vector
     */
    void clear() { data_.clear(); }

 private:
    vbs::LoanableSequence<void*, N> dds_load_seq_;
    std::vector<T> data_;
    T vail_data_;
};

class LoanableCollectionFlatDataImpl {
    friend class DataReaderImpl;
    friend class LoanableCollectionFlatData;

 public:
    /**
     * LoanableCollectionImpl constructor.
     */
    LoanableCollectionFlatDataImpl() {
        dds_reader_ = nullptr;
        dny_dds_infos_ = nullptr;
    }

    /**
     * LoanableCollectionImpl deconstructor.
     */
    ~LoanableCollectionFlatDataImpl() { clear(); }

    /**
     * get the length of data vector.
     *
     * @return length.
     */
    size_t length() {
        size_t len = dny_dds_infos_->length();
        return len;
    }

    /**
     * get data. T is the type of data
     *
     * @param i the index of data.
     * @return data.
     */
    void* operator[](size_t i) { return get_ptr(i); }

    void* get_ptr(size_t i) {
        size_t len = length();
        if (i >= len)
            return nullptr;
        return static_cast<void*>(dds_load_seq_.buffer()[i]);
    }

    /**
     * clear data vector
     */
    void clear() {
        if ((dds_reader_ != nullptr) && (dny_dds_infos_ != nullptr)) {
            try {
                (void)dds_reader_->return_loan(dds_load_seq_, std::get<1>(dny_dds_infos_->dny_dds_infos_));
            } catch (const std::exception& /*exception*/) {
                return;
            }
        }
    }

 private:
    vbs::LoanableSequence<void*, std::true_type> dds_load_seq_;
    vbs::DataReaderImpl* dds_reader_;
    std::shared_ptr<SampleInfoImpl> dny_dds_infos_;
};

}  // namespace vbs
#endif  // INCLUDE_CORE_LOANABLECOLLECTIONIMPL_HPP_
