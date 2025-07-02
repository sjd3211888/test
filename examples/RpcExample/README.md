
# 描述

本示例演示了VBS-RPC框架基于TCP通信的配置和使用。
该Demo包括一个interface "RpcDemo", 及四个operation "SyncDemo", "AsyncDemo", "AsyncStreamDemo"和"FireForgetDemo"，
分别展示了如何实现同步请求调用、异步请求调用、异步流式请求调用及请求不回复调用。

# 注意

1. 首先编写IDL文件，通过工具生成相关文件
2. 用户需要通过XML配置IP地址及端口号
3. 执行程序前需要设置环境变量以寻找所需动态库

# 代码目录

```shell
RpcExample
├── CMakeLists.txt
├── README.md
├── RpcDemo.idl                  # IDL配置文件
├── RpcDemo.cxx                  # IDL生成的消息结构体源文件
├── RpcDemo.h                    # IDL生成的消息结构体头文件
├── SafeEnum.h                   # IDL生成的辅助文件
├── RpcDemoClient.h              # IDL生成的client头文件
├── RpcDemoClientFactory.h       # IDL生成的client工厂类头文件
├── RpcDemoServerImpl.h          # IDL生成的server服务对象头文件
├── RpcDemoXML.xml:              # 用户配置XML文件
├── RpcDemoClientExample.cxx     # 用户实现client端文件
├── RpcDemoServerExample.cxx     # 用户实现server端文件
└── RpcDemoServerImplExample.cxx # 用户实现server服务对象源文件
```

# 运行

## server

```shell
$ ./RpcDemoServer
```

## client

```shell
$ ./RpcDemoClient
```

成功运行的标志：client端打印出各Demo success的日志

# 终止

client端运行完所有Demo后自行退出
server端保持运行，需要手动按 ctrl+c 终止程序
