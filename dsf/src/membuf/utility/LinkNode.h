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

#ifndef MEMBUF_SRC_UTILITY_LINK_NODE_H
#define MEMBUF_SRC_UTILITY_LINK_NODE_H

template <typename T>
struct List;

template <typename T>
struct LinkNode {
    LinkNode() {
        link_.prev_ = 0;
        link_.next_ = 0;
    }

    void remove() {
        // Notice: Just used in scenes careless num of link!!!
        link_.prev_->link_.next_ = link_.next_;
        link_.next_->link_.prev_ = link_.prev_;
    }

    T* next() { return link_.next_; }

    const T* next() const { return link_.next_; }

    T* prev() { return link_.prev_; }

    const T* prev() const { return link_.prev_; }

    friend struct List<T>;

    struct Chain {
        T* volatile next_;
        T* volatile prev_;
    };  // __cacheline_aligned;

    Chain link_;
};

#endif  // MEMBUF_SRC_UTILITY_LINK_NODE_H
