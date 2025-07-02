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

#ifndef _RpcDemoCLIENTFACTORY_74726400_H_
#define _RpcDemoCLIENTFACTORY_74726400_H_

#include "RpcDemo.h"
#include "RpcDemoClient.h"

class RpcDemoFactory : public vbs::rpc::client::RpcClientFactory {
 public:
    vbs::rpc::RpcClient* CreateRpcClient(const std::string& profile_file_name,
                                         const std::string& profile_name) override {
        vbs::rpc::rpcxml::RpcXMLParser rpcConfig(profile_file_name);
        rpcConfig.RpcXML_Init_Client(profile_name);
        if (rpcConfig.RpcXML_Init_Client(profile_name) != vbs::rpc::ReturnMessage::OK) {
            RpcLogError(RPC_CLIENT, "load xml err in client init") return NULL;
        }

        std::string Proxy_Name = rpcConfig.RpcXML_Client_get_proxy();
        vbs::rpc::ProxyMB* ProxyMB_Obj = nullptr;
        ProxyMB_Obj = CreateRpcProxyMB(Proxy_Name);
        if (ProxyMB_Obj == nullptr) {
            ProxyMB_Obj = new vbs::rpc::ProxyMB(rpcConfig);
            std::function<vbs::rpc::ProxyMB*()> func = [ProxyMB_Obj]() {
                return ProxyMB_Obj;
            };
            RegisterRpcProxyMB(Proxy_Name, func);
            ProxyMB_Obj = CreateRpcProxyMB(Proxy_Name);
        }

        std::string interface_cfg = rpcConfig.RpcXML_Client_get_class();

        if (false) {
        }

        else if (interface_cfg == "RpcDemo") {
            vbs::rpc::RpcClient* RetRpcClient = ProxyMB_Obj->CreateRpcClient(interface_cfg);
            if (RetRpcClient == nullptr) {
                RetRpcClient = new RpcDemoClient(*ProxyMB_Obj->transport, interface_cfg);
                std::function<vbs::rpc::RpcClient*()> client_func = [RetRpcClient]() {
                    return RetRpcClient;
                };
                RetRpcClient->set_proxyMB(ProxyMB_Obj);
                ProxyMB_Obj->RegisterRpcClient(interface_cfg, client_func);
            }

            std::string operation_name;
            std::string register_name;
            operation_name = "SyncDemo";
            register_name = interface_cfg + operation_name;
            ProxyMB_Obj->registerClass<res_type>(register_name);
            operation_name = "AsyncDemo";
            register_name = interface_cfg + operation_name;
            ProxyMB_Obj->registerClass<res_type>(register_name);
            operation_name = "AsyncStreamDemo";
            register_name = interface_cfg + operation_name;
            ProxyMB_Obj->registerClass<res_type>(register_name);

            return RetRpcClient;
        }
        return NULL;
    }
};
#endif  // _RpcDemoCLIENTFACTORY_74726400_H_
