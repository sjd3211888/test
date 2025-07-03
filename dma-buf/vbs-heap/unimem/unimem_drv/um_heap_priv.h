// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#ifndef __UM_HEAP_PRIV_H__
#define __UM_HEAP_PRIV_H__

#include <linux/cdev.h>
#include "uapi/um-heap.h"
#include "um-heap.h"

struct unimem_heap_cma;

struct unimem_heap_mm_db_entry {
	struct rb_node entry;
	struct unimem_heap_buffer *buf;
	void *client;
	u32 peer_id;
	u32 uuid;
	u32 refcount;
};

struct unimem_heap_mm_db {
	struct rb_root root;
	struct mutex lock;
};

struct unimem_heap {
	const char *name;
	struct unimem_heap_cma *cma;
	dev_t devt;
	struct list_head list;
	struct device *dev;
	struct cdev cdev;

	enum heap_type type;
	int32_t idr_base;
	int32_t idr_current;
	int32_t idr_end;
	struct idr idr_alloc;
	struct mutex idr_lock;

	struct mutex buf_lock;
	struct list_head buf_list;

	struct unimem_heap_mm_db *export_db;

	struct mutex file_priv_lock;
	struct list_head file_priv_list;
};

struct unimem_heap_file_priv {
	struct unimem_heap *heap;
	pid_t pid;
	// event resource
	wait_queue_head_t event_wait;
	struct list_head event_list;
	spinlock_t event_lock;

	// query result
	struct mutex query_lock;
	enum buf_query_t query_type;
	union {
		char *name;
		__u32 uuid;
		pid_t pid;
	} query_val;
	struct list_head query_list;

	// list link for file priv
	struct list_head list;
};

struct um_heap_event_link {
	struct um_heap_event *event;
	struct list_head link;
};

struct um_heap_query_result_link {
	struct unimem_heap_buffer *buf;
	struct list_head link;
};

struct dma_buf *alloc_system_heap_buff(struct unimem_heap *dev_heap,
				       unsigned long len,
				       unsigned long fd_flags,
				       unsigned long buf_flags, char *name);
int unimem_heap_init_debugfs(void);
int unimem_heap_deinit_debugfs(void);
struct unimem_heap *find_heap_by_buffer_id(int id);
struct unimem_heap *find_heap_by_type(enum heap_type type);

#endif //__UM_HEAP_PRIV_H__
