// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#include <linux/sched/signal.h>
#include "um_heap_buf.h"
#include "um_heap_priv.h"

void free_system_heap_buff(struct unimem_heap_buffer *buffer)
{
	pgoff_t pg;
	struct unimem_heap *heap = (struct unimem_heap *)buffer->buf_priv;

	mutex_lock(&heap->buf_lock);
	list_del(&buffer->list);
	mutex_unlock(&heap->buf_lock);

	if (buffer->uuid > 0) {
		mutex_lock(&heap->idr_lock);
		idr_remove(&heap->idr_alloc, buffer->uuid);
		mutex_unlock(&heap->idr_lock);
		buffer->uuid = 0;
	}

	kfree(buffer->name);

	for (pg = 0; pg < buffer->pagecount; pg++) {
		buffer->pages[pg]->mapping = NULL;
		__free_page(buffer->pages[pg]);
	}

	kvfree(buffer->pages);
	kfree(buffer);
}

struct dma_buf *alloc_system_heap_buff(struct unimem_heap *heap,
				       unsigned long len,
				       unsigned long fd_flags,
				       unsigned long buf_flags, char *name)
{
	struct unimem_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	size_t size = PAGE_ALIGN(len);
	pgoff_t pagecount = size >> PAGE_SHIFT;
	struct dma_buf *dmabuf = NULL;
	int ret = -ENOMEM;
	pgoff_t pg;
	u32 gfp_flag = 0;

	buffer =
		kzalloc(sizeof(*buffer) + BUF_USER_NUM_MAX *
						  sizeof(struct buff_user_info),
			GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->len = size;

	buffer->pages = kvmalloc_array(pagecount, sizeof(*buffer->pages),
				       GFP_KERNEL | __GFP_ZERO);
	if (!buffer->pages) {
		ret = -ENOMEM;
		goto err0;
	}

	gfp_flag = (buf_flags & UNIMEM_HIGHMEM_USER) ? GFP_HIGHUSER :
						       GFP_KERNEL;

	for (pg = 0; pg < pagecount; pg++) {
		/*
		 * Avoid trying to allocate memory if the process
		 * has been killed by SIGKILL
		 */
		if (fatal_signal_pending(current))
			goto err1;

		buffer->pages[pg] = alloc_page(gfp_flag | __GFP_ZERO);
		if (!buffer->pages[pg])
			goto err1;
	}

	buffer->buf_priv = heap;
	buffer->pagecount = pagecount;
	buffer->flags = buf_flags;
	buffer->uuid = 0;
	buffer->owner.pid = current->tgid;
	buffer->owner.ref = 1;
	buffer->free = free_system_heap_buff;
	buffer->name =
		(name) ? kstrndup(name, UNIMEM_HEAP_BUF_NAME_LEN, GFP_KERNEL) :
			 NULL;
	mutex_lock(&heap->buf_lock);
	list_add(&buffer->list, &heap->buf_list);
	mutex_unlock(&heap->buf_lock);

	/* create the dmabuf */
	if (buffer->name)
		exp_info.exp_name = buffer->name;
	exp_info.ops = &system_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto err1;
	}

	buffer->dmabuf = dmabuf;

	return dmabuf;

err1:
	while (pg > 0)
		__free_page(buffer->pages[--pg]);
	kvfree(buffer->pages);
err0:
	kfree(buffer);

	return ERR_PTR(ret);
}
