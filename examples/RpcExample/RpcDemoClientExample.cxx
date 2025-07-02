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

#include "RpcDemoClient.h"
#include "RpcDemoClientFactory.h"

#include <unistd.h>
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

RpcDemoServerObj* ServerObj = nullptr;

void RPC_Sync_Demo() {
    int32_t ret = 0;

    std::cout << "RpcSyncDemo: RPC_Sync_Demo start\n";

    for (int32_t i = 0; i < 10; i++) {
        req_type req = {i, i};
        res_type res;

        //发送Sync请求，阻塞等待响应并检查调用结果，-1为永不超时
        ret = ServerObj->SyncDemo(&req, &res, -1);
        if (ret != vbs::rpc::ReturnMessage::OK || res.sum() != i + i) {
            std::cout << "RpcSyncDemo: client send sync request fail: " << ret << "\n";
            return;
        } else {
            std::cout << "RpcSyncDemo: client send sync request success: " << i << " " << i << "\n";
        }

        vbs::rpc::sleep(100);  // 间隔100毫秒
    }

    std::cout << "RpcSyncDemo: RPC_Sync_Demo success\n";
}

void RPC_Async_Demo() {
    int32_t ret = 0;
    int32_t demo_pass = 0;
    int32_t expected_sum = 0;

    std::cout << "RpcAsyncDemo: RPC_Async_Demo start\n";

    //实现异步回调函数
    auto Async_callback = [&](int32_t ret, vbs::rpc::RpcMessageType* res) {
        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcAsyncDemo: client recv async request result fail " << ret << "\n";
            demo_pass = -1;
            return;
        }
        res_type* response = static_cast<res_type*>(res);
        if (response->sum() != expected_sum) {
            std::cout << "RpcAsyncDemo: client recv async request result fail " << response->sum() << "\n";
            demo_pass = -1;
            return;
        } else {
            std::cout << "RpcAsyncDemo: client recv async request result success: " << response->sum() << "\n";
        }

        expected_sum += 2;

        if (expected_sum == 10 * 2) {
            demo_pass = 1;
        }
    };

    for (int32_t i = 0; i < 10; i++) {
        req_type req = {i, i};

        //发送Async请求，异步返回响应并检查调用结果，超时时间500毫秒
        ret = ServerObj->AsyncDemo(&req, Async_callback, 500);
        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcAsyncDemo: client send async request fail: " << ret << "\n";
            return;
        } else {
            std::cout << "RpcAsyncDemo: client send async request success: " << i << " " << i << "\n";
        }

        vbs::rpc::sleep(100);  // 间隔100毫秒
    }

    while (1) {
        vbs::rpc::sleep(100);
        if (demo_pass == 1) {
            break;
        }
        if (demo_pass == -1) {
            return;
        }
    }

    std::cout << "RpcAsyncDemo: RPC_Async_Demo success\n";
}

void RPC_AsyncStream_Demo() {
    int32_t ret = 0;
    int32_t demo_pass = 0;
    int32_t AsyncStream_cnt = 0;

    req_type req = {0, 0};

    //实现异步回调函数
    auto Async_callback = [&](int32_t ret, vbs::rpc::RpcMessageType* res) {
        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcAsyncStreamDemo: client recv async stream request result fail " << ret << "\n";
            demo_pass = -1;
            return;
        }

        res_type* response = static_cast<res_type*>(res);

        std::cout << "RpcAsyncStreamDemo: client recv async stream request result: " << response->sum() << "\n";

        AsyncStream_cnt++;

        if (AsyncStream_cnt == 3 * 10) {
            demo_pass = 1;
        }
    };

    for (int32_t i = 0; i < 10; i++) {
        req_type req = {i, i};

        //发送Async请求，流式返回响应并检查调用结果，永不超时
        ret = ServerObj->AsyncStreamDemo(&req, Async_callback, -1);
        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcAsyncStreamDemo: client send async stream request fail: " << ret << "\n";
            return;
        } else {
            std::cout << "RpcAsyncStreamDemo: client send async stream request success: " << i << " " << i << "\n";
        }

        vbs::rpc::sleep(100);  // 间隔1000毫秒
    }

    while (1) {
        vbs::rpc::sleep(500);
        if (demo_pass == 1) {
            break;
        }
        if (demo_pass == -1) {
            return;
        }
    }

    std::cout << "RpcAsyncStreamDemo: RPC_AsyncStream_Demo success\n";
}

void RPC_FireForget_Demo() {
    int32_t ret = 0;

    for (int32_t i = 0; i < 500; i++) {
        req_type req = {i, i};

        //发送Fire&Forget请求，检查调用结果，无响应，直接返回
        ret = ServerObj->FireForgetDemo(&req);
        if (ret != vbs::rpc::ReturnMessage::OK) {
            std::cout << "RpcFireForgetDemo: client send fire&forget request fail: " << ret << "\n";
            return;
        } else {
            std::cout << "RpcFireForgetDemo: client send fire&forget request success: " << i << " " << i << "\n";
        }

        vbs::rpc::sleep(100);  // 间隔100毫秒
    }

    std::cout << "RpcFireForgetDemo::RPC_FireForget_Demo success\n";
}

int main() {
    RpcDemoFactory factory;

    //创建client并转换为IDL对应类型的子类，参数XML路径及想要创建的client的profile name
    RpcDemoClient* client = static_cast<RpcDemoClient*>(factory.CreateRpcClient("RpcDemoXML.xml", "RpcDemo"));
    if (client == nullptr) {
        std::cout << "RpcDemo: create rpc client fail\n";
        return -1;
    }

    std::cout << "RpcDemo: create rpc client success\n";

    //从client中获取ServerObj
    ServerObj = client->get_serverobj();

    RPC_Sync_Demo();

    RPC_Async_Demo();

    RPC_AsyncStream_Demo();

    RPC_FireForget_Demo();

    //销毁client
    factory.DestroyRpcClient(client);
}