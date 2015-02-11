/*****************************
 * S3C6410 FIMD的头文件
 * author: zht
 * date: 2013-01-23
 *****************************/

#ifndef __REGS_FB_H
#define __REGS_FB_H

/* Video controls */
#define VIDCON0		(0x00)
#define VIDCON1		(0x04)
#define VIDCON2		(0x08)

/* Video time controls */
#define VIDTCON0	(0x10)
#define VIDTCON1	(0x14)
#define VIDTCON2	(0x18)

/* Window 0~4 controls */
#define WINCON0		(0x20)
#define WINCON1		(0x24)
#define WINCON2		(0x28)
#define WINCON3		(0x2C)
#define WINCON4		(0x30)

/* Video window 0 */
#define VIDOSD0A	(0x40) 	/* position control A */
#define VIDOSD0B	(0x44) 	/* position control B */
#define VIDOSD0C	(0x48) 	/* size control */

/* Video window 1 */
#define VIDOSD1A	(0x50) 	/* position control A */
#define VIDOSD1B	(0x54)  /* position control B */
#define VIDOSD1C	(0x58)  /* alpha control */
#define VIDOSD1D	(0x5C) 	/* size control */

/* Video window 2 */
#define VIDOSD2A	(0x60) 	/* position control A */
#define VIDOSD2B	(0x64)  /* position control B */
#define VIDOSD2C	(0x68)  /* alpha control */
#define VIDOSD2D	(0x6C)  /* size control */

/* Video window 3 */
#define VIDOSD3A	(0x70) 	/* position control A */
#define VIDOSD3B	(0x74)  /* position control B */
#define VIDOSD3C	(0x78)  /* alpha control */

/* Video window 4 */
#define VIDOSD4A	(0x80)  /* position control A */
#define VIDOSD4B	(0x84)  /* position control B */
#define VIDOSD4C	(0x88)  /* alpha control */

/* Window 0~4 buffer start */
#define VIDW00ADD0B0	(0xA0)  /* Win0 buf0 start addr */
#define VIDW00ADD0B1	(0xA4)  /* Win0 buf1 start addr */
#define VIDW01ADD0B0	(0xA8)  /* Win1 buf0 start addr */
#define VIDW01ADD0B1	(0xAC)  /* Win1 buf1 start addr */
#define VIDW02ADD0	(0xB0)  /* Win2 buf  start addr */
#define VIDW03ADD0	(0xB8)  /* Win3 buf  start addr */
#define VIDW04ADD0	(0xC0)  /* Win4 buf  start addr */

/* Window 0~4 buffer end */
#define VIDW00ADD1B0	(0xD0)  /* Win0 buf0 end addr */
#define VIDW00ADD1B1	(0xD4)  /* Win0 buf1 end addr */
#define VIDW01ADD1B0	(0xD8)  /* Win1 buf0 end addr */
#define VIDW01ADD1B1	(0xDC)  /* Win1 buf1 end addr */
#define VIDW02ADD1	(0xE0)  /* Win2 buf  end addr */
#define VIDW03ADD1	(0xE8)  /* Win3 buf  end addr */
#define VIDW04ADD1	(0xF0)  /* Win4 buf  end addr */

/* Window 0~4 size */
#define VIDW00ADD2	(0x100) /* Win0 buf size */
#define VIDW01ADD2	(0x104) /* Win1 buf size */
#define VIDW02ADD2	(0x108) /* Win2 buf size */
#define VIDW03ADD2	(0x10C) /* Win3 buf size */
#define VIDW04ADD2	(0x110) /* Win4 buf size */

/* Video interrupt */
#define VIDINTCON0	(0x130)	/* Interrupt Control */
#define VIDINTCON1	(0x134) /* Interrupt Pending */

/* Win 1~4 color key */
#define W1KEYCON0	(0x140)	/* Win1 Color key control */
#define W1KEYCON1	(0x144)	/* Win1 Color key value */
#define W2KEYCON0	(0x148) /* Win2 Color key control */
#define W2KEYCON1	(0x14C) /* Win2 Color key value */
#define W3KEYCON0	(0x150)	/* Win3 Color key control */
#define W3KEYCON1	(0x154)	/* Win3 Color key value */
#define W4KEYCON0	(0x158)	/* Win4 Color key control */
#define W4KEYCON1	(0x15C)	/* Win4 Color key value	*/

/* Dithering mode */
#define DITHMODE	(0x170)

/* Win 0~4 color control */
#define WIN0MAP		(0x180)
#define WIN1MAP		(0x184)
#define WIN2MAP		(0x188)
#define WIN3MAP		(0x18C)
#define WIN4MAP		(0x190)

/* Window Palette control */
#define WPALCON		(0x1A0)

/* Window 2 palette data */
#define W2PDATA01	(0x300)	/* Index 0,1 */
#define W2PDATA23	(0x304)	/* Index 2,3 */
#define W2PDATA45	(0x308)	/* Index 4,5 */
#define W2PDATA67	(0x30C)	/* Index 6,7 */
#define W2PDATA89	(0x310)	/* Index 8,9 */
#define W2PDATAAB	(0x314)	/* Index A,B */
#define W2PDATACD	(0x318)	/* Index C,D */
#define W2PDATAEF	(0x31C)	/* Index E,F */

/* Window 3 palette data */
#define W3PDATA01	(0x320)	/* Index 0,1 */
#define W3PDATA23	(0x324)	/* Index 2,3 */
#define W3PDATA45	(0x328)	/* Index 4,5 */
#define W3PDATA67	(0x32C)	/* Index 6,7 */
#define W3PDATA89	(0x330)	/* Index 8,9 */
#define W3PDATAAB	(0x334)	/* Index A,B */
#define W3PDATACD	(0x338)	/* Index C,D */
#define W3PDATAEF	(0x33C)	/* Index E,F */

/* Window 4 palette data */
#define W4PDATA01	(0x340)	/* Index 0,1 */
#define W4PDATA23	(0x344)	/* Index 2,3 */

/* Window 0~4 palette */
#define TFTPAL2(x)	((0x300 + (x)*4))
#define TFTPAL3(x) 	((0x320 + (x)*4))
#define TFTPAL4(x)	((0x340 + (x)*4))
#define TFTPAL0(x)	((0x400 + (x)*4))
#define TFTPAL1(x)	((0x800 + (x)*4))


/* VIDCON0 */
#define VIDCON0_INTERLACE_MASK		(0x1 << 29)
#define VIDCON0_PROGRESSIVE		(0x0 << 29)
#define VIDCON0_INTERLACE		(0x1 << 29)

#define VIDCON0_VIDOUT_MASK		(0x3 << 26)
#define VIDCON0_VIDOUT_RGB		(0x0 << 26)
#define VIDCON0_VIDOUT_TV		(0x1 << 26)
#define VIDCON0_VIDOUT_I80_LDI0		(0x2 << 26)
#define VIDCON0_VIDOUT_I80_LDI1		(0x3 << 26)
#define VIDCON0_VIDOUT(x) 		(((x) & 0x3) << 26)

#define VIDCON0_L1_DATA_MASK		(0x7 << 23)
#define VIDCON0_L1_DATA_16BPP		(0x0 << 23)
#define VIDCON0_L1_DATA_18BPP16		(0x1 << 23)
#define VIDCON0_L1_DATA_18BPP9		(0x2 << 23)
#define VIDCON0_L1_DATA_24BPP		(0x3 << 23)
#define VIDCON0_L1_DATA_18BPP		(0x4 << 23)
#define VIDCON0_L1_DATA_16BPP8		(0x5 << 23)

#define VIDCON0_L0_DATA_MASK		(0x7 << 20)
#define VIDCON0_L0_DATA_16BPP		(0x0 << 20)
#define VIDCON0_L0_DATA_18BPP16		(0x1 << 20)
#define VIDCON0_L0_DATA_18BPP9		(0x2 << 20)
#define VIDCON0_L0_DATA_24BPP		(0x3 << 20)
#define VIDCON0_L0_DATA_18BPP		(0x4 << 20)
#define VIDCON0_L0_DATA_16BPP8		(0x5 << 20)

#define VIDCON0_PNRMODE_MASK		(0x3 << 17)
#define VIDCON0_PNRMODE_RGB		(0x0 << 17)
#define VIDCON0_PNRMODE_BGR		(0x1 << 17)
#define VIDCON0_PNRMODE_SERIAL_RGB	(0x2 << 17)
#define VIDCON0_PNRMODE_SERIAL_BGR	(0x3 << 17)

#define VIDCON0_CLKVALUP_MASK		(0x1 << 16)
#define VIDCON0_CLKVALUP_ALWAYS		(0x0 << 16)
#define VIDCON0_CLKVALUP_STF		(0x1 << 16)

#define VIDCON0_CLKVAL_MASK		(0xff << 6)
#define VIDCON0_CLKVAL(x)		(((x) & 0xff) << 6)

#define VIDCON0_VCLKFREE_MASK		(0x1 << 5)
#define VIDCON0_VCLKFREE_NORMAL		(0x0 << 5)
#define VIDCON0_VCLKFREE_FREERUN	(0x1 << 5)

#define VIDCON0_CLKDIR_MASK		(0x1 << 4)
#define VIDCON0_CLKDIR_DIRECT		(0x0 << 4)
#define VIDCON0_CLKDIR_DIVIDED		(0x1 << 4)

#define VIDCON0_CLKSEL_MASK		(0x3 << 2)
#define VIDCON0_CLKSEL_HCLK		(0x0 << 2)
#define VIDCON0_CLKSEL_LCD		(0x1 << 2)
#define VIDCON0_CLKSEL_27M		(0x3 << 2)

#define VIDCON0_ENVID_MASK		(0x1 << 1)
#define VIDCON0_ENVID_DISABLE		(0x0 << 1)
#define VIDCON0_ENVID_ENABLE		(0x1 << 1)

#define VIDCON0_ENVID_F_MASK		(0x1 << 0)
#define VIDCON0_ENVID_F_DISABLE		(0x0 << 0)
#define VIDCON0_ENVID_F_ENABLE		(0x1 << 0)


/* VIDCON1 */
#define VIDCON1_LINECNT_MASK		(0x7ff << 16)
#define VIDCON1_LINECNT_GET(v)		(((v) >> 16) & 0x7ff)

#define VIDCON1_FSTATUS_MASK		(0x1 << 15)
#define VIDCON1_FSTATUS_ODD		(0x0 << 15)
#define VIDCON1_FSTATUS_EVEN		(0x1 << 15)

#define VIDCON1_VSTATUS_MASK		(0x3 << 13)
#define VIDCON1_VSTATUS_VSYNC		(0x0 << 13)
#define VIDCON1_VSTATUS_BACKPORCH	(0x1 << 13)
#define VIDCON1_VSTATUS_ACTIVE		(0x2 << 13)
#define VIDCON1_VSTATUS_FRONTPORCH	(0x3 << 13)

#define VIDCON1_VCLK_MASK		(0x1 << 7)
#define VIDCON1_VCLK_FALLING		(0x0 << 7)
#define VIDCON1_VCLK_RISING		(0x1 << 7)

#define VIDCON1_HSYNC_MASK		(0x1 << 6)
#define VIDCON1_HSYNC_NORMAL		(0x0 << 6)
#define VIDCON1_HSYNC_INVERT		(0x1 << 6)

#define VIDCON1_VSYNC_MASK		(0x1 << 5)
#define VIDCON1_VSYNC_NORMAL		(0x0 << 5)
#define VIDCON1_VSYNC_INVERT		(0x1 << 5)

#define VIDCON1_VDEN_MASK		(0x1 << 4)
#define VIDCON1_VDEN_NORMAL		(0x0 << 4)
#define VIDCON1_VDEN_INVERT		(0x1 << 4)


/* VIDCON2 */
#define VIDCON2_EN601_MASK		(0x1 << 23)
#define VIDCON2_EN601_DISABLE		(0x0 << 23)
#define VIDCON2_EN601_ENABLE		(0x1 << 23)

#define VIDCON2_TVFMTSEL0_MASK		(0x1 << 14)
#define VIDCON2_TVFMTSEL0_HARDWARE	(0x0 << 14)
#define VIDCON2_TVFMTSEL0_SOFTWARE	(0x1 << 14)

#define VIDCON2_TVFMTSEL1_MASK		(0x3 << 12)
#define VIDCON2_TVFMTSEL1_RGB		(0x0 << 12)
#define VIDCON2_TVFMTSEL1_YUV422	(0x1 << 12)
#define VIDCON2_TVFMTSEL1_YUV444	(0x2 << 12)

#define VIDCON2_ORGYUV_MASK		(0x1 << 8) /* YUV order */
#define VIDCON2_ORGYUV_YCBCR		(0x0 << 8)
#define VIDCON2_ORGYUV_CBCRY		(0x1 << 8)

#define VIDCON2_YUVORD_MASK		(0x1 << 7) /* Chroma order */
#define VIDCON2_YUVORD_CBCR		(0x0 << 7)
#define VIDCON2_YUVORD_CRCB		(0x1 << 7)


/* VIDEO Time Control 0 */
#define VIDTCON0_VBPDE_MASK		(0xff << 24) /* for YUV */
#define VIDTCON0_VBPDE(x)		(((x) & 0xff) << 24)

#define VIDTCON0_VBPD_MASK(x)		(0xff << 16)
#define VIDTCON0_VBPD(x)		(((x) & 0xff) << 16)

#define VIDTCON0_VFPD_MASK(x)		(0xff << 8)
#define VIDTCON0_VFPD(x) 		(((x) & 0xff) << 8)

#define VIDTCON0_VSPW_MASK(x)		(0xff << 0)
#define VIDTCON0_VSPW(x) 		(((x) & 0xff) << 0)


/* VIDEO Time Control 1 */
#define VIDTCON1_VFPDE_MASK		(0xff << 24) /* for YUV */
#define VIDTCON1_VFPDE(x)		(((x) & 0xff) << 24)

#define VIDTCON1_HBPD_MASK(x)		(0xff << 16)
#define VIDTCON1_HBPD(x)		(((x) & 0xff) << 16)

#define VIDTCON1_HFPD_MASK(x)		(0xff << 8)
#define VIDTCON1_HFPD(x) 		(((x) & 0xff) << 8)

#define VIDTCON1_HSPW_MASK(x)		(0xff << 0)
#define VIDTCON1_HSPW(x) 		(((x) & 0xff) << 0)


/* VIDEO Time Control 2 */
#define VIDTCON2_LINEVAL_MASK		(0x7ff << 11)
#define VIDTCON2_LINEVAL(x) 		(((x) & 0x7FF) << 11)

#define VIDTCON2_HOZVAL_MASK		(0x7ff << 0)
#define VIDTCON2_HOZVAL(x)  		(((x) & 0x7FF) << 0)


/* Window 0~4 Control */
#define WINCONx_RANGE_MASK 		(0x3 << 26)
#define WINCONx_RANGE_WIDE		(0x0 << 26)
#define WINCONx_RANGE_NARROW		(0x3 << 26)

/* win0 local: PostProcessor
 * win1 local: TV scaler or Camera preview
 * win2 local: TV scaler or Camera codec */
#define WINCONx_ENLOCAL_MASK		(0x1 << 22)
#define WINCONx_ENLOCAL_DMA		(0x0 << 22)
#define WINCONx_ENLOCAL_LOCAL		(0x1 << 22)

#define WINCONx_BUFSEL_MASK		(0x1 << 20)
#define WINCONx_BUFSEL_0		(0x0 << 20)
#define WINCONx_BUFSEL_1		(0x1 << 20)

/* double buffer auto control */
#define WINCONx_BUFAUTOEN_MASK		(0x1 << 19)
#define WINCONx_BUFAUTOEN_DISABLE	(0x0 << 19)
#define WINCONx_BUFAUTOEN_ENABLE	(0x1 << 19)

#define WINCONx_BITSWP_MASK		(0x1 << 18)
#define WINCONx_BITSWP_DISABLE		(0x0 << 18)
#define WINCONx_BITSWP_ENABLE		(0x1 << 18)

#define WINCONx_BYTSWP_MASK		(0x1 << 17)
#define WINCONx_BYTSWP_DISABLE		(0x0 << 17)
#define WINCONx_BYTSWP_ENABLE		(0x1 << 17)

#define WINCONx_HAWSWP_MASK		(0x1 << 16)
#define WINCONx_HAWSWP_DISABLE		(0x0 << 16)
#define WINCONx_HAWSWP_ENABLE		(0x1 << 16)

#define WINCONx_WSWP_MASK		(0x1 << 15)
#define WINCONx_WSWP_DISABLE		(0x0 << 15)
#define WINCONx_WSWP_ENABLE		(0x1 << 15)

#define WINCONx_INRGB_MASK		(0x1 << 13)
#define WINCONx_INRGB_RGB		(0x0 << 13)
#define WINCONx_INRGB_YUV		(0x1 << 13)

#define WINCONx_BURSTLEN_MASK		(0x3 << 9)
#define WINCONx_BURSTLEN_16WORD		(0x0 << 9)
#define WINCONx_BURSTLEN_8WORD		(0x1 << 9)
#define WINCONx_BURSTLEN_4WORD		(0x2 << 9)

#define WINCONx_BLD_PIX_MASK		(0x1 << 6)
#define WINCONx_BLD_PIX_PLANE		(0x0 << 6)
#define WINCONx_BLD_PIX_PIXEL		(0x1 << 6)

#define WINCONx_BPPMODE_MASK		(0xf << 2)
#define WINCONx_BPPMODE_1BPP		(0x0 << 2)
#define WINCONx_BPPMODE_2BPP		(0x1 << 2)
#define WINCONx_BPPMODE_4BPP		(0x2 << 2)
#define WINCONx_BPPMODE_8BPP_PAL	(0x3 << 2)
#define WINCONx_BPPMODE_8BPP_A232	(0x4 << 2)
#define WINCONx_BPPMODE_16BPP_565	(0x5 << 2)
#define WINCONx_BPPMODE_16BPP_A555	(0x6 << 2)
#define WINCONx_BPPMODE_16BPP_I555	(0x7 << 2)
#define WINCONx_BPPMODE_18BPP_666	(0x8 << 2)
#define WINCONx_BPPMODE_18BPP_A665	(0x9 << 2)
#define WINCONx_BPPMODE_19BPP_A666	(0xa << 2)
#define WINCONx_BPPMODE_24BPP_888	(0xb << 2)
#define WINCONx_BPPMODE_24BPP_A887	(0xc << 2)
#define WINCONx_BPPMODE_25BPP_A888	(0xd << 2)
#define WINCONx_BPPMODE_28BPP_A888	(0xd << 2)

#define WINCONx_ALPHA_SEL_MASK		(0x1 << 1)
#define WINCONx_ALPHA_SEL_0		(0x0 << 1)
#define WINCONx_ALPHA_SEL_1		(0x1 << 1)

#define WINCONx_ENWIN_MASK 		(0x1 << 0)
#define WINCONx_ENWIN_DISABLE 		(0x0 << 0)
#define WINCONx_ENWIN_ENABLE		(0x1 << 0)


/* Window 1 specific */
#define WINCON1_LOCALSEL_MASK		(0x1 << 23)
#define WINCON1_LOCALSEL_TV		(0x0 << 23)
#define WINCON1_LOCALSEL_CAMPREV	(0x1 << 23)


/* Window 2 specific */
#define WINCON2_LOCALSEL_MASK		(0x1 << 23)
#define WINCON2_LOCALSEL_TV		(0x0 << 23)
#define WINCON2_LOCALSEL_CAMCODEC	(0x1 << 23)


/* Window 0~4 Position Control A reg */
#define VIDOSDxA_TOPLEFT_X_MASK		(0x7ff << 11)
#define VIDOSDxA_TOPLEFT_X(x)		(((x) & 0x7ff) << 11)

#define VIDOSDxA_TOPLEFT_Y_MASK		(0x7ff << 0)
#define VIDOSDxA_TOPLEFT_Y(x)		(((x) & 0x7ff) << 0)


/* Window 0~4 Position Control B reg */
#define VIDOSDxB_BOTRIGHT_X_MASK	(0x7ff << 11)
#define VIDOSDxB_BOTRIGHT_X(x)		(((x) & 0x7ff) << 11)

#define VIDOSDxB_BOTRIGHT_Y_MASK	(0x7ff << 0)
#define VIDOSDxB_BOTRIGHT_Y(x)		(((x) & 0x7ff) << 0)


/* Window 0 Size Control C reg */
#define VIDOSD0C_OSDSIZE_MASK		(0xffffff << 0)
#define VIDOSD0C_OSDSIZE(x)		(((x) & 0xffffff) << 0)


/* Window 1~4 Position Control C reg */
#define VIDOSD14C_ALPHA0_R_MASK		(0xf << 20)
#define VIDOSD14C_ALPHA0_R(x)		(((x) & 0xf) << 20)

#define VIDOSD14C_ALPHA0_G_MASK		(0xf << 16)
#define VIDOSD14C_ALPHA0_G(x)		(((x) & 0xf) << 16)

#define VIDOSD14C_ALPHA0_B_MASK		(0xf << 12)
#define VIDOSD14C_ALPHA0_B(x)		(((x) & 0xf) << 12)

#define VIDOSD14C_ALPHA1_R_MASK		(0xf << 8)
#define VIDOSD14C_ALPHA1_R(x)		(((x) & 0xf) << 8)

#define VIDOSD14C_ALPHA1_G_MASK		(0xf << 4)
#define VIDOSD14C_ALPHA1_G(x)		(((x) & 0xf) << 4)

#define VIDOSD14C_ALPHA1_B_MASK		(0xf << 0)
#define VIDOSD14C_ALPHA1_B(x)		(((x) & 0xf) << 0)


/* Window 1~2 Position Control D reg */
#define VIDOSD12D_OSDSIZE_MASK		(0xffffff << 0)
#define VIDOSD12D_OSDSIZE(x)		(((x) & 0xffffff) << 0)


/* Window 0~4 buffer start address */
#define VIDWxADD0_VBANK_MASK 		(0xff << 24)
#define VIDWxADD0_VBANK(x) 		(((x) & 0xff) << 24)

#define VIDWxADD0_VBASEU_MASK		(0xffffff << 0)
#define VIDWxADD0_VBASEU(x)		(((x) & 0xffffff) << 0)

/* Window 0~4 buffer end address */
#define VIDWxADD1_VBASEL_MASK		(0xffffff << 0)
#define VIDWxADD1_VBASEL(x)		(((x) & 0xffffff) << 0)


/* Win 0~4 buffer Size */
#define VIDWxADD2_OFFSIZE_MASK		(0x1fff << 13)
#define VIDWxADD2_OFFSIZE(x)		(((x) & 0x1fff) << 13)

#define VIDWxADD2_PAGEWIDTH_MASK	(0x1fff << 0)
#define VIDWxADD2_PAGEWIDTH(x)		(((x) & 0x1fff) << 0)


/* Video Interrupt Control */
#define VIDINTCON0_FIFOINTERVAL_MASK	(0x3f << 20)
#define VIDINTCON0_FIFOINTERVAL(x)	(((x) & 0x3f) << 20)

#define VIDINTCON0_MAINLCD_MASK		(0x1 << 19)
#define VIDINTCON0_MAINLCD_DISABLE	(0x0 << 19)
#define VIDINTCON0_MAINLCD_ENABLE	(0x1 << 19)

#define VIDINTCON0_SUBLCD_MASK		(0x1 << 18)
#define VIDINTCON0_SUBLCD_DISABLE	(0x0 << 18)
#define VIDINTCON0_SUBLCD_ENABLE	(0x1 << 18)

#define VIDINTCON0_I80IF_MASK		(0x1 << 17)
#define VIDINTCON0_I80IF_DISABLE	(0x0 << 17)
#define VIDINTCON0_I80IF_ENABLE		(0x1 << 17)

#define VIDINTCON0_FRAMESEL0_MASK 	(0x3 << 15)
#define VIDINTCON0_FRAMESEL0_BACK	(0x0 << 15)
#define VIDINTCON0_FRAMESEL0_VSYNC 	(0x1 << 15)
#define VIDINTCON0_FRAMESEL0_ACTIVE	(0x2 << 15)
#define VIDINTCON0_FRAMESEL0_FRONT 	(0x3 << 15)

#define VIDINTCON0_FRAMESEL1_MASK 	(0x3 << 13)
#define VIDINTCON0_FRAMESEL1_NONE	(0x0 << 13)
#define VIDINTCON0_FRAMESEL1_BACK	(0x1 << 13)
#define VIDINTCON0_FRAMESEL1_VSYNC 	(0x2 << 13)
#define VIDINTCON0_FRAMESEL1_FRONT 	(0x3 << 13)

#define VIDINTCON0_INTFRMEN_MASK	(0x1 << 12)
#define VIDINTCON0_INTFRMEN_DISABLE	(0x0 << 12)
#define VIDINTCON0_INTFRMEN_ENABLE 	(0x1 << 12)

#define VIDINTCON0_FIFOSEL_MASK		(0x7f << 5)
#define VIDINTCON0_FIFOSEL_WIN4_DIS	(0x0 << 11)
#define VIDINTCON0_FIFOSEL_WIN4_EN	(0x1 << 11)
#define VIDINTCON0_FIFOSEL_WIN3_DIS	(0x0 << 10)
#define VIDINTCON0_FIFOSEL_WIN3_EN	(0x1 << 10)
#define VIDINTCON0_FIFOSEL_WIN2_DIS	(0x0 << 9)
#define VIDINTCON0_FIFOSEL_WIN2_EN	(0x1 << 9)
#define VIDINTCON0_FIFOSEL_WIN1_DIS	(0x0 << 6)
#define VIDINTCON0_FIFOSEL_WIN1_EN	(0x1 << 6)
#define VIDINTCON0_FIFOSEL_WIN0_DIS	(0x0 << 5)
#define VIDINTCON0_FIFOSEL_WIN0_EN	(0x1 << 5)

#define VIDINTCON0_FIFOLEVEL_MASK	(0x7 << 2)
#define VIDINTCON0_FIFOLEVEL_25		(0x0 << 2)
#define VIDINTCON0_FIFOLEVEL_50		(0x1 << 2)
#define VIDINTCON0_FIFOLEVEL_75		(0x2 << 2)
#define VIDINTCON0_FIFOLEVEL_EMPTY 	(0x3 << 2)
#define VIDINTCON0_FIFOLEVEL_FULL	(0x4 << 2)

#define VIDINTCON0_FIFOINT_MASK		(0x1 << 1)
#define VIDINTCON0_FIFOINT_DISABLE	(0x0 << 1)
#define VIDINTCON0_FIFOINT_ENABLE	(0x1 << 1)

#define VIDINTCON0_INT_MASK		(0x1 << 0)
#define VIDINTCON0_INT_DISABLE		(0x0 << 0)
#define VIDINTCON0_INT_ENABLE		(0x1 << 0)


/* VIDEO Interrupt pending */
#define VIDINTCON1_INTI80PEND		(0x1 << 2)
#define VIDINTCON1_INTFRMPEND		(0x1 << 1)
#define VIDINTCON1_INTFIFOPEND		(0x1 << 0)


/* Window 1~4 colour-key control */
#define WxKEYCON0_KEYBL_MASK		(0x1 << 26)
#define WxKEYCON0_KEYBL_DIS		(0x0 << 26)
#define WxKEYCON0_KEYBL_EN		(0x1 << 26)

#define WxKEYCON0_KEYEN_MASK		(0x1 << 25)
#define WxKEYCON0_KEYEN_DIS		(0x0 << 25)
#define WxKEYCON0_KEYEN_EN		(0x1 << 25)

#define WxKEYCON0_DIRCON_MASK		(0x1 << 24)
#define WxKEYCON0_DIRCON_MATCH_FG	(0x0 << 24)
#define WxKEYCON0_DIRCON_MATCH_BG	(0x1 << 24)

#define WxKEYCON0_COMPKEY_MASK		(0xffffff << 0)
#define WxKEYCON0_COMPKEY(x)		(((x) & 0xffffff) << 0)

#define WxKEYCON1_COLVAL_MASK		(0xffffff << 0)
#define WxKEYCON1_COLVAL(x)		(((x) & 0xffffff) << 0)


/* Dithering mode */
#define DITHMODE_RED_MASK		(0x3 << 5)
#define DITHMODE_RED_8BIT		(0x0 << 5)
#define DITHMODE_RED_6BIT		(0x1 << 5)
#define DITHMODE_RED_5BIT		(0x2 << 5)

#define DITHMODE_GREEN_MASK		(0x3 << 3)
#define DITHMODE_GREEN_8BIT		(0x0 << 3)
#define DITHMODE_GREEN_6BIT		(0x1 << 3)
#define DITHMODE_GREEN_5BIT		(0x2 << 3)

#define DITHMODE_BLUE_MASK		(0x3 << 1)
#define DITHMODE_BLUE_8BIT		(0x0 << 1)
#define DITHMODE_BLUE_6BIT		(0x1 << 1)
#define DITHMODE_BLUE_5BIT		(0x2 << 1)

#define DITHMODE_DITHER_MASK		(0x1 << 0)
#define DITHMODE_DITHER_DISABLE		(0x0 << 0)
#define DITHMODE_DITHER_ENABLE		(0x1 << 0)


/* Win0~4 blanking (Color map) */
#define WINxMAP_MAP_MASK		(0x1 << 24)
#define WINxMAP_MAP_DISABLE		(0x0 << 24)
#define WINxMAP_MAP_ENABLE		(0x1 << 24)

#define WINxMAP_MAP_COLOUR_MASK		(0xffffff << 0)
#define WINxMAP_MAP_COLOUR(x)		(((x) & 0xffffff) << 0)


/* Window Palette Control */
#define WPALCON_PAL_ACCESS_MASK		(0x1 << 9)
#define WPALCON_PAL_ACCESS_NORMAL	(0x0 << 9) /* LCD access */
#define WPALCON_PAL_ACCESS_ENABLE	(0x1 << 9) /* ARM access */

#define WPALCON_W4PAL_MASK		(0x1 << 8)
#define WPALCON_W4PAL_565	 	(0x0 << 8)
#define WPALCON_W4PAL_A555		(0x1 << 8)

#define WPALCON_W3PAL_MASK		(0x1 << 7)
#define WPALCON_W3PAL_565	 	(0x0 << 7)
#define WPALCON_W3PAL_A555		(0x1 << 7)

#define WPALCON_W2PAL_MASK		(0x1 << 6)
#define WPALCON_W2PAL_565	 	(0x0 << 6)
#define WPALCON_W2PAL_A555		(0x1 << 6)

#define WPALCON_W1PAL_MASK		(0x7 << 3)
#define WPALCON_W1PAL_A888		(0x0 << 3)
#define WPALCON_W1PAL_888		(0x1 << 3)
#define WPALCON_W1PAL_A666		(0x2 << 3)
#define WPALCON_W1PAL_A665		(0x3 << 3)
#define WPALCON_W1PAL_666		(0x4 << 3)
#define WPALCON_W1PAL_A555		(0x5 << 3)
#define WPALCON_W1PAL_565	 	(0x6 << 3)

#define WPALCON_W0PAL_MASK		(0x7 << 0)
#define WPALCON_W0PAL_A888		(0x0 << 0)
#define WPALCON_W0PAL_888		(0x1 << 0)
#define WPALCON_W0PAL_A666		(0x2 << 0)
#define WPALCON_W0PAL_A665		(0x3 << 0)
#define WPALCON_W0PAL_666		(0x4 << 0)
#define WPALCON_W0PAL_A555		(0x5 << 0)
#define WPALCON_W0PAL_565	 	(0x6 << 0)


/**********************
 * HOST IF registers 
 **********************/

/* Host I/F A */
#define S3C_HOSTIFAREG(x)	((x) + S3C64XX_VA_HOSTIFA)
#define S3C_HOSTIFAREG_PHYS(x)	((x) + S3C64XX_PA_HOSTIFA)

/* Host I/F B - Modem I/F */
#define S3C_HOSTIFBREG(x)	((x) + S3C64XX_VA_HOSTIFB)
#define S3C_HOSTIFBREG_PHYS(x)	((x) + S3C64XX_PA_HOSTIFB)

#define S3C_INT2AP		S3C_HOSTIFBREG(0x8000)
#define S3C_INT2MSM		S3C_HOSTIFBREG(0x8004)
#define S3C_MIFCON		S3C_HOSTIFBREG(0x8008)
#define S3C_MIFPCON		S3C_HOSTIFBREG(0x800C)
#define S3C_MSMINTCLR		S3C_HOSTIFBREG(0x8010)

#define MIFCON_INT2MSM_MASK	(0x1 << 3)
#define MIFCON_INT2MSM_DIS	(0x0 << 3)
#define MIFCON_INT2MSM_EN	(0x1 << 3)

#define MIFCON_INT2AP_MASK	(0x1 << 2)
#define MIFCON_INT2AP_DIS	(0x0 << 2)
#define MIFCON_INT2AP_EN	(0x1 << 2)

#define MIFCON_WAKEUP_MASK	(0x1 << 1)
#define MIFCON_WAKEUP_DIS	(0x0 << 1)
#define MIFCON_WAKEUP_EN	(0x1 << 1)

#define MIFPCON_SEL_VSYNC_DIR_MASK	(0x1 << 5)
#define MIFPCON_SEL_VSYNC_DIR_OUT	(0x0 << 5)
#define MIFPCON_SEL_VSYNC_DIR_IN	(0x1 << 5)

#define MIFPCON_INT2M_LEVEL_MASK	(0x1 << 4)
#define MIFPCON_INT2M_LEVEL_DIS		(0x0 << 4)
#define MIFPCON_INT2M_LEVEL_EN		(0x1 << 4)

#define MIFPCON_SEL_MASK		(0x1 << 3)
#define MIFPCON_SEL_NORMAL		(0x0 << 3)
#define MIFPCON_SEL_BYPASS		(0x1 << 3)

#define MIFPCON_SEL_RS0			0
#define MIFPCON_SEL_RS1			1
#define MIFPCON_SEL_RS2			2
#define MIFPCON_SEL_RS3			3
#define MIFPCON_SEL_RS4			4
#define MIFPCON_SEL_RS5			5
#define MIFPCON_SEL_RS6			6

#endif /*  __REGS_FB_H */

