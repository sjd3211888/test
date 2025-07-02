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

#include <queue>
#include <tuple>
#include <mutex>
#include <erpc/server/RpcServer.h>
#include <erpc/server/RpcServerFactory.h>
#include "RpcDemoServerImpl.h"

vbs::rpc::RpcServer* server = nullptr;
std::mutex AsyncStream_lock;
std::queue<std::tuple<uint32_t, uint32_t, int32_t>> AsyncStream_queue;
int64_t GetCurTime();

void handle_AsyncStream() {
    int32_t ret = 0;
    int32_t sum = 0;
    uint32_t conn_id = 0;
    uint32_t req_id = 0;

    AsyncStream_lock.lock();

    if (AsyncStream_queue.size() == 0) {
        AsyncStream_lock.unlock();
        return;
    }

    auto& AsyncStream_info = AsyncStream_queue.front();

    AsyncStream_queue.pop();

    AsyncStream_lock.unlock();

    conn_id = std::get<0>(AsyncStream_info);
    req_id = std::get<1>(AsyncStream_info);
    sum = std::get<2>(AsyncStream_info);

    for (int32_t i = 1; i <= 3; i++) {  //AsyncStream总共发送3次
        res_type response = {sum};

        //Async请求流式响应，用户主动多次返回响应，CONTINUE表明后续还有回复，COMPLETE表明最后一次回复
        if (i == 3) {
            ret = server->send_async(conn_id, req_id, "AsyncStreamDemo", &response, vbs::rpc::StreamStatus::COMPLETE);
        } else {
            ret = server->send_async(conn_id, req_id, "AsyncStreamDemo", &response, vbs::rpc::StreamStatus::CONTINUE);
        }

        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcDemo: server send async stream response fail: " << ret << "\n";
        } else {
            std::cout << "RpcDemo: server send async stream response success: " << sum << "\n";
        }
    }
}

int main() {
    int32_t ret = 0;
    vbs::rpc::RpcServerFactory factory;

    //创建server，参数XML路径及想要创建的server的profile name
    server = factory.CreateRpcServer("RpcDemoXML.xml", "RpcDemo");
    if (server == nullptr) {
        std::cout << "RpcDemo: create rpc server fail\n";
        return -1;
    }

    std::cout << "RpcDemo: create rpc server success\n";

    //创建用户实现的服务对象，注意参数为profile name，必须和创建server的profile name一致
    RpcDemo_Impl impl_EEA("RpcDemo");

    //注册服务对象，该对象的生命周期必须长与server对象的生命周期
    ret = server->register_impl(&impl_EEA);
    if (ret != vbs::rpc::ReturnMessage::OK) {
        std::cout << "RpcDemo: server register impl fail\n";
        return -1;
    }

    std::cout << "RpcDemo: server register impl success\n";

    while (1) {
        vbs::rpc::sleep(100);
        handle_AsyncStream();
    }

    //销毁server
    factory.DestroyRpcServer(server);
}
