/************************************
 * Test for delay function (ndelay, udelay, mdelay)
 * user timeval and timespec
 * Author: zht
 * Date: 2012-08-23
 ************************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/time.h> /* timeval... */
#include <linux/delay.h> /* ndelay... */


static int delay_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;
	struct timeval tval1, tval2, tval3, tval4;
	struct timespec tspec1, tspec2, tspec3, tspec4;

	/* get current time */
	do_gettimeofday(&tval1);
	getnstimeofday(&tspec1);

	/* delay 30ns */
	ndelay(30);
	getnstimeofday(&tspec2);
	do_gettimeofday(&tval2);

	/* delay 30us */
	udelay(30);
	do_gettimeofday(&tval3);
	getnstimeofday(&tspec3);

	/* delay 30ms */
	mdelay(30);
	do_gettimeofday(&tval4);
	getnstimeofday(&tspec4);

	/* print time */
	ret += sprintf(page+ret, "----Current time----\n");
	ret += sprintf(page+ret, "timeval:  %lds -- %ldus\n", tval1.tv_sec, tval1.tv_usec);
	ret += sprintf(page+ret, "timespec: %lds -- %ldns\n", tspec1.tv_sec, tspec1.tv_nsec);

	ret += sprintf(page+ret, "----Delay 30ns----\n");
	ret += sprintf(page+ret, "timeval:  %lds -- %ldus\n", tval2.tv_sec, tval2.tv_usec);
	ret += sprintf(page+ret, "timespec: %lds -- %ldns\n", tspec2.tv_sec, tspec2.tv_nsec);

	ret += sprintf(page+ret, "----Delay 30us----\n");
	ret += sprintf(page+ret, "timeval:  %lds -- %ldus\n", tval3.tv_sec, tval3.tv_usec);
	ret += sprintf(page+ret, "timespec: %lds -- %ldns\n", tspec3.tv_sec, tspec3.tv_nsec);

	ret += sprintf(page+ret, "----Delay 30ms----\n");
	ret += sprintf(page+ret, "timeval:  %lds -- %ldus\n", tval4.tv_sec, tval4.tv_usec);
	ret += sprintf(page+ret, "timespec: %lds -- %ldns\n", tspec4.tv_sec, tspec4.tv_nsec);

	return ret;
}


static int __init my_init(void)
{
	create_proc_read_entry("proc_delay1", 0, NULL,
			delay_proc_read, NULL);
	return 0;
}

static void __exit my_exit(void)
{
	remove_proc_entry("proc_delay1", NULL);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

