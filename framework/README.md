## 1. 代码仓库简介

framework作为应用层和通信协议层的中间衔接层，其核心作用如下:

* **易用性：** 设计清晰、简洁的接口，易于使用。遵循一致性原则，保持接口的一致性和统一性，避免过于复杂和冗长的接口。考虑用户的使用场景和需求，提供方便的操作函数和数据访问函数。
* **错误处理：** 考虑错误处理和异常情况的处理方式，以提高健壮性和可靠性。使用错误码和回调函数来处理错误情况，并为用户提供详细的错误信息。
* **数据封装和隐藏：** 将实现细节隐藏在类的内部，提供公共接口来访问和操作数据。通过抽象接口类、模版数据类型、前置声明等手段封装数据，保护数据的完整性和一致性，减少对内部实现的直接访问，确保底层数据类型对用户隐藏。
* **文档和示例：** 提供清晰、详细的文档和示例代码，使得用户可以快速了解和使用API。文档应该包括API的功能、接口说明、示例用法以及常见问题的解答。

## 2. 代码目录

framework 源代码在 haloosspace/vbs/vbspro/framework目录下，目录结构如下图所示：

```Plain
framework
├── README.md                    # framework仓库的readme
├── CMakeLists.txt               # cmake编译脚本
├── vui                          # 用户接口层：vbs user interface，提供易用的通信组件接口封装
│   ├── include/vbs              # 包含participant、topic、reader、writer相关通信组件的接口封装
│   │   ├── qos                  # vbs相关qos配置的接口封装
│   │   ├── status               # vbs相关状态的接口封装
│   │   └──  types               # 动态数据类型相关的接口封装
│   ├── src                      # 同include
│   │   ├── status
│   │   └──  types
├── vmw                          # 框架层：vbs Middleware wrapper，通信组件的具体实现
│   ├── include
│   │   ├── builtin              # 内置的reader+writer通信组合
│   │   ├── context              # 运行时上下文支持
│   │   ├── core                 # 运行时功能组件
│   │   ├── dispatcher           # 通信链路路由
│   │   ├── domain               # participant通信组件
│   │   ├── pub                  # publisher通信组件
│   │   ├── qos                  # 通信服务质量配置
│   │   ├── sub                  # subscriber通信组件
│   │   ├── topic                # topic通信组件
│   │   └── utils                # 其他功能组件
│   ├── src                      # 同include
│   │   ├── builtin
│   │   ├── context
│   │   ├── core
│   │   ├── dispatcher
│   │   ├── domain
│   │   ├── pub
│   │   ├── qos
│   │   ├── sub
│   │   ├── topic
└   └   └── utils
```

## 3. 使用入门

请参考: [示例指南](../docs/developer-guide/vbspro_examples.md)
