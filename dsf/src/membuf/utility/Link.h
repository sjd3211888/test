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

#ifndef MEMBUF_SRC_UTILITY_LINK_H
#define MEMBUF_SRC_UTILITY_LINK_H

#include "LinkNode.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>

template <typename T>
struct Link {
    struct Iterator {
        Iterator() noexcept : elem_(nullptr), next_(nullptr) {}

        Iterator(T* elem) noexcept : elem_(elem), next_(next_of(elem)) {}  //NOLINT

        Iterator(const Iterator& rhs) noexcept : elem_(rhs.elem_), next_(rhs.next_) {}

        Iterator& operator=(const Iterator& other) noexcept {
            if (this != &other) {
                elem_ = other.elem_;
                next_ = other.next_;
            }
            return *this;
        }

        bool operator==(const Iterator& rhs) const noexcept { return elem_ == rhs.elem_; }

        bool operator!=(const Iterator& rhs) const noexcept { return !(*this == rhs); }

        void reset() noexcept {
            elem_ = 0;
            next_ = 0;
        }

        T* operator->() const noexcept { return elem_; }

        T& operator*() const noexcept { return *elem_; }

        T* value() const noexcept { return elem_; }

        Iterator operator++(int) noexcept {
            Iterator i = *this;

            elem_ = next_;
            next_ = next_of(elem_);

            return i;
        }

        Iterator& operator++() noexcept {
            elem_ = next_;
            next_ = next_of(elem_);

            return *this;
        }

     private:
        static T* next_of(T* elem) noexcept { return elem == 0 ? 0 : Link<T>::next_of(elem); }

        static T* prev_of(T* elem) noexcept { return elem == 0 ? 0 : Link<T>::prev_of(elem); }

     private:
        T* elem_;
        T* next_;
    };

    struct ReverseIterator {
        ReverseIterator() noexcept : elem_(0), next_(0) {}

        explicit ReverseIterator(T* elem) noexcept : elem_(elem), next_(next_of(elem)) {}

        ReverseIterator(const ReverseIterator& rhs) noexcept : elem_(rhs.elem_), next_(rhs.next_) {}

        ReverseIterator& operator=(const ReverseIterator& other) noexcept {
            if (this != &other) {
                elem_ = other.elem_;
                next_ = other.next_;
            }
            return *this;
        }

        bool operator==(const ReverseIterator& rhs) const noexcept { return elem_ == rhs.elem_; }

        bool operator!=(const ReverseIterator& rhs) const noexcept { return !(*this == rhs); }

        void reset() noexcept {
            elem_ = 0;
            next_ = 0;
        }

        T* operator->() noexcept { return elem_; }

        T& operator*() noexcept { return *elem_; }

        T* value() const noexcept { return elem_; }

        ReverseIterator operator++(int32_t) noexcept {
            ReverseIterator i = *this;

            elem_ = next_;
            next_ = next_of(elem_);

            return i;
        }

        ReverseIterator& operator++() noexcept {
            elem_ = next_;
            next_ = next_of(elem_);

            return *this;
        }

     private:
        static T* next_of(T* elem) noexcept { return elem == 0 ? 0 : Link<T>::prev_of(elem); }

     private:
        T* elem_;
        T* next_;
    };

    Link() : num_(0U) {
        head_.next_ = sentinel();
        head_.prev_ = sentinel();
    }

    Link(Link&& other) noexcept { *this = std::move(other); }

    Link& operator=(Link&& other) noexcept {
        if (this != &other) {
            this->reset();
            this->concat(other);
        }
        return *this;
    }

    bool empty() const { return head_.next_ == sentinel(); }

    size_t size() const { return num_; }

    Iterator begin() const { return head_.next_; }

    Iterator end() const { return const_cast<T*>(sentinel()); }

    ReverseIterator rbegin() const { return head_.prev_; }

    ReverseIterator rend() const { return const_cast<T*>(sentinel()); }

    bool is_back(T* elem) const { return elem == head_.prev_; }

    bool is_front(T* elem) const { return elem == head_.next_; }

    T* front() const { return empty() ? 0 : head_.next_; }

    T* back() const { return empty() ? 0 : head_.prev_; }

    void push_back(T& elem) {
        elem.link_.next_ = sentinel();
        elem.link_.prev_ = sentinel()->link_.prev_;
        sentinel()->link_.prev_->link_.next_ = &elem;
        sentinel()->link_.prev_ = &elem;

        num_++;
    }

    void push_front(T& elem) {
        elem.link_.prev_ = sentinel();
        elem.link_.next_ = sentinel()->link_.next_;
        sentinel()->link_.next_->link_.prev_ = &elem;
        sentinel()->link_.next_ = &elem;

        num_++;
    }

    void push_back(Iterator& elem) { push_back(*elem); }

    void push_front(Iterator& elem) { push_front(*elem); }

    T* pop_front() {
        Iterator i = begin();
        if (i == end()) {
            return 0;
        }

        erase(i);
        return &(*i);
    }

    void concat(Link<T>& other) {
        if (other.empty())
            return;

        other.head_.prev_->link_.next_ = sentinel();
        other.head_.next_->link_.prev_ = sentinel();
        sentinel()->link_.prev_->link_.next_ = other.head_.next_;
        sentinel()->link_.prev_ = other.head_.prev_;

        num_ += other.size();
        other.reset();
    }

    void remove(T& elem) {
        elem.remove();
        num_--;
    }

    void erase(Iterator elem) {
        if (!elem.value())
            return;

        remove(*elem);
    }

    void clear() {
        while (pop_front() != 0) {}
    }

    Iterator next_of(Iterator& i) const { return (i == end()) ? end() : ++i; }

 private:
    static T* next_of(T* elem) { return elem->link_.next_; }

    static T* prev_of(T* elem) { return elem->link_.prev_; }

    void reset() {
        head_.next_ = sentinel();
        head_.prev_ = sentinel();
        num_ = 0;
    }

    T* sentinel() {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t volatile*>(&(head_.next_)) -
                                    reinterpret_cast<uint8_t volatile*>(&(static_cast<T*>(nullptr)->link_)));
    }

    const T* sentinel() const {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t const volatile*>(&(head_.next_)) -
                                    reinterpret_cast<uint8_t volatile*>(&(static_cast<T*>(nullptr)->link_)));
    }

 private:
    typename LinkNode<T>::Chain head_;
    size_t num_;
};

#endif  // MEMBUF_SRC_UTILITY_LINK_H
