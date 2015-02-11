/*********************
 * 创建可记录多个字符串的proc文件
 * 利用list_head存储
 * author: zht
 * date: 2013-03-19
 ********************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h> /* copy_to|from_user */
#include <linux/proc_fs.h>

/* 链表的头节点 */
struct proc_head {
	int file_size;
	struct list_head head;
};

/* 链表的数据结构体 */
struct proc_item {
	char *str;
	struct list_head item;
};

struct proc_head *dev;


//proc文件对应的读函数
int my_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	//遍历链表，显示每个字符串的信息
	//list_for_each/list_for_each_entry
	int ret = 0;
	struct proc_item *pos;

	list_for_each_entry(pos, &dev->head, item) {
		ret += sprintf(page+ret, "%s", pos->str);
	}
	ret += sprintf(page+ret, "file size is %d bytes\n", dev->file_size);

	return ret;
}

//proc的写函数
int my_proc_write(struct file *filp, const char __user *buf, unsigned long count, void *data)
{
	//分配proc_item
	//分配proc_item->str字符串的内存
	//初始化proc_item
	//将proc_item加入链表头dev
	//更新dev->file_size
	struct proc_item *tmp = kzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp->str = (char *)kzalloc((count+1), GFP_KERNEL);
	if (!tmp->str) {
		kfree(tmp);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&tmp->item);

	/* (to, from, count) */
	if (copy_from_user(tmp->str, buf, count)) {
		kfree(tmp->str);
		kfree(tmp);
		return -EFAULT;
	}

	list_add_tail(&tmp->item, &dev->head);
	dev->file_size += count;

	return count;
}


static int __init my_init(void)
{
	//创建文件/proc/proc_list
	struct proc_dir_entry *file;

	file = create_proc_entry("proc_list", 
		0644, NULL);
	if (!file)
		return -1;
	file->read_proc = my_proc_read;
	file->write_proc = my_proc_write;

	//分配并初始化链表头
	dev = (struct proc_head *)kzalloc(sizeof(struct proc_head), GFP_KERNEL);
	if (!dev) {
		remove_proc_entry("proc_list", NULL);
		return -ENOMEM;
	}

	dev->file_size = 0;
	INIT_LIST_HEAD(&dev->head);

	return 0;
}

static void __exit my_exit(void)
{
	//删除/proc/proc_list
	//释放整个链表
	//用list_for_each_entry_safe
	//释放链表头
	struct proc_item *tmp1, *tmp2;

	remove_proc_entry("proc_list", NULL);
	list_for_each_entry_safe(tmp1, tmp2, &dev->head, item) {
		list_del(&tmp1->item);
		kfree(tmp1->str);
		kfree(tmp1);
	}

	kfree(dev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

