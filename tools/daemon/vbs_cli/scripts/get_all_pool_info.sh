#!/bin/bash

# 定义输出文件路径
output_file=$(date +%Y%m%d_%H%M%S)+"PoolInfo.txt"

# 清空output_file文件内容
> $output_file

# 输出所有队列信息
./vbs_cli membuf_cli list > $output_file

# 添加换行符
echo -e "\n" >> $output_file
# 使用awk获取UUID的一列，然后使用循环调用`./vbs_cli membuf_cli info --UUID="$uuid"`
./vbs_cli membuf_cli list | awk '{print $5}' | while read uuid; do
  if [ "$uuid" = "UUID" ]; then continue; fi  # 忽略标题行
  echo "get pool id: $uuid info success"
  ./vbs_cli membuf_cli info --uuid="$uuid" >> $output_file
  echo -e " " >> $output_file
done