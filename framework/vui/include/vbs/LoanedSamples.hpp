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

#ifndef VBS_LOANEDSAMPLES_HPP_
#define VBS_LOANEDSAMPLES_HPP_

#include "vbs/LoanableCollection.hpp"
#include "vbs/SampleInfo.hpp"

namespace vbs {

class LoanedSampleInfo {
 public:
    /**
     * Constructor
     * 
     * @note @li Thread-Safe: Yes
     */
    LoanedSampleInfo();

    /**
     * Copy Constructor
     * 
     * @note @li Thread-Safe: Yes
     */
    LoanedSampleInfo(const LoanedSampleInfo& sample_info);

    /**
     * Constructor with parameters
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param valid valid_data
     * @param len payload_len
     * @param stimestamp source_timestamp
     * @param rtimestamp reception_timestamp
     * @param pub_sequence_number publication_sequence_number
     * @param recept_sequence_number reception_sequence_number
     * @param e2e_status e2e_exception_status
     */
    LoanedSampleInfo(bool valid, uint32_t len, int64_t stimestamp, int64_t rtimestamp, uint64_t pub_sequence_number,
                     uint64_t recept_sequence_number, const E2EExceptionStatus& e2e_status);

    /**
     * Deonstructor
     * 
     * @note @li Thread-Safe: Yes
     */
    ~LoanedSampleInfo();

    /**
     * Determines if the data at a given index is valid.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return true if data is valid; otherwise, false.
     */
    bool valid_data();

    /**
     * Retrieves the length of data before it was serialized.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Length of the data in its original (unserialized) form.
     */
    uint32_t payload_len();

    /**
     * Retrieves the timestamp when the data was originally created.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Timestamp of the data's creation.
     */
    int64_t source_timestamp();

    /**
     * Retrieves the timestamp when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Timestamp of the data's reception.
     */
    int64_t reception_timestamp();

    /**
     * Retrieves the sequence number for the data's publication.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Publication sequence number.
     */
    uint64_t publication_sequence_number();

    /**
     * Retrieves the sequence number for when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Reception sequence number.
     */
    uint64_t reception_sequence_number();

    /**
     * Retrieves the e2e exception status for when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return e2e exception status.
     */
    E2EExceptionStatus e2e_exception_status();

 private:
    bool valid_data_;
    uint32_t payload_len_;
    int64_t source_timestamp_;
    int64_t reception_timestamp_;
    uint64_t publication_sequence_number_;
    uint64_t reception_sequence_number_;
    E2EExceptionStatus e2e_exception_status_;
};

template <typename T, typename N>
class LoanedSamples;

template <typename T, typename N = std::true_type>
class LoanedSamplesIterator {
 public:
    /**
     * Constructor that initializes the Iterator with a LoanableCollection and an index.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param data Pointer to the associated LoanableCollection.
     * @param index Initial position of the iterator.
     */
    LoanedSamplesIterator(LoanedSamples<T, N>* data, const size_t& index) : index_(index), data_(data) {}

    /**
     * Prefix increment operator. Advances the iterator to the next position.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Reference to the incremented iterator.
     */
    LoanedSamplesIterator<T, N>& operator++() {
        index_++;
        return *this;
    }

    /**
     * Equality comparison operator.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param iter Another iterator to compare with.
     * @return True if both iterators point to the same position in the same collection, false otherwise.
     */
    bool operator==(const LoanedSamplesIterator<T, N>& iter) { return data_ == iter.data_ && index_ == iter.index_; }

    /**
     * Inequality comparison operator.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param iter Another iterator to compare with.
     * @return True if the iterators do not point to the same position or are from different collections.
     */
    bool operator!=(const LoanedSamplesIterator<T, N>& iter) { return !(*this == iter); }

    /**
     * Dereference operator. Retrieves the data the iterator currently points to.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Shared pointer to the data.
     */
    std::shared_ptr<T> operator*() { return data_->get_ptr(index_); }

    /**
     * get_ptr. Retrieves the data the iterator currently points to.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Shared pointer to the data.
     */
    std::shared_ptr<T> get_ptr() { return data_->get_ptr(index_); }

    /**
     * get data. T is the type of data
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return data.
     */
    template <typename Enabler = N>
    typename std::enable_if<Enabler::value, T>::type& get_data() {
        return (*data_)[index_];
    }

    /**
     * get data. T is the type of data
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return data.
     */
    const T get_data() const { return (*data_)[index_]; }

    /**
     * get info
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return info.
     */
    LoanedSampleInfo get_info() const {
        return LoanedSampleInfo(data_->valid_data(index_), data_->payload_len(index_), data_->source_timestamp(index_),
                                data_->reception_timestamp(index_), data_->publication_sequence_number(index_),
                                data_->reception_sequence_number(index_), data_->e2e_exception_status(index_));
    }

 private:
    size_t index_;               // Current position of the iterator.
    LoanedSamples<T, N>* data_;  // Associated LoanableCollection.
};

template <typename T, typename N = std::true_type>
class LoanedSamples {
    friend class DataReader;

 public:
    /**
     * Default constructor for SampleInfo.
     * 
     * @note @li Thread-Safe: Yes
     */
    LoanedSamples() : is_flatdata_(false) {
        datas_ = std::make_shared<LoanableCollection<T, N>>();
        flat_datas_ = std::make_shared<LoanableCollectionPtr<T, N>>();
        infos_ = std::make_shared<SampleInfo>();
    }

    /**
     * Destructor for SampleInfo. It's declared as virtual to ensure correct 
     * behavior when deleting objects through pointers-to-base.
     * 
     * @note @li Thread-Safe: Yes
     */
    ~LoanedSamples() {
        datas_ = nullptr;
        flat_datas_ = nullptr;
        infos_ = nullptr;
    }

    /**
     * Returns an iterator to the beginning of the collection.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Iterator pointing to the first data element.
     */
    LoanedSamplesIterator<T, N> begin() { return LoanedSamplesIterator<T, N>(this, 0); }

    /**
     * Returns an iterator to the end of the collection.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return Iterator pointing past the last data element.
     */
    LoanedSamplesIterator<T, N> end() { return LoanedSamplesIterator<T, N>(this, length()); }

    /**
     * Retrieves the number of data elements in the collection.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @return The number of data elements.
     */
    size_t length() const {
        if (is_flatdata_) {
            return flat_datas_->length();
        }
        return datas_->length();
    }

    /**
     * Overloaded index operator to access data at a specified position.
     * This function template uses SFINAE (Substitution Failure Is Not An Error) to ensure that 
     * the overload is enabled only when the value member of the Enabler type is true.
     * If the index i is greater than or equal to the length of the array, it returns the default data element.
     * Otherwise, depending on the value of data_from_, it returns the data element from the corresponding source.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @tparam Enabler A type parameter, defaulting to N, used for SFINAE. 
     *         This function is only enabled if Enabler::value is true.
     * @param i The index of the data at the specified position.
     * @return Reference to the data element at the specified position.
     */
    template <typename Enabler = N>
    typename std::enable_if<Enabler::value, T>::type& operator[](const size_t& i) {
        if (is_flatdata_) {
            return (*flat_datas_)[i];
        }
        return (*datas_)[i];
    }

    /**
     * Index operator to access data at a specified position.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i the index of data.
     * @return Reference to the data element at the specified position.
     */
    const T& operator[](const size_t& i) {
        if (is_flatdata_) {
            return (*flat_datas_)[i];
        }
        return (*datas_)[i];
    }

    /**
     * Retrieves a shared pointer to data at a specified position.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the data element.
     * @return Shared pointer to the data element.
     */
    std::shared_ptr<T> get_ptr(size_t i) {
        if (is_flatdata_) {
            return nullptr;
        }
        return datas_->get_ptr(i);
    }

    /**
     * Clears the data vectors.
     * 
     * @note @li Thread-Safe: Yes
     */
    void clear() {
        datas_->clear();
        flat_datas_->clear();
        infos_->clear();
    }

    /**
     * Determines if the data at a given index is valid.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return true if data is valid; otherwise, false.
     */
    bool valid_data(const size_t& i) { return infos_->valid_data(i); }

    /**
     * Retrieves the length of data before it was serialized.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return Length of the data in its original (unserialized) form.
     */
    uint32_t payload_len(const size_t& i) { return infos_->payload_len(i); }

    /**
     * Retrieves the timestamp when the data was originally created.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return Timestamp of the data's creation.
     */
    int64_t source_timestamp(const size_t& i) { return infos_->source_timestamp(i); }

    /**
     * Retrieves the timestamp when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return Timestamp of the data's reception.
     */
    int64_t reception_timestamp(const size_t& i) { return infos_->reception_timestamp(i); }

    /**
     * Retrieves the sequence number for the data's publication.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return Publication sequence number.
     */
    uint64_t publication_sequence_number(const size_t& i) { return infos_->publication_sequence_number(i); }

    /**
     * Retrieves the sequence number for when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return Reception sequence number.
     */
    uint64_t reception_sequence_number(const size_t& i) { return infos_->reception_sequence_number(i); }

    /**
     * Retrieves the e2e exception status for when the data was received.
     * 
     * @note @li Thread-Safe: Yes
     *
     * @param i Index of the desired data sample.
     * @return e2e exception status.
     */
    E2EExceptionStatus e2e_exception_status(const size_t& i) { return infos_->e2e_exception_status(i); }

 private:
    std::shared_ptr<LoanableCollection<T, N>> datas_;
    std::shared_ptr<LoanableCollectionPtr<T, N>> flat_datas_;
    std::shared_ptr<SampleInfo> infos_;
    bool is_flatdata_;
};

}  // namespace vbs

#endif  // VBS_LOANEDSAMPLES_HPP_
