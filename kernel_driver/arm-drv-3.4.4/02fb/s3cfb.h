/*********************************
 * S3C6410 fb驱动的头文件
 * 定义fb以及FIMD对应的私有结构体
 * author: zht
 * date: 2013-01-23
 *********************************/

#ifndef _S3CFB_H_
#define _S3CFB_H_

#include <linux/fb.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include "regs-fb.h"

/**********************
 * 一些默认的参量定义
 **********************/
#define MHZ 	(1000 * 1000)
#define ON 	1
#define OFF	0

#define S3CFB_PIXEL_BPP_8	8
#define S3CFB_PIXEL_BPP_16	16  /* RGB 5-6-5 */
#define S3CFB_PIXEL_BPP_24	24  /* RGB 8-8-8 */
#define S3CFB_PIXEL_BPP_25	25  /* ARGB 1-8-8-8 */
#define S3CFB_PIXEL_BPP_28	28  /* ARGB 4-8-8-8 */

#define S3CFB_OUTPUT_RGB	0
#define S3CFB_OUTPUT_TV		1

#define S3CFB_MAX_NUM		5

#define PAL_BUF_CLEAR		(0x80000000)
#define S3CFB_COLOR_KEY_DIR_BG 		0
#define S3CFB_COLOR_KEY_DIR_FG 		1

#define S3CFB_ALPHA_MODE_PLANE		0
#define S3CFB_ALPHA_MODE_PIXEL		1
#define S3CFB_MAX_ALPHA_LEVEL		0xf


/**************************
 * 默认的寄存器设置
 * 主要用于初始化fimd_info_t
 **************************/

#define VIDCON0_DEF	VIDCON0_PROGRESSIVE | \
			VIDCON0_VIDOUT_RGB | \
			VIDCON0_L1_DATA_16BPP | \
			VIDCON0_L0_DATA_16BPP | \
			VIDCON0_PNRMODE_RGB | \
			VIDCON0_CLKVALUP_ALWAYS | \
			VIDCON0_VCLKFREE_NORMAL | \
			VIDCON0_CLKDIR_DIVIDED | \
			VIDCON0_CLKSEL_HCLK | \
			VIDCON0_ENVID_DISABLE | \
			VIDCON0_ENVID_F_DISABLE

#define DITHMODE_DEF	(DITHMODE_RED_5BIT | \
			DITHMODE_GREEN_6BIT | \
			DITHMODE_BLUE_5BIT) & \
			DITHMODE_DITHER_DISABLE

/* 针对8bpp图像的窗口设定 */
#define WINCON0_BPP8_DEF  WINCONx_BYTSWP_ENABLE | \
			WINCONx_BURSTLEN_4WORD | \
			WINCONx_BPPMODE_8BPP_PAL

#define WINCON1_BPP8_DEF  WINCONx_BYTSWP_ENABLE | \
			WINCONx_BURSTLEN_4WORD | \
			WINCONx_BPPMODE_8BPP_PAL | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_ALPHA_SEL_1

/* 针对16bpp图像的窗口设定 */
#define WINCON0_BPP16_DEF  WINCONx_ENLOCAL_DMA | \
			WINCONx_BUFSEL_0 | \
			WINCONx_BUFAUTOEN_DISABLE | \
			WINCONx_HAWSWP_ENABLE | \
			WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_16BPP_565 | \
			WINCONx_ENWIN_DISABLE

#define WINCON1_BPP16_DEF  WINCONx_ENLOCAL_DMA | \
			WINCONx_BUFSEL_0 | \
			WINCONx_BUFAUTOEN_DISABLE | \
			WINCONx_HAWSWP_ENABLE | \
			WINCONx_BURSTLEN_16WORD | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_BPPMODE_16BPP_565 | \
			WINCONx_ALPHA_SEL_1 | \
			WINCONx_ENWIN_DISABLE

#define WINCON2_BPP16_DEF  WINCONx_ENLOCAL_DMA | \
			WINCONx_HAWSWP_ENABLE | \
			WINCONx_BURSTLEN_4WORD | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_BPPMODE_16BPP_565 | \
			WINCONx_ALPHA_SEL_1 | \
			WINCONx_ENWIN_DISABLE

#define WINCON3_BPP16_DEF  WINCONx_HAWSWP_ENABLE | \
			WINCONx_BURSTLEN_4WORD | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_BPPMODE_16BPP_565 | \
			WINCONx_ALPHA_SEL_1 | \
			WINCONx_ENWIN_DISABLE

#define WINCON4_BPP16_DEF  WINCONx_HAWSWP_ENABLE | \
			WINCONx_BURSTLEN_4WORD | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_BPPMODE_16BPP_565 | \
			WINCONx_ALPHA_SEL_1 | \
			WINCONx_ENWIN_DISABLE

/* 针对24bpp图像的窗口设定 */
#define WINCON0_BPP24_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888

#define WINCON1_BPP24_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888 | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_ALPHA_SEL_1

#define WINCON2_BPP24_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888 | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_ALPHA_SEL_1

#define WINCON3_BPP24_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888 | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_ALPHA_SEL_1

#define WINCON4_BPP24_DEF  WINCONx_HAWSWP_DISABLE | \
			WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888 | \
			WINCONx_BLD_PIX_PLANE | \
			WINCONx_ALPHA_SEL_1

/* 针对28bpp图像的窗口设定 */
#define WINCON0_BPP28_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_24BPP_888

#define WINCON1_BPP28_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_28BPP_A888 | \
			WINCONx_BLD_PIX_PIXEL | \
			WINCONx_ALPHA_SEL_1

#define WINCON2_BPP28_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_28BPP_A888 | \
			WINCONx_BLD_PIX_PIXEL | \
			WINCONx_ALPHA_SEL_1

#define WINCON3_BPP28_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_28BPP_A888 | \
			WINCONx_BLD_PIX_PIXEL | \
			WINCONx_ALPHA_SEL_1

#define WINCON4_BPP28_DEF  WINCONx_BURSTLEN_16WORD | \
			WINCONx_BPPMODE_28BPP_A888 | \
			WINCONx_BLD_PIX_PIXEL | \
			WINCONx_ALPHA_SEL_1

#define VIDOSD14C_DEF 	VIDOSD14C_ALPHA1_R(S3CFB_MAX_ALPHA_LEVEL) | \
			VIDOSD14C_ALPHA1_G(S3CFB_MAX_ALPHA_LEVEL) | \
			VIDOSD14C_ALPHA1_B(S3CFB_MAX_ALPHA_LEVEL)

#define VIDINTCON0_DEF 	VIDINTCON0_FRAMESEL0_VSYNC | \
			VIDINTCON0_FRAMESEL1_NONE | \
			VIDINTCON0_INTFRMEN_DISABLE | \
			VIDINTCON0_FIFOSEL_WIN0_EN | \
			VIDINTCON0_FIFOLEVEL_25 | \
			VIDINTCON0_FIFOINT_DISABLE | \
			VIDINTCON0_INT_ENABLE

#define WxKEYCON0_DEF 	WxKEYCON0_KEYBL_DIS | \
			WxKEYCON0_KEYEN_DIS | \
			WxKEYCON0_DIRCON_MATCH_FG | \
			WxKEYCON0_COMPKEY(0x0)

#define WxKEYCON1_DEF	WxKEYCON1_COLVAL(0xffffff)



/*************************
 * fb驱动支持的ioctls
 *************************/
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
#if defined(CONFIG_S3CFB_DOUBLE_BUFFER)
#define S3CFB_CHANGE_WIN0		_IOW('F', 308, int)
#define S3CFB_CHANGE_WIN1		_IOW('F', 309, int)
#endif 


/***********************
 * s3cfb_test01.c中用到的私有结构体 
 ***********************/

/* 本结构用于描述一个窗口
 * 可以通过ioctl命令改变一个窗口的信息 */
typedef struct {
	int bpp;
	int left_x;
	int top_y;
	int width;
	int height;
} s3cfb_win_info_t;

/* 本结构用于描述color key的设置 */
typedef struct {
	int direction;
	unsigned int compkey_red;
	unsigned int compkey_green;
	unsigned int compkey_blue;
} s3cfb_color_key_t;


typedef struct {
	unsigned int colval_red;
	unsigned int colval_green;
	unsigned int colval_blue;
} s3cfb_color_val_t;


typedef struct {
	dma_addr_t map_dma_f1;
	dma_addr_t map_dma_f2;
} s3cfb_dma_info_t;


/*****************************
 * 私有结构体，用于描述一个framebuffer
 * 由于6410支持5层窗口的层叠，因此本结构最多分配5个
 *****************************/
typedef struct {
	struct fb_info	fb;
	unsigned int	win_id;
	unsigned int	bytes_per_pixel;

	/* 窗口缓冲区1的物理和虚拟地址 */
	dma_addr_t	map_dma_f1;
	unsigned char *	map_cpu_f1;
	unsigned int	map_size_f1;

	/* 窗口缓冲区2的物理和虚拟地址 */
	dma_addr_t	map_dma_f2;
	unsigned char *	map_cpu_f2;
	unsigned int	map_size_f2;

	/* 缓存调色板中的颜色 */
	unsigned int	palette_ready;
	unsigned int	pal_buf[256];
	unsigned int	pseudo_pal[16];

	unsigned int 	register_ok;
	unsigned int	buffer_ok;
	unsigned int 	cmap_ok;
} s3cfb_info_t;


/*************************
 * 用于描述FIMD以及LCD屏的私有结构体
 * 本结构只会初始化一个
 *************************/
typedef struct {
	struct device	*dev;
	struct clk	*clk;
	struct resource	*mem;
	int irq;	 /* 中断号 */
	int win_num; 	 /* 支持的窗口总数 */
	int palette_win; /* 修改了调色板的窗口 */

	/* screen信息，默认用于设置win0 */
	int width;
	int height;
	int xres;
	int yres;
	int xoffset;
	int yoffset;

	/* OSD信息，默认用于设置win1~4 */
	int osd_width;
	int osd_height;
	int osd_xres;
	int osd_yres;

	/* 默认的bpp，对应的字节数，像素时钟等 */
	int def_bpp;
	int def_bytes;
	unsigned long pixclock;

	int hsync_len;
	int left_margin;
	int right_margin;
	int vsync_len;
	int upper_margin;
	int lower_margin;
	int sync;
	int cmap_grayscale;

	/* FIMD中的寄存器 */
	unsigned long vidcon0;
	unsigned long vidcon1;
	unsigned long vidcon2;
	unsigned long vidtcon0;
	unsigned long vidtcon1;
	unsigned long vidtcon2;

	unsigned long wincon0;
	unsigned long wincon1;
	unsigned long wincon2;
	unsigned long wincon3;
	unsigned long wincon4;

	unsigned long vidosd0a;
	unsigned long vidosd0b;
	unsigned long vidosd0c;
	unsigned long vidosd1a;
	unsigned long vidosd1b;
	unsigned long vidosd1c;
	unsigned long vidosd1d;
	unsigned long vidosd2a;
	unsigned long vidosd2b;
	unsigned long vidosd2c;
	unsigned long vidosd2d;
	unsigned long vidosd3a;
	unsigned long vidosd3b;
	unsigned long vidosd3c;
	unsigned long vidosd4a;
	unsigned long vidosd4b;
	unsigned long vidosd4c;

	unsigned long vidw00add0b0;
	unsigned long vidw00add0b1;
	unsigned long vidw01add0;
	unsigned long vidw01add0b0;
	unsigned long vidw01add0b1;

	unsigned long vidw00add1b0;
	unsigned long vidw00add1b1;
	unsigned long vidw01add1;
	unsigned long vidw01add1b0;
	unsigned long vidw01add1b1;

	unsigned long vidw00add2b0;
	unsigned long vidw00add2b1;

	unsigned long vidw02add0;
	unsigned long vidw03add0;
	unsigned long vidw04add0;

	unsigned long vidw02add1;
	unsigned long vidw03add1;
	unsigned long vidw04add1;

	unsigned long vidw00add2;
	unsigned long vidw01add2;
	unsigned long vidw02add2;
	unsigned long vidw03add2;
	unsigned long vidw04add2;

	unsigned long vidintcon;
	unsigned long vidintcon0;
	unsigned long vidintcon1;
	unsigned long w1keycon0;
	unsigned long w1keycon1;
	unsigned long w2keycon0;
	unsigned long w2keycon1;
	unsigned long w3keycon0;
	unsigned long w3keycon1;
	unsigned long w4keycon0;
	unsigned long w4keycon1;

	unsigned long win0map;
	unsigned long win1map;
	unsigned long win2map;
	unsigned long win3map;
	unsigned long win4map;

	unsigned long wpalcon;
	unsigned long dithmode;
} fimd_info_t;


/**********************
 * 函数原型声明
 **********************/

extern void s3cfb_init_hw(fimd_info_t *);
extern void s3cfb_enable_local(int win, int in_yuv, int sel);
extern void s3cfb_enable_dma(int win);
extern void s3cfb_set_output_path(int out);
extern void s3cfb_enable_rgbport(int on);

/* static void s3cfb_init_fimd(fimd_info_t *);
static int s3cfb_init_regs(s3cfb_info_t *);
static void s3cfb_init_fbinfo(s3cfb_info_t *, char *, int);
static int s3cfb_win_init(int);
static void s3cfb_win_release(int);

static void s3cfb_activate_var(s3cfb_info_t *, struct fb_var_screeninfo *);
static void s3cfb_set_fb_addr(s3cfb_info_t *);
static int s3cfb_set_alpha_level(s3cfb_info_t *, unsigned int, unsigned int);
static int s3cfb_set_alpha_mode(s3cfb_info_t *, int);
static int s3cfb_set_win_position(s3cfb_info_t *, int left_x, int top_y, int width, int height);
static int s3cfb_set_win_size(s3cfb_info_t *, int, int);
static int s3cfb_set_fb_size(s3cfb_info_t *);
static void s3cfb_set_output_path(int);
static void s3cfb_enable_rgbport(int);

static void s3cfb_pre_init(void);
static int s3cfb_set_gpio(void);

static int s3cfb_map_video_memory(s3cfb_info_t *);
static void s3cfb_unmap_video_memory(s3cfb_info_t *);
static int s3cfb_onoff_win(s3cfb_info_t *, int);
static int s3cfb_onoff_color_key_alpha(s3cfb_info_t *, int);
static int s3cfb_onoff_color_key(s3cfb_info_t *, int);
static int s3cfb_set_color_key_reg(s3cfb_info_t *, s3cfb_color_key_t *);
static int s3cfb_set_color_key_value(s3cfb_info_t *, s3cfb_color_val_t *);
static int s3cfb_set_bpp(s3cfb_info_t *, int);
static void s3cfb_stop_lcd(void);
static void s3cfb_start_lcd(void);
static int s3cfb_init_win(s3cfb_info_t *fbi, int bpp, int left_x, int top_y, int width, int height, int onoff);
static void s3cfb_update_palette(s3cfb_info_t *, unsigned int, unsigned int);
*/

extern int soft_cursor(struct fb_info *, struct fb_cursor *);

#endif	/* _S3CFB_H_ */

