/*******************************
 * 4.3 inch LCD screen init
 * author: zht
 * date: 2012-04-20
 *******************************/

#include "regs-fb.h"
#include "s3cfb.h"

#define S3CFB_HFP		2	/* front porch */
#define S3CFB_HSW		41	/* hsync width */
#define S3CFB_HBP		2	/* back porch */

#define S3CFB_VFP		2	/* front porch */
#define S3CFB_VSW		10	/* vsync width */
#define S3CFB_VBP		2	/* back porch */

#define S3CFB_HRES		480	/* horizon pixel  x resolition */
#define S3CFB_VRES		272	/* line cnt       y resolution */

#define S3CFB_HRES_OSD		480	/* horizon pixel  x resolition */
#define S3CFB_VRES_OSD		272	/* line cnt       y resolution */

#define S3CFB_VFRAME_FREQ     	60	/* frame rate freq */

#define S3CFB_PIXEL_CLOCK	(S3CFB_VFRAME_FREQ * (S3CFB_HFP + S3CFB_HSW + S3CFB_HBP + S3CFB_HRES) * (S3CFB_VFP + S3CFB_VSW + S3CFB_VBP + S3CFB_VRES))


/*********************
 * 针对4.3寸的LCD屏幕设置FIMD
 *********************/
void s3cfb_init_hw(fimd_info_t *fimd)
{
	printk("4.3 inch LCD be initialized\n");

	fimd->vidcon1 = VIDCON1_HSYNC_INVERT | 
			VIDCON1_VSYNC_INVERT | 
			VIDCON1_VDEN_NORMAL;

	fimd->vidtcon0 = VIDTCON0_VBPD(S3CFB_VBP - 1) | 
			VIDTCON0_VFPD(S3CFB_VFP - 1) | 
			VIDTCON0_VSPW(S3CFB_VSW - 1);

	fimd->vidtcon1 = VIDTCON1_HBPD(S3CFB_HBP - 1) | 
			VIDTCON1_HFPD(S3CFB_HFP - 1) | 
			VIDTCON1_HSPW(S3CFB_HSW - 1);

	fimd->vidtcon2 = VIDTCON2_LINEVAL(S3CFB_VRES - 1) | 
			VIDTCON2_HOZVAL(S3CFB_HRES - 1);

	fimd->vidosd0a = VIDOSDxA_TOPLEFT_X(0) | 
			VIDOSDxA_TOPLEFT_Y(0);

	fimd->vidosd0b = VIDOSDxB_BOTRIGHT_X(S3CFB_HRES - 1) | 
			VIDOSDxB_BOTRIGHT_Y(S3CFB_VRES - 1);

	fimd->vidosd1a = VIDOSDxA_TOPLEFT_X(0) | 
			VIDOSDxA_TOPLEFT_Y(0);

	fimd->vidosd1b = VIDOSDxB_BOTRIGHT_X(S3CFB_HRES_OSD - 1) | 
			VIDOSDxB_BOTRIGHT_Y(S3CFB_VRES_OSD - 1);

	fimd->width = S3CFB_HRES;
	fimd->height = S3CFB_VRES;
	fimd->xres = S3CFB_HRES;
	fimd->yres = S3CFB_VRES;

	fimd->osd_width = S3CFB_HRES_OSD;
	fimd->osd_height = S3CFB_VRES_OSD;
	fimd->osd_xres = S3CFB_HRES_OSD;
	fimd->osd_yres = S3CFB_VRES_OSD;

	fimd->pixclock = S3CFB_PIXEL_CLOCK;
	fimd->hsync_len = S3CFB_HSW;
	fimd->vsync_len = S3CFB_VSW;
	fimd->left_margin = S3CFB_HFP;
	fimd->upper_margin = S3CFB_VFP;
	fimd->right_margin = S3CFB_HBP;
	fimd->lower_margin = S3CFB_VBP;
}

EXPORT_SYMBOL(s3cfb_init_hw);

