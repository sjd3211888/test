// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#ifndef __UM_HEAP_H__
#define __UM_HEAP_H__
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/dma-buf.h>

#define UNIMEM_CPU_WRITE (1 << 0)
#define UNIMEM_CPU_READ (1 << 1)
#define UNIMEM_CAMERA_WRITE (1 << 2)
#define UNIMEM_CAMERA_READ (1 << 3)
#define UNIMEM_NPU_WRITE (1 << 4)
#define UNIMEM_NPU_READ (1 << 5)
#define UNIMEM_VDEC_WRITE (1 << 6)
#define UNIMEM_VDEC_READ (1 << 7)
#define UNIMEM_VENC_WRITE (1 << 8)
#define UNIMEM_VENC_READ (1 << 9)
#define UNIMEM_HIGHMEM_USER (1 << 10)
#define UNIMEM_CPU_RDWR (UNIMEM_CPU_WRITE | UNIMEM_CPU_READ)
#define UNIMEM_CAMERA_RDWR (UNIMEM_CAMERA_WRITE | UNIMEM_CAMERA_READ)
#define UNIMEM_NPU_RDWR (UNIMEM_NPU_WRITE | UNIMEM_NPU_READ)
#define UNIMEM_VDEC_RDWR (UNIMEM_VDEC_WRITE | UNIMEM_VDEC_READ)
#define UNIMEM_VENC_RDWR (UNIMEM_VENC_WRITE | UNIMEM_VENC_READ)

#define UNIMEM_HEAP_BUF_NAME_LEN 256

enum heap_type {
	HEAP_PUB,
	HEAP_ISP,
	HEAP_VPU,
	HEAP_NPU,
	HEAP_X280_SCHED_SRAM,
	HEAP_NPU_RESERVED,
	HEAP_SYS,
	HEAP_MAX
};

struct dma_buf *device_heap_alloc_buffer(enum heap_type type, uint32_t flag,
					 size_t size);
struct dma_buf *system_heap_alloc_buffer(uint32_t flag, size_t size,
					 char *name);

#endif //__UM_HEAP_H__
