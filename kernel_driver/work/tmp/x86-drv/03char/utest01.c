/*****************
 * 用户态ioctl的测试例子
 * author: zht
 * date: 2013-03-21
 *****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MEM_TYPE	'Z'
#define MEM_RESET 	_IO(MEM_TYPE, 1)
#define MEM_RESIZE 	_IOW(MEM_TYPE, 2, int)

int main(void)
{
	int fd, ret;

	fd = open("/dev/char2", O_RDWR);
	if (fd < 0) {
		perror("open /dev/char2");
		exit(1);
	}

	/* reset设备 */
	ret = ioctl(fd, MEM_RESET);
	if (ret == 0)
		printf("Reset OK\n");
	else
		printf("Reset Error\n");

	exit(0);
}

