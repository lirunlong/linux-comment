/**********************************
 * S3C6410显示控制器(FIMD)的fb驱动
 * 支持3.4.4内核
 * author: zht
 * date: 2013-03-11
 **********************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/fb.h> /* fb_info */

#include <plat/gpio-cfg.h>
#include <mach/regs-lcd.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>

#if defined(CONFIG_PM)
#include <plat/pm.h>
#endif

#include "regs-fb.h"
#include "s3cfb.h"

/* FIMD寄存器的虚拟基地址 */
static void __iomem *io_base;

/* 显示控制器的私有结构体 */
fimd_info_t fimd;

/* 每个窗口的私有结构体，最多5个 */
s3cfb_info_t s3cfb_info[S3CFB_MAX_NUM];


/*********************
 * 启动向LCD屏输出信号
 *********************/
static void 
s3cfb_start_lcd(void)
{
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);

	tmp = readl(io_base + VIDCON0);
	tmp |= VIDCON0_ENVID_ENABLE | VIDCON0_ENVID_F_ENABLE;
	writel(tmp, (io_base + VIDCON0));

	local_irq_restore(flags);
}


/*********************
 * 停止向LCD屏输出
 *********************/
static void 
s3cfb_stop_lcd(void)
{
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);

	tmp = readl(io_base + VIDCON0);
	tmp &= ~(VIDCON0_ENVID_MASK | VIDCON0_ENVID_F_MASK);
	tmp |= (VIDCON0_ENVID_DISABLE | VIDCON0_ENVID_F_DISABLE);
	writel(tmp, (io_base + VIDCON0));

	local_irq_restore(flags);
}


/**********************
 * 使能或关闭指定的window(0~4)
 **********************/
static int 
s3cfb_onoff_win(s3cfb_info_t *fbi, int onoff)
{
	int win_num = fbi->win_id;
	int value;

	if (onoff) {
		value =readl(io_base + WINCON0 + (0x04 * win_num));
		value |= WINCONx_ENWIN_ENABLE;
		writel(value , (io_base + WINCON0 + (0x04 * win_num)));
	} else {
		value = readl(io_base + WINCON0 + (0x04 * win_num));
		value &= ~WINCONx_ENWIN_MASK;
		value |= WINCONx_ENWIN_DISABLE;
		writel(value , (io_base + WINCON0 + (0x04 * win_num)));
	}

	return 0;
}


/************************
 * s3cfb_pre_init()
 * 使能frame中断
 ************************/
static void 
s3cfb_pre_init(void)
{
	fimd.vidintcon0 &= ~VIDINTCON0_FRAMESEL0_MASK;
	fimd.vidintcon0 |= VIDINTCON0_FRAMESEL0_VSYNC;
	fimd.vidintcon0 |= VIDINTCON0_INTFRMEN_ENABLE;

	writel(fimd.vidintcon0, (io_base + VIDINTCON0));
}


/*********************
 * s3cfb_set_gpio()
 * 设置LCD屏的IO并使能屏的电源
 *********************/
static int 
s3cfb_set_gpio(void)
{
	unsigned long val;
	int i, err;

	/* select TFT LCD type (RGB I/F) */
	val = readl(S3C64XX_SPCON);
	val &= ~0x3;
	val |= (1 << 0);
	writel(val, S3C64XX_SPCON);

	/* 设定LCD的IO */
	for (i = 0; i < 16; i++)
		s3c_gpio_cfgpin(S3C64XX_GPI(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 12; i++)
		s3c_gpio_cfgpin(S3C64XX_GPJ(i), S3C_GPIO_SFN(2));

	/* GPF14为LCD屏背光增压芯片的开关 */
	printk("Open LCD power1\n");
	if (gpio_is_valid(S3C64XX_GPF(14))) {
		err = gpio_request(S3C64XX_GPF(14), "GPF");
		if (err) {
			printk("failed to request GPF for lcd\n");
			return err;
		}
		gpio_direction_output(S3C64XX_GPF(14), 1);
		gpio_set_value(S3C64XX_GPF(14), 1);
		gpio_free(S3C64XX_GPF(14));
	}
	
	printk("open LCD power2\n");
	if (gpio_is_valid(S3C64XX_GPE(0))) {
		err = gpio_request(S3C64XX_GPE(0), "GPE");
		if (err) {
			printk("failed to request GPE for lcd\n");
			return err;
		}
		gpio_direction_output(S3C64XX_GPE(0), 1);
		gpio_set_value(S3C64XX_GPE(0), 1);
		gpio_free(S3C64XX_GPE(0));
	}

	return 0;
}


/**********************
 * 为给定的window分配framebuffer
 * win0/win1可能支持双缓冲
 **********************/
static int 
s3cfb_map_video_memory(s3cfb_info_t *fbi)
{
	unsigned int buf_size;

	/* 为给定window分配缓冲区
	 * 如果支持双缓冲，则smem_len应为两个缓冲区的长度 */
	buf_size = PAGE_ALIGN(fbi->fb.fix.smem_len);
	fbi->map_cpu_f1 = dma_alloc_writecombine(fimd.dev, buf_size, &fbi->map_dma_f1, GFP_KERNEL);
	fbi->map_size_f1 = buf_size;

	/* 如果缓冲区分配成功，初始化fbi中的相关成员 */
	if (!fbi->map_cpu_f1) {
		return -ENOMEM;
	} 
	else {
		printk("Win%d--buf0: vir=0x%p; phy=0x%x; size=0x%x\n",
			fbi->win_id, fbi->map_cpu_f1, 
			fbi->map_dma_f1, fbi->map_size_f1);
		memset(fbi->map_cpu_f1, 0x0, fbi->map_size_f1);

		fbi->fb.screen_base = fbi->map_cpu_f1;
		fbi->fb.fix.smem_start = fbi->map_dma_f1;
	}

#if defined(CONFIG_S3CFB_DOUBLE_BUFFER)
	/* 缓冲区2已经和缓冲区1一起分配，下面确定其地址即可 */
	if (fbi->win_id < 2 && fbi->map_cpu_f1) {
		fbi->map_size_f2 = fbi->fb.fix.smem_len / 2;
		fbi->map_cpu_f2 = fbi->map_cpu_f1 + fbi->map_size_f2;
		fbi->map_dma_f2 = fbi->map_dma_f1 + fbi->map_size_f2;

		printk("Win%d--buf1: vir=0x%p; phy=0x%x; size=0x%x\n",
			fbi->win_id, fbi->map_cpu_f2, 
			fbi->map_dma_f2, fbi->map_size_f2);
	}
#endif

	return 0;
}


/***********************
 * 释放指定window的缓冲区
 * 当win0/win1支持双缓冲时，缓冲区分配在一起，只须free一次
 ***********************/
static void 
s3cfb_unmap_video_memory(s3cfb_info_t *fbi)
{
	dma_free_writecombine(fimd.dev, fbi->map_size_f1, fbi->map_cpu_f1,  fbi->map_dma_f1);
}


/***********************
 * s3cfb_check_line_count()
 * 如果行输出计数没有启动，则启动输出
 ***********************/
static void 
s3cfb_check_line_count(void)
{
	int timeout = 30 * 5300;
	unsigned int cfg;
	int i, line;

	i = 0;
	do {
		line = VIDCON1_LINECNT_GET(readl(io_base + VIDCON1));
		if (!line)
			break;
		i++;
	} while (i < timeout);

	if (i == timeout) {
		printk("line count mismatch\n");
		cfg = readl(io_base + VIDCON0);
		cfg |= (VIDCON0_ENVID_F_ENABLE | VIDCON0_ENVID_ENABLE);
		writel(cfg, (io_base + VIDCON0));
	}
}


/*********************
 * s3cfb_enable_local0()
 * 使能窗口0输出，设定窗口0的输入来源为PostProcessor
 *********************/
static void 
s3cfb_enable_local0(int in_yuv)
{
	unsigned int value;

	fimd.wincon0 = readl(io_base + WINCON0);
	fimd.wincon0 &= ~WINCONx_ENWIN_MASK;
	writel(fimd.wincon0, (io_base + WINCON0));	

	fimd.wincon0 &= ~(WINCONx_ENLOCAL_MASK | WINCONx_INRGB_MASK);
	value = WINCONx_ENLOCAL_LOCAL | WINCONx_ENWIN_ENABLE;

	if (in_yuv)
		value |= WINCONx_INRGB_YUV;

	writel((fimd.wincon0 | value), (io_base + WINCON0));
}


/*********************
 * s3cfb_enable_local1()
 * 使能窗口1输出，设定窗口1的输入来源为TV scaler或Camera Preview
 * 参数sel应用于选择TV或Camera
 *********************/
static void 
s3cfb_enable_local1(int in_yuv, int sel)
{
	unsigned int value;

	fimd.wincon1 = readl(io_base + WINCON1);
	fimd.wincon1 &= ~WINCONx_ENWIN_MASK;
	writel(fimd.wincon1, (io_base + WINCON1));

	fimd.wincon1 &= ~(WINCONx_ENLOCAL_MASK | WINCONx_INRGB_MASK);
	fimd.wincon1 &= ~(WINCON1_LOCALSEL_MASK);
	value = sel | WINCONx_ENLOCAL_LOCAL | WINCONx_ENWIN_ENABLE;

	if (in_yuv)
		value |= WINCONx_INRGB_YUV;

	writel((fimd.wincon1 | value), (io_base + WINCON1));
}


/*********************
 * s3cfb_enable_local2()
 * 使能窗口2输出，窗口2的输入为TV scaler或Camera codec
 * 参数sel用于选择TV scaler或camera codec
 *********************/
static void 
s3cfb_enable_local2(int in_yuv, int sel)
{
	unsigned int value;

	fimd.wincon2 = readl(io_base + WINCON2);
	fimd.wincon2 &= ~(WINCONx_ENWIN_MASK | 
			  WINCON2_LOCALSEL_MASK);
	writel(fimd.wincon2, (io_base + WINCON2));

	fimd.wincon2 &= ~(WINCONx_ENLOCAL_MASK | 
			  WINCONx_INRGB_MASK);
	value = sel | WINCONx_ENLOCAL_LOCAL | WINCONx_ENWIN_ENABLE;

	if (in_yuv)
		value |= WINCONx_INRGB_YUV;

	writel((fimd.wincon2 | value), (io_base + WINCON2));
}


/**********************
 * s3cfb_enable_dma0()
 * 设定窗口0的数据源为Dedicated-DMA(即frame buffer)
 **********************/
static void 
s3cfb_enable_dma0(void)
{
	u32 value;

	fimd.wincon0 &= ~(WINCONx_ENLOCAL_MASK | 
			  WINCONx_INRGB_MASK);
	value = WINCONx_ENLOCAL_DMA | WINCONx_ENWIN_ENABLE;

	writel((fimd.wincon0 | value), (io_base + WINCON0));
}


/**********************
 * s3cfb_enable_dma1()
 * 设定窗口1的数据源为Dedicated-DMA(即frame buffer)
 **********************/
static void 
s3cfb_enable_dma1(void)
{
	u32 value;

	fimd.wincon1 &= ~(WINCONx_ENLOCAL_MASK | 
			  WINCONx_INRGB_MASK);
	value = WINCONx_ENLOCAL_DMA | WINCONx_ENWIN_ENABLE;

	writel((fimd.wincon1 | value), (io_base + WINCON1));
}


/**********************
 * s3cfb_enable_dma2()
 * 设定窗口2的数据源为Dedicated-DMA(即frame buffer)
 **********************/
static void 
s3cfb_enable_dma2(void)
{
	u32 value;

	fimd.wincon2 &= ~(WINCONx_ENLOCAL_MASK | 
			  WINCONx_INRGB_MASK);
	value = WINCONx_ENLOCAL_DMA | WINCONx_ENWIN_ENABLE;

	writel((fimd.wincon2 | value), (io_base + WINCON2));
}


/********************
 * s3cfb_enable_local()
 * 函数导出为全局的
 * 选择win0/1/2的数据源为PostProcessor，TV或Camera
 ********************/
void s3cfb_enable_local(int win, int in_yuv, int sel)
{
	s3cfb_check_line_count();

	switch (win) {
	case 0:
		s3cfb_enable_local0(in_yuv);
		break;
	case 1:
		s3cfb_enable_local1(in_yuv, sel);
		break;
	case 2:
		s3cfb_enable_local2(in_yuv, sel);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(s3cfb_enable_local);


/********************
 * s3cfb_enable_dma()
 * 函数导出为全局的
 * 选择win0/1/2的数据源为frame buffer(使用Dedicated-DMA传输)
 ********************/
void s3cfb_enable_dma(int win)
{
	s3cfb_stop_lcd();
	
	switch (win) {
	case 0:
		s3cfb_enable_dma0();
		break;
	case 1:
		s3cfb_enable_dma1();
		break;
	case 2:
		s3cfb_enable_dma2();
		break;
	default:
		break;
	}

	s3cfb_start_lcd();
}

EXPORT_SYMBOL(s3cfb_enable_dma);


/**********************
 * s3cfb_set_output_path()
 * 导出为全局函数
 * 设置输出为LCD, TV或I80
 **********************/
void s3cfb_set_output_path(int out)
{
	unsigned int tmp;

	tmp = readl(io_base + VIDCON0);

	/* 如果输出为LCD mode, 则扫描模式只能是progressive mode */
	tmp &= ~VIDCON0_INTERLACE_MASK;
	if (out == S3CFB_OUTPUT_RGB)
		tmp |= VIDCON0_PROGRESSIVE;

	/* 设定输出模式 */
	tmp &= ~VIDCON0_VIDOUT_MASK;
	tmp |= VIDCON0_VIDOUT(out);

	writel(tmp, (io_base + VIDCON0));
}

EXPORT_SYMBOL(s3cfb_set_output_path);


/**************************
 * s3cfb_enable_rgbport()
 * on == 0: 输出信号采用RGB格式
 * on == 1: 输出信号采用YUV格式
 **************************/
void s3cfb_enable_rgbport(int on)
{
	if (on)
		writel((VIDCON2_ORGYUV_CBCRY | VIDCON2_YUVORD_CRCB), (io_base + VIDCON2));
	else
		writel(0, (io_base + VIDCON2));
}

EXPORT_SYMBOL(s3cfb_enable_rgbport);


#if defined(CONFIG_S3CFB_DOUBLE_BUFFER)
/************************
 * s3cfb_change_buff()
 * 如果WIN0或WIN1支持双缓冲，则根据参数切换
 ************************/
static void 
s3cfb_change_buff(int req_win, int req_fb)
{
	switch (req_win) {
	case 0: /* win0 */
		fimd.wincon0 &= ~WINCONx_BUFSEL_MASK; 
		if (req_fb == 0)
			fimd.wincon0 |= WINCONx_BUFSEL_0;
		else
			fimd.wincon0 |= WINCONx_BUFSEL_1;
		writel((fimd.wincon0 | WINCONx_ENWIN_ENABLE), (io_base + WINCON0));
		break;
	case 1: /* win1 */
		fimd.wincon1 &= ~WINCONx_BUFSEL_MASK; 
		if (req_fb == 0)
			fimd.wincon1 |= WINCONx_BUFSEL_0;
		else
			fimd.wincon1 |= WINCONx_BUFSEL_1;
		writel((fimd.wincon1 | WINCONx_ENWIN_ENABLE), (io_base + WINCON1));
		break;
	/* win2/3/4 don't support double buffer */
	default:
		printk("Win %d don't support double buffer\n", req_win);
		break;
	}
}
#endif


/**************************
 * 根据不同窗口，写FIMD的实际寄存器
 * 注意！win1~4默认是关闭的
 **************************/
static int 
s3cfb_init_regs(s3cfb_info_t *fbi)
{
	struct fb_var_screeninfo *var = &fbi->fb.var;
	unsigned long flags = 0, page_width = 0;
	unsigned long video_phy_f1 = fbi->map_dma_f1;
	unsigned long video_phy_f2 = fbi->map_dma_f2;
	int win_num = fbi->win_id;

	local_irq_save(flags);

	/* 计算显示一行需要的字节数 */
	page_width = var->xres * fbi->bytes_per_pixel;

	/* win0 */
	if (win_num == 0) {
		fimd.vidcon0 &= ~(VIDCON0_ENVID_MASK | 
				  VIDCON0_ENVID_F_MASK);
		writel(fimd.vidcon0, (io_base + VIDCON0));
		fimd.vidcon0 |= VIDCON0_CLKVAL((clk_get_rate(fimd.clk)/fimd.pixclock) - 1);
 	}

	/* 将缓冲区0的物理起始，结束地址以及size写入寄存器 */
	writel(video_phy_f1, (io_base + VIDW00ADD0B0 + (0x08 * win_num)));
	writel(VIDWxADD1_VBASEL(video_phy_f1 + (page_width *var->yres)), (io_base + VIDW00ADD1B0 + (0x08 * win_num)));
	writel(VIDWxADD2_PAGEWIDTH(page_width), (io_base + VIDW00ADD2 + (0x04 * win_num)));

	/* 如果是win0和win1，则写入缓冲区1的相关信息(无size信息) */
	if (win_num < 2) {
		writel(video_phy_f2, (io_base + VIDW00ADD0B1 + (0x08 * win_num)));
		writel(VIDWxADD1_VBASEL(video_phy_f2 + (page_width * var->yres)), (io_base + VIDW00ADD1B1 + (0x08 * win_num)));
	}

	/* 写入不同窗口的相关寄存器 */
	switch (win_num) {
	case 0:
		writel(fimd.wincon0,  (io_base + WINCON0));
		writel(fimd.vidcon0,  (io_base + VIDCON0));
		writel(fimd.vidcon1,  (io_base + VIDCON1));
		writel(fimd.vidtcon0, (io_base + VIDTCON0));
		writel(fimd.vidtcon1, (io_base + VIDTCON1));
		writel(fimd.vidtcon2, (io_base + VIDTCON2));
		writel(fimd.dithmode, (io_base + DITHMODE));
		writel(fimd.vidintcon0, (io_base + VIDINTCON0));
		writel(fimd.vidintcon1, (io_base + VIDINTCON1));
		writel(fimd.vidosd0a, (io_base + VIDOSD0A));
		writel(fimd.vidosd0b, (io_base + VIDOSD0B));
		writel(fimd.vidosd0c, (io_base + VIDOSD0C));

		s3cfb_onoff_win(fbi, ON);
		break;
	case 1:
		writel(fimd.wincon1,  (io_base + WINCON1));
		writel(fimd.vidosd1a, (io_base + VIDOSD1A));
		writel(fimd.vidosd1b, (io_base + VIDOSD1B));
		writel(fimd.vidosd1c, (io_base + VIDOSD1C));
		writel(fimd.vidosd1d, (io_base + VIDOSD1D));

		s3cfb_onoff_win(fbi, OFF);
		break;
	case 2:
		writel(fimd.wincon2,  (io_base + WINCON2));
		writel(fimd.vidosd2a, (io_base + VIDOSD2A));
		writel(fimd.vidosd2b, (io_base + VIDOSD2B));
		writel(fimd.vidosd2c, (io_base + VIDOSD2C));
		writel(fimd.vidosd2d, (io_base + VIDOSD2D));

		s3cfb_onoff_win(fbi, OFF);
		break;
	case 3:
		writel(fimd.wincon3,  (io_base + WINCON3));
		writel(fimd.vidosd3a, (io_base + VIDOSD3A));
		writel(fimd.vidosd3b, (io_base + VIDOSD3B));
		writel(fimd.vidosd3c, (io_base + VIDOSD3C));

		s3cfb_onoff_win(fbi, OFF);
		break;
	case 4:	
		writel(fimd.wincon4,  (io_base + WINCON4));
		writel(fimd.vidosd4a, (io_base + VIDOSD4A));
		writel(fimd.vidosd4b, (io_base + VIDOSD4B));
		writel(fimd.vidosd4c, (io_base + VIDOSD4C));

		s3cfb_onoff_win(fbi, OFF);
		break;
	}

	writel(fimd.wpalcon,  (io_base + WPALCON));
	local_irq_restore(flags);

	return 0;
}


/*************************
 * s3cfb_activate_var()
 * 根据fb的可变参数设置对应的寄存器
 * 但本函数一次将所有窗口的信息的修改了，应考虑修改
 *************************/
static void 
s3cfb_activate_var(s3cfb_info_t *fbi, struct fb_var_screeninfo *var)
{
	u32 value;

	printk("%s: var->bpp = %d\n", __FUNCTION__, var->bits_per_pixel);

	switch (var->bits_per_pixel) {
	case 8: /* 8位bpp需要启用调色板 */
		fimd.wincon0 = WINCON0_BPP8_DEF;
		fimd.wincon1 = WINCON1_BPP8_DEF; 	
		fimd.def_bpp = S3CFB_PIXEL_BPP_8;
		fimd.def_bytes = 1;
		fimd.wpalcon = WPALCON_W0PAL_565;
		break;
	case 16:
		fimd.wincon0 = WINCON0_BPP16_DEF;
		fimd.wincon1 = WINCON1_BPP16_DEF;
		fimd.wincon2 = WINCON2_BPP16_DEF;
		fimd.wincon3 = WINCON3_BPP16_DEF;
		fimd.wincon4 = WINCON4_BPP16_DEF;
		fimd.def_bpp = S3CFB_PIXEL_BPP_16;
		fimd.def_bytes = 2;
		break;
	case 24:	
		fimd.wincon0 = WINCON0_BPP24_DEF;
		fimd.wincon1 = WINCON1_BPP24_DEF;
		fimd.wincon2 = WINCON2_BPP24_DEF;
		fimd.wincon3 = WINCON3_BPP24_DEF;
		fimd.wincon4 = WINCON4_BPP24_DEF;
		fimd.def_bpp = S3CFB_PIXEL_BPP_24;
		fimd.def_bytes = 4;
		break;
	case 28:
		fimd.wincon0 = WINCON0_BPP28_DEF;
		fimd.wincon1 = WINCON1_BPP28_DEF;
		fimd.wincon2 = WINCON2_BPP28_DEF;
		fimd.wincon3 = WINCON3_BPP28_DEF;
		fimd.wincon4 = WINCON4_BPP28_DEF;

        	fimd.def_bpp = S3CFB_PIXEL_BPP_28;
		fimd.def_bytes = 4;
		if((fbi->win_id == 0) && 
		   (fbi->fb.var.bits_per_pixel == 28))
			fbi->fb.var.bits_per_pixel = 24;
		break;
	}

	/* 将修改过的参数写入寄存器 */
	writel(fimd.wincon0, (io_base + WINCON0));
	writel(fimd.wincon1, (io_base + WINCON1));
	writel(fimd.wincon2, (io_base + WINCON2));
	writel(fimd.wincon3, (io_base + WINCON3));
	writel(fimd.wincon4, (io_base + WINCON4));
	writel(fimd.wpalcon, (io_base + WPALCON));

	/* 下面只默认使能了WIN0，并使能到LCD的输出
	 * 其他窗口的打开需要通过ioctl命令 */
	value = fimd.wincon0 | WINCONx_ENWIN_ENABLE;
	writel(value, (io_base + WINCON0));

	value = fimd.vidcon0 | 
		VIDCON0_ENVID_ENABLE | VIDCON0_ENVID_F_ENABLE;
	writel(value, (io_base + VIDCON0));
}


/* JJNAHM comment.
 * We had some problems related to frame buffer address.
 * We used 2 frame buffers (FB0 and FB1) and GTK used FB1.
 * When GTK launched, GTK set FB0's address to FB1's address.
 * (GTK calls s3c_fb_pan_display() and then it calls this s3c_fb_set_lcdaddr())
 * Even though fbi->win_id is not 0, above original codes set ONLY FB0's address.
 * So, I modified the codes like below.
 * It works by fbi->win_id value.
 * Below codes are not verified yet
 * and there are nothing about Double buffering features
 */

/************************
 * s3cfb_set_fb_addr()
 * 根据var->yoffset设置fb0的物理地址(xoffset默认为0)
 * 如果不支持虚拟屏，那么yoffset应该为0
 ************************/
static void 
s3cfb_set_fb_addr(s3cfb_info_t *fbi)
{
	unsigned long video_phy_f1 = fbi->map_dma_f1;
	unsigned long start_addr, end_addr;
	unsigned int start;

	start = fbi->fb.fix.line_length * fbi->fb.var.yoffset;

	/* 根据yoffset计算缓冲区的起始和结束物理地址 */
	start_addr = video_phy_f1 + start;
	end_addr = start_addr + (fbi->fb.fix.line_length * fbi->fb.var.yres);

	/* 根据win_id，设定对应窗口缓冲区的地址，未考虑双缓冲 */
	switch (fbi->win_id) {
	case 0:
		fimd.vidw00add0b0 = start_addr;
		fimd.vidw00add1b0 = end_addr;
		writel(fimd.vidw00add0b0, (io_base + VIDW00ADD0B0));
		writel(fimd.vidw00add1b0, (io_base + VIDW00ADD1B0));
        	break;
	case 1:
		fimd.vidw01add0b0 = start_addr;
		fimd.vidw01add1b0 = end_addr;
		writel(fimd.vidw01add0b0, (io_base + VIDW01ADD0B0));
		writel(fimd.vidw01add1b0, (io_base + VIDW01ADD1B0));
		break;
	case 2:
		fimd.vidw02add0 = start_addr;
		fimd.vidw02add1 = end_addr;
		writel(fimd.vidw02add0, (io_base + VIDW02ADD0));
		writel(fimd.vidw02add1, (io_base + VIDW02ADD1));
	        break;
	case 3:
		fimd.vidw03add0 = start_addr;
		fimd.vidw03add1 = end_addr;
		writel(fimd.vidw03add0, (io_base + VIDW03ADD0));
		writel(fimd.vidw03add1, (io_base + VIDW03ADD1));
		break;
	case 4:
		fimd.vidw04add0 = start_addr;
		fimd.vidw04add1 = end_addr;
		writel(fimd.vidw04add0, (io_base + VIDW04ADD0));
		writel(fimd.vidw04add1, (io_base + VIDW04ADD1));
		break;
	}
}


/************************
 * s3cfb_set_alpha_level()
 * 设置Window1~4的透明度级别(整个窗口采用同样的透明级别)
 * level: alpha级别(0为完全透明，0xf为不透明)
 * alpha_index: 1为使用alpha1,0为使用alpha0
 ************************/
static int 
s3cfb_set_alpha_level(s3cfb_info_t *fbi, 
		unsigned int level, unsigned int alpha_index)
{
	unsigned long alpha_val;
	int win_num = fbi->win_id;

	if (win_num == 0) {
		printk("WIN0 do not support alpha blending.\n");
		return -1;
	}

	alpha_val = readl(io_base + (VIDOSD0C + (0x10 * win_num)));

	if (alpha_index == 0) {
		alpha_val &= ~(VIDOSD14C_ALPHA0_R_MASK | 
			       VIDOSD14C_ALPHA0_G_MASK | 
			       VIDOSD14C_ALPHA0_B_MASK);
		alpha_val |= (VIDOSD14C_ALPHA0_R(level) | 
			      VIDOSD14C_ALPHA0_G(level) | 
			      VIDOSD14C_ALPHA0_R(level));
	} else {
		alpha_val &= ~(VIDOSD14C_ALPHA1_R_MASK | 
			       VIDOSD14C_ALPHA1_G_MASK | 
			       VIDOSD14C_ALPHA1_B_MASK);
		alpha_val |= (VIDOSD14C_ALPHA1_R(level) | 
			      VIDOSD14C_ALPHA1_G(level) | 
			      VIDOSD14C_ALPHA1_R(level));
	}

	writel(alpha_val, (io_base + VIDOSD0C + (0x10 * win_num)));

	return 0;
}


/*********************
 * s3cfb_set_alpha_mode()
 * 设置Window1~4的alpha模式
 *********************/
static int 
s3cfb_set_alpha_mode(s3cfb_info_t *fbi, int mode)
{
	unsigned long alpha_mode;
	int win_num = fbi->win_id;

	if (win_num == 0) {
		printk("WIN0 do not support alpha blending.\n");
		return -1;
	}

	alpha_mode = readl(io_base + WINCON0 + (0x04 * win_num));
	alpha_mode &= ~(WINCONx_BLD_PIX_MASK | WINCONx_ALPHA_SEL_MASK);

	switch (mode) {
	/* Plane Blending: (全屏幕采用同一个alpha值，驱动默认为alpha1)
	 * 选择0(使用alpha0的值进行混合)
	 * 选择1(使用alpha1的值进行混合) */
	case S3CFB_ALPHA_MODE_PLANE:
		alpha_mode |= (WINCONx_BLD_PIX_PLANE | 
			      WINCONx_ALPHA_SEL_1);
		writel(alpha_mode, (io_base + WINCON0 + (0x04*win_num)));
		break;

	/* Pixel Blending: (按每个像素进行alpha混合)
	 * 选择0(由AEN值决定采用alpha0还是alpha1,AEN为25bpp的bit24)
	 * 选择1(用每个像素的bit[27:24]作为alpha值) */
	case S3CFB_ALPHA_MODE_PIXEL:
		if (fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_28)
			alpha_mode |= WINCONx_BLD_PIX_PIXEL | WINCONx_ALPHA_SEL_1;
		else 
			alpha_mode |= WINCONx_BLD_PIX_PIXEL | WINCONx_ALPHA_SEL_0;
		writel(alpha_mode, (io_base + WINCON0 + (0x04*win_num)));
		break;
	}

	return 0;
}


/**********************
 * s3cfb_set_win_position()
 * 设置window0~4的屏幕位置
 * left_x和top_y记录在var->xoffset/yoffset中
 **********************/
static int 
s3cfb_set_win_position(s3cfb_info_t *fbi, 
		int left_x, int top_y, int width, int height)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num = fbi->win_id;
	int value;

	/* 左上角位置 */
	value = VIDOSDxA_TOPLEFT_X(left_x) | 
		VIDOSDxA_TOPLEFT_Y(top_y);
	writel(value, (io_base + VIDOSD0A + (0x10 * win_num)));

	/* 右下角位置 */
	value = VIDOSDxB_BOTRIGHT_X(left_x + width - 1) |
		VIDOSDxB_BOTRIGHT_Y(top_y + height - 1);
	writel(value, (io_base + VIDOSD0B + (0x10 * win_num)));

	var->xoffset = left_x;
	var->yoffset = top_y;

	return 0;
}


/*********************
 * s3cfb_set_win_size()
 * 设置win0/1/2的size寄存器，win3/4无size寄存器
 *********************/
static int 
s3cfb_set_win_size(s3cfb_info_t *fbi, int width, int height)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num = fbi->win_id;

	if (win_num == 0) {
		writel(VIDOSD0C_OSDSIZE(width * height), (io_base + VIDOSD0C));
	} 
	else if (win_num == 1) {
		writel(VIDOSD12D_OSDSIZE(width * height), (io_base + VIDOSD1D));
	}
	else if (win_num == 2) {
		writel(VIDOSD12D_OSDSIZE(width * height), (io_base + VIDOSD2D));
	}
	else {
		printk("Only win 0/1/2 support size reg\n");
	}

	var->xres = width;
	var->yres = height;

	return 0;
}


/**********************
 * s3cfb_set_fb_size()
 * 设置Window的framebuffer大小
 **********************/
int s3cfb_set_fb_size(s3cfb_info_t *fbi)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num = fbi->win_id;
	unsigned long page_width = 0;
	unsigned long value = 0;

	page_width = var->xres * fimd.def_bytes;

	/* 根据win_num，设置缓冲区的结束地址
	 * 如果是支持双缓冲区的win0/win1，则只设置buf0 */
	value = readl(io_base + (VIDW00ADD0B0 + (0x08*win_num)));
	value += page_width * var->yres;
	writel(VIDWxADD1_VBASEL(value), (io_base + VIDW00ADD1B0 + (0x08 * win_num)));

	/* 如果是win0或win1，则设置buf1
	 * 注意！原始驱动是if (win_num == 1) */
	if ((win_num == 0) || (win_num == 1)) {
		value = readl(io_base + (VIDW00ADD0B1 + (0x08*win_num)));
		value += page_width * var->yres;
		writel(VIDWxADD1_VBASEL(value), (io_base + VIDW00ADD1B1 + (0x08 * win_num)));
	}

	/* 删除原代码中虚拟屏大小信息 */

	return 0;
}


/**************************
 * 使能或关闭color key和透明度的混合操作
 * 支持window 1~4
 **************************/
static int 
s3cfb_onoff_color_key_alpha(s3cfb_info_t *fbi, int onoff)
{
	int win_num = fbi->win_id - 1;
	int value = 0;

	if ((win_num < 0) || (win_num > 3)) {
		printk("Only win1~4 upport colorkey & alpla\n");
		return -1;
	}

	if (onoff) {
		value = readl(io_base + W1KEYCON0 + (0x08 * win_num));
		value |= WxKEYCON0_KEYBL_EN;
		writel(value, (io_base + W1KEYCON0 + (0x08 * win_num)));
	} else {
		value = readl(io_base + W1KEYCON0 + (0x08 * win_num));
		value &= ~WxKEYCON0_KEYBL_MASK;
		value |= WxKEYCON0_KEYBL_DIS;
		writel(value , (io_base + W1KEYCON0 + (0x08 * win_num)));
	}

	return 0;
}


/************************
 * 使能或关闭color key
 * 支持win 1~4
 ************************/
static int 
s3cfb_onoff_color_key(s3cfb_info_t *fbi, int onoff)
{
	int win_num = fbi->win_id - 1;
	int value = 0;
	
	if (win_num < 0) {
		printk("Window 0 cannot support colorkey\n");
		return -1;
	}

	if (onoff) { 
		/* ON */
		value = readl(io_base + W1KEYCON0 + (0x08 * win_num));
		value |= WxKEYCON0_KEYEN_EN;
		writel(value, (io_base + W1KEYCON0 + (0x08 * win_num)));
	} 
	else { 
		/* OFF */
		value = readl(io_base + W1KEYCON0 + (0x08 * win_num));
		value &= ~WxKEYCON0_KEYEN_MASK;
		value |= WxKEYCON0_KEYEN_DIS;
		writel(value, (io_base + W1KEYCON0 + (0x08 * win_num)));
	}

	return 0;
}


/***********************
 * 设置color key的比较值
 * 支持window 1~4
 ***********************/
static int 
s3cfb_set_color_key_reg(s3cfb_info_t *fbi, s3cfb_color_key_t *ck)
{
	unsigned int compkey = 0, value = 0;
	int win_num = fbi->win_id;

	if (win_num == 0) {
		printk("WIN0 do not support color key\n");
		return -1;
	}

	/* 只有Win1~4支持color key */
	win_num--;

	/* RGB 5-6-5 mode */
	if (fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_16) {
		compkey = (((ck->compkey_red & 0x1f) << 19) | 0x70000);
		compkey |= (((ck->compkey_green & 0x3f) << 10) | 0x300);
		compkey |= (((ck->compkey_blue & 0x1f) << 3 ) | 0x7);
	} 
	/* RGB 8-8-8 mode  */
	else if (fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_24 || 
		 fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_28) {
		compkey = ((ck->compkey_red & 0xff) << 16);
		compkey |= ((ck->compkey_green & 0xff) << 8);
		compkey |= ((ck->compkey_blue & 0xff) << 0);
	} 
	else {
		printk("Invalid BPP has been given!\n");
	}

	/* 如果前景色和colorkey值相符，则显示背景色 */
	if (ck->direction == S3CFB_COLOR_KEY_DIR_BG) {
		value = WxKEYCON0_COMPKEY(compkey) | 
			WxKEYCON0_DIRCON_MATCH_FG;
		writel(value, (io_base + W1KEYCON0 + (0x08 * win_num)));
	}
	/* 如果背景色和colorkey值相符，则显示前景色 */
	else if (ck->direction == S3CFB_COLOR_KEY_DIR_FG) {
		value = WxKEYCON0_COMPKEY(compkey) |
			WxKEYCON0_DIRCON_MATCH_BG;
		writel(value, (io_base + W1KEYCON0 + (0x08 * win_num)));
	}
	else {
		printk("Color key direction is not correct: %d!\n", ck->direction);
	}

	return 0;
}


/*********************
 * 设置color key的值
 * 支持win1~4
 *********************/
static int 
s3cfb_set_color_key_value(s3cfb_info_t *fbi, s3cfb_color_val_t *ck)
{
	unsigned int colval = 0;
	int win_num = fbi->win_id;

	if (win_num == 0) {
		printk("WIN0 do not support color key value\n");
		return -1;
	}

	win_num--;

	/* RGB 5-6-5 mode */
	if (fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_16) {
		colval  = (((ck->colval_red & 0x1f) << 19) | 0x70000);
		colval |= (((ck->colval_green & 0x3f) << 10) | 0x300);
		colval |= (((ck->colval_blue & 0x1f)  << 3 )| 0x7);
	} 
	/* RGB 8-8-8 mode  */
	else if (fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_24 || 
		 fbi->fb.var.bits_per_pixel == S3CFB_PIXEL_BPP_28) {
		colval  = ((ck->colval_red & 0xff) << 16);
		colval |= ((ck->colval_green & 0xff) << 8);
		colval |= ((ck->colval_blue & 0xff) << 0);
	} else {
		printk("Invalid BPP has been given!\n");
	}

	writel(WxKEYCON1_COLVAL(colval), (io_base + W1KEYCON1 + (0x08 * win_num)));

	return 0;
}


/*********************
 * 设定窗口支持的BPP
 * 支持window 0~4
 *********************/
static int 
s3cfb_set_bpp(s3cfb_info_t *fbi, int bpp)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num = fbi->win_id;
	unsigned int val;

	val = readl(io_base + WINCON0 + (0x04 * win_num));
	val &= ~(WINCONx_BPPMODE_MASK | WINCONx_BLD_PIX_MASK);
	val |= WINCONx_ALPHA_SEL_1;

	switch (bpp) {
	case 1:
	case 2:
	case 4:
	case 8:
		var->bits_per_pixel = bpp;
		fbi->bytes_per_pixel = 1;
		break;
	case 16:
		val |= (WINCONx_BPPMODE_16BPP_565 | 
			WINCONx_BLD_PIX_PLANE);
		writel(val, (io_base + WINCON0 + (0x04 * win_num)));
		var->bits_per_pixel = bpp;
		fbi->bytes_per_pixel = 2;
		break;
	case 24:
		val |= (WINCONx_BPPMODE_24BPP_888 | 
			WINCONx_BLD_PIX_PLANE);
		writel(val, (io_base + WINCON0 + (0x04 * win_num)));
		var->bits_per_pixel = bpp;
		fbi->bytes_per_pixel = 4;
		break;
	case 28:
		val |= (WINCONx_BPPMODE_28BPP_A888 | 
			WINCONx_BLD_PIX_PIXEL);
		writel(val, (io_base + WINCON0 + (0x04 * win_num)));
		var->bits_per_pixel = bpp;
		fbi->bytes_per_pixel = 4;
		break;
	}

	return 0;
}


/*************************
 * 初始化一个窗口
 *************************/
static int 
s3cfb_init_win(s3cfb_info_t *fbi, int bpp, 
		int left_x, int top_y, 
		int width, int height, int onoff)
{
	s3cfb_onoff_win(fbi, OFF);
	s3cfb_set_bpp(fbi, bpp);
	s3cfb_set_win_position(fbi, left_x, top_y, width, height);
	s3cfb_set_win_size(fbi, width, height);
	s3cfb_set_fb_size(fbi);
	s3cfb_onoff_win(fbi, onoff);

	return 0;
}


/************************
 * fb_ops->fb_ioctl
 ************************/
static int 
s3cfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	s3cfb_info_t *fbi = container_of(info, s3cfb_info_t, fb);
	s3cfb_win_info_t win_info;
	s3cfb_color_key_t colkey;
	s3cfb_color_val_t colval;
	s3cfb_dma_info_t dma_info;
	struct fb_var_screeninfo *var= &info->var;
	unsigned int alpha_level, alpha_mode;

	switch (cmd) {
	/* 获得缓冲区的DMA地址(支持双缓冲) */
	case S3CFB_GET_DMAINFO:
		dma_info.map_dma_f1 = fbi->map_dma_f1;
		dma_info.map_dma_f2 = fbi->map_dma_f2;
		if(copy_to_user((void __user *)arg, (void *)&dma_info, sizeof(s3cfb_dma_info_t)))
			return -EFAULT;
		break;

	/* 设置窗口的信息，窗口默认关闭 */
	case S3CFB_OSD_SET_INFO:
		if (copy_from_user(&win_info, (void __user *)arg, sizeof(s3cfb_win_info_t)))
			return -EFAULT;
		s3cfb_init_win(fbi, win_info.bpp, 
			win_info.left_x, win_info.top_y, 
			win_info.width, win_info.height, OFF);
		break;

	/* 开启指定窗口 */
	case S3CFB_OSD_START:
		s3cfb_onoff_win(fbi, ON);
		break;

	/* 关闭指定窗口 */
	case S3CFB_OSD_STOP:
		s3cfb_onoff_win(fbi, OFF);
		break;

	/* 设置alpha0的透明度级别(范围0x0~0xf)
	 * 0x0为完全透明，0xf为完全不透明 */
	case S3CFB_OSD_ALPHA0_SET:
		alpha_level = (unsigned int)arg;
		if (alpha_level > S3CFB_MAX_ALPHA_LEVEL)
			alpha_level = S3CFB_MAX_ALPHA_LEVEL;

		s3cfb_set_alpha_level(fbi, alpha_level, 0);
		break;

	/* 设置alpha1的透明度级别(范围0x0~0xf) */
	case S3CFB_OSD_ALPHA1_SET:
		alpha_level = (unsigned int)arg;
		if (alpha_level > S3CFB_MAX_ALPHA_LEVEL)
			alpha_level = S3CFB_MAX_ALPHA_LEVEL;

		s3cfb_set_alpha_level(fbi, alpha_level, 1);
		break;

	/* 设置alpha模式 */
	case S3CFB_OSD_ALPHA_MODE:
		alpha_mode = (unsigned int)arg;
		s3cfb_set_alpha_mode(fbi, alpha_mode);
		break;

	/* 将图像向左移动一个像素 */
	case S3CFB_OSD_MOVE_LEFT:
		if (var->xoffset > 0)
			var->xoffset--;

		s3cfb_set_win_position(fbi, var->xoffset, 
			var->yoffset, var->xres, var->yres);
		break;

	/* 将图像向右移动一个像素(但不能超过屏幕宽度) */
	case S3CFB_OSD_MOVE_RIGHT:
		if (var->xoffset < (fimd.width - var->xres))
			var->xoffset++;

		s3cfb_set_win_position(fbi, var->xoffset, 
			var->yoffset, var->xres, var->yres);
		break;

	/* 将图像向上移动一个像素 */
	case S3CFB_OSD_MOVE_UP:
		if (var->yoffset > 0)
			var->yoffset--;

		s3cfb_set_win_position(fbi, var->xoffset, 
			var->yoffset, var->xres, var->yres);
		break;

	/* 将图像向下移动一个像素 */
	case S3CFB_OSD_MOVE_DOWN:
		if (var->yoffset < (fimd.height - var->yres))
			var->yoffset++;

		s3cfb_set_win_position(fbi, var->xoffset, 
			var->yoffset, var->xres, var->yres);
		break;

	/* 启动colorkey */
	case S3CFB_COLOR_KEY_START:
		s3cfb_onoff_color_key(fbi, ON);
		break;

	/* 关闭colorkey */
	case S3CFB_COLOR_KEY_STOP:
		s3cfb_onoff_color_key(fbi, OFF);
		break;

	/* 启动colorkey和透明度的混合 */
	case S3CFB_COLOR_KEY_ALPHA_START:
		s3cfb_onoff_color_key_alpha(fbi, ON);
		break;

	/* 关闭colorkey和透明度的混合 */
	case S3CFB_COLOR_KEY_ALPHA_STOP:
		s3cfb_onoff_color_key_alpha(fbi, OFF);
		break;

	/* 设置colorkey的比较值 */
	case S3CFB_COLOR_KEY_SET_INFO:
		if (copy_from_user(&colkey, (void __user *)arg, sizeof(s3cfb_color_key_t)))
			return -EFAULT;

		s3cfb_set_color_key_reg(fbi, &colkey);
		break;

	/* 设置colorkey的值 */
	case S3CFB_COLOR_KEY_VALUE:
		if (copy_from_user(&colval, (void __user *)arg, sizeof(s3cfb_color_val_t)))
			return -EFAULT;

		s3cfb_set_color_key_value(fbi, &colval);
		break;

	/* 在VSYNC阶段产生frame中断
	 * arg = 0: 关闭; arg != 0: 使能 */
	case S3CFB_SET_VSYNC_INT:
		fimd.vidintcon0 &= ~VIDINTCON0_FRAMESEL0_MASK;
		fimd.vidintcon0 |= VIDINTCON0_FRAMESEL0_VSYNC;

		if (arg) {
			fimd.vidintcon0 |= VIDINTCON0_INTFRMEN_ENABLE;
		} else {
			fimd.vidintcon0 &= ~VIDINTCON0_INTFRMEN_MASK;
			fimd.vidintcon0 |= VIDINTCON0_INTFRMEN_DISABLE;
		}

		writel(fimd.vidintcon0, (io_base + VIDINTCON0));
		break;

#if defined(CONFIG_S3CFB_DOUBLE_BUFFER)
	/* 切换win0/win1的buffer, arg为0或非0对应buf0/buf1 */
	case S3CFB_CHANGE_WIN0:
		s3cfb_change_buff(0, (int)arg);
		break;

	case S3CFB_CHANGE_WIN1:
		s3cfb_change_buff(1, (int)arg);
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}


/*************************
 * fb_ops->fb_check_var
 * 检测fb_var_screeninfo的内容是否是有效值
 *************************/
static int 
s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	s3cfb_info_t *fbi = container_of(info, s3cfb_info_t, fb);

	printk("check_var(var=%p, info=%p)\n", var, info);

	switch (var->bits_per_pixel) {
	case 8:
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 0;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		fbi->bytes_per_pixel = 1;
		break;
	case 16:	
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		fbi->bytes_per_pixel = 2;
		break;
	case 24:	
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		fbi->bytes_per_pixel = 4;
		break;
	case 28:	
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 4;
		fbi->bytes_per_pixel = 4;
		break;
	default:
		printk("Cannot support bpp %d\n", var->bits_per_pixel);
		return -1;
	}

	/* WIN0不支持透明度 */
	if (fbi->win_id == 0) {
		var->transp.length = 0;
	}
	
	return 0;
}


/****************************
 * fb_ops->fb_set_par
 * 更新fimd结构体并设置6410寄存器
 ****************************/
static int 
s3cfb_set_par(struct fb_info *info)
{
	s3cfb_info_t *fbi = container_of(info, s3cfb_info_t, fb);
	struct fb_var_screeninfo *var = &info->var;

        if (var->bits_per_pixel == 16 || 
	    var->bits_per_pixel == 24 || 
	    var->bits_per_pixel == 28)
		fbi->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	else
		fbi->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;

	fbi->fb.fix.line_length = var->width * fbi->bytes_per_pixel;

	/* 根据var中的内容更新fimd，并设置6410寄存器 */
	s3cfb_activate_var(fbi, var);

	return 0;
}


/**************************
 * fb_ops->fb_pan_display()
 * 根据yoffset的值刷新给定window的buf0
 **************************/
static int 
s3cfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	s3cfb_info_t *fbi = container_of(info, s3cfb_info_t, fb);

	printk("s3c_fb_pan_display: not support virtual screen\n");

	if (var->xoffset != 0)
		return -EINVAL;

	if (var->yoffset != 0)
		return -EINVAL;

	fbi->fb.var.xoffset = var->xoffset;
	fbi->fb.var.yoffset = var->yoffset;

	s3cfb_set_fb_addr(fbi);

	return 0;
}


/***********************
 * fb_ops->fb_blank
 * 控制屏幕内容的显示和消除
 ***********************/
static int 
s3cfb_blank(int blank_mode, struct fb_info *info)
{
	printk("blank(mode=%d)\n", blank_mode);

	switch (blank_mode) {
	case VESA_NO_BLANKING: /* 打开LCD */
		s3cfb_start_lcd();
		break;
	case VESA_VSYNC_SUSPEND: /* 关闭LCD */
	case VESA_HSYNC_SUSPEND:
	case VESA_POWERDOWN:
		s3cfb_stop_lcd();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


/*********************
 * 更新fbi->pal_buf中的内容
 * 未更新到6410寄存器中
 *********************/
static void 
s3cfb_update_palette(s3cfb_info_t *fbi, 
		unsigned int regno, unsigned int val)
{
	unsigned long flags;

	local_irq_save(flags);

	fbi->pal_buf[regno] = val;

	if (!fbi->palette_ready) {
		fbi->palette_ready = 1;
		fimd.palette_win = fbi->win_id;
	}

	local_irq_restore(flags);
}


/**********************
 * fb_ops->fb_setcolreg
 * 设置调色板中的给定颜色
 **********************/
static int 
s3cfb_setcolreg(unsigned int regno, unsigned int red, 
		unsigned int green, unsigned int blue, 
		unsigned int transp, struct fb_info *info)
{
	s3cfb_info_t *fbi = container_of(info, s3cfb_info_t, fb);
	unsigned int *pal = fbi->fb.pseudo_palette;
	unsigned int val = 0;

	val  = (red & 0xff) << fbi->fb.var.red.offset;
	val |= (green & 0xff) << fbi->fb.var.green.offset;
	val |= (blue & 0xff) << fbi->fb.var.blue.offset;
	val |= (transp & 0xff) << fbi->fb.var.transp.offset;

	/* 在真彩模式下，使用fb_info中的16色伪调色板
	 * 伪彩模式使用256色调色板 */
	switch (fbi->fb.fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		if (regno < 16)
			pal[regno] = val;
		break;
	case FB_VISUAL_PSEUDOCOLOR:
		if (regno < 256)
			s3cfb_update_palette(fbi, regno, val);
		break;
	default:
		return 1; /* unknown type */
	}

	printk("set palette (%d) to color (0x%08x)\n", regno, val);
	return 0;
}


static struct fb_ops s3cfb_ops = {
	.owner		= THIS_MODULE,
	.fb_ioctl	= s3cfb_ioctl,
	.fb_check_var	= s3cfb_check_var,
	.fb_set_par	= s3cfb_set_par,
	.fb_blank	= s3cfb_blank,
	.fb_pan_display	= s3cfb_pan_display,
	.fb_setcolreg	= s3cfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= soft_cursor,
};


/************************
 * FIMD结构体的默认初始化
 ************************/
static void 
s3cfb_init_fimd(fimd_info_t *dev)
{
	dev->vidcon0  	= VIDCON0_DEF;
	dev->dithmode 	= DITHMODE_DEF;

#if defined(CONFIG_S3CFB_BPP_8)
	dev->wincon0 	= WINCON0_BPP8_DEF;
	dev->wincon1 	= WINCON1_BPP8_DEF; 
	dev->wpalcon 	= WPALCON_W0PAL_565;
	dev->def_bpp 	= S3CFB_PIXEL_BPP_8;
	dev->def_bytes  = 1;

#elif defined(CONFIG_S3CFB_BPP_16)
	dev->wincon0 	= WINCON0_BPP16_DEF;
	dev->wincon1 	= WINCON1_BPP16_DEF;
	dev->wincon2 	= WINCON2_BPP16_DEF;
	dev->wincon3 	= WINCON3_BPP16_DEF;
	dev->wincon4 	= WINCON4_BPP16_DEF;
	dev->wpalcon 	= WPALCON_W0PAL_565;
	dev->def_bpp 	= S3CFB_PIXEL_BPP_16;
	dev->def_bytes  = 2;

#elif defined(CONFIG_S3CFB_BPP_24)
	dev->wincon0 	= WINCON0_BPP24_DEF;
	dev->wincon1 	= WINCON1_BPP24_DEF;
	dev->wincon2 	= WINCON2_BPP24_DEF;
	dev->wincon3 	= WINCON3_BPP24_DEF;
	dev->wincon4 	= WINCON4_BPP24_DEF;
	dev->wpalcon 	= WPALCON_W0PAL_888;
	dev->def_bpp 	= S3CFB_PIXEL_BPP_24;
	dev->def_bytes  = 4;

#elif defined (CONFIG_S3CFB_BPP_28)
	dev->wincon0 	= WINCON0_BPP28_DEF;
	dev->wincon1 	= WINCON1_BPP28_DEF;
	dev->wincon2 	= WINCON2_BPP28_DEF;
	dev->wincon3 	= WINCON3_BPP28_DEF;
	dev->wincon4 	= WINCON4_BPP28_DEF;
	dev->wpalcon 	= WPALCON_W0PAL_888;
	dev->def_bpp 	= S3CFB_PIXEL_BPP_28;
	dev->def_bytes  = 4;
#endif

	dev->vidosd1c = VIDOSD14C_DEF;
	dev->vidosd2c = VIDOSD14C_DEF;
	dev->vidosd3c = VIDOSD14C_DEF;
	dev->vidosd4c = VIDOSD14C_DEF;

	dev->vidintcon0 = VIDINTCON0_DEF;
	dev->vidintcon1 = 0;

	dev->xoffset = 0;
	dev->yoffset = 0;
	dev->sync = 0;

	dev->w1keycon0 = WxKEYCON0_DEF;
	dev->w1keycon1 = WxKEYCON1_DEF;
	dev->w2keycon0 = WxKEYCON0_DEF;
	dev->w2keycon1 = WxKEYCON1_DEF;
	dev->w3keycon0 = WxKEYCON0_DEF;
	dev->w3keycon1 = WxKEYCON1_DEF;
	dev->w4keycon0 = WxKEYCON0_DEF;
	dev->w4keycon1 = WxKEYCON1_DEF;

	if (CONFIG_S3CFB_NUM > 5)
		dev->win_num = 5;
	else
		dev->win_num = CONFIG_S3CFB_NUM;
};


/*************************
 * 初始化给定窗口的s3cfb_info_t
 *************************/
static void 
s3cfb_init_fbinfo(s3cfb_info_t *fbi, char *name, int win)
{
	int i = 0;

	if (win == 0) {
		s3cfb_init_hw(&fimd);
		s3cfb_set_gpio();
	}

	/* 初始化s3cfb_info_t，清空调色板缓冲区 */
	fbi->win_id = win;
	fbi->bytes_per_pixel = fimd.def_bytes;

	for (i = 0; i < 256; i++)
		fbi->pal_buf[i] = PAL_BUF_CLEAR;

	/* 初始化fb_info中的var参数 */
	fbi->fb.var.nonstd = 0;
	fbi->fb.var.activate = FB_ACTIVATE_NOW;
	fbi->fb.var.accel_flags = 0;
	fbi->fb.var.vmode = FB_VMODE_NONINTERLACED;

	fbi->fb.var.xoffset = fimd.xoffset;
	fbi->fb.var.yoffset = fimd.yoffset;

	if (win == 0) {
		fbi->fb.var.height = fimd.height;
		fbi->fb.var.width = fimd.width;
		fbi->fb.var.xres = fimd.xres;
		fbi->fb.var.yres = fimd.yres;
	} else {
		fbi->fb.var.height = fimd.osd_height;
		fbi->fb.var.width = fimd.osd_width;
		fbi->fb.var.xres = fimd.osd_xres;
		fbi->fb.var.yres = fimd.osd_yres;
	}

	fbi->fb.var.bits_per_pixel = fimd.def_bpp;
        fbi->fb.var.pixclock = fimd.pixclock;
	fbi->fb.var.hsync_len = fimd.hsync_len;
	fbi->fb.var.left_margin = fimd.left_margin;
	fbi->fb.var.right_margin = fimd.right_margin;
	fbi->fb.var.vsync_len = fimd.vsync_len;
	fbi->fb.var.upper_margin = fimd.upper_margin;
	fbi->fb.var.lower_margin = fimd.lower_margin;
	fbi->fb.var.sync = fimd.sync;
	fbi->fb.var.grayscale = fimd.cmap_grayscale;

	/* 初始化fb_info中的fix参数 */
	sprintf(fbi->fb.fix.id, "%s%d", name, win);
	fbi->fb.fix.type = FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux = 0;
	fbi->fb.fix.xpanstep = 0;
	fbi->fb.fix.ypanstep = 1;
	fbi->fb.fix.ywrapstep = 0;
	fbi->fb.fix.accel = FB_ACCEL_NONE;

	fbi->fb.fix.smem_len =  fbi->fb.var.xres * 
				fbi->fb.var.yres * 
				fbi->bytes_per_pixel;
	fbi->fb.fix.line_length = fbi->fb.var.width * 
				  fbi->bytes_per_pixel;

#if defined(CONFIG_S3CFB_DOUBLE_BUFFER)
	if (win < 2)
		fbi->fb.fix.smem_len *= 2;
#endif

	/* 初始化fb_info中的其他参数 */
	fbi->fb.fbops = &s3cfb_ops;
	fbi->fb.flags = FBINFO_FLAG_DEFAULT;
	fbi->fb.pseudo_palette = &fbi->pseudo_pal;

}


/**************************
 * 窗口的初始化函数
 * 遍历当前所有窗口的s3cfb_info_t(最多5个)
 * 初始化并注册对应的fb_info
 **************************/
static int
s3cfb_win_init(int win_num)
{
	s3cfb_info_t *fbi;
	int i, ret;

	for (i = 0; i < win_num; i++) {
		fbi = &s3cfb_info[i];
		memset(fbi, 0, sizeof(s3cfb_info_t));

		/* 初始化s3cfb_info_t及fb_info */
		s3cfb_init_fbinfo(fbi, "fb-win", i);
		s3cfb_check_var(&fbi->fb.var, &fbi->fb);

		/* 分配显示缓冲区 */
		ret = s3cfb_map_video_memory(fbi);
		if (ret) {
			printk("Failed to allocate video RAM: %d\n", ret);
			return -ENOMEM;
		}
		fbi->buffer_ok = 1;

		/* 写对应窗口的寄存器 */
		s3cfb_init_regs(fbi);

		/* 分配colormap
		 * win0/1为256色; win2/3/4为16色 */
		if (i < 2) {
			ret = fb_alloc_cmap(&fbi->fb.cmap, 256, 0);
			if (ret)
				return ret;
		} 
		else {
			ret = fb_alloc_cmap(&fbi->fb.cmap, 16, 0);
			if (ret)
				return ret;
		}
		fbi->cmap_ok = 1;

		/* 将fb_info注册到fb子系统 */
		ret = register_framebuffer(&fbi->fb);
		if (ret) {
			printk("Failed to register fb%d\n", i);
			return ret;
		}
		fbi->register_ok = 1;
		printk("register fb%d:%s\n",fbi->fb.node, fbi->fb.fix.id);
	}

	return 0;
}


/**************************
 * 窗口的释放函数
 * 依次注销fb_info并释放frame buffer
 **************************/
static void 
s3cfb_win_release(int win_num)
{
	int i;
	for (i = 0; i < win_num; i++) {
		if (s3cfb_info[i].register_ok)
			unregister_framebuffer(&s3cfb_info[i].fb);
		if (s3cfb_info[i].cmap_ok)
			fb_dealloc_cmap(&s3cfb_info[i].fb.cmap);
		if (s3cfb_info[i].buffer_ok)
			s3cfb_unmap_video_memory(&s3cfb_info[i]);
	}
}


/***************************
 * s3cfb_write_palette()
 * 设置给定窗口的调色板寄存器
 ***************************/
static void 
s3cfb_write_palette(s3cfb_info_t *fbi)
{
	unsigned int i;
	unsigned short ent;
	unsigned int win_num = fbi->win_id;

	fbi->palette_ready = 0;

	/* 允许由ARM访问调色板寄存器 */
	writel((fimd.wpalcon | WPALCON_PAL_ACCESS_ENABLE), 
		(io_base + WPALCON));

	/* 将fbi中的调色板颜色写入对应窗口的调色板
	 * 清空fbi->pal_buf中的颜色 */
	switch (win_num) {
	case 0: /* win 0 */
		for (i = 0; i < 256; i++) {
			writel(fbi->pal_buf[i], (io_base+TFTPAL0(i)));
			fbi->pal_buf[i] = PAL_BUF_CLEAR;
		}
		break;

	case 1: /* win 1 */
		for (i = 0; i < 256; i++) {
			writel(fbi->pal_buf[i], (io_base+TFTPAL1(i)));
			fbi->pal_buf[i] = PAL_BUF_CLEAR;
		}
		break;

	case 2: /* win 2 */
		for (i = 0; i < 16; i++) {
			ent = (short)fbi->pal_buf[i];
			writew(ent, (io_base+TFTPAL2(i)));
			fbi->pal_buf[i] = PAL_BUF_CLEAR;
		}
	case 3: /* win 3 */
		for (i = 0; i < 16; i++) {
			ent = (short)fbi->pal_buf[i];
			writel(ent, (io_base+TFTPAL3(i)));
			fbi->pal_buf[i] = PAL_BUF_CLEAR;
		}
		break;
	case 4: /* win 4 */
		for (i = 0; i < 4; i++) {
			ent = (short)fbi->pal_buf[i];
			writel(ent, (io_base+TFTPAL4(i)));
			fbi->pal_buf[i] = PAL_BUF_CLEAR;
		}
		break;
	default:
		printk("Only support win0~win4\n");
	}

	writel(fimd.wpalcon, (io_base + WPALCON));
}


/**********************
 * 中断处理函数
 * 将改变过的palette更新到调色板寄存器
 **********************/
static irqreturn_t 
s3cfb_irq(int irq, void *dev_id)
{
	s3cfb_info_t *info;

	/* 如果更新了某个窗口的palette，则将其写入寄存器 */
	info = &s3cfb_info[fimd.palette_win];
	if (info->palette_ready)
		s3cfb_write_palette(info);

	/* 清空中断状态位 */
	writel(readl(io_base + VIDINTCON1), (io_base + VIDINTCON1));

	return IRQ_HANDLED;
}


/***************************
 * platform_driver->probe
 ***************************/
static int __init 
s3cfb_probe(struct platform_device *pdev)
{
	struct resource *res1, *res2;
	int ret, size;

	fimd.dev = &pdev->dev;

	/* 1.获取FIMD的寄存器地址以及中断号 */
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res2 = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res1 || !res2) {
		dev_err(fimd.dev, "No enough resources\n");
		return -ENOENT;
	}

	/* 2.request_mem_region/ioremap */
	size = (res1->end - res1->start) + 1;
	fimd.mem = request_mem_region(res1->start, size, pdev->name);
	if (!fimd.mem) {
		dev_err(fimd.dev, "failed to request mem region\n");
		return -ENOENT;
	}

	io_base = ioremap(res1->start, size);
	if (!io_base) {
		dev_err(fimd.dev, "failed to ioremap()\n");
		ret = -ENXIO;
		goto err_map;
	}

	/* 3.获得并使能FIMD的时钟 */
	fimd.clk = clk_get(NULL, "lcd");
	if (!fimd.clk || IS_ERR(fimd.clk)) {
		dev_err(fimd.dev, "failed to get clock\n");
		ret = -ENOENT;
		goto err_clk;
	}
	clk_enable(fimd.clk);
	printk("LCD clock enabled:%ld Mhz\n", clk_get_rate(fimd.clk)/MHZ);

	/* 使能中断 */
	s3cfb_pre_init();

	/* 4.注册中断处理函数 */
	fimd.irq = res2->start; /* 62 */
	ret = request_irq(fimd.irq, s3cfb_irq, 
			IRQF_SHARED, "s3c-lcd", &fimd);
	if (ret) {
		dev_err(fimd.dev, "failed to request irq(%d)\n", fimd.irq);
		goto err_irq;
	}

	msleep(5);

	/* 5.初始化fimd结构体(未写入寄存器) */
	s3cfb_init_fimd(&fimd);

	/* 6.初始化每个窗口的s3cfb_info_t，并注册fb_info */
	ret = s3cfb_win_init(fimd.win_num);
	if (ret)
		goto err_win;

	return 0;
err_win:
	s3cfb_win_release(fimd.win_num);
	free_irq(fimd.irq, &fimd);
err_irq:
	clk_disable(fimd.clk);
	clk_put(fimd.clk);
err_clk:
	iounmap(io_base);
err_map:
	release_resource(fimd.mem);
	kfree(fimd.mem);

	return ret;
}


/***************************
 * platform_driver->remove
 ***************************/
static int 
s3cfb_remove(struct platform_device *pdev)
{
	s3cfb_stop_lcd();
	msleep(1);

	s3cfb_win_release(fimd.win_num);
	free_irq(fimd.irq, &fimd);

	clk_disable(fimd.clk);
	clk_put(fimd.clk);
	fimd.clk = NULL;

	iounmap(io_base);
	release_resource(fimd.mem);
	kfree(fimd.mem);

	return 0;
}


/*****************************
 * dev_pm_ops->suspend
 *****************************/
static int 
s3cfb_suspend(struct device *dev)
{
	s3cfb_stop_lcd();
	msleep(1);
	clk_disable(fimd.clk);

	return 0;
}


/**************************
 * dev_pm_ops->resume
 **************************/
static int 
s3cfb_resume(struct device *dev)
{
	clk_enable(fimd.clk);
	s3cfb_set_gpio();
	s3cfb_start_lcd();

	return 0;
}


static struct dev_pm_ops s3cfb_pm_ops = {
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
};

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
        .driver		= {
		.name	= "shrek-fb",
		.owner	= THIS_MODULE,
		.pm	= &s3cfb_pm_ops,
	},
};


static int __init s3cfb_init(void)
{
	return platform_driver_register(&s3cfb_driver);
}


static void __exit s3cfb_exit(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_init);
module_exit(s3cfb_exit);

MODULE_AUTHOR("ZHT");
MODULE_DESCRIPTION("S3C6410 Framebuffer Driver");
MODULE_LICENSE("GPL");

