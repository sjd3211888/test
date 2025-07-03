// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#ifndef __UM_HEAP_BUF_H__
#define __UM_HEAP_BUF_H__
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>

#define BUF_USER_NUM_MAX (128)

struct buff_user_info {
	pid_t pid;
	u32 ref;
};

struct unimem_heap_buffer {
	void *buf_priv;
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct page *cma_pages;
	struct page **pages;
	pgoff_t pagecount;
	int vmap_cnt;
	void *vaddr;

	char *name;
	uint32_t uuid;
	void (*free)(struct unimem_heap_buffer *buffer);
	uint32_t flags;
	struct dma_buf *dmabuf;
	struct list_head list;
	struct buff_user_info owner;
	uint32_t user_num;
	struct buff_user_info user[0];
};

struct unimem_heap_buffer_attachment {
	struct device *dev;
	struct sg_table table;
	struct list_head list;
	bool mapped;
};

extern const struct dma_buf_ops system_heap_buf_ops;
int unimem_heap_buffer_add_pid_info(struct unimem_heap_buffer *buf, pid_t pid);
void unimem_heap_buffer_remove_pid_info(struct unimem_heap_buffer *buf,
					pid_t pid, bool ignore_ref);
bool unimem_heap_buffer_is_used_by_process(struct unimem_heap_buffer *buf,
					   pid_t pid);
#endif //__UM_HEAP_BUF_H__
