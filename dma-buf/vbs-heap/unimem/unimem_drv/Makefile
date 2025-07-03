# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 Li Auto Inc. and its affiliates
ifneq ($(KERNELRELEASE),)

ifeq ($(CONFIG_DMABUF_VBS_HEAP), y)
obj-y += um_heap_buf.o um_heap_drv.o um_heap_system.o um_heap_debugfs.o
else
um_heap-y = um_heap_drv.o um_heap_buf.o um_heap_debugfs.o um_heap_system.o
obj-m = um_heap.o
endif

else

ifeq ($(CONFIG_GCOV_KERNEL), y)
GCOV_PROFILE := y
endif

KSRC := /lib/modules/$(shell uname -r)/build

all: clean modules

modules:
	$(MAKE) -C $(KSRC) M=$(shell pwd) modules

clean:
	$(MAKE) -C $(KSRC) M=$(shell pwd) clean

endif
