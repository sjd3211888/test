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

#ifndef _RpcDemoSERVER_IMPL_EXAMPLE_1850320115_H_
#define _RpcDemoSERVER_IMPL_EXAMPLE_1850320115_H_

#include "RpcDemo.h"

class RpcDemo_Impl : public vbs::rpc::server::RpcServerImpl {
 public:
    RpcDemo_Impl(std::string profile_name) : RpcServerImpl(profile_name) {
        this->exception_func =
            std::bind(&RpcDemo_Impl::on_exception_detected, this, std::placeholders::_1, std::placeholders::_2);

        this->IDL_table["SyncDemo"] =
            new vbs::rpc::server::IDL_information(std::bind(&RpcDemo_Impl::SyncDemo, this, std::placeholders::_1,
                                                            std::placeholders::_2, std::placeholders::_3),
                                                  new req_type(), new res_type());

        this->IDL_table["AsyncDemo"] =
            new vbs::rpc::server::IDL_information(std::bind(&RpcDemo_Impl::AsyncDemo, this, std::placeholders::_1,
                                                            std::placeholders::_2, std::placeholders::_3),
                                                  new req_type(), new res_type());

        this->IDL_table["AsyncStreamDemo"] =
            new vbs::rpc::server::IDL_information(std::bind(&RpcDemo_Impl::AsyncStreamDemo, this, std::placeholders::_1,
                                                            std::placeholders::_2, std::placeholders::_3),
                                                  new req_type(), new res_type());

        this->IDL_table["FireForgetDemo"] =
            new vbs::rpc::server::IDL_information(std::bind(&RpcDemo_Impl::FireForgetDemo, this, std::placeholders::_1,
                                                            std::placeholders::_2, std::placeholders::_3),
                                                  new req_type(), nullptr);
    }

    virtual ~RpcDemo_Impl() {}

    void on_exception_detected(int32_t error_number, uint32_t conn_id);

    void SyncDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req, vbs::rpc::RpcMessageType* res);

    void AsyncDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req, vbs::rpc::RpcMessageType* res);

    void AsyncStreamDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req,
                         vbs::rpc::RpcMessageType* res);

    void FireForgetDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req, vbs::rpc::RpcMessageType*);
};

#endif  // _RpcDemoSERVER_IMPL_EXAMPLE_1850320115_H_
