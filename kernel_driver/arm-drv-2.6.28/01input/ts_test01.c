/*****************************
 * S3C6410触摸屏的input驱动
 * 支持2.6.28内核，在3.4.4中也可以用
 * 但是3.4.4有更好的AD的处理方式
 * author: zht
 * date: 2013-01-22
 *****************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h> /* udelay */
#include <linux/slab.h> /* kmalloc */
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/timer.h> /* timer_list */
#include <linux/interrupt.h> /* request_irq */
#include <linux/platform_device.h>

#include <linux/clk.h> /* clk_enable/clk_disable */
#include <linux/input.h> /* input_dev */

#include <plat/ts.h> /* s3c_ts_mach_info */
#include <plat/regs-adc.h>

#define S3C_TSVERSION	0x0201
#define S3C_DRVNAME	"S3C6410 input driver(2.6.28)"

/* 令触摸屏进入等待笔按下|抬起中断状态 */
#define WAIT4INT(x)  (((x)<<8) | \
		S3C_ADCTSC_YM_SEN | \
		S3C_ADCTSC_YP_SEN | \
		S3C_ADCTSC_XP_SEN | \
		S3C_ADCTSC_XY_PST(3))

#define AUTOPST	(S3C_ADCTSC_YM_SEN | \
		S3C_ADCTSC_YP_SEN | \
		S3C_ADCTSC_XP_SEN | \
		S3C_ADCTSC_AUTO_PST | \
		S3C_ADCTSC_XY_PST(0))

/* 触摸屏的私有结构体 */
struct s3cts_priv {
	int phy_base;
	int phy_size;
	int irq_ts;
	int irq_adc;

	/* request的资源 */
	struct resource *ts_req;
	struct clk	*ts_clock;

	struct timer_list ts_timer;
	struct input_dev *idev;
	struct device 	*dev;
	int		xp;
	int 		yp;
	int 		count;
	int 		shift;
	char 		phys[32];
	char 		name[32];
};

/* ADC控制器的虚拟基地址 */
static	void __iomem	*ts_base;


/**************************
 * 定时器的处理函数
 * 函数负责向input子系统提交input_event
 **************************/
static void 
ts_timer_func(unsigned long data)
{
	struct s3cts_priv *priv = (void *)data;
	unsigned long data0;
	unsigned long data1;
	int updown;

	data0 = readl(ts_base+S3C_ADCDAT0);
	data1 = readl(ts_base+S3C_ADCDAT1);

	/* updown = 0: 笔抬起；=1: 笔按下 */
	updown = (!(data0 & S3C_ADCDAT0_UPDOWN)) &&
		 (!(data1 & S3C_ADCDAT1_UPDOWN));

	/* 如果笔按下 */
	if (updown) {
		if (priv->count) {
			input_report_abs(priv->idev, ABS_X, priv->xp>>priv->shift);
			input_report_abs(priv->idev, ABS_Y, priv->yp>>priv->shift);
			input_report_key(priv->idev, BTN_TOUCH, 1);
			input_report_abs(priv->idev, ABS_PRESSURE, 1);
			input_sync(priv->idev);
		}

		priv->xp = 0;
		priv->yp = 0;
		priv->count = 0;
		
		/* 启动ADC转换 */
		writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, (ts_base+S3C_ADCTSC));
		writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START, (ts_base+S3C_ADCCON));
	}
	else { /* 笔抬起 */
		priv->count = 0;

		/* 向input子系统提交input_event，报告笔抬起事件 */
		input_report_key(priv->idev, BTN_TOUCH, 0);
		input_report_abs(priv->idev, ABS_PRESSURE, 0);
		input_sync(priv->idev);

		/* 设定ADC控制器，等待笔再次按下 */
		writel(WAIT4INT(0), (ts_base+S3C_ADCTSC));
	}
}


/*******************************
 * 触摸笔按下/抬起中断的处理函数
 *******************************/
static irqreturn_t 
stylus_updown(int irq, void *dev_id)
{
	struct s3cts_priv *priv = dev_id;

	//等待笔抬起中断
	writel(WAIT4INT(1), ts_base+S3C_ADCTSC);

	/* 直接调用定时器处理函数 */
	ts_timer_func((unsigned long)priv);

	/* 清除中断标记 */
       	writel(0x0, ts_base+S3C_ADCCLRWK);
        writel(0x0, ts_base+S3C_ADCCLRINT);
        
	return IRQ_HANDLED;
}


/*************************
 * ADC转换中断的处理函数
 *************************/
static irqreturn_t 
stylus_action(int irq, void *dev_id)
{
	struct s3cts_priv *priv = dev_id;
	unsigned long data0;
	unsigned long data1;

	data0 = readl(ts_base+S3C_ADCDAT0);
	data1 = readl(ts_base+S3C_ADCDAT1);

	/* 记录采样到的坐标 */
	priv->xp += data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
	priv->yp += data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
	priv->count++;

	/* ADC转换中断会连续执行4次;
	 * 如果已经采样了4次，则启动定时器，在下一个tick时将坐标值传递给子系统。因此，每秒钟最多向用户态提供200次坐标 */
	if (priv->count < (1<<priv->shift)) {
		writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, ts_base+S3C_ADCTSC);
		writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START, ts_base+S3C_ADCCON);
	} else {
		mod_timer(&priv->ts_timer, jiffies+1);
		/* 等待笔抬起中断 */
		writel(WAIT4INT(1), ts_base+S3C_ADCTSC);
	}

       	writel(0x0, ts_base+S3C_ADCCLRWK);
        writel(0x0, ts_base+S3C_ADCCLRINT);
	
	return IRQ_HANDLED;
}


/************************
 * platform_driver->probe
 ************************/
static int 
s3cts_probe(struct platform_device *pdev)
{
	struct s3cts_priv *priv;
	struct input_dev *idev;
	int ret, value;
	struct resource *res1, *res2, *res3;
	struct s3c_ts_mach_info *ts_cfg = pdev->dev.platform_data;
	struct device *dev = &pdev->dev;

	/* 1.从pdev中获得资源 */
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res2 = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	res3 = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res1 || !res2 || !res3 || !ts_cfg) {
		dev_err(dev, "no enouch resources or pdata\n");
		return -ENOENT;
	}

	/* 2.分配并初始化私有结构体 */
	priv = (struct s3cts_priv *)kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->phy_base = res1->start;
	priv->phy_size = res1->end - res1->start + 1;
	priv->irq_ts = res2->start;
	priv->irq_adc = res3->start;
	priv->shift = ts_cfg->oversampling_shift;
	priv->count = 0;
	priv->xp = 0;
	priv->yp = 0;

	if (pdev->id == -1) {
		sprintf(priv->phys, "%s-only", pdev->name);
		sprintf(priv->name, "%s-only", "Input-ts");
	} else {
		sprintf(priv->phys, "%s-%d", pdev->name, pdev->id);
		sprintf(priv->name, "%s-%d", "Input-ts", pdev->id);
	}

	/* 初始化定时器 */
	setup_timer(&priv->ts_timer, 
		ts_timer_func, 
		(unsigned long)priv);

	/* 3. request_mem_region/ioremap */
	priv->ts_req = request_mem_region(priv->phy_base, priv->phy_size, pdev->name);
	if (!priv->ts_req) {
		dev_err(dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	ts_base = ioremap(priv->phy_base, priv->phy_size);
	if (!ts_base) {
		dev_err(dev, "failed to ioremap()\n");
		ret = -EINVAL;
		goto err_map;
	}

	/* 4. 获得并使能ADC的时钟 */
	priv->ts_clock = clk_get(dev, "adc");
	if (IS_ERR(priv->ts_clock)) {
		dev_err(dev, "failed to find adc clock source\n");
		ret = PTR_ERR(priv->ts_clock);
		goto err_clk;
	}
	clk_enable(priv->ts_clock);

	/* 5. 初始化部分寄存器 */
	if ((ts_cfg->presc & 0xFF) > 0) {
		value = S3C_ADCCON_PRSCEN | 
			S3C_ADCCON_PRSCVL(ts_cfg->presc&0xFF);
		writel(value, ts_base+S3C_ADCCON);
	} else {
		writel(0, ts_base+S3C_ADCCON);
	}

	if ((ts_cfg->delay & 0xFFFF) > 0) {
		value = ts_cfg->delay & 0xFFFF;
		writel(value, ts_base+S3C_ADCDLY);
	}

	/* 默认分辨率为12bit */
	value = readl(ts_base+S3C_ADCCON) | S3C_ADCCON_RESSEL_12BIT;
	writel(value, ts_base+S3C_ADCCON);

	/* 使触摸屏进入等待笔按下状态 */	
	writel(WAIT4INT(0), ts_base+S3C_ADCTSC);
	
	/* 6.分配input子系统要求的结构体input_dev */
	idev = input_allocate_device();
	if (!idev) {
		ret = -ENOMEM;
		goto err_alloc;
	}
	priv->idev = idev;

	/* 7.初始化input_dev */
	set_bit(EV_SYN, idev->evbit);
	set_bit(EV_KEY, idev->evbit);
	set_bit(EV_ABS, idev->evbit);
	set_bit(BTN_TOUCH, idev->keybit);

	input_set_abs_params(idev, ABS_X, 0, 0xFFF, 0, 0);
	input_set_abs_params(idev, ABS_Y, 0, 0xFFF, 0, 0);
	input_set_abs_params(idev, ABS_PRESSURE, 0, 1, 0, 0);

	idev->name = priv->name;
	idev->phys = priv->phys;
	idev->id.bustype = BUS_HOST;
	idev->id.vendor  = 0x1234;
	idev->id.product = 0x5678;
	idev->id.version = S3C_TSVERSION;
	
	/* 8.为笔抬起按下中断注册中断处理函数 */
	ret = request_irq(priv->irq_ts, 
		stylus_updown, 
		IRQF_SAMPLE_RANDOM, 
		"shrek_updown", priv);
	if (ret) {
		dev_err(dev,"Could not request IRQ%d\n", priv->irq_ts);
		ret = -EIO;
		goto err_irq1;
	}

	/* 9.为ADC转换中断注册中断处理函数 */
	ret = request_irq(priv->irq_adc, 
		stylus_action, 
		IRQF_SAMPLE_RANDOM, 
		"s3c_action", priv);
	if (ret) {
		dev_err(dev, "Could not request IRQ%d\n", priv->irq_adc);
		ret = -EIO;
		goto err_irq2;
	}

	/* 10.将input_dev注册到input子系统 */
	ret = input_register_device(idev);
	if(ret) {
		dev_err(dev, "Could not register input driver(ts)\n");
		ret = -EIO;
		goto reg_fail;
	}

	printk("%s got loaded successfully: 12 bits\n", idev->name);
	platform_set_drvdata(pdev, priv);

	return 0;

reg_fail:
	free_irq(priv->irq_adc, priv);
err_irq2:
	free_irq(priv->irq_ts, priv);
err_irq1:
	input_free_device(idev);
err_alloc:
	clk_disable(priv->ts_clock);
	clk_put(priv->ts_clock);
err_clk:
	iounmap(ts_base);
err_map: 
	//release_mem_region(xxx);
	release_resource(priv->ts_req);
	kfree(priv->ts_req);
err_req:
	kfree(priv);
	return ret;
}


/**********************
 * platform_driver->remove
 **********************/
static int 
s3cts_remove(struct platform_device *pdev)
{
	struct s3cts_priv *priv = platform_get_drvdata(pdev);

	free_irq(priv->irq_adc, priv);
	free_irq(priv->irq_ts, priv);
	
	input_unregister_device(priv->idev);
	input_free_device(priv->idev);

	clk_disable(priv->ts_clock);
	clk_put(priv->ts_clock);

	iounmap(ts_base);
	release_resource(priv->ts_req);
	kfree(priv->ts_req);

	kfree(priv);

	return 0;
}


#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

/**************************
 * platform_driver->suspend
 **************************/
static int 
s3cts_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3cts_priv *priv = platform_get_drvdata(pdev);

	adccon = readl(ts_base+S3C_ADCCON);
	adctsc = readl(ts_base+S3C_ADCTSC);
	adcdly = readl(ts_base+S3C_ADCDLY);

	disable_irq(priv->irq_ts);
	disable_irq(priv->irq_adc);

	clk_disable(priv->ts_clock);

	return 0;
}


/*************************
 * platform_driver->resume
 *************************/
static int 
s3cts_resume(struct platform_device *pdev)
{
	struct s3cts_priv *priv = platform_get_drvdata(pdev);

	clk_enable(priv->ts_clock);

	writel(adccon, ts_base+S3C_ADCCON);
	writel(adctsc, ts_base+S3C_ADCTSC);
	writel(adcdly, ts_base+S3C_ADCDLY);

	/* 令触摸屏控制器处于等待笔按下状态 */
	writel(WAIT4INT(0), ts_base+S3C_ADCTSC);

	enable_irq(priv->irq_ts);
	enable_irq(priv->irq_adc);

	return 0;
}

#else
#define s3cts_suspend NULL
#define s3cts_resume  NULL
#endif /* CONFIG_PM */


static struct platform_driver s3cts_drv = {
       .probe          = s3cts_probe,
       .remove         = s3cts_remove,
       .suspend        = s3cts_suspend,
       .resume         = s3cts_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "shrek-ts",
	},
};


static char banner[] = "S3C Touchscreen driver, (c)2013 Uplooking Ltd.,Co\n";

static int __init s3cts_init(void)
{
	printk(banner);
	return platform_driver_register(&s3cts_drv);
}

static void __exit s3cts_exit(void)
{
	platform_driver_unregister(&s3cts_drv);
}

module_init(s3cts_init);
module_exit(s3cts_exit);

MODULE_AUTHOR("ZHT");
MODULE_DESCRIPTION("S3C touchscreen driver");
MODULE_LICENSE("GPL");

