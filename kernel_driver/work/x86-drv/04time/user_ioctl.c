#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MEM_TYPE 	'Z' 
#define MEM_RESET 	_IO(MEM_TYPE, 1)
#define MEM_NEWBUF 	_IOW(MEM_TYPE, 2, int)


int main(void)
{
	int fd, ret;

	fd = open("/dev/mem0", O_RDWR);
	if (fd == -1) {
		printf("Cannot open /dev/char0\n");
		exit(EXIT_FAILURE);
	}

	ret = ioctl(fd, MEM_RESET);
	if (ret == 0) {
		printf("Reset /dev/mem0 succeed\n");
		exit(0);
	} else {
		printf("Cannot reset /dev/mem0\n");
		exit(1);
	}

	exit(0);
}

