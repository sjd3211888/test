# **IDLGEN**

## **项目概述**

IDLGEN是一个功能强大的接口定义语言(IDL)代码生成器，支持多种编程语言和通信协议。它能够将IDL文件转换为各种目标语言的代码，包括C++、Java、C、Rust等，并支持DDS、REST、VBSCDR等多种通信协议。

## **主要功能**

### **1. 多语言代码生成**

- 支持C++、Java、C、Rust等多种编程语言
- 自动生成类型定义、序列化代码
- 支持模板化代码生成

### **2. 多种通信协议支持**

- **DDS** **模式** : 支持发布/订阅模式，支持兼容RTI和VBS
- **RPC** **模式** : 支持远程过程调用

## **项目结构**

```Python
idlgen/
├── .git/                          # Git版本控制目录
├── .gradle/                       # Gradle缓存目录
├── build/                         # 构建输出目录
├── thirdparty/                    # 第三方依赖
│   ├── idl-parser/                # IDL语法解析器
│   └── vbscdr/                    # VBS CDR序列化库
├── src/                           # 源代码目录
│   └── main/
│       └── java/
│           └── com/
│               └── eprosima/
│                   ├── vbsdds/   # DDS模式代码生成器
│                   │   ├── vbsddsgen.java          # DDS生成器主类
│                   │   ├── exceptions/             # 异常处理类
│                   │   ├── idl/                    # IDL处理模块
│                   │   │   ├── grammar/            # 语法解析
│                   │   │   └── templates/          # 代码模板
│                   │   │       ├── cpp/            # C++模板
│                   │   │       ├── java/           # Java模板
│                   │   │       ├── c/              # C模板
│                   │   │       ├── rust/           # Rust模板
│                   │   │       └── xml/            # XML模板
│                   │   ├── solution/               # 解决方案生成
│                   │   └── util/                   # 工具类
│                   ├── vbsrpc/   # RPC模式代码生成器
│                   │   ├── vbsrpcgen.java          # RPC生成器主类
│                   │   ├── exceptions/             # 异常处理类
│                   │   ├── idl/                    # IDL处理模块
│                   │   │   ├── grammar/            # 语法解析
│                   │   │   └── templates/          # 代码模板
│                   │   ├── solution/               # 解决方案生成
│                   │   ├── util/                   # 工具类
│                   │   └── wadl/                   # WADL处理模块
│                   └── vbscdr/   # CDR序列化支持
│                       ├── idl/                    # IDL处理
│                       └── exceptions/             # 异常处理
├── scripts/                      # 脚本文件
│   ├── idlgen                    # Linux/Mac启动脚本
│   ├── idlgen.bat                # Windows启动脚本
│   └── idlgen.in                 # 脚本模板
├── cmake/                        # CMake配置文件
│   └── dev/
│       └── java_support.cmake    # Java支持配置
├── gradle/                       # Gradle包装器
│   └── wrapper/
│       ├── gradle-wrapper.jar
│       └── gradle-wrapper.properties
├── .cz.json                     # Commitizen配置
├── .gitignore                   # Git忽略文件
├── .pre-commit-config.yaml      # 预提交钩子配置
├── CMakeLists.txt               # CMake主配置文件
├── LICENSE                      # 许可证文件
├── README.md                    # 项目说明文档
├── build.gradle                 # Gradle构建配置
├── colcon.pkg                   # Colcon包配置
├── conanfile.py                 # Conan包管理器配置
└── gradlew                      # Gradle包装器脚本
```

## **使用入门**

详细的使用指南和开发文档请参考：VBSPRO IDL-GEN 使用说明书
