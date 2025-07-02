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

#include <iostream>
#include <queue>
#include <tuple>
#include <mutex>
#include <erpc/server/RpcServer.h>
#include "RpcDemoServerImpl.h"
#include <chrono>

int64_t GetCurTime(){
    // 获取当前时间点（UTC 时间，自 1970-01-01 00:00:00）
    auto now = std::chrono::system_clock::now();
    
    // 转换为微秒精度的时间戳
    auto micro_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    );
    int64_t microseconds = micro_since_epoch.count();
    return microseconds;
    //std::cout << "微秒级时间戳: " << microseconds << std::endl;
}
extern std::mutex AsyncStream_lock;
extern std::queue<std::tuple<uint32_t, uint32_t, int32_t>> AsyncStream_queue;

// 该接口由用户实现具体内容
void RpcDemo_Impl::on_exception_detected(int32_t error_number, uint32_t conn_id) {
    (void)conn_id;

    if (error_number == vbs::rpc::ReturnMessage::CONNECTION_CLOSE) {
        std::cout << "RpcDemo: server detect exception: CONNECTION_CLOSE\n";
    }
}

// 该接口由用户实现具体内容
void RpcDemo_Impl::SyncDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req,
                            vbs::rpc::RpcMessageType* res) {
    req_type* request = dynamic_cast<req_type*>(req);
    int64_t t1 = GetCurTime();

    if (context->req_mode != vbs::rpc::RequestMode::SYNC) {
        return;
    }

    res_type* response = dynamic_cast<res_type*>(res);
    response->sum() = request->add1() + request->add2();

    int64_t t2 = GetCurTime();
    std::cout << "RpcDemo: server operation SyncDemo recv request: " << request->add1() << " " << request->add2()
              << "time  " << t1 << "   " << t2 << "\n";
    //Sync请求，自动返回响应
}

// 该接口由用户实现具体内容
void RpcDemo_Impl::AsyncDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req,
                             vbs::rpc::RpcMessageType* res) {
    int32_t ret = 0;

    req_type* request = dynamic_cast<req_type*>(req);
    int64_t t1 = GetCurTime();

    if (context->req_mode != vbs::rpc::RequestMode::ASYNC) {
        return;
    }

    res_type* response = dynamic_cast<res_type*>(res);
    response->sum() = request->add1() + request->add2();

    //Async请求，用户主动返回响应，COMPLETE表明只回复一次
    int64_t t2 = GetCurTime();
    ret = this->srv->send_async(context->conn_id, context->req_id, "AsyncDemo", response,
                                vbs::rpc::StreamStatus::COMPLETE);
    if (ret != vbs::rpc::ReturnMessage::OK) {
        std::cout << "RpcDemo: server send async response fail: " << ret << "\n";
    } else {
        std::cout << "RpcDemo: server operation AsyncDemo recv request: " << request->add1() << " " << request->add2()
              << "time  " << t1 << "   " << t2 << "\n";
        std::cout << "RpcDemo: server send async response success: " << response->sum() << "\n";
    }
}

// 该接口由用户实现具体内容
void RpcDemo_Impl::AsyncStreamDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req,
                                   vbs::rpc::RpcMessageType* res) {

    req_type* request = dynamic_cast<req_type*>(req);
    int64_t t1 = GetCurTime();


    if (context->req_mode != vbs::rpc::RequestMode::ASYNC) {
        return;
    }

    res_type* response = dynamic_cast<res_type*>(res);
    response->sum() = request->add1() + request->add2();
    int64_t t2 = GetCurTime();
    //用户存储请求信息，在其他位置（回调外）进行多次响应
    AsyncStream_lock.lock();
    AsyncStream_queue.push(std::make_tuple(context->conn_id, context->req_id, response->sum()));
    AsyncStream_lock.unlock();

    std::cout << "RpcDemo: server operation AsyncStreamDemo recv request: " << request->add1() << " " << request->add2()
              << "time  " << t1 << "   " << t2 << "\n";
}

// 该接口由用户实现具体内容
void RpcDemo_Impl::FireForgetDemo(vbs::rpc::ServerContext* context, vbs::rpc::RpcMessageType* req,
                                  vbs::rpc::RpcMessageType*) {
    req_type* request = dynamic_cast<req_type*>(req);
    int64_t t1 = GetCurTime();
    std::cout << "RpcDemo: server operation FireForgetDemo recv request: " << request->add1() << " " << request->add2()
               << "time  " << t1 << "\n";

    if (context->req_mode != vbs::rpc::RequestMode::FIRE_AND_FORGET) {
        return;
    }

    //Fire&Forget请求，无需返回响应
}