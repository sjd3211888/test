// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#include <linux/dma-buf.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/version.h>
#include "um_heap_buf.h"
#include "um-heap.h"

static vm_fault_t unimem_heap_buffer_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct unimem_heap_buffer *buffer = vma->vm_private_data;
	struct inode *inode = file_inode(vma->vm_file);

	if (vmf->pgoff > buffer->pagecount)
		return VM_FAULT_SIGBUS;

	vmf->page = buffer->pages[vmf->pgoff];
	/* fix pthread_cond_wait crash issue when pthread_cond_t variable stored in heap
	* only set mapping field for page allocated from system
	*/
	if (!buffer->cma_pages)
		vmf->page->mapping = inode ? inode->i_mapping : NULL;
	get_page(vmf->page);

	return 0;
}

static const struct vm_operations_struct unimem_heap_buffer_vm_ops = {
	.fault = unimem_heap_buffer_vm_fault,
};

static int unimem_heap_buffer_mmap(struct dma_buf *dmabuf,
				   struct vm_area_struct *vma)
{
	struct unimem_heap_buffer *buffer = dmabuf->priv;

	if ((vma->vm_flags & (VM_SHARED | VM_MAYSHARE)) == 0)
		return -EINVAL;

	if (0 == (buffer->flags & UNIMEM_CPU_RDWR)) // cpu can't access
		return -EACCES;
	else if ((buffer->flags & UNIMEM_CPU_READ) &&
		 !(buffer->flags & UNIMEM_CPU_WRITE)) { // only read
		if (vma->vm_flags & VM_WRITE)
			return -EACCES;
	} else if ((buffer->flags & UNIMEM_CPU_WRITE) &&
		   !(buffer->flags & UNIMEM_CPU_READ)) { //only write
		if (vma->vm_flags & VM_READ)
			return -EACCES;
	}

	vma->vm_ops = &unimem_heap_buffer_vm_ops;
	vma->vm_private_data = buffer;
#ifdef PLATFORM_ZEBU
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#endif

	return 0;
}

static void *unimem_heap_buffer_do_vmap(struct unimem_heap_buffer *buffer)
{
	void *vaddr;

#ifdef PLATFORM_ZEBU
	vaddr = vmap(buffer->pages, buffer->pagecount,
		VM_MAP, pgprot_writecombine(PAGE_KERNEL));
#else
	vaddr = vmap(buffer->pages, buffer->pagecount, VM_MAP, PAGE_KERNEL);
#endif
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
static void *unimem_heap_buffer_vmap(struct dma_buf *dmabuf)
#else
static int unimem_heap_buffer_vmap(struct dma_buf *dmabuf,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
				   struct dma_buf_map *map)
#else
				   struct iosys_map *map)
#endif
#endif
{
	struct unimem_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;

	if (!(buffer->flags & UNIMEM_CPU_RDWR) ||
	    buffer->flags & UNIMEM_HIGHMEM_USER)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
		return ERR_PTR(-EACCES);
#else
		return -EACCES;
#endif

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		// dma_buf_map_set_vaddr(map, buffer->vaddr);
		mutex_unlock(&buffer->lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
		return buffer->vaddr;
#else
		map->vaddr = buffer->vaddr;
		map->is_iomem = false;

		return 0;
#endif
	}

	vaddr = unimem_heap_buffer_do_vmap(buffer);
	if (IS_ERR(vaddr)) {
		mutex_unlock(&buffer->lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
		return vaddr;
#else
		return PTR_ERR(vaddr);
#endif
	}
	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;
	// dma_buf_map_set_vaddr(map, buffer->vaddr);
	mutex_unlock(&buffer->lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	return buffer->vaddr;
#else
	map->vaddr = buffer->vaddr;
	map->is_iomem = false;

	return 0;
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
static void unimem_heap_buffer_vunmap(struct dma_buf *dmabuf, void *vaddr)
#else
static void unimem_heap_buffer_vunmap(struct dma_buf *dmabuf,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
				      struct dma_buf_map *map)
#else
				      struct iosys_map *map)
#endif
#endif
{
	struct unimem_heap_buffer *buffer = dmabuf->priv;

	if (buffer->flags & UNIMEM_HIGHMEM_USER)
		return;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
		vunmap(buffer->vaddr);
#else
		if (!map->is_iomem)
			vunmap(map->vaddr);
#endif
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	// dma_buf_map_clear(map);
}

static void unimem_heap_buffer_release(struct dma_buf *dmabuf)
{
	struct unimem_heap_buffer *buffer = dmabuf->priv;

	// pr_err("release device buffer!\n");

	if (buffer->vmap_cnt > 0) {
		WARN(1, "%s: buffer still mapped in the kernel\n", __func__);
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}

	/* release memory */
	if (buffer->free)
		buffer->free(buffer);
	dmabuf->priv = NULL;
}

static struct sg_table *
unimem_heap_buffer_map_dumb(struct dma_buf_attachment *attachment,
			    enum dma_data_direction direction)
{
	pr_err("%s: Error, no device map ops!\n", __func__);
	return NULL;
}

static void unimem_heap_buffer_unmap_dumb(struct dma_buf_attachment *attachment,
					  struct sg_table *table,
					  enum dma_data_direction direction)
{
	pr_err("%s: Error, no device unmap ops!\n", __func__);
}

const struct dma_buf_ops system_heap_buf_ops = {
	.map_dma_buf = unimem_heap_buffer_map_dumb,
	.unmap_dma_buf = unimem_heap_buffer_unmap_dumb,
	.mmap = unimem_heap_buffer_mmap,
	.vmap = unimem_heap_buffer_vmap,
	.vunmap = unimem_heap_buffer_vunmap,
	.release = unimem_heap_buffer_release,
};

int unimem_heap_buffer_add_pid_info(struct unimem_heap_buffer *buf, pid_t pid)
{
	int index = 0, checked_num = 0;
	int insert_id = -1;

	mutex_lock(&buf->lock);
	if (buf->owner.pid == pid) {
		buf->owner.ref++;
		mutex_unlock(&buf->lock);
		return BUF_USER_NUM_MAX;
	}
	while (index < BUF_USER_NUM_MAX) {
		if (buf->user[index].pid > 0) {
			if (buf->user[index].pid == pid) {
				buf->user[index].ref++;
				mutex_unlock(&buf->lock);
				return index;
			}
			checked_num++;
		} else if (buf->user[index].pid == 0) {
			if (-1 == insert_id) //find first available index
				insert_id = index;
		} else
			pr_err("buf->user[%d] is %d", index,
			       buf->user[index].pid);

		if (checked_num == buf->user_num && -1 != insert_id)
			break;

		index++;
	}
	// need add pid
	if (-1 != insert_id) {
		buf->user[insert_id].pid = pid;
		buf->user[insert_id].ref = 1;
		buf->user_num++;
	}
	mutex_unlock(&buf->lock);

	if (-1 == insert_id)
		pr_err("No space to store importer pid:%d!\n", pid);

	return insert_id;
}

void unimem_heap_buffer_remove_pid_info(struct unimem_heap_buffer *buf,
					pid_t pid, bool ignore_ref)
{
	int index = 0;

	mutex_lock(&buf->lock);
	if (buf->owner.pid == pid) {
		if (ignore_ref)
			buf->owner.ref = 0;
		else
			buf->owner.ref--;
		if (buf->owner.ref == 0)
			buf->owner.pid = 0;
		mutex_unlock(&buf->lock);
		return;
	}
	while (index < BUF_USER_NUM_MAX) {
		if (pid == buf->user[index].pid) {
			if (ignore_ref)
				buf->user[index].ref = 0;
			else
				buf->user[index].ref--;
			if (buf->user[index].ref == 0) {
				buf->user[index].pid = 0;
				buf->user_num--;
			}
			break;
		}
		index++;
	}
	mutex_unlock(&buf->lock);
	if (index == BUF_USER_NUM_MAX)
		pr_warn("process(pid:%d) haven't use this buf!\n", pid);
}

bool unimem_heap_buffer_is_used_by_process(struct unimem_heap_buffer *buf,
					   pid_t pid)
{
	int index = 0;
	bool is_used = false;

	mutex_lock(&buf->lock);
	if (pid == buf->owner.pid)
		is_used = true;
	else if (buf->user_num > 0) {
		while (index < BUF_USER_NUM_MAX) {
			if (pid == buf->user[index].pid) {
				is_used = true;
				break;
			}
			index++;
		}
	}
	mutex_unlock(&buf->lock);
	return is_used;
}
