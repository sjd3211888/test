# vbspro_daemon代码仓库说明

## 1. 代码仓库简介

daemon仓库用于集中存放各类实用脚本、工具类程序及自动化代码，旨在提升开发、测试、部署与运维效率。

## 2. 代码目录

vbspro_daemon 源代码在 haloosspace/vbs/vbspro/tools 目录下，目录结构如下图所示：

```Plain
tools
├── daemon
│   ├── CMakeLists.txt
│   ├── discovery_server        # 中心化服务发现server
│   └── vbs_cli                 # 统计和调测工具
```

## 3. 主要功能说明

* 中心化服务发现server

中心化架构通信中，中心化服务节点是单独的应用。只有启动中心化服务节点，writer和reader才可以通过中心化服务节点完成发现后，进行通信。为了保证发现速度，需要先启动中心化服务节点。

* 统计工具

支持实时查看VBS节点的各种状态（包括发现状态、报文统计等），及动态修改调测配置。

* 调测工具
ping: 信号模拟仿真工具，通过简单的命令行参数设置，模拟DDS信号。需要指定数据类型对应的xml，使用VBS的动态类型机制构建或者解析CDR数据，同时也支持RTT测试
spy: 用于查看当前网络下所有的实体信息，能够动态监测数据流
record: DDS数据录制工具，能够录制任意domain下的任意topic的数据，依赖topic的类型发现机制，数据保存为sqlite文件
replay: 用于对record记录的sqlite数据文件进行数据回灌
