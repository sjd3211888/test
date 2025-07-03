// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/page_ref.h>
#include <linux/platform_device.h>
#include <linux/dma-buf.h>
#include <linux/cred.h>
#include <linux/poll.h>
#include <linux/version.h>
#include "um-heap.h"
#include "uapi/um-heap.h"
#include "um_heap_buf.h"
#include "um_heap_priv.h"

#define DRV_MODULE_VERSION "1.0"

#define UNIMEM_MEM_DEV_MAX (MINORMASK + 1)
#define BUF_INFO_RECORD_MAX (128)

#define HEAP_IDR_RESERVED_START (0x70000001)

static struct class unimem_mem_class;
static dev_t unimem_mem_major;
static DEFINE_XARRAY_ALLOC(unimem_mem_minors);

static uint32_t max_pending_exports = UINT_MAX;
module_param(max_pending_exports, uint, 0444);

static LIST_HEAD(unimem_heap_list);
static DEFINE_MUTEX(unimem_heap_list_lock);

int unimem_heap_ioctl_get_event(struct file *filp, void *data);
int unimem_heap_ioctl_buf_info(struct file *filp, void *data);


static int unimem_heap_mm_db_init(struct unimem_heap_mm_db **db)
{
	int ret = 0;
	struct unimem_heap_mm_db *db_ptr = NULL;

	db_ptr = kzalloc(sizeof(*db_ptr), GFP_KERNEL);
	if (db_ptr == NULL) {
		ret = -ENOMEM;
		goto err_exit;
	}
	db_ptr->root = RB_ROOT;
	mutex_init(&db_ptr->lock);
	*db = db_ptr;

err_exit:
	return ret;
}

static int unimem_heap_mm_db_deinit(struct unimem_heap_mm_db **db)
{
	int ret = 0;
	struct unimem_heap_mm_db *db_ptr = *db;
	struct unimem_heap_mm_db_entry *e;
	struct rb_node *n;

	mutex_lock(&db_ptr->lock);
	while ((n = rb_first(&db_ptr->root))) {
		e = rb_entry(n, struct unimem_heap_mm_db_entry, entry);
		rb_erase(&e->entry, &db_ptr->root);
		kfree(e);
	}
	mutex_unlock(&db_ptr->lock);
	kfree(db_ptr);
	*db = NULL;

	return ret;
}

static int unimem_heap_open(struct inode *inode, struct file *file)
{
	struct unimem_heap *heap;
	struct unimem_heap_file_priv *priv;

	heap = xa_load(&unimem_mem_minors, iminor(inode));
	if (!heap) {
		pr_err("dma_heap: minor %d unknown.\n", iminor(inode));
		return -ENODEV;
	}

	/* instance data as context */
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->heap = heap;
	priv->pid = current->tgid;
	/* init event resource */
	INIT_LIST_HEAD(&priv->event_list);
	init_waitqueue_head(&priv->event_wait);
	// coverity[side_effect_free:SUPPRESS]
	spin_lock_init(&priv->event_lock);

	mutex_init(&priv->query_lock);
	INIT_LIST_HEAD(&priv->query_list);

	mutex_lock(&heap->file_priv_lock);
	list_add(&priv->list, &heap->file_priv_list);
	mutex_unlock(&heap->file_priv_lock);

	file->private_data = priv;
	nonseekable_open(inode, file);

	return 0;
}

static long unimem_heap_ioctl_allocate(struct file *file, void *data)
{
	struct unimem_heap_allocation_data *heap_allocation = data;
	struct unimem_heap_file_priv *file_priv = file->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct unimem_heap_buffer *buffer;
	struct dma_buf *dmabuf;
	char *name = NULL;
	int ret = -ENOMEM;
	u32 valid_flag = 0;

	if (heap_allocation->fd >= 0)
		return -EINVAL;

	valid_flag = (heap->cma) ? UNIMEM_VALID_FLAGS : UNIMEM_CPU_RDWR |
				UNIMEM_HIGHMEM_USER | UNIMEM_EXPORT_AUTHENTICATION_FLAG;

	if (heap_allocation->fd_flags & ~UNIMEM_HEAP_VALID_FD_FLAGS)
		return -EINVAL;

	if (heap_allocation->buf_flags & ~valid_flag)
		return -EINVAL;

	if (heap_allocation->name) {
		name = strndup_user((char __user *)heap_allocation->name,
				    UNIMEM_HEAP_BUF_NAME_LEN);
		//check name not exist
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buffer, &heap->buf_list, list) {
			if (buffer->name && !strcmp(buffer->name, name)) {
				mutex_unlock(&heap->buf_lock);
				kfree(name);
				return -EEXIST;
			}
		}
		mutex_unlock(&heap->buf_lock);
	}

	if (heap->cma) {
		pr_err("err heap type!\n", heap->name);
		return -EINVAL;
	} else
		dmabuf = alloc_system_heap_buff(heap, heap_allocation->len,
						heap_allocation->fd_flags,
						heap_allocation->buf_flags,
						name);

	kfree(name);

	if (IS_ERR(dmabuf))
		return PTR_ERR(dmabuf);

	ret = dma_buf_fd(dmabuf, heap_allocation->fd_flags);
	if (ret < 0) {
		dma_buf_put(dmabuf);
		/* just return, as put will call release and that will free */
		return ret;
	}

	buffer = dmabuf->priv;
	mutex_lock(&heap->idr_lock);
	//heap belong to cma
	if (heap->cma)
		buffer->uuid = idr_alloc(&heap->idr_alloc, dmabuf, heap->idr_base,
					heap->idr_end, GFP_KERNEL);
	else {	// heap belong to system
		buffer->uuid = idr_alloc(&heap->idr_alloc, dmabuf, heap->idr_current,
					heap->idr_end, GFP_KERNEL);
			//if idr_current > idr_end,reset idr_current
			heap->idr_current =
				heap->idr_current+1 == heap->idr_end?heap->idr_base:heap->idr_current+1;
	}
	mutex_unlock(&heap->idr_lock);

	heap_allocation->fd = ret;
	if (heap->cma)
		heap_allocation->phy_addr = page_to_phys(buffer->cma_pages);

	return 0;
}

static long unimem_heap_ioctl_export(struct file *file, void *data)
{
	struct unimem_heap_buf_export *export_data = data;
	struct unimem_heap_file_priv *file_priv = file->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct unimem_heap_mm_db *export_db = heap->export_db;
	struct unimem_heap_mm_db_entry *entry = NULL;
	struct unimem_heap_mm_db_entry *temp = NULL;
	struct dma_buf *dmabuf = NULL;
	struct unimem_heap_buffer *buffer;
	struct rb_node *node = NULL;
	struct rb_node **link = NULL;
	struct rb_node *parent = NULL;
	uint32_t current_pending_exports = 0U;
	long ret = 0;

	if (export_data->fd < 0)
		return -EINVAL;

	dmabuf = dma_buf_get(export_data->fd);
	if (IS_ERR(dmabuf))
		return -EINVAL;

	buffer = (struct unimem_heap_buffer *)dmabuf->priv;
	if (buffer->owner.pid != current->tgid) {
		ret = -EPERM;
		goto err_exit;
	}

	if (!(buffer->flags & UNIMEM_EXPORT_AUTHENTICATION_FLAG)) {
		ret = -EINVAL;
		goto err_exit;
	}

	mutex_lock(&export_db->lock);

	for (node = rb_first(&export_db->root); node; node = rb_next(node)) {
		entry = rb_entry(node, struct unimem_heap_mm_db_entry, entry);
		if (file == entry->client)
			current_pending_exports += entry->refcount;
	}

	if (max_pending_exports <= current_pending_exports) {
		ret = -EINVAL;
		goto unlock;
	}

	for (node = rb_first(&export_db->root); node; node = rb_next(node)) {
		entry = rb_entry(node, struct unimem_heap_mm_db_entry, entry);
		if ((entry != NULL) &&
		    (file == entry->client) &&
		    (dmabuf->priv == entry->buf) &&
		    (export_data->peer_id == entry->peer_id)) {
			break;
		}
		entry = NULL;
	}

	if (entry) {
		entry->refcount++;
		export_data->uuid = entry->uuid;
		goto unlock;
	} else {
		entry = kzalloc(sizeof(*entry), GFP_KERNEL);
		if (entry == NULL) {
			ret = -ENOMEM;
			goto unlock;
		}

		entry->client = file;
		entry->buf = dmabuf->priv;
		entry->peer_id = export_data->peer_id;
		entry->uuid = entry->buf->uuid;
		entry->refcount = 1;

		link = &export_db->root.rb_node;

		while (*link) {
			parent = *link;
			temp = rb_entry(parent, struct unimem_heap_mm_db_entry, entry);
			link = (temp->uuid > entry->uuid) ?
					(&parent->rb_left) : (&parent->rb_right);
		}
		rb_link_node(&entry->entry, parent, link);
		rb_insert_color(&entry->entry, &export_db->root);
		export_data->uuid = entry->uuid;
	}

unlock:
	mutex_unlock(&export_db->lock);
err_exit:
	dma_buf_put(dmabuf);
	return ret;
}

static long unimem_heap_ioctl_flink(struct file *file, void *data)
{
	struct unimem_heap_buf_flink *flink_data = data;
	struct unimem_heap_file_priv *file_priv = file->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct dma_buf *dmabuf;
	struct unimem_heap_buffer *buffer;
	int ret = 0;

	if (flink_data->fd < 0)
		return -EINVAL;

	dmabuf = dma_buf_get(flink_data->fd);
	if (IS_ERR(dmabuf))
		return -EINVAL;

	buffer = dmabuf->priv;

	if (!buffer->uuid) {
		mutex_lock(&heap->idr_lock);
		//heap belong to cma
		if (heap->cma)
			ret = idr_alloc(&heap->idr_alloc, dmabuf, heap->idr_base,
					heap->idr_end, GFP_KERNEL);
		else {	// heap belong to system
			ret = idr_alloc(&heap->idr_alloc, dmabuf, heap->idr_current,
					heap->idr_end, GFP_KERNEL);
			//if idr_current > idr_end,reset idr_current
			heap->idr_current =
				heap->idr_current+1 == heap->idr_end?heap->idr_base:heap->idr_current+1;
		}
		mutex_unlock(&heap->idr_lock);
		if (ret >= 0)
			buffer->uuid = ret;
	}

	dma_buf_put(dmabuf);
	flink_data->uuid = buffer->uuid;

	return ret;
}

static long unimem_heap_ioctl_import(struct file *file, void *data)
{
	struct unimem_heap_buf_import *import_data = data;
	struct unimem_heap_file_priv *file_priv = file->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct unimem_heap_mm_db *export_db = heap->export_db;
	struct unimem_heap_mm_db_entry *entry = NULL;
	struct rb_node *node = NULL;
	struct unimem_heap_buffer *buf;
	struct dma_buf *dmabuf = NULL;
	char *name = NULL;
	int successfully_authentication = 1;

	if (!import_data->uuid && !import_data->name)
		return -EINVAL;

	if (import_data->uuid) {
		mutex_lock(&heap->idr_lock);
		dmabuf = idr_find(&heap->idr_alloc, import_data->uuid);
		mutex_unlock(&heap->idr_lock);
	} else if (import_data->name) {
		name = strndup_user((char __user *)import_data->name,
				    UNIMEM_HEAP_BUF_NAME_LEN);
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buf, &heap->buf_list, list) {
			if (buf->name && !strcmp(buf->name, name)) {
				dmabuf = buf->dmabuf;
				break;
			}
		}
		mutex_unlock(&heap->buf_lock);
	} else
		return -EINVAL;

	// check authentication
	if (IS_ERR_OR_NULL(dmabuf)) {
		kfree(name);
		return -EINVAL;
	}
	buf = dmabuf->priv;
	if (buf->flags & UNIMEM_EXPORT_AUTHENTICATION_FLAG) {
		mutex_lock(&export_db->lock);

		for (node = rb_first(&export_db->root); node; node = rb_next(node)) {
			entry = rb_entry(node, struct unimem_heap_mm_db_entry, entry);
			if ((entry != NULL) &&
				(entry->uuid == import_data->uuid ||
				(name && entry->buf->name &&
				!strcmp(entry->buf->name, name))) &&
				(entry->peer_id != -1 && current->tgid == entry->peer_id)) {
				break;
			}
			entry = NULL;
		}

		if (entry == NULL && current_uid().val != 0) {
			/* Authentication failed, but the process continues and finally returns with the flag - ESPIPE */
			successfully_authentication = 0;
		}
		if (entry != NULL) {
			entry->refcount--;
			if (entry->refcount == 0) {
				rb_erase(&entry->entry, &export_db->root);
				kfree(entry);
			}
		}
		mutex_unlock(&export_db->lock);
	}
	kfree(name);

	/* fd will be got in the next steps. But it exist an corner case:
	 * The dmabuf's refcount is 0 and clean work is queued in delayed work,
	 * but clean work isn't actually do. It need test refcount and increase to
	 * make sure refcount GT 0 before increase.
	 */
	if (!IS_ERR_OR_NULL(dmabuf)) {
		//add refcount to avoid release
		//redundant dmabuf check make coverity happy
		if (dmabuf && dmabuf->file &&
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
			likely(get_file_rcu(&dmabuf->file))) {
#else
			likely(get_file_rcu(dmabuf->file))) {
#endif
			import_data->fd =
				dma_buf_fd(dmabuf, import_data->fd_flags);
			if (import_data->fd < 0) {
				fput(dmabuf->file);
				return import_data->fd;
			}

			buf = dmabuf->priv;
			//add info
			unimem_heap_buffer_add_pid_info(buf, current->tgid);
		} else{
			return -ENOENT;
		}
	} else{
		return -EINVAL;
	}
	if (successfully_authentication)
		return 0;
	return ESPIPE;
}

int unimem_heap_ioctl_clear_bufinfo(struct file *filp, void *data)
{
	int fd = *(int *)data;
	struct dma_buf *dmabuf = dma_buf_get(fd);

	if (!IS_ERR(dmabuf)) {
		struct unimem_heap_buffer *buf = dmabuf->priv;

		unimem_heap_buffer_remove_pid_info(buf, current->tgid, false);
		dma_buf_put(dmabuf);
	} else
		return -EINVAL;
	return 0;
}

int unimem_heap_ioctl_device_info(struct file *file, void *data)
{
	struct unimem_heap_device_info *info = data;
	struct unimem_heap_file_priv *file_priv = file->private_data;
	struct unimem_heap *heap = file_priv->heap;

	if (heap->type >= HEAP_MAX)
		return -EINVAL;

	strncpy(info->name, heap->name, sizeof(info->name) - 1);
	info->type = heap->type;

	info->idr_start = heap->idr_base;
	info->idr_end = heap->idr_end;

	return 0;
}

static long unimem_heap_ioctl(struct file *file, unsigned int ucmd,
			      unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	unsigned int in_size, out_size;
	int ret = 0;

	out_size = _IOC_SIZE(ucmd);
	in_size = out_size;

	/* If necessary, allocate buffer for ioctl argument */
	if (out_size > sizeof(stack_kdata)) {
		kdata = kmalloc(out_size, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}

	if (copy_from_user(kdata, (void __user *)arg, in_size) != 0) {
		ret = -EFAULT;
		goto err;
	}

	switch (ucmd) {
	case UNIMEM_HEAP_IOCTL_ALLOC:
		ret = unimem_heap_ioctl_allocate(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_FLINK:
		ret = unimem_heap_ioctl_flink(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_IMPORT:
		ret = unimem_heap_ioctl_import(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_QUERY_INFO:
		ret = unimem_heap_ioctl_buf_info(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_CLEAR_BUFINFO:
		ret = unimem_heap_ioctl_clear_bufinfo(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_GET_EVENT:
		ret = unimem_heap_ioctl_get_event(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_DEVICE_INFO:
		ret = unimem_heap_ioctl_device_info(file, kdata);
		break;
	case UNIMEM_HEAP_IOCTL_EXPORT:
		ret = unimem_heap_ioctl_export(file, (void *)kdata);
		break;
	default:
		ret = -ENOTTY;
		goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;
err:
	if (kdata != stack_kdata)
		kfree(kdata);
	return ret;
}

__poll_t unimem_heap_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct unimem_heap_file_priv *file_priv = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &file_priv->event_wait, wait);

	if (!list_empty(&file_priv->event_list))
		mask |= EPOLLIN | EPOLLRDNORM;
	return mask;
}

struct _pid_list {
	int max;
	int num;
	pid_t pids[0];
};
struct _uuid_list {
	int max;
	int num;
	__u32 uuids[0];
};
static int __insert_uuid(struct _uuid_list *list, __u32 uuid)
{
	int idx = -1;

	if (list->num < list->max) {
		idx = 0;
		while (idx < list->num) {
			if (uuid == list->uuids[idx])
				break;
			idx++;
		}
		if (idx == list->num) {
			list->uuids[idx] = uuid;
			list->num++;
		}
	} else
		pr_err("UUID list is full!\n");
	return idx;
}
static int __insert_pid(struct _pid_list *list, pid_t pid)
{
	int idx = -1;

	if (list->num < list->max) {
		idx = 0;
		while (idx < list->num) {
			if (pid == list->pids[idx])
				break;
			idx++;
		}
		if (idx == list->num) {
			list->pids[idx] = pid;
			list->num++;
		}
	} else
		pr_err("PID list is full!\n");
	return idx;
}

int unimem_heap_release(struct inode *inode, struct file *filp)
{
	struct unimem_heap_file_priv *file_priv = filp->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct unimem_heap_mm_db *export_db = heap->export_db;
	struct unimem_heap_mm_db_entry *entry = NULL;
	struct rb_node *node = NULL;
	struct um_heap_event_link *e, *et;
	struct um_heap_query_result_link *query_result, *query_result_tmp;
	struct unimem_heap_file_priv *priv_entry;
	struct um_heap_event_exit *exit_event;
	uint32_t event_len = 0;
	struct unimem_heap_buffer *buf;
	int i = 0, idx = 0, user_num = 0;
	struct _pid_list *pids;
	struct _uuid_list *uuids;
	bool need_del = false;

	//detach from priv list of heap
	mutex_lock(&heap->file_priv_lock);
	list_del(&file_priv->list);
	mutex_unlock(&heap->file_priv_lock);

	/* remove query result */
	list_for_each_entry_safe(query_result, query_result_tmp,
				 &file_priv->query_list, link) {
		list_del(&query_result->link);
		kfree(query_result);
	}

	mutex_lock(&export_db->lock);
	do {
		for (node = rb_first(&export_db->root), need_del = false;
			node; node = rb_next(node)) {
			entry = rb_entry(node, struct unimem_heap_mm_db_entry, entry);
			if ((entry != NULL) && (entry->client == filp)) {
				need_del = true;
				break;
			}
		}
		if (need_del) {
			rb_erase(&entry->entry, &export_db->root);
			kfree(entry);
		}
	} while (need_del);
	mutex_unlock(&export_db->lock);

	/* Remove unconsumed events */
	spin_lock_irq(&file_priv->event_lock);
	list_for_each_entry_safe(e, et, &file_priv->event_list, link) {
		list_del(&e->link);
		kfree(e->event);
		kfree(e);
	}
	spin_unlock_irq(&file_priv->event_lock);

	kfree(file_priv);
	file_priv = NULL;

	//collect info of current process
	pids = kzalloc(sizeof(struct _pid_list) +
			       BUF_INFO_RECORD_MAX * sizeof(pid_t),
		       GFP_KERNEL);
	pids->max = BUF_INFO_RECORD_MAX;
	uuids = kzalloc(sizeof(struct _uuid_list) +
				BUF_INFO_RECORD_MAX * sizeof(int),
			GFP_KERNEL);
	uuids->max = BUF_INFO_RECORD_MAX;

	mutex_lock(&heap->buf_lock);
	list_for_each_entry(buf, &heap->buf_list, list) {
		if (unimem_heap_buffer_is_used_by_process(buf, current->tgid)) {
			//remove current process info
			unimem_heap_buffer_remove_pid_info(buf, current->tgid,
							   true);

			// record pid and uuid
			user_num = buf->user_num;
			idx = 0;
			__insert_uuid(uuids, buf->uuid);
			mutex_lock(&buf->lock);
			if (buf->owner.pid > 0)
				__insert_pid(pids, buf->owner.pid);
			while (user_num > 0 && idx < BUF_USER_NUM_MAX) {
				if (buf->user[idx].pid > 0) {
					__insert_pid(pids, buf->user[idx].pid);
					user_num--;
				}
				idx++;
			}
			mutex_unlock(&buf->lock);
		}
	}
	mutex_unlock(&heap->buf_lock);

	//create event
	event_len = sizeof(*exit_event) + uuids->num * sizeof(uint32_t);
	exit_event = kzalloc(event_len, GFP_KERNEL);
	exit_event->base.type = UM_HEAP_EVENT_EXIT;
	exit_event->base.length = event_len;
	exit_event->pid = current->tgid;
	exit_event->exit_code = current->exit_code;
	exit_event->num_uuid = uuids->num;
	memcpy(exit_event->uuid_list, uuids->uuids,
	       uuids->num * sizeof(uint32_t));

	kfree(uuids);

	/* notify each process open the device*/
	mutex_lock(&heap->file_priv_lock);
	list_for_each_entry(priv_entry, &heap->file_priv_list, list) {
		bool notify = false;

		for (i = 0; i < pids->num; i++) {
			if (pids->pids[i] == priv_entry->pid) {
				notify = true;
				break;
			}
		}

		if (!notify)
			continue;

		e = kzalloc(sizeof(*e), GFP_KERNEL);
		//init event
		e->event = kzalloc(exit_event->base.length, GFP_KERNEL);
		memcpy(e->event, exit_event, exit_event->base.length);

		spin_lock_irq(&priv_entry->event_lock);
		list_add_tail(&e->link, &priv_entry->event_list);
		spin_unlock_irq(&priv_entry->event_lock);
		wake_up_interruptible_poll(&priv_entry->event_wait,
					   EPOLLIN | EPOLLRDNORM);
	}
	mutex_unlock(&heap->file_priv_lock);

	kfree(pids);
	kfree(exit_event);

	return 0;
}

int unimem_heap_ioctl_get_event(struct file *filp, void *data)
{
	struct unimem_heap_get_event *event_data = data;
	struct unimem_heap_file_priv *file_priv = filp->private_data;
	char __user *buffer = event_data->buf;
	size_t count = event_data->len;
	int ret = 0;

	for (;;) {
		struct um_heap_event_link *e = NULL;

		spin_lock_irq(&file_priv->event_lock);
		if (!list_empty(&file_priv->event_list)) {
			e = list_first_entry(&file_priv->event_list,
					     struct um_heap_event_link, link);
			// file_priv->event_space += e->event->length;
			list_del(&e->link);
		}
		spin_unlock_irq(&file_priv->event_lock);

		if (e == NULL) {
			if (ret)
				break;

			if (filp->f_flags & O_NONBLOCK) {
				ret = -EAGAIN;
				break;
			}

			// mutex_unlock(&file_priv->event_read_lock);
			ret = wait_event_interruptible(
				file_priv->event_wait,
				!list_empty(&file_priv->event_list));
			// if (ret >= 0)
			// 	ret = mutex_lock_interruptible(&file_priv->event_read_lock);
			// if (ret)
			// 	return ret;
		} else {
			unsigned int length = e->event->length;

			if (length > count - ret) {
put_back_event:
				spin_lock_irq(&file_priv->event_lock);
				// file_priv->event_space -= length;
				list_add(&e->link, &file_priv->event_list);
				spin_unlock_irq(&file_priv->event_lock);
				wake_up_interruptible_poll(
					&file_priv->event_wait,
					EPOLLIN | EPOLLRDNORM);
				if (ret == 0)
					ret = -ENOMEM;
				break;
			}

			if (copy_to_user(buffer + ret, e->event, length)) {
				if (ret == 0)
					ret = -EFAULT;
				goto put_back_event;
			}

			ret += length;
			kfree(e->event);
			kfree(e);
		}
	}
	return ret;
}

static void um_heap_buf_info_fill(struct unimem_heap_buffer *buf,
				  struct unimem_heap_buf_info *info)
{
	int num = 0;

	mutex_lock(&buf->lock);
	info->uuid = buf->uuid;
	if (buf->name)
		snprintf(info->name, sizeof(info->name), "%s", buf->name);
	info->size = buf->len;
	info->refcount = file_count(buf->dmabuf->file);
	info->owner = buf->owner.pid;
	info->user_cnt = 0;

	while (info->user_cnt < buf->user_num && num < BUF_USER_NUM_MAX) {
		if (buf->user[num].pid > 0)
			info->user[info->user_cnt++] = buf->user[num].pid;
		num++;
	}
	mutex_unlock(&buf->lock);
}

int unimem_heap_ioctl_buf_info(struct file *filp, void *data)
{
	struct unimem_heap_file_priv *file_priv = filp->private_data;
	struct unimem_heap *heap = file_priv->heap;
	struct unimem_heap_buf_query *query_data = data;
	struct unimem_heap_buffer *buf;
	struct unimem_heap_buf_info *info;
	struct unimem_heap_file_priv *priv_entry;
	struct um_heap_query_result_link *query_result, *query_result_tmp;
	char *name = NULL;
	int32_t len = 0;
	char __user *dst = query_data->data;
	int ret = 0;

	info = kzalloc(sizeof(*info) + BUF_USER_NUM_MAX * sizeof(pid_t),
		       GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	mutex_lock(&file_priv->query_lock);
	if (!list_empty(&file_priv->query_list)) { // remain info of last query
		//check need clean or not
		bool need_clean = false;
		if (file_priv->query_type != query_data->query_type) {
			need_clean = true;
		} else {
			switch (query_data->query_type) {
			case UM_QUERY_BY_PID:
				need_clean = (file_priv->query_val.pid !=
					      query_data->pid);
				break;
			case UM_QUERY_BY_UUID:
				need_clean = (file_priv->query_val.uuid !=
					      query_data->uuid);
				break;
			case UM_QUERY_BY_NAME:
				name = strndup_user(
					(char __user *)query_data->name,
					UNIMEM_HEAP_BUF_NAME_LEN);
				need_clean = !!strcmp(file_priv->query_val.name,
						      name);
				kfree(name);
				break;
			case UM_QUERY_BY_HEAP:
			default:
				break;
			}
		}
		if (need_clean) {
			list_for_each_entry_safe(query_result, query_result_tmp,
						 &file_priv->query_list, link) {
				list_del(&query_result->link);
				kfree(query_result);
			}
			if (file_priv->query_type == UM_QUERY_BY_NAME) {
				kfree(file_priv->query_val.name);
				file_priv->query_val.name = NULL;
			}
		} else {
			mutex_unlock(&file_priv->query_lock);
			goto read_info;
		}
	}
	mutex_unlock(&file_priv->query_lock);

	switch (query_data->query_type) {
	case UM_QUERY_BY_NAME:
		file_priv->query_val.name =
			strndup_user((char __user *)query_data->name,
				     UNIMEM_HEAP_BUF_NAME_LEN);
		file_priv->query_type = UM_QUERY_BY_NAME;
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buf, &heap->buf_list, list) {
			if (buf->name &&
			    !strcmp(buf->name, file_priv->query_val.name)) {
				query_result = kzalloc(sizeof(*query_result),
						       GFP_KERNEL);
				if (query_result) {
					query_result->buf = buf;
					mutex_lock(&file_priv->query_lock);
					list_add_tail(&query_result->link,
						      &file_priv->query_list);
					mutex_unlock(&file_priv->query_lock);
				} else
					ret = -ENOMEM;
				break;
			}
		}
		mutex_unlock(&heap->buf_lock);
		// kfree(name);
		break;
	case UM_QUERY_BY_UUID:
		file_priv->query_type = UM_QUERY_BY_UUID;
		file_priv->query_val.uuid = query_data->uuid;
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buf, &heap->buf_list, list) {
			if (buf->uuid == query_data->uuid) {
				query_result = kzalloc(sizeof(*query_result),
						       GFP_KERNEL);
				if (query_result) {
					query_result->buf = buf;
					mutex_lock(&file_priv->query_lock);
					list_add_tail(&query_result->link,
						      &file_priv->query_list);
					mutex_unlock(&file_priv->query_lock);
				} else
					ret = -ENOMEM;
				break;
			}
		}
		mutex_unlock(&heap->buf_lock);
		break;
	case UM_QUERY_BY_PID:
		mutex_lock(&heap->file_priv_lock);
		list_for_each_entry(priv_entry, &heap->file_priv_list, list) {
			if (priv_entry->pid == query_data->pid)
				break;
		}
		mutex_unlock(&heap->file_priv_lock);
		if (list_entry_is_head(priv_entry, &heap->file_priv_list,
				       list)) {
			//can't find pid
			ret = -EINVAL;
			break;
		}

		file_priv->query_type = UM_QUERY_BY_PID;
		file_priv->query_val.pid = query_data->pid;
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buf, &heap->buf_list, list) {
			if (unimem_heap_buffer_is_used_by_process(
				    buf, query_data->pid)) {
				query_result = kzalloc(sizeof(*query_result),
						       GFP_KERNEL);
				if (query_result) {
					query_result->buf = buf;
					mutex_lock(&file_priv->query_lock);
					list_add_tail(&query_result->link,
						      &file_priv->query_list);
					mutex_unlock(&file_priv->query_lock);
				} else {
					ret = -ENOMEM;
					break;
				}
			}
		}
		mutex_unlock(&heap->buf_lock);
		break;
	case UM_QUERY_BY_HEAP:
		file_priv->query_type = UM_QUERY_BY_HEAP;
		mutex_lock(&heap->buf_lock);
		list_for_each_entry(buf, &heap->buf_list, list) {
			query_result =
				kzalloc(sizeof(*query_result), GFP_KERNEL);
			if (query_result) {
				query_result->buf = buf;
				mutex_lock(&file_priv->query_lock);
				list_add_tail(&query_result->link,
					      &file_priv->query_list);
				mutex_unlock(&file_priv->query_lock);
			} else {
				ret = -ENOMEM;
				break;
			}
		}
		mutex_unlock(&heap->buf_lock);
		break;
	default:
		ret = -EINVAL;
		goto exit;
		break;
	}

read_info:
	mutex_lock(&file_priv->query_lock);
	if (ret < 0) { // error
		list_for_each_entry_safe(query_result, query_result_tmp,
					 &file_priv->query_list, link) {
			list_del(&query_result->link);
			kfree(query_result);
		}
		if (file_priv->query_type == UM_QUERY_BY_NAME)
			kfree(file_priv->query_val.name);

	} else if (ret == 0) { //fill info
		if (list_empty(&file_priv->query_list))
			ret = -EINVAL;
		else {
			list_for_each_entry_safe(query_result, query_result_tmp,
						 &file_priv->query_list, link) {
				if (IS_ERR_OR_NULL(query_result->buf->dmabuf)) {
					list_del(&query_result->link);
					continue;
				}
				um_heap_buf_info_fill(query_result->buf, info);
				len = sizeof(*info) +
				      info->user_cnt * sizeof(pid_t);
				if (query_data->data_len - ret >= len &&
				    !copy_to_user(dst + ret, info, len)) {
					ret += len;
					list_del(&query_result->link);
					kfree(query_result);
				} else
					break;
			}
			if (ret == 0)
				ret = -ENOMEM;
			else {
				query_data->data_len = ret;
				ret = list_empty(&file_priv->query_list) ?
					      0 :
					      -EAGAIN;
				if (ret == 0) { // read done
					if (query_data->query_type ==
					    UM_QUERY_BY_NAME)
						kfree(file_priv->query_val.name);
				}
			}
		}
	}
	mutex_unlock(&file_priv->query_lock);

exit:
	kfree(info);
	return ret;
}

static const struct file_operations unimem_heap_fops = {
	.owner = THIS_MODULE,
	.open = unimem_heap_open,
	.unlocked_ioctl = unimem_heap_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = unimem_heap_ioctl,
#endif
	.poll = unimem_heap_poll,
	.release = unimem_heap_release,
};

static struct unimem_heap *unimem_heap_add(char *name, struct device *dev)
{
	struct unimem_heap *heap, *h;
	struct device *dev_ret;
	unsigned int minor;
	int ret;

	/* add device*/
	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);

	heap->name = name;
	/* check the name is unique */
	mutex_lock(&unimem_heap_list_lock);
	list_for_each_entry(h, &unimem_heap_list, list) {
		if (!strcmp(h->name, heap->name)) {
			mutex_unlock(&unimem_heap_list_lock);
			pr_err("dma_heap: Already registered heap named %s\n",
			       heap->name);
			goto err0;
		}
	}
	mutex_unlock(&unimem_heap_list_lock);

	/* Find unused minor number */
	ret = xa_alloc(&unimem_mem_minors, &minor, heap,
		       XA_LIMIT(0, UNIMEM_MEM_DEV_MAX - 1), GFP_KERNEL);
	if (ret < 0) {
		pr_err("dma_heap: Unable to get minor number for heap\n");
		goto err0;
	}

	/* Create device */
	heap->devt = MKDEV(MAJOR(unimem_mem_major), minor);

	cdev_init(&heap->cdev, &unimem_heap_fops);
	ret = cdev_add(&heap->cdev, heap->devt, 1);
	if (ret < 0) {
		pr_err("dma_heap: Unable to add char device\n");
		goto err1;
	}

	dev_ret = device_create(&unimem_mem_class, dev, heap->devt, NULL,
				heap->name);
	if (IS_ERR(dev_ret)) {
		pr_err("dma_heap: Unable to create device\n");
		goto err2;
	}
	heap->dev = dev_ret;
	mutex_init(&heap->buf_lock);
	INIT_LIST_HEAD(&heap->buf_list);
	mutex_init(&heap->file_priv_lock);
	INIT_LIST_HEAD(&heap->file_priv_list);

	/* Add heap to the list */
	mutex_lock(&unimem_heap_list_lock);
	list_add(&heap->list, &unimem_heap_list);
	mutex_unlock(&unimem_heap_list_lock);

	if (unimem_heap_mm_db_init(&heap->export_db) != 0)
		goto err2;

	return heap;

err2:
	cdev_del(&heap->cdev);
err1:
	xa_erase(&unimem_mem_minors, minor);
err0:
	kfree(heap);
	return NULL;
}

static int unimem_heap_remove(struct unimem_heap *heap)
{
	if (list_empty(&heap->buf_list)) {
		mutex_lock(&unimem_heap_list_lock);
		list_del(&heap->list);
		mutex_unlock(&unimem_heap_list_lock);
		unimem_heap_mm_db_deinit(&heap->export_db);
		device_destroy(&unimem_mem_class, heap->devt);
		cdev_del(&heap->cdev);
		xa_erase(&unimem_mem_minors, MINOR(heap->devt));
		kfree(heap);
	} else {
		pr_err("Buffer list of heap %s isn't empty!\n", heap->name);
	}
	return 0;
}

static struct device *system_dev;
static struct device *unimem_heap_sys_create(void)
{
	struct unimem_heap *heap;

	heap = unimem_heap_add("system", NULL);

	heap->cma = NULL;
	//initialize idr&type
	heap->idr_base = HEAP_IDR_RESERVED_START;
	heap->idr_current = heap->idr_base;
	heap->idr_end = __INT32_MAX__;
	heap->type = HEAP_SYS;
	mutex_init(&heap->idr_lock);
	idr_init_base(&heap->idr_alloc, heap->idr_base);

	dev_set_drvdata(heap->dev, heap);

	return heap->dev;
}

static int unimem_heap_sys_remove(struct device *dev)
{
	struct unimem_heap *heap = dev_get_drvdata(dev);

	return unimem_heap_remove(heap);
}

static char *unimem_heap_devnode(
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
		struct device *dev,
#else
		const struct device *dev,
#endif
		umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return kasprintf(GFP_KERNEL, "unimem_heap/%s", dev_name(dev));
}

static struct class unimem_mem_class = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	.owner = THIS_MODULE,
#endif
	.name = "unify memory",
	.devnode = unimem_heap_devnode,
};

static const struct of_device_id unimem_mem_of_match[] = {
	{ .compatible = "li,device-cma" },
	{}
};
MODULE_DEVICE_TABLE(of, unimem_mem_of_match);

static struct platform_driver unimem_mem_driver = {
	.probe = NULL,
	.remove = NULL,
	.driver  = {
		.name  = "schumacher_unify_mem",
		.of_match_table = unimem_mem_of_match,
	},
};

static int __init unimem_mem_init(void)
{
	int ret;

	ret = class_register(&unimem_mem_class);
	if (ret)
		return ret;

	ret = alloc_chrdev_region(&unimem_mem_major, 0, UNIMEM_MEM_DEV_MAX,
				  "schumacher_unify_mem");
	if (ret < 0) {
		pr_err("%s: failed to allocate char dev region\n", __func__);
		goto unregister_class;
	}

	ret = platform_driver_register(&unimem_mem_driver);
	if (ret < 0) {
		pr_err("%s: failed to register unify mem driver\n", __func__);
		goto unregister_chrdev;
	}

	unimem_heap_init_debugfs();

	//create system heap
	system_dev = unimem_heap_sys_create();

	return 0;

unregister_chrdev:
	unregister_chrdev_region(unimem_mem_major, UNIMEM_MEM_DEV_MAX);
unregister_class:
	class_unregister(&unimem_mem_class);
	return ret;
}

static void __exit unimem_mem_exit(void)
{
	unimem_heap_sys_remove(system_dev);
	unimem_heap_deinit_debugfs();

	platform_driver_unregister(&unimem_mem_driver);
	unregister_chrdev_region(unimem_mem_major, UNIMEM_MEM_DEV_MAX);
	class_unregister(&unimem_mem_class);
}

struct unimem_heap *find_heap_by_type(enum heap_type type)
{
	struct unimem_heap *heap = NULL, *h;

	mutex_lock(&unimem_heap_list_lock);
	list_for_each_entry(h, &unimem_heap_list, list) {
		if (h->type == type) {
			heap = h;
			break;
		}
	}
	mutex_unlock(&unimem_heap_list_lock);

	return heap;
}

struct unimem_heap *find_heap_by_buffer_id(int id)
{
	struct unimem_heap *heap = NULL, *h;

	mutex_lock(&unimem_heap_list_lock);
	list_for_each_entry(h, &unimem_heap_list, list) {
		if (id > h->idr_base && id < h->idr_end) {
			heap = h;
			break;
		}
	}
	mutex_unlock(&unimem_heap_list_lock);

	return heap;
}

struct dma_buf *system_heap_alloc_buffer(uint32_t flag, size_t size, char *name)
{
	struct unimem_heap *heap = dev_get_drvdata(system_dev);

	//system heap isn't contiguous, device can't access without smmu
	if (flag & ~UNIMEM_CPU_RDWR)
		return ERR_PTR(-EINVAL);

	return alloc_system_heap_buff(heap, size, O_RDWR | O_CLOEXEC, flag,
				      name);
}
EXPORT_SYMBOL_GPL(system_heap_alloc_buffer);

module_init(unimem_mem_init);
module_exit(unimem_mem_exit);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif

MODULE_AUTHOR("Yu Jin <yujin1@lixiang.com>");
MODULE_DESCRIPTION("Schumacher Platform unify memory driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRV_MODULE_VERSION);
