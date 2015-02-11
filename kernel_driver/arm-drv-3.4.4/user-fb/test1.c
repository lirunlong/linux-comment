/********************
 * 在fb0上打印图片
 ********************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "fblib.h"

static struct fb_val fb0;

int main(int argc, char **argv)
{
	int i, j, fd, ret;

	memset(&fb0, 0, sizeof(struct fb_val));
	strcpy(fb0.fb_name, "/dev/fb0");

	/* 打开/dev/fb0 */
	fb0.fd = open(fb0.fb_name, O_RDWR);
	if (fb0.fd < 0) {
		printf("open %s error!\n", fb0.fb_name);
		exit(1);
	}
	printf("open %s\n", fb0.fb_name);

	/* 初始化fb_val */
	fb0.win_num = 0;
	ret = fb_init(&fb0);
	if (ret) {
		perror("fb0");
		close(fb0.fd);
		exit(1);
	}

	/* 将framebuffer填充为红色 */
	for(i = 0; i < fb0.xres; i++){
		for(j = 0; j < fb0.yres; j++){
			draw_point(&fb0, i, j, MKBPP565(255, 0, 0));
		}
	}
	
	sleep(2);

	/* 打开图片0 */
	fd = open("/usr/local/app_fb/565_0.bin", O_RDWR);
	if(fd < 0){
		printf("open pic error!!!\n");
		fb_exit(&fb0);
		exit(1);
	}
	draw_picture(&fb0, fd);
	sleep(2);
	close(fd);

	/* munmap并关闭fb0 */
	fb_exit(&fb0);

	return 0;
}

