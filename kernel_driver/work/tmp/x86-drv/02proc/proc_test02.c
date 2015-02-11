/*********************
 * 创建可读写的proc文件
 * author: zht
 * date: 2013-03-19
 ********************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h> /* copy_to|from_user */
#include <linux/proc_fs.h>

static char *str = NULL;

//proc文件对应的读函数
int my_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;

	ret = sprintf(page, "%s", str);

	return ret;
}

//proc的写函数
int my_proc_write(struct file *filp, const char __user *buf, unsigned long count, void *data)
{
	char *tmp = kzalloc(count, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	/* copy_to_user/copy_from_user(to,from,count) */
	if (copy_from_user(tmp, buf, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	kfree(str);
	str = tmp;

	return count;
}


static int __init my_init(void)
{
	//创建文件/proc/proc_rw
	struct proc_dir_entry *file;

	file = create_proc_entry("proc_rw", 
		0644, NULL);
	if (!file)
		return -1;
	file->read_proc = my_proc_read;
	file->write_proc = my_proc_write;

	return 0;
}

static void __exit my_exit(void)
{
	//删除/proc/proc_rw
	remove_proc_entry("proc_rw", NULL);
	kfree(str);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

