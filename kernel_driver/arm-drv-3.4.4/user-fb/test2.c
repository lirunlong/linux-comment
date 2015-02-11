/*******************
 * 打开/dev/fb1，并设置透明度
 * 连续打印两幅图片，程序退出后，关闭fb1
 * author: zht
 * date: 2012-08-02
 *******************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "fblib.h"

static struct fb_val fb1;

int main(int argc, char **argv)
{
	int fd, i, j, ret;

	memset(&fb1, 0, sizeof(struct fb_val));
	strcpy(fb1.fb_name, "/dev/fb1");

	/* 打开fb1 */
	fb1.fd = open(fb1.fb_name, O_RDWR);
	if (fb1.fd < 0) {
		printf("open %s error!\n", fb1.fb_name);
		exit(1);
	}
	printf("open %s\n", fb1.fb_name);

	/* 初始化fb_val */
	fb1.win_num = 1;
	fb1.alpha = 0xa;
	ret = fb_init(&fb1);
	if (ret) {
		perror("fb1");
		close(fb1.fd);
		exit(1);
	}

	/* 将framebuffer填充为绿色 */
	for(i = 0; i < fb1.xres; i++){
		for(j = 0; j < fb1.yres; j++){
			draw_point(&fb1, i, j, MKBPP565(0, 255, 0));
		}
	}
	sleep(2);

	/* 打印图片1 */
	fd = open("/usr/local/app_fb/565_1.bin", O_RDWR);
	if(fd < 0){
		printf("open pic1 error!!!\n");
		fb_exit(&fb1);
		exit(1);
	}
	draw_picture(&fb1, fd);
	close(fd);
	sleep(2);

	/* 打印图片2 */
	fd = open("/usr/local/app_fb/565_2.bin", O_RDWR);
	if(fd < 0){
		printf("open pic2 error!!!\n");
		fb_exit(&fb1);
		exit(1);
	}
	draw_picture(&fb1, fd);
	close(fd);
	sleep(2);

	/* 关闭fb */
	fb_exit(&fb1);

	return 0;
}

