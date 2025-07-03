// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#ifndef __UM_HEAP_UAPI_H__
#define __UM_HEAP_UAPI_H__
#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * DOC: Unify Memory Userspace API
 */
/* Valid FD_FLAGS are O_CLOEXEC, O_RDONLY, O_WRONLY, O_RDWR */
#define UNIMEM_HEAP_VALID_FD_FLAGS (O_CLOEXEC | O_ACCMODE)

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
#define UNIMEM_EXPORT_AUTHENTICATION_FLAG (1 << 31)
#define UNIMEM_CPU_RDWR (UNIMEM_CPU_WRITE | UNIMEM_CPU_READ)
#define UNIMEM_CAMERA_RDWR (UNIMEM_CAMERA_WRITE | UNIMEM_CAMERA_READ)
#define UNIMEM_NPU_RDWR (UNIMEM_NPU_WRITE | UNIMEM_NPU_READ)
#define UNIMEM_VDEC_RDWR (UNIMEM_VDEC_WRITE | UNIMEM_VDEC_READ)
#define UNIMEM_VENC_RDWR (UNIMEM_VENC_WRITE | UNIMEM_VENC_READ)
#define UNIMEM_VALID_FLAGS                                        \
	(UNIMEM_CPU_RDWR | UNIMEM_CAMERA_RDWR | UNIMEM_NPU_RDWR | \
	 UNIMEM_VDEC_RDWR | UNIMEM_VENC_RDWR | UNIMEM_EXPORT_AUTHENTICATION_FLAG)

#define UNIMEM_HEAP_BUF_NAME_LEN 256

enum um_heap_event_type {
	UM_HEAP_EVENT_EXIT = 1,
	UM_HEAP_EVENT_MAX,
};
struct um_heap_event {
	__u32 type;
	__u32 length;
};

struct um_heap_event_exit {
	struct um_heap_event base;
	pid_t pid; //exit process
	int exit_code; //exit code
	__u32 num_uuid;
	__u32 uuid_list[0];
};

/**
 * struct unimem_heap_allocation_data - metadata passed from userspace for
 *                                      allocations
 * @len:		size of the allocation
 * @fd:			will be populated with a fd which provides the
 *			handle to the allocated dma-buf
 * @fd_flags:		file descriptor flags used when allocating
 * @buf_flags:	flags passed to buffer
 *
 * Provided by userspace as an argument to the ioctl
 */
struct unimem_heap_allocation_data {
	__u64 len;
	__s32 fd;
	__u32 fd_flags;
	__u64 buf_flags;
	__u64 phy_addr;
	char *name;
};

struct unimem_heap_buf_flink {
	/** Handle for the object being named */
	__s32 fd;

	/** Returned global uuid */
	__u32 uuid;
};

struct unimem_heap_buf_export {
	__s32 fd;

	__u32 uuid;
	__s32 peer_id;
};

struct unimem_heap_buf_import {
	/** global id of buffer*/
	__u32 uuid;
	/** global name of buffer*/
	char *name;

	/** return handle for the object */
	__s32 fd;

	__u32 fd_flags;
};

struct unimem_heap_get_event {
	char *buf;
	__u32 len;
};

enum buf_query_t {
	UM_QUERY_BY_NAME, //按名称查询
	UM_QUERY_BY_UUID, //按uuid查询
	UM_QUERY_BY_PID, //按进程号查询
	UM_QUERY_BY_HEAP, //查询设备所有
	UM_QUERY_TYPE_MAX
};

struct unimem_heap_buf_info {
	__u32 uuid;
	char name[UNIMEM_HEAP_BUF_NAME_LEN];
	size_t size;
	pid_t owner;
	__u32 refcount;
	__u32 user_cnt;
	pid_t user[0];
};

struct unimem_heap_buf_query {
	union {
		char *name;
		__u32 uuid;
		pid_t pid;
	};
	enum buf_query_t query_type;
	__u32 data_len;
	char *data;
};

struct unimem_heap_device_info {
	char name[64];
	__u32 type;
	__u32 page_num;
	__u32 page_used;
	__u32 pfn;
	__s32 idr_start;
	__s32 idr_end;
};

#define UNIMEM_HEAP_IOC_MAGIC 'H'

/**
 * DOC: UNIMEM_HEAP_IOCTL_ALLOC - allocate memory from pool
 *
 * Takes a unimem_heap_allocation_data struct and returns it with the fd field
 * populated with the dmabuf handle of the allocation.
 */
#define UNIMEM_HEAP_IOCTL_ALLOC \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x0, struct unimem_heap_allocation_data)
#define UNIMEM_HEAP_IOCTL_FLINK \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x1, struct unimem_heap_buf_flink)
#define UNIMEM_HEAP_IOCTL_IMPORT \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x2, struct unimem_heap_buf_import)
#define UNIMEM_HEAP_IOCTL_QUERY_INFO \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x3, struct unimem_heap_buf_query)
#define UNIMEM_HEAP_IOCTL_CLEAR_BUFINFO _IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x4, int)
#define UNIMEM_HEAP_IOCTL_GET_EVENT \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x5, struct unimem_heap_get_event)
#define UNIMEM_HEAP_IOCTL_DEVICE_INFO \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0x6, struct unimem_heap_device_info)
#define UNIMEM_HEAP_IOCTL_EXPORT \
	_IOWR(UNIMEM_HEAP_IOC_MAGIC, 0xA, struct unimem_heap_buf_export)
#endif //__UM_HEAP_UAPI_H__
