// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include "uapi/um-heap.h"
#include "um_heap_priv.h"
#include "um_heap_buf.h"

static uint32_t debugfs_query_type = UM_QUERY_TYPE_MAX;
static uint32_t debugfs_query_param;
static struct list_head query_result_list;

// struct unimem_heap_buffer *_find_buffer_by_id(uint32_t id)
// {
// 	struct unimem_heap *heap;
// 	struct unimem_heap_buffer *buffer;

// 	heap = find_heap_by_buffer_id(id);
// 	if (heap == NULL)
// 		return NULL;

// 	list_for_each_entry(buffer, &heap->buf_list, list) {
// 		if (buffer && buffer->uuid == id)
// 			return buffer;
// 	}
// 	return NULL;
// }

int unimem_heap_dump_buffer(struct unimem_heap_buffer *buf, char *dump_buf, size_t size)
{
	int write = 0;
	int attach_count = 0, idx = 0, buf_num = 0;
	struct unimem_heap_buffer_attachment *attach;

	mutex_lock(&buf->lock);
	/*
	 * snprintf return value is the number of characters which would be
	 * generated for the given input, excluding the trailing null. If the
	 * return is greater than or equal to size, the resulting string is
	 * truncated.
	 */
	write = snprintf(dump_buf, size, "%s\t0x%x\t0x%lx\t0x%08lx\t%lu\t%d\t",
					buf->name?buf->name:"N/A", buf->uuid, buf->len,
					page_to_pfn(buf->pages[0]), buf->pagecount, buf->owner.pid);
	if (write >= size) //string is truncated, no space to write
		goto exit;

	for (idx = 0; idx < BUF_USER_NUM_MAX && buf_num < buf->user_num; idx++) {
		if (buf->user[idx].pid > 0) {
			write += snprintf(dump_buf + write, size - write, "%d,", buf->user[idx].pid);
			buf_num++;
			if (write >= size)
				goto exit;
		}
	}
	write += snprintf(dump_buf + write, size - write, "\n");
	if (write >= size)
		goto exit;

	if (!list_empty(&buf->attachments)) {
		write += snprintf(dump_buf + write, size - write, "Attached Devices:\n");
		if (write >= size)
			goto exit;

		list_for_each_entry(attach, &buf->attachments, list) {
			write += snprintf(dump_buf + write, size - write, "%s\t", dev_name(attach->dev));
			if (write >= size)
				goto exit;
		}

		write += snprintf(dump_buf + write, size - write, "\n");
		attach_count++;
	}
exit:
	mutex_unlock(&buf->lock);
	return write;
}

static ssize_t unimem_buf_info_read(struct file *fp, char __user *user_buf,
								size_t count, loff_t *ppos)
{
	int ret, written = 0;
	struct unimem_heap *heap;
	struct unimem_heap_buffer *buf;
	struct um_heap_query_result_link *query_result, *tmp;
	int heap_idx = 0;
	char *tmp_buf;

	if (debugfs_query_type == UM_QUERY_TYPE_MAX)
		return 0;

	tmp_buf = (char *)kzalloc(sizeof(char)*count, GFP_KERNEL);

	if (list_empty(&query_result_list) && debugfs_query_type != UM_QUERY_TYPE_MAX) {
		size_t total_buf_size = 0;
		uint32_t total_buf_num = 0;
		switch (debugfs_query_type) {
		case UM_QUERY_BY_HEAP:
			heap = find_heap_by_type(debugfs_query_param);
			if (!heap)
				break;
			mutex_lock(&heap->buf_lock);
			list_for_each_entry(buf, &heap->buf_list, list) {
				query_result = kzalloc(sizeof(*query_result), GFP_KERNEL);
				if (query_result) {
					query_result->buf = buf;
					list_add_tail(&query_result->link, &query_result_list);
					total_buf_size += buf->len;
					total_buf_num++;
				} else {
					pr_err("kzalloc failed!\n");
					break;
				}
			}
			mutex_unlock(&heap->buf_lock);
			break;
		case UM_QUERY_BY_PID:
			for (heap_idx = HEAP_PUB; heap_idx < HEAP_MAX; heap_idx++) {
				heap = find_heap_by_type(heap_idx);
				if (!heap)
					continue;
				mutex_lock(&heap->buf_lock);
				list_for_each_entry(buf, &heap->buf_list, list) {
					if (buf && unimem_heap_buffer_is_used_by_process(buf, debugfs_query_param)) {
						query_result = kzalloc(sizeof(*query_result), GFP_KERNEL);
						if (query_result) {
							query_result->buf = buf;
							list_add_tail(&query_result->link, &query_result_list);
							total_buf_size += buf->len;
							total_buf_num++;
						} else {
							pr_err("kzalloc failed!\n");
							break;
						}
					}
				}
				mutex_unlock(&heap->buf_lock);
			}
			break;
		case UM_QUERY_BY_UUID:
			heap = find_heap_by_buffer_id(debugfs_query_param);
			if (!heap)
				break;
			mutex_lock(&heap->buf_lock);
			list_for_each_entry(buf, &heap->buf_list, list) {
				if (buf && buf->uuid == debugfs_query_param) {
					query_result = kzalloc(sizeof(*query_result), GFP_KERNEL);
					if (query_result) {
						query_result->buf = buf;
						list_add_tail(&query_result->link, &query_result_list);
						total_buf_size += buf->len;
						total_buf_num++;
					} else {
						pr_err("kzalloc failed!\n");
						break;
					}
					break;
				}
			}
			mutex_unlock(&heap->buf_lock);
			break;
		default:
			break;
		}

		if (!list_empty(&query_result_list))
			written = snprintf(tmp_buf, count,
						"Total buffer number: %d, Total buffer size: 0x%lx\n"
						"\nDevice Buffer Objects:\n"
						"%-8s\t%-8s\t%-8s\t%-8s\t%-8s\t%-8s\t%-8s\n",
						total_buf_num, total_buf_size, "name", "uuid",
						"size", "startPFN", "page_num", "owner", "user");
		else
			written = snprintf(tmp_buf, count, "%s",
						"No result of query!!\n");

		if (written >= count)
			goto exit;
	}

	if (!list_empty(&query_result_list)) {
		// print buf info
		list_for_each_entry_safe(query_result, tmp, &query_result_list, link) {
			int w = unimem_heap_dump_buffer(query_result->buf, tmp_buf + written, count - written);
			if (w >= count - written) { //full
				break;
			} else { //remove
				written += w;
				list_del(&query_result->link);
				kfree(query_result);
			}
		}
	}

exit:
	if (list_empty(&query_result_list))
		debugfs_query_type = UM_QUERY_TYPE_MAX;

	ret = copy_to_user(user_buf, tmp_buf, written);
	if (ret != 0) {
		pr_err("copy error\n");
		kfree(tmp_buf);
		return -EIO;
	}

	kfree(tmp_buf);

	*ppos += written;
	return written;
}

static ssize_t unimem_buf_info_write(struct file *fp, const char __user *user_buf,
									size_t count, loff_t *ppos)
{
	int ret;
	char kbuf[64] = {0};
	char *key = NULL, *val = kbuf;

	ret = copy_from_user(kbuf, user_buf, count);
	if (ret) {
		pr_err("%s: copy_from_user error\n", __func__);
		return -EIO;
	}

	key = strsep(&val, " ");
	if (!key || !val) {
		pr_info("Invalide input: %s", kbuf);
		goto exit;
	}

	if (!strncasecmp(key, "pid", 3)) {
		debugfs_query_type = UM_QUERY_BY_PID;
		ret = kstrtoint(val, 0, &debugfs_query_param);
	} else if (!strncasecmp(key, "uuid", 4)) {
		debugfs_query_type = UM_QUERY_BY_UUID;
		ret = kstrtoint(val, 0, &debugfs_query_param);
	} else if (!strncasecmp(key, "heap", 4)) {
		debugfs_query_type = UM_QUERY_BY_HEAP;

		if (!strncasecmp(val, "isp", 3))
			debugfs_query_param = HEAP_ISP;
		else if (!strncasecmp(val, "vpu", 3))
			debugfs_query_param = HEAP_VPU;
		else if (!strncasecmp(val, "npu", 3))
			debugfs_query_param = HEAP_NPU;
		else if (!strncasecmp(val, "pub", 3))
			debugfs_query_param = HEAP_PUB;
		else if (!strncasecmp(val, "sys", 3))
			debugfs_query_param = HEAP_SYS;
		else {
			debugfs_query_type = UM_QUERY_TYPE_MAX;
			pr_info("Invalide heap type: %s\n", val);
		}
	} else {
		debugfs_query_type = UM_QUERY_TYPE_MAX;
		pr_info("Invalide input: %s\n"
						"sample:\n"
						"echo pid 12 > bufinfo\n"
						"echo uuid 12 > bufinfo\n"
						"echo heap isp > bufinfo\n", kbuf);
	}

exit:
	*ppos += count;
	return count;
}

static const struct file_operations unimem_buf_info_fops = {
	.read = unimem_buf_info_read,
	.write = unimem_buf_info_write,
};

int unimem_heap_init_debugfs(void)
{
	struct dentry *d, *debugfs_dir;
	int err = 0;

	debugfs_dir = debugfs_create_dir("unimem_heap", NULL);
	if (IS_ERR(debugfs_dir))
		return PTR_ERR(debugfs_dir);

	d = debugfs_create_file("bufinfo", 0444, debugfs_dir,
				NULL, &unimem_buf_info_fops);
	if (IS_ERR(d)) {
		pr_debug("failed to create node bufinfo\n");
		debugfs_remove_recursive(debugfs_dir);
		debugfs_dir = NULL;
		err = PTR_ERR(d);
	}

	INIT_LIST_HEAD(&query_result_list);

	return err;
}

int unimem_heap_deinit_debugfs(void)
{
	struct dentry *d = debugfs_lookup("unimem_heap", NULL);

	if (IS_ERR(d))
		return PTR_ERR(d);
	debugfs_remove_recursive(d);
	return 0;
}
