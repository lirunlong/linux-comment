/************************
 * fb访问的库函数集
 * author: zht
 * date: 2013-02-02
 ************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "fblib.h"


/************************
 * 显示fb_fix_screeninfo中的信息
 ************************/
static int 
test_fix_screeninfo(struct fb_val *fb)
{
	if(ioctl(fb->fd, FBIOGET_FSCREENINFO, &(fb->fix_info))){
		printf("FBIOGET_FSCREENINFO error!\n");
		return -1;
	}	
#if 1
	printf("The fix_screeninfo:\n\
		id = %s\n\
		smem_start = %x\n\
		smem_len = %u\n\
		type = %u\n\
		type_aux = %u\n\
		visual = %u\n\
		xpanstep = %u\n\
		ypanstep = %u\n\
		ywrapstep = %u\n\
		line_length = %u\n\
		long mmio_start = %lu\n\
		mmio_len = %u\n\
		accel = %u\n",			\
		fb->fix_info.id,		\
		fb->fix_info.smem_start,	\
		fb->fix_info.smem_len, 		\
		fb->fix_info.type, 		\
		fb->fix_info.type_aux, 		\
		fb->fix_info.visual, 		\
		fb->fix_info.xpanstep, 		\
		fb->fix_info.ypanstep, 		\
		fb->fix_info.ywrapstep, 	\
		fb->fix_info.line_length,	\
		fb->fix_info.mmio_start,	\
		fb->fix_info.mmio_len, 	\
		fb->fix_info.accel
		);
#endif
	return 0;
}


/************************
 * 显示fb_var_screeninfo中的信息
 ************************/
static int 
test_var_screeninfo(struct fb_val *fb)
{
	if(ioctl(fb->fd, FBIOGET_VSCREENINFO, &(fb->var_info))){
		printf("FBIOGET_VSCREENINFO error!!\n");
		return -1;
	}
#if 1
	printf("The fb_var_screeninfo:\n\
		xres = %u\n\
		yres = %u\n\
		xres_virtual = %u\n\
		yres_virtual = %u\n\
		xoffset = %u\n\
		yoffset = %u\n\
		bits_per_pixel = %u\n\
		grayscale = %u\n\
		red.offset = %u\n\
		red.length =%u\n\
		red.msb_right = %u\n\
		green.offset = %u\n\
		green.length = %u\n\
		green.msb_right = %u\n\
		blue.offset = %u\n\
		blue.length = %u\n\
		blue.msb_right = %u\n\
		transp.offset = %u\n\
		transp.length = %u\n\
		transp.msb_right = %u\n\
		nonstd = %u\n\
		activate = %u\n\
		height = %u\n\
		width = %u\n\
		accel_flags = %u\n\
		pixclock = %u\n\
		left_margin = %u\n\
		right_margin = %u\n\
		upper_margin = %u\n\
		lower_margin = %u\n\
		hsync_len = %u\n\
		vsync_len = %u\n\
		sync = %u\n\
		vmode = %u\n\
		rotate = %u\n",\
		fb->var_info.xres,\
		fb->var_info.yres,\
		fb->var_info.xres_virtual,\
		fb->var_info.yres_virtual,\
		fb->var_info.xoffset,\
		fb->var_info.yoffset,\
		fb->var_info.bits_per_pixel,\
		fb->var_info.grayscale,\
		fb->var_info.red.offset,\
		fb->var_info.red.length,\
		fb->var_info.red.msb_right,\
		fb->var_info.green.offset,\
		fb->var_info.green.length,\
		fb->var_info.green.msb_right,\
		fb->var_info.blue.offset,\
		fb->var_info.blue.length,\
		fb->var_info.blue.msb_right,\
		fb->var_info.transp.offset,\
		fb->var_info.transp.length,\
		fb->var_info.transp.msb_right,\
		fb->var_info.nonstd,\
		fb->var_info.activate,\
		fb->var_info.height,\
		fb->var_info.width,\
		fb->var_info.accel_flags,\
		fb->var_info.pixclock,\
		fb->var_info.left_margin,\
		fb->var_info.right_margin,\
		fb->var_info.upper_margin,\
		fb->var_info.lower_margin,\
		fb->var_info.hsync_len,\
		fb->var_info.vsync_len,\
		fb->var_info.sync,\
		fb->var_info.vmode,\
		fb->var_info.rotate);
#endif
	return 0;
}


/****************************
 * 显示一个8bpp/16bpp/32bpp像素
 ****************************/
static void 
pic_8bpp(struct fb_val *fb, int x, int y, char color)
{
	char *vir = fb->mmap_vir;
	*(vir + y*fb->yres + x) = color;
}

static void 
pic_16bpp(struct fb_val *fb, int x, int y, short color)
{
	short *vir = fb->mmap_vir;
	*(vir + y*fb->yres + x) = color;
}

static void 
pic_32bpp(struct fb_val *fb, int x, int y, int color)
{
	int *vir = fb->mmap_vir;
	*(vir + y*fb->yres + x) = color;
}


/**************************
 * 填充framebuffer的一个像素
 **************************/
void draw_point(struct fb_val *fb, int x, int y, int color)
{
	switch(fb->bpp){
		case 8:
			pic_8bpp(fb, x, y, (char)color);
			break;
		case 16:
			pic_16bpp(fb, x, y, (short)color);	
			break;
		case 32:
			pic_32bpp(fb, x, y, (int)color);
			break;
		default:
			printf("Cannot support bpp %d\n", fb->bpp);
			break;
	}
}


/***********************
 * 显示一张图片
 ***********************/
int draw_picture(struct fb_val *fb, int pic_fd)
{
	int i;
	char *pic_buf;

	/* 映射图片 */
	pic_buf = mmap(NULL, fb->fb_length, PROT_READ | PROT_WRITE, MAP_SHARED, pic_fd, 0);
	if (pic_buf == MAP_FAILED) {
		printf("picture mmap error!!\n");
		return -1;
	}

	/* 将图片内容填入framebuffer */
	for (i = 0; i < fb->fb_length; i++) {
		((char *)fb->mmap_vir)[i] = pic_buf[i];
	}

	if (munmap(pic_buf, fb->fb_length)) {
                printf("munmap error!!\n");
		return -1;
	}
}


/***********************
 * 初始化fb_val
 ***********************/
int fb_init(struct fb_val *fb)
{
	/* 获得fb_info中的fix和var信息 */
	test_fix_screeninfo(fb);
	test_var_screeninfo(fb);

	fb->bpp = fb->var_info.bits_per_pixel;
	fb->xres = fb->var_info.xres;
	fb->yres = fb->var_info.yres;
	fb->fb_length = fb->fix_info.smem_len >> 1;

	switch (fb->bpp) {
	case 8:
		fb->bytes_per_pixel = 1;
		break;
	case 16:
		fb->bytes_per_pixel = 2;
		break;
	case 24:
	case 28:
		fb->bytes_per_pixel = 4;
		break;
	default:
		printf("%s cannot support bpp %d\n", fb->fb_name, fb->fd);
		return -1;
	}

	/* 对framebuffer发送ioctl命令
	 * 窗口0默认打开，其他窗口默认关闭 */
	if (fb->win_num != 0) {
		ioctl(fb->fd, S3CFB_OSD_START, ON);
		ioctl(fb->fd, S3CFB_COLOR_KEY_ALPHA_START);
		ioctl(fb->fd, S3CFB_OSD_ALPHA1_SET, 1);
		ioctl(fb->fd, S3CFB_OSD_ALPHA1_SET, fb->alpha);
	}

	/* 映射framebuffer */
	fb->mmap_vir = mmap(NULL, fb->fb_length, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
	if(fb->mmap_vir == MAP_FAILED){
		printf("mmap error!!\n");
		return -1;
	}

	return 0;
}


/*************************
 * 取消映射并关闭fb
 *************************/
int fb_exit(struct fb_val *fb)
{
	int ret;

	if(munmap(fb->mmap_vir, fb->fb_length)){
		printf("munmap %s error!!\n", fb->fb_name);
		return -1;
	}

	if (fb->win_num != 0)
		ioctl(fb->fd, S3CFB_OSD_STOP);

	ret = close(fb->fd);
	if (ret) {
		printf("close %s error!\n", fb->fb_name);
		return -1;
	}

	return 0;
}

