### 编译 X86_64 版本驱动

使用以下命令:

```bash
cd <your_path>/unimem_drv
make
```
成功后，会生成 um_heap.ko

### 交叉编译 unimem_heap 驱动

需要先安装交叉编译工具 aarch64-linux-gnu-gcc。

交叉编译工具安装好后，使用以下命令:

```bash
cd <your_path>/devmem_driver
ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- make KSRC=<target kernel source>
```
成功后，会生成 arm64 的 um_heap.ko
```bash
file um_heap.ko
um_heap.ko: ELF 64-bit LSB relocatable, ARM aarch64, version 1 (SYSV), BuildID[sha1]=c49aa479017538f8d9283b6029cc9ccc67a4a7c8, not stripped
```

### 通过debugfs查询buffer信息
1. 查询heap上所有buffer信息，heap种类为[sys/pub/isp/vpu/npu]
例如：查询sys heap上的buffer信息
```bash
echo "heap sys" > /sys/kernel/debug/unimem_heap/bufinfo
cat /sys/kernel/debug/unimem_heap/bufinfo
```
2. 查询特定进程所使用的buffer信息
例如：查询进程12345所使用的buffer信息
```bash
echo "pid 12345" > /sys/kernel/debug/unimem_heap/bufinfo
cat /sys/kernel/debug/unimem_heap/bufinfo
```
3.  查询特定UUID的buffer信息
例如：查询UUID为23456的buffer信息
```bash
echo "uuid 23456" > /sys/kernel/debug/unimem_heap/bufinfo
cat /sys/kernel/debug/unimem_heap/bufinfo
```
