执行./vbs_cli -whitelist [设备ip] 后，输入下列查询命令(其中"<>"中参数是必选，"[]"中参数为可选)：
~. 查询指定domain下的participant信息
part <domain id> [pid:-1] [detail:0] 回车
~. 查询指定participant下writer、reader的概要信息
part-detail <domain id> <guid> [timeout] 回车
~. 查询指定writer/reader匹配的对端guid信息
match <domain id> <guid> [timeout] 回车
~. 查询指定writer的发包信息
send <domain id> <guid> [timeout] 回车
~. 查询指定reader的收包信息
recv <domain id> <guid> [timeout] 回车
~. 查询指定writer/reader的qos信息
qos <domain id> <guid> [timeout] 回车
~. 查询与指定reader/writer的match的writer/reader的proxy信息
proxy <domain> <guid> [timeout];
~. 查询指定domain，指定topic下的所有reader writer guid信息
topic <domain id> <topic_name> [timeout] 回车
~. 修改日志等级
config <domain> <pid> logLevel:0/1/2/4/8 [host name:default using local host name] [timeout]
~. 修改日志抑制周期
config <domain> <pid> logPeriod:0 [host name:default using local host name] [timeout];
~. 增加MessageBrief/messageTrace中过滤使用的的topic信息(配置成ALL过滤所有topic)
config <domain> <pid> addTopicFilter:topicName [host name:default using local host name] [timeout]
~. 设置brief记录模式
config <domain> <pid> briefOutputMode:0/1/2 [host name:default using local host name] [timeout]
~. 设置brief内存中存储的最大日志条数
config <domain> <pid> briefOutLimit:N [host name:default using local host name] [timeout]
~. 查询指定进程、topic的历史消息概要信息
msgbrief <domain> <pid> <topic_name> [host name:default using local host name] [timeout]
~. 清除MessageBrief中所有信息
config <domain> <pid> clearMsgBrief [host name:default using local host name] [timeout]
~. 设置message traceMask(可设置为65535 trace所有节点)
config <domain> <pid> msgTraceMask:N [host name:default using local host name] [timeout]
~. 清除messageTrace中所有信息
config <domain> <pid> clearMsgTrace [host name:default using local host name] [timeout]


typediscovery：
执行./vbs_cli后
1、发现所有类型（默认domain 49）
typediscovery
2、发现指定类型
typediscovery -type 类型名
3、指定运行时间，单位毫秒
typediscovery -wait 20000 -type 类型名
4、打印数据内容
typediscovery -data -type 类型名
全量参数实例：
typediscovery -type TypeLookup -wait 10000 -logLevel 0 -domain 21 -check -data
例：可以启动pub：./TypeLookupExample publisher 0



dsf命令
## membuf_cli使用方法说明
本工具支持list、info、remove（todo）、bufPool、bufNode、bufNodeAll enableDfx 功能，以如下测试用例为例：
```
TEST(SHMMessageObjectManagement, FUNCTEST_MEMMBUF_009_01) {
  {
    const auto pool = BufferPoolOwner::CreateBufferPoolOwner("pool_09_01", 4 * 1024, 4, 1024, getpid(), 512U, Normal);
    // printf("pool size:%ld\n", param.poolSize);
    constexpr PID ReaderPid = 10U;
    const auto userId = pool->RegisterUser(ReaderPid);
    const auto userId2 = pool->RegisterUser(12U);
    const std::shared_ptr<Buffer> buffer_0 = pool->AllocBuffer(1024);
    const std::shared_ptr<Buffer> buffer_1 = pool->AllocBuffer(1024);
    const std::shared_ptr<Buffer> buffer_2 = pool->AllocBuffer(1024);
    sleep(1000);
  }
  BufferPoolUserManager::GetInstance().ClosePool();
}
```
###  使用实例:list
运行指令
```
./vbs_cli membuf_cli list
```
运行结果
```
name                                              size                owner process       refcount            uuid                related process
pool_09_01                                        8192                2316357             2                   1879048195          -1
shmmq_DiscoveryServiceRecvQueue_DFX               208896              2237925             2                   1879048194          -1
shmmq_DiscoveryServiceRecvQueue                   835584              0                   2                   1879048193          2237925
```
###  使用实例:info
运行指令
```
./vbs_cli membuf_cli info --UUID=1879048195
```
运行结果
```
pool_Id        poolSize       bufCount       bufMaxSize     shareMemPtr         Fd   refState  offset         used           free           usedCount      freeCount      maxAllocSize   minAllocSize
1879048216     4608           4              2048           0x7f3a3d108000      4    0         512            0              4608           0              4              2048           10

```
###
###  使用实例:remove(todo)
运行指令
```
./vbs_cli membuf_cli remove --UUID=1879048195
```
运行结果
```

```
###
###  使用实例:bufpool
运行指令
```
./vbs_cli membuf_cli bufpool --UUID=1879048195
```
运行结果
```
UserPids: 0         1         2         3         4         5         6         7         8         9         10        11
          2339693   10        12        0         0         0         0         0         0         0         0         0

UserStates: 0              1              2              3              4              5              6              7              8              9              10             11
            using          using          using          invalid        invalid        invalid        invalid        invalid        invalid        invalid        invalid        invalid
```
###
###  使用实例:bufNodeAll
运行指令
```
./vbs_cli membuf_cli bufNodeAll --UUID=1879048195
```
运行结果
```
RefState: 0       1       2       3       4       5       6       7       8       9       10      11
          using   free    free    free    free    free    free    free    free    free    free    free
          using   free    free    free    free    free    free    free    free    free    free    free
          using   free    free    free    free    free    free    free    free    free    free    free
          free    free    free    free    free    free    free    free    free    free    free    free
```
###  使用实例:bufNode
运行指令及结果
```
lixiang@PC-YLX3VRF4:~/ztt/vbs-membuf7/dsf/build/tools$ ./vbs_cli membuf_cli bufNode --UUID=1879048195 --bufNodeIndex=0
bufnode0 :
RefState: 0       1       2       3       4       5       6       7       8       9       10      11
          using   free    free    free    free    free    free    free    free    free    free    free
lixiang@PC-YLX3VRF4:~/ztt/vbs-membuf7/dsf/build/tools$ ./vbs_cli membuf_cli bufNode --UUID=1879048195 --bufNodeIndex=1
bufnode1 :
RefState: 0       1       2       3       4       5       6       7       8       9       10      11
          using   free    free    free    free    free    free    free    free    free    free    free
lixiang@PC-YLX3VRF4:~/ztt/vbs-membuf7/dsf/build/tools$ ./vbs_cli membuf_cli bufNode --UUID=1879048195 --bufNodeIndex=2
bufnode2 :
RefState: 0       1       2       3       4       5       6       7       8       9       10      11
          using   free    free    free    free    free    free    free    free    free    free    free
lixiang@PC-YLX3VRF4:~/ztt/vbs-membuf7/dsf/build/tools$ ./vbs_cli membuf_cli bufNode --UUID=1879048195 --bufNodeIndex=3
bufnode3 :
RefState: 0       1       2       3       4       5       6       7       8       9       10      11
          free    free    free    free    free    free    free    free    free    free    free    free
```
###  使用实例:enableDfx
运行指令
```
./vbs_cli membuf_cli enableDfx --UUID=1879048221 --enable=1
Dfx enable:1
```

###  使用实例:get_all_pool_info.sh
```
./get_all_pool_info.sh
get pool id: 1879048221 info success
```
会在当前目录下生成结果文件PoolInfo.txt，示例内容如下：
```
name                                              size                ownerProcess        refcount            UUID                relatedProcess
membuf_monitor_test_002                           8192                4074050             2                   1879048221          -1


poolId         poolSize       bufCount       bufMaxSize     shareMemPtr         fd   refState  offset         used           free           usedCount      freeCount      maxAllocSize   minAllocSize
1879048221     4608           4              2048           0x7efed1c74000      4    0         512            0              4608           0              4              2048           10

```

###  使用实例:get_all_mq_info.sh
```
./get_all_mq_info.sh
get mq id: FUNCTEST_DSF_SHMMQ_MONITOR_001 info success
```
会在当前目录下生成结果文件MqInfo.txt，示例内容如下：
```
mqName                                                                          size                ownerProcess        refcount            relatedProcess
shmmq_FUNCTEST_DSF_SHMMQ_MONITOR_001                                            4096                931167              4                   -1
shmmq_DiscoveryServiceRecvQueue_DFX                                             163840              2820304             2                   -1
shmmq_DiscoveryServiceRecvQueue                                                 647168              2820304             2                   -1
MQ Shm Info:
name                                    mode      queue_size  empty_size  w_idx_m   r_idx_m   w_idx   r_idx   creation_time_s r_pids  w_pids      avg_latency_us max_num_in_queue    max_num_in_queue_ts
shmmq_FUNCTEST_DSF_SHMMQ_MONITOR_001    OVERWRITE 32          32          2         2         2       2       0               0       0           0              1                   0
```

### 参考文档
```
https://li.feishu.cn/docx/YgyMdWU6LoQcKOx1R9PcwzmqnWc?from=from_copylink
```