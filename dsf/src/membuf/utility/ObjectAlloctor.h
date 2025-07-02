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

#ifndef MEMBUF_SRC_UTILITY_OBJECT_ALLOCTOR_H
#define MEMBUF_SRC_UTILITY_OBJECT_ALLOCTOR_H

#include "Link.h"
#include "DsfLog.hpp"
#include <new>
#include <utility>
#include "membuf/BaseType.h"

template <typename T>
class ObjectAllocator {
 public:
    explicit ObjectAllocator(size_t capacity) {
        for (size_t i = 0U; i < capacity; i++) {
            auto elem = new (std::nothrow) Element;
            if (elem != nullptr) {
                elems_.push_back(elem->node_);
            } else {
                DSF_LOG_ERROR(DSF_MEMBUF, mbuf::ReturnCode_t::RETCODE_MEMBUF_ELEM_IS_NULL, "elem is nullptr!");
            }
        }
    }

    ~ObjectAllocator() {
        while (!elems_.empty()) {
            auto elem_head = elems_.pop_front();
            if (elem_head) {
                Element* elem = reinterpret_cast<Element*>(elem_head);
                delete elem;
            }
        }
    }

    // Alloc memory but do not construct !!!
    T* Alloc() {
        Element* elem = reinterpret_cast<Element*>(elems_.pop_front());  //NOLINT
        if (!elem)
            return nullptr;
        return reinterpret_cast<T*>(elem->buff_);  //NOLINT
    }

    // Free memory but do not destruct !!!
    void Free(T& elem) { elems_.push_front(*(reinterpret_cast<ElemNode*>(&elem))); }  //NOLINT

    // Alloc memory and construct !!!
    template <class... Args>
    T* New(Args&&... args) {
        return new (Alloc()) T(std::forward<Args>(args)...);
    }

    // Free memory and destruct !!!
    void Delete(T& elem) {
        static_cast<void>(elem.~T());
        Free(elem);
    }

    size_t GetAvailableSize() const { return elems_.size(); }

 private:
    struct ElemNode : LinkNode<ElemNode> {};

    union Element {
        Element() {}
        ElemNode node_;
        char buff_[sizeof(T)];
    };

 private:
    Link<ElemNode> elems_;
};

#endif  // MEMBUF_SRC_UTILITY_OBJECT_ALLOCTOR_H
