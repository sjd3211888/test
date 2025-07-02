// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
// feature: Remove redundant code 1. Remove MuxType(the source of data)
// 2. Remove unused member variables in DataWriterImpl/DataReadImpl/DomainParticipantListener
// 3. Remove member variables about shm type discovery 4. Delete some commented code
// feature: Encapsulation LoanableCollection and Samplelnfo
// feature: Upload trace data
// feature: VBS framework adapts to membuf
// ------------------------------------------------------------------

#ifndef VBS_LOANABLECOLLECTION_HPP_
#define VBS_LOANABLECOLLECTION_HPP_

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <type_traits>

#include "vbs/Utils.hpp"
#include "vbs/Global.hpp"

namespace vbs {

template <typename T>
struct is_pointer {
    static constexpr bool value = false;
};

template <typename T>
struct is_pointer<T*> {
    static constexpr bool value = true;
};

class LoanableCollectionFlatDataImpl;

class LoanableCollectionFlatData {
    /**
     * Granting access to the DataReader class to access private members of this class.
     */
    friend class DataReader;

 public:
    /**
     * Default constructor for LoanableCollectionFlatData.
     * 
     * @note @li Thread-Safe: Yes
     */
    LoanableCollectionFlatData();

    /**
     * Destructor for LoanableCollectionFlatData. Responsible for cleaning up 
     * any resources associated with the instance.
     * 
     * @note @li Thread-Safe: Yes
     */
    ~LoanableCollectionFlatData();

    /**
     * Retrieves a pointer to the underlying LoanableCollectionFlatDataImpl instance.
     * 
     * @note @li Thread-Safe: No
     * @note @li Lock-Free: No
     *
     * @return Pointer to the LoanableCollectionFlatDataImpl instance.
     */
    std::shared_ptr<LoanableCollectionFlatDataImpl>& get_instance() { return flat_data_impl_; }

    size_t length();

    /**
     * get data. T is the type of data
     *
     * @param i the index of data.
     * @return data.
     */
    void* operator[](size_t i);

 private:
    /**
     * Pointer to the actual implementation of the LoanableCollectionFlatData.
     */
    std::shared_ptr<LoanableCollectionFlatDataImpl> flat_data_impl_;
};

template <typename T, typename N = std::true_type>
class LoanableCollection {
    /**
     * Grant DataReader access to private members of this class.
     */
    friend class DataReader;

 public:
    /**
     * Default constructor. Initializes flat_data_ member.
     */
    LoanableCollection() : flat_data_(new LoanableCollectionFlatData()) {
        if constexpr (is_pointer<T>::value) {
            data_ = nullptr;
        }
    }

    /**
     * Destructor. Deletes the flat_data_ member and clears the data.
     */
    ~LoanableCollection() {
        delete flat_data_;
        clear();
    }

    template <typename X, typename Y = std::true_type>
    class Iterator {
     public:
        /**
         * Constructor that initializes the Iterator with a LoanableCollection and an index.
         *
         * @param data Pointer to the associated LoanableCollection.
         * @param index Initial position of the iterator.
         */
        Iterator(LoanableCollection<X, Y>* data, const size_t& index) : index_(index), data_(data) {}

        /**
         * Prefix increment operator. Advances the iterator to the next position.
         *
         * @return Reference to the incremented iterator.
         */
        Iterator<X, N>& operator++() {
            index_++;
            return *this;
        }

        /**
         * Equality comparison operator.
         *
         * @param iter Another iterator to compare with.
         * @return True if both iterators point to the same position in the same collection, false otherwise.
         */
        bool operator==(const Iterator<X, Y>& iter) { return data_ == iter.data_ && index_ == iter.index_; }

        /**
         * Inequality comparison operator.
         *
         * @param iter Another iterator to compare with.
         * @return True if the iterators do not point to the same position or are from different collections.
         */
        bool operator!=(const Iterator<X, Y>& iter) { return !(*this == iter); }

        /**
         * Dereference operator. Retrieves the data the iterator currently points to.
         *
         * @return Shared pointer to the data.
         */
        std::shared_ptr<X> operator*() { return data_->get_ptr(index_); }

     private:
        size_t index_;                    // Current position of the iterator.
        LoanableCollection<X, Y>* data_;  // Associated LoanableCollection.
    };

    /**
     * Retrieves the number of data elements in the collection.
     *
     * @return The number of data elements.
     */
    size_t length() const { return dds_data_.size(); }

    /**
     * Overloaded index operator to access data at a specified position.
     * This function template uses SFINAE (Substitution Failure Is Not An Error) to ensure that 
     * the overload is enabled only when the value member of the Enabler type is true.
     * If the index i is greater than or equal to the length of the array, it returns the default data element.
     * Otherwise, depending on the value of data_from_, it returns the data element from the corresponding source.
     *
     * @tparam Enabler A type parameter, defaulting to N, used for SFINAE. 
     *         This function is only enabled if Enabler::value is true.
     * @param i The index of the data at the specified position.
     * @return Reference to the data element at the specified position.
     */
    template <typename Enabler = N>
    typename std::enable_if<Enabler::value, T>::type& operator[](const size_t& i) {
        size_t len = length();
        if (i >= len) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                    << "i=" << i << " is equal or bigger than len=" << len);
            assert(false);
        }
        return *dds_data_[i];
    }

    /**
     * Index operator to access data at a specified position.
     *
     * @param i the index of data.
     * @return Reference to the data element at the specified position.
     */
    const T& operator[](const size_t& i) const {
        size_t len = length();
        if (i >= len) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                    << "i=" << i << " is equal or bigger than len=" << len);
            assert(false);
        }
        return *dds_data_[i];
    }

    /**
     * Retrieves a shared pointer to data at a specified position.
     *
     * @param i Index of the data element.
     * @return Shared pointer to the data element.
     */
    std::shared_ptr<T> get_ptr(size_t i) {
        size_t len = length();
        if (i >= len) {
            VbsLogE(VbsDecToHex(ReturnCode_t::RETCODE_BAD_PARAMETER)
                    << "i=" << i << " is equal or bigger than len=" << len);
            assert(false);
        }
        return dds_data_[i];
    }

    /**
     * Returns an iterator to the beginning of the collection.
     *
     * @return Iterator pointing to the first data element.
     */
    Iterator<T, N> begin() { return Iterator<T, N>(this, 0); }

    /**
     * Returns an iterator to the end of the collection.
     *
     * @return Iterator pointing past the last data element.
     */
    Iterator<T, N> end() { return Iterator<T, N>(this, length()); }

    /**
     * Clears the data vectors.
     */
    void clear() { dds_data_.clear(); }

    /**
     * Retrieves a pointer to the underlying LoanableCollectionFlatData instance.
     *
     * @return Pointer to the LoanableCollectionFlatData instance.
     */
    LoanableCollectionFlatData* get_flat_data() { return flat_data_; }

 private:
    std::vector<std::shared_ptr<T>> dds_data_;  // Data from DDS.
    LoanableCollectionFlatData* flat_data_;     // Underlying flat data representation.
    T data_;                                    // Default data element.
};

template <typename T, typename N = std::true_type>
class LoanableCollectionPtr {
    friend class DataReader;

 public:
    /**
     * LoanableCollectionPtr constructor.
     */
    LoanableCollectionPtr() : flat_data_(new LoanableCollectionFlatData()) {
        if constexpr (is_pointer<T>::value) {
            vail_data_ = nullptr;
        }
    }

    /**
     * LoanableCollectionPtr constructor.
     */
    ~LoanableCollectionPtr() {
        delete flat_data_;
        clear();
    }

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
    const T& operator[](const size_t& i) const {
        const size_t len = length();
        if (i >= len) {
            return vail_data_;
        }
        return data_[i];
    }

    /**
     * clear data vector
     */
    void clear() { data_.clear(); }

    /**
     * Retrieves a pointer to the underlying LoanableCollectionFlatData instance.
     *
     * @return Pointer to the LoanableCollectionFlatData instance.
     */
    LoanableCollectionFlatData* get_flat_data() { return flat_data_; }

 private:
    std::vector<T> data_;
    LoanableCollectionFlatData* flat_data_;
    T vail_data_;
};

}  // namespace vbs

#endif  // VBS_LOANABLECOLLECTION_HPP_
