# SecureExample说明

## 描述

本示例演示了安全示例的配置和使用

## 编译

```Plain
sudo ./cmake-build.sh install
```

## 运行

需要启动中心化服务发现程序。

```Plain
sudo mkdir -p /dev/socket/vbs
sudo chmod 777 /dev/socket/vbs
cd <your_install_dir>/tool
./DServer
```

其次，分别启动pub端和sub端程序。

* pub端

```Plain
cd <your_install_dir>/examples
export LD_LIBRARY_PATH=<your_install_dir>/lib
 ./SecureExample -pub -xml secure_auth_client.xml //认证示例
```

* sub端

```Plain
cd <your_install_dir>/examples
export LD_LIBRARY_PATH=<your_install_dir>/lib
 ./SecureExample -sub -xml secure_auth_client.xml
```

成功运行的标志：打印出“helloworld”日志

## 终止

sub端按exit终止进程
