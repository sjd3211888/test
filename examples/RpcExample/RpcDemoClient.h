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

#ifndef _RpcDemoCLIENT_1431800331_H_
#define _RpcDemoCLIENT_1431800331_H_

#include "RpcDemo.h"
int64_t GetCurTime();

class RpcDemoServerObj : public vbs::rpc::client::RpcServerObj {
 public:
    RpcDemoServerObj(vbs::rpc::transport::ProxyTransport& in_transport, const std::string in_interface)
        : vbs::rpc::client::RpcServerObj(in_transport, in_interface) {}

    ~RpcDemoServerObj() {}

    int32_t SyncDemo(/*in*/ req_type* req, /*out*/ res_type* res, int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->sync_send(get_interface().c_str(), "SyncDemo", req, res, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t SyncDemo(/*in*/ req_type* req, std::function<void(int32_t, vbs::rpc::RpcMessageType*)> func,
                     int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->async_send(get_interface().c_str(), "SyncDemo", req, func, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t AsyncDemo(/*in*/ req_type* req, /*out*/ res_type* res, int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->sync_send(get_interface().c_str(), "AsyncDemo", req, res, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t AsyncDemo(/*in*/ req_type* req, std::function<void(int32_t, vbs::rpc::RpcMessageType*)> func,
                      int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->async_send(get_interface().c_str(), "AsyncDemo", req, func, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t AsyncStreamDemo(/*in*/ req_type* req, /*out*/ res_type* res, int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->sync_send(get_interface().c_str(), "AsyncStreamDemo", req, res, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t AsyncStreamDemo(/*in*/ req_type* req, std::function<void(int32_t, vbs::rpc::RpcMessageType*)> func,
                            int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->async_send(get_interface().c_str(), "AsyncStreamDemo", req, func, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t FireForgetDemo(/*in*/ req_type* req, int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->sync_send(get_interface().c_str(), "FireForgetDemo", req, nullptr, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t FireForgetDemo(/*in*/ req_type* req, std::function<void(int32_t, vbs::rpc::RpcMessageType*)> func,
                           int32_t expire) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->async_send(get_interface().c_str(), "FireForgetDemo", req, func, expire);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }

    int32_t FireForgetDemo(/*in*/ req_type* req) {
        int64_t t1 = GetCurTime();
        int32_t ret = get_transport()->fire_forget_send(get_interface().c_str(), "FireForgetDemo", req);
        int64_t t2 = GetCurTime();
        std::cout << t1 << "     " << t2 << std::endl;
        return ret;
    }
};

class RpcDemoClient : public vbs::rpc::RpcClient {
 public:
    RpcDemoClient(vbs::rpc::transport::ProxyTransport& in_transport, std::string in_interface_name)
        : vbs::rpc::RpcClient(in_transport, in_interface_name) {
        ServerObj = new RpcDemoServerObj(in_transport, in_interface_name);
    }

    ~RpcDemoClient() { delete ServerObj; }

    RpcDemoServerObj* get_serverobj() { return ServerObj; }

 private:
    RpcDemoServerObj* ServerObj = NULL;
};

#endif  // _RpcDemoCLIENT_1431800331_H_
