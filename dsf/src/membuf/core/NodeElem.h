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

#ifndef DSFDDS_NODEELEM_H
#define DSFDDS_NODEELEM_H
#include "LinkNode.h"

namespace mbuf {

struct NodeElem : public LinkNode<NodeElem> {
    explicit NodeElem(void* node) : node(node) {}
    void* GetNode() const { return node; }

 private:
    void* node;
};

}  // namespace mbuf
#endif  // DSFDDS_NODEELEM_H
