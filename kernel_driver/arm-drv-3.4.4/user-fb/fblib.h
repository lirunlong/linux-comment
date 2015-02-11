/**********************
 * drawfb库的头文件
 **********************/

#ifndef __DRAW_FB
#define __DRAW_FB

#include <linux/fb.h>

/***********************
 * 记录fb信息的核心结构体
 ***********************/
struct fb_val {
	int fd;
	char fb_name[32];
	int win_num;
	struct fb_fix_screeninfo fix_info;
	struct fb_var_screeninfo var_info;
	int bytes_per_pixel;
	int bpp;
	int xres;
	int yres;
	int alpha;
	int fb_length;
	void *mmap_vir; 
};


/*************************
 * s3cfb驱动支持的ioctls
 *************************/
#define ON 	1
#define OFF	0

#define S3CFB_OSD_START		_IO('F', 201)
#define S3CFB_OSD_STOP		_IO('F', 202)
#define S3CFB_OSD_MOVE_LEFT	_IO('F', 203)
#define S3CFB_OSD_MOVE_RIGHT	_IO('F', 204)
#define S3CFB_OSD_MOVE_UP	_IO('F', 205)
#define S3CFB_OSD_MOVE_DOWN	_IO('F', 206)
#define S3CFB_OSD_SET_INFO	_IOW('F', 207, s3cfb_win_info_t)
#define S3CFB_OSD_ALPHA0_SET	_IOW('F', 208, unsigned int)
#define S3CFB_OSD_ALPHA1_SET	_IOW('F', 209, unsigned int)
#define S3CFB_OSD_ALPHA_MODE	_IOW('F', 210, unsigned int)

#define S3CFB_COLOR_KEY_START		_IO('F', 300)
#define S3CFB_COLOR_KEY_STOP		_IO('F', 301)
#define S3CFB_COLOR_KEY_ALPHA_START	_IO('F', 302)
#define S3CFB_COLOR_KEY_ALPHA_STOP	_IO('F', 303)
#define S3CFB_COLOR_KEY_SET_INFO	_IOW('F', 304, s3cfb_color_key_t)
#define S3CFB_COLOR_KEY_VALUE		_IOW('F', 305, s3cfb_color_val_t)

#define S3CFB_GET_DMAINFO		_IOR('F', 306, s3cfb_dma_info_t)
#define S3CFB_SET_VSYNC_INT		_IOW('F', 307, int)
#define S3CFB_CHANGE_WIN0		_IOW('F', 308, int)
#define S3CFB_CHANGE_WIN1		_IOW('F', 309, int)

#define MKBPP565(r,g,b)\
	((((r) & 0x1f) << 11) | (((g) & 0x3f) << 5) | ((b) & 0x1f))

#define MKBPP888(r,g,b)\
	((((r) & 0xff) << 24) | (((g) & 0xff) << 16) | ((b) & 0xff))

extern void draw_point(struct fb_val *, int, int, int);
extern int draw_picture(struct fb_val *, int);
extern int fb_init(struct fb_val *);
extern int fb_exit(struct fb_val *);

#endif //__DRAW_FB

