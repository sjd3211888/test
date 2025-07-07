#!/bin/bash

# 定义输出文件路径
output_file=$(date +%Y%m%d_%H%M%S)+"MqInfo.txt"

# 清空output_file文件内容
> $output_file

# 添加换行符
echo -e "\n" >> $output_file

# 输出所有pool信息 方便查询name和UUID的映射关系
./vbs_cli shmmq_cli list > $output_file
# 添加空行间隔开
echo -e " " >> $output_file
# 使用awk获取name一列，然后使用循环调用`./vbs_cli shmmq_cli info --name="$name"`
./vbs_cli shmmq_cli list | awk '{print $1}' | while read name; do
    # 忽略标题行 和 DS队列
  if [[ "$name" == "QueueName" || "$name" == *"DiscoveryService"* ]]; then
    continue
  fi
  name=${name#shmmq_}
  echo "get mq name: $name info success"
  ./vbs_cli shmmq_cli info --name="$name" >> $output_file
  # 添加空行间隔开
  echo -e " " >> $output_file
done