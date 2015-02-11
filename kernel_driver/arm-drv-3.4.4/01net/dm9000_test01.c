/******************************
 * DM9000网卡的驱动
 * from drivers/net/ethernet/davicom/dm9000.c
 * 支持linux-3.4.4;不支持linux-2.6.28
 * 本驱动比标准驱动简单一些，只支持16位dm9000芯片
 * author: zht
 * date: 2013-01-17
 ******************************/
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/netdevice.h> /* net_device */
#include <linux/etherdevice.h>
#include <linux/skbuff.h> /* sk_buff */
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/dm9000.h> /* dm9000_plat_data */

#include "dm9000.h" /* 寄存器的头文件 */

#define CARDNAME	"dm9000"
#define DRV_VERSION	"2.0-344"

/* 发送超时时间，默认5秒 */
static int watchdog = 5000;
module_param(watchdog, int, 0400);
MODULE_PARM_DESC(watchdog, "transmit timeout in milliseconds");


/****************************
 * 私有结构体，每个网卡一个
 ****************************/
typedef struct dm9k_priv {
	/* 为INDEX/DATA端口申请的资源以及虚拟地址 */
	struct resource	*addr_req;
	struct resource *data_req;
	void __iomem	*io_addr;
	void __iomem	*io_data;

	/* queue_pkt_len记录TXFIFO中要发送包的大小
	 * queue_ip_summed记录TXFIFO中要发送的包是否由网络层进行校验 */
	u16		tx_pkt_cnt;
	u16		queue_pkt_len;
	u16		queue_ip_summed;
	u8		imr_all;
	int		ip_summed;

	/* 用于记录3个函数，分别用于从DM9000接收一个数据包；发送数据包；丢弃数据包 */
	void (*inblk)(void __iomem *port, void *data, int length);
	void (*outblk)(void __iomem *port, void *data, int length);
	void (*dumpblk)(void __iomem *port, int length);

	spinlock_t	lock;

	struct device	*dev;
	struct net_device *ndev;
	struct mii_if_info mii;
} dm9k_priv_t;



/************************
 * 软件复位DM9000
 ************************/
static void 
dm9000_reset(dm9k_priv_t *priv)
{
	dev_dbg(priv->dev, "reset device\n");

	/* RESET device */
	writeb(DM9000_NCR, priv->io_addr);
	writeb(NCR_RST, priv->io_data);
	udelay(200);
}


/*********************
 * 读/写DM9000寄存器
 *********************/
static u8 
ior(dm9k_priv_t *priv, int reg)
{
	writeb(reg, priv->io_addr);
	return readb(priv->io_data);
}

static void 
iow(dm9k_priv_t *priv, int reg, int value)
{
	writeb(reg, priv->io_addr);
	writeb(value, priv->io_data);
}


/**************************
 * DM9000 FIFO的访问函数集
 * outblk: 向TXFIFO中写入一个数据包
 * inblk: 从RXFIFO中读一个数据包
 * dumpblk: 丢弃一个坏包
 * reg: DATA端口的虚拟地址
 * data: 数据包在内存中的基地址
 * count: 数据包的字节数
 **************************/
static void 
dm9000_outblk_16bit(void __iomem *reg, void *data, int count)
{
	writesw(reg, data, (count+1) >> 1);
}

static void 
dm9000_inblk_16bit(void __iomem *reg, void *data, int count)
{
	readsw(reg, data, (count+1) >> 1);
}

static void 
dm9000_dumpblk_16bit(void __iomem *reg, int count)
{
	int i;
	int tmp;

	count = (count + 1) >> 1;

	for (i = 0; i < count; i++)
		tmp = readw(reg);
}


/************************
 * iounmap以及释放申请的资源
 * 针对网卡的INDEX和DATA端口
 ************************/
static void
dm9000_release_board(dm9k_priv_t *priv)
{
	/* iounmap */
	iounmap(priv->io_addr);
	iounmap(priv->io_data);

	/* release the resources */
	//release_mem_region(物理地址，范围)
	release_resource(priv->data_req);
	kfree(priv->data_req);

	release_resource(priv->addr_req);
	kfree(priv->addr_req);
}


/****************************
 * 设置网卡的多播掩码并使能接收
 * 多播地址是第一个字节的bit0为1的所有MAC地址
 * 广播地址是6个字节全部为0xFF的MAC地址
 ****************************/
static void
dm9000_hash_table_unlocked(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	struct netdev_hw_addr *ha;
	int i, oft;
	u32 hash_val;
	u16 hash_table[4];
	u8 rcr = RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN;

	/* 将net_device->dev_addr中的MAC地址写入DM9000寄存器 */
	for (i = 0, oft = DM9000_PAR; i < 6; i++, oft++)
		iow(priv, oft, ndev->dev_addr[i]);

	/* Clear Hash Table */
	for (i = 0; i < 4; i++)
		hash_table[i] = 0x0;

	/* broadcast address */
	hash_table[3] = 0x8000;

	if (ndev->flags & IFF_PROMISC)
		rcr |= RCR_PRMSC;

	if (ndev->flags & IFF_ALLMULTI)
		rcr |= RCR_ALL;

	/* 遍历设备支持的所有多播地址
	 * 将该地址计算出6位的CRC值后，设置hash_table的相应位 */
	netdev_for_each_mc_addr(ha, ndev) {
		hash_val = ether_crc_le(6, ha->addr) & 0x3f;
		hash_table[hash_val / 16] |= (u16) 1 << (hash_val % 16);
	}

	/* 将准备好的多播掩码写入DM9000的多播掩码寄存器 */
	for (i = 0, oft = DM9000_MAR; i < 4; i++) {
		iow(priv, oft++, hash_table[i]);
		iow(priv, oft++, hash_table[i] >> 8);
	}

	/* 设置接收控制寄存器 */
	iow(priv, DM9000_RCR, rcr);
}


/***************************
 * 初始化DM9000的寄存器
 ***************************/
static void
dm9000_init_regs(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	unsigned int imr;

	/* 使能或关闭接收校验
	 * features: 设备当前激活的特性 */
	if (ndev->features & NETIF_F_RXCSUM) 
		iow(priv, DM9000_RCSR, RCSR_CSUM);
	else
		iow(priv, DM9000_RCSR, 0);

	iow(priv, DM9000_TCR, 0);/* 清空发送控制寄存器 */

	/* 清空网络状态和中断状态寄存器 */
	iow(priv, DM9000_NSR, NSR_TX2END | NSR_TX1END);
	iow(priv, DM9000_ISR, ISR_CLR_STATUS);

	/* 设置多播掩码并使能接收 */
	dm9000_hash_table_unlocked(ndev);

	/* 使能中断(PAR位对应FIFO内部指针自动回卷) */
	imr = IMR_PAR | IMR_PTM | IMR_PRM | IMR_LNKCHNG;
	iow(priv, DM9000_IMR, imr);
	priv->imr_all = imr;

	/* Init Driver variable */
	priv->tx_pkt_cnt = 0;
	priv->queue_pkt_len = 0;
	ndev->trans_start = jiffies;
}


/***************************
 * net_device_ops->ndo_set_rx_mode
 * 在持有锁的情况下设置MAC地址，多播掩码以及接收寄存器
 ***************************/
static void
dm9000_hash_table(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	dm9000_hash_table_unlocked(ndev);
	spin_unlock_irqrestore(&priv->lock, flags);
}


/*******************************
 * net_device_ops->ndo_do_ioctl
 *******************************/
static int 
dm9000_ioctl(struct net_device *ndev, struct ifreq *req, int cmd)
{
	dm9k_priv_t *priv = netdev_priv(ndev);

	if (!netif_running(ndev))
		return -EINVAL;

	return generic_mii_ioctl(&priv->mii, if_mii(req), cmd, NULL);
}


/********************************
 * net_device_ops->ndo_set_features
 * 设置网卡的属性(features)及对应的寄存器
 ********************************/
static int 
dm9000_set_features(struct net_device *ndev, netdev_features_t features)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	netdev_features_t changed = ndev->features ^ features;
	unsigned long flags;

	if (!(changed & NETIF_F_RXCSUM))
		return 0;

	spin_lock_irqsave(&priv->lock, flags);
	iow(priv, DM9000_RCSR, (features & NETIF_F_RXCSUM) ? RCSR_CSUM : 0);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}


/***************************
 * net_device_ops->ndo_tx_timeout
 * 当发送超时后由网络子系统调用(在中断上下文调用)
 ***************************/
static void 
dm9000_timeout(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	u8 reg_save;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	reg_save = readb(priv->io_addr);

	/* 停止发送队列；软件复位网卡；重新初始化网卡寄存器 */
	netif_stop_queue(ndev);
	dm9000_reset(priv);
	dm9000_init_regs(ndev);

	/* 重新唤醒发送队列 */
	netif_wake_queue(ndev);

	writeb(reg_save, priv->io_addr);
	spin_unlock_irqrestore(&priv->lock, flags);
}


/***************************
 * 发送一个数据包
 * 根据skb->ip_summed的设置决定是否由DM9000进行校验
 ***************************/
static void 
dm9000_send_packet(struct net_device *ndev, int ip_summed, u16 pkt_len)
{
	dm9k_priv_t *priv = netdev_priv(ndev);

	/* 根据skb->ip_summed的设置(参数ip_summed)
	 * 确定是否由DM9000硬件对IP,TCP,UDP包进行校验 */
	if (priv->ip_summed != ip_summed) {
		if (ip_summed == CHECKSUM_NONE)
			iow(priv, DM9000_TCCR, 0);
		else
			iow(priv, DM9000_TCCR, TCCR_IP | TCCR_UDP | TCCR_TCP);
		priv->ip_summed = ip_summed;
	}

	/* 将要发送包的长度写入DM9000寄存器 */
	iow(priv, DM9000_TXPLL, pkt_len);
	iow(priv, DM9000_TXPLH, pkt_len >> 8);

	/* 启动TX FIFO中数据包的发送 */
	iow(priv, DM9000_TCR, TCR_TXREQ);
}


/***************************
 * net_device_ops->ndo_start_xmit
 * 发送一个数据包，由网络子系统创建的内核线程来调用
 ***************************/
static int
dm9000_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	unsigned long flags;
	dm9k_priv_t *priv = netdev_priv(ndev);

	/* 如果TX FIFO的两个slot都有数据包等待发送，则返回 */
	if (priv->tx_pkt_cnt > 1)
		return NETDEV_TX_BUSY;

	spin_lock_irqsave(&priv->lock, flags);

	/* 将MWCMD寄存器的偏移值写入INDEX端口后
	 * 每向DATA端口写入2个字节，TX FIFO的内部指针+=2 */
	writeb(DM9000_MWCMD, priv->io_addr);
	(priv->outblk)(priv->io_data, skb->data, skb->len);

	/* 更新相关的计数 */	
	ndev->stats.tx_bytes += skb->len;
	priv->tx_pkt_cnt++;

	/* 如果是TX FIFO中的第一个包，则马上发送;
	 * 如果是TX FIFO中的第二个包，则等第一个包发完后再发送 */
	if (priv->tx_pkt_cnt == 1) {
		dm9000_send_packet(ndev, skb->ip_summed, skb->len);
	} else {
		priv->queue_pkt_len = skb->len;
		priv->queue_ip_summed = skb->ip_summed;
		netif_stop_queue(ndev);
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	/* 释放sk_buff */
	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}


/***************************
 * 由DM9000的中断处理函数调用
 * 当TX FIFO中的数据包发送完成后调用
 ***************************/
static void 
dm9000_tx_done(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	int tx_status = ior(priv, DM9000_NSR);

	if (tx_status & (NSR_TX2END | NSR_TX1END)) {
		/* 如果是TXFIFO中的某个包发完，则更改相关计数 */
		priv->tx_pkt_cnt--;
		ndev->stats.tx_packets++;

		/* 如果TXFIFO中还有包没发，则启动发送 */
		if (priv->tx_pkt_cnt > 0)
			dm9000_send_packet(ndev, priv->queue_ip_summed,
					   priv->queue_pkt_len);
		netif_wake_queue(ndev);
	}
}

struct dm9000_rxhdr {
	u8	RxPktReady;
	u8	RxStatus;
	__le16	RxLen;
} __packed;


/***************************
 * 由DM9000的中断处理函数调用
 * 处理数据包的接收
 ***************************/
static void
dm9000_rx(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	struct dm9000_rxhdr rxhdr;
	struct sk_buff *skb;
	u8 rxbyte, *rdptr;
	bool GoodPacket;
	int RxLen;

	/* 在do-while循环中，处理RXFIFO中收到的所有包 */
	do {
		/* 读MRCMDX寄存器后，RXFIFO的内部读指针保持不变，并且DM9000将RXFIFO中的数据读入DATA端口 */
		ior(priv, DM9000_MRCMDX);

		/* 从DATA端口读一个字节(数据包的首字节) */
		rxbyte = readb(priv->io_data);

		/* 首字节bit0为0：RXFIFO中没有数据包要处理
		 * 首字节bit0为1：RXFIFO中有数据包等待处理
		 * 首字节bit1为1：状态位错误
		 * 首字节bit[2-7]:说明收到包的类型以及校验状态
		 * 参P24(32H) */
		if (rxbyte & DM9000_PKT_ERR) {
			dev_warn(priv->dev, "status check fail: %d\n", rxbyte);
			iow(priv, DM9000_RCR, 0x00);	/* 停止接收 */
			iow(priv, DM9000_ISR, IMR_PAR);	/* ? */
			return;
		}

		/* 如果bit0==0，则说明没有包要接收，返回 */
		if (!(rxbyte & DM9000_PKT_RDY))
			return;

		/* 假定要接收的为好包 */
		GoodPacket = true;

		/* 将MRCMD写入INDEX端口后，读RXFIFO后内部读指针会+=2
		 * 读出数据包的4个字节包头 */
		writeb(DM9000_MRCMD, priv->io_addr);
		(priv->inblk)(priv->io_data, &rxhdr, sizeof(rxhdr));

		/* 检测数据包的长度(不能小于64字节) */
		RxLen = le16_to_cpu(rxhdr.RxLen);
		if (RxLen < 0x40) {
			GoodPacket = false;
			dev_dbg(priv->dev, "RX: Packet too short\n");
		}
		if (RxLen > DM9000_PKT_MAX) {
			dev_dbg(priv->dev, "RX: Packet too long(%x)\n", RxLen);
		}

		/* 如果接收到的包有错误，则更新错误计数 */
		if (rxhdr.RxStatus & (RSR_FOE | RSR_CE | RSR_AE |
				      RSR_PLE | RSR_RWTO |
				      RSR_LCS | RSR_RF)) {
			GoodPacket = false;
			if (rxhdr.RxStatus & RSR_FOE) {
				dev_dbg(priv->dev, "fifo error\n");
				ndev->stats.rx_fifo_errors++;
			}
			if (rxhdr.RxStatus & RSR_CE) {
				dev_dbg(priv->dev, "crc error\n");
				ndev->stats.rx_crc_errors++;
			}
			if (rxhdr.RxStatus & RSR_RF) {
				dev_dbg(priv->dev, "length error\n");
				ndev->stats.rx_length_errors++;
			}
		}

		/* 如果是好包则分配sk_buff，并将其从RXFIFO中拷出来 */
		if (GoodPacket &&
		    ((skb = netdev_alloc_skb(ndev, RxLen + 4)) != NULL)) {
			skb_reserve(skb, 2);
			rdptr = (u8 *)skb_put(skb, RxLen - 4);

			/* 从RXFIFO中接收数据包 */
			(priv->inblk)(priv->io_data, rdptr, RxLen);
			ndev->stats.rx_bytes += RxLen;

			/* 在skb中记录包的类型，如果使能了接收校验，
			 * 则通知网络子系统不必再进行校验 */
			skb->protocol = eth_type_trans(skb, ndev);
			if (ndev->features & NETIF_F_RXCSUM) {
				if ((((rxbyte & 0x1c) << 3) & rxbyte) == 0)
					skb->ip_summed = CHECKSUM_UNNECESSARY;
				else
					skb_checksum_none_assert(skb);
			}

			/* 将sk_buff提交给子系统 */
			netif_rx(skb);
			ndev->stats.rx_packets++;

		} else {
			/* 如果是坏包则丢弃 */
			(priv->dumpblk)(priv->io_data, RxLen);
		}
	} while (rxbyte & DM9000_PKT_RDY);
}


/*************************
 * 由中断处理函数调用
 * 处理网线连接状态的改变
 *************************/
static void
dm9000_carrier_change(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	u8 old_carrier = netif_carrier_ok(ndev) ? 1 : 0;
	u8 new_carrier;
	u8 nsr, ncr;

	/* 读网络状态和网络控制寄存器 */
	nsr = ior(priv, DM9000_NSR);
	ncr = ior(priv, DM9000_NCR);
	
	new_carrier = (nsr & NSR_LINKST) ? 1 : 0;
	if (new_carrier == old_carrier)
		return;

	/* 如果载波状态发生变化 */
	if (new_carrier) {
		netif_carrier_on(ndev);
		dev_info(priv->dev, 
			"%s: link up, %dMbps, %s-duplex\n",
			ndev->name, (nsr&NSR_SPEED) ? 10 : 100,
			(ncr&NCR_FDX) ? "full" : "half");
	} else {
		netif_carrier_off(ndev);
		dev_info(priv->dev, 
			"%s: link down\n", ndev->name);
	}
}


/*************************
 * DM9000的中断处理函数
 *************************/
static irqreturn_t 
dm9000_interrupt(int irq, void *dev_id)
{
	struct net_device *ndev = dev_id;
	dm9k_priv_t *priv = netdev_priv(ndev);
	int int_status;
	u8 reg_save;

	spin_lock(&priv->lock);
	reg_save = readb(priv->io_addr);

	/* 屏蔽DM9000的所有中断 */
	iow(priv, DM9000_IMR, IMR_PAR);

	/* 获取并清空DM9000的中断状态位 */
	int_status = ior(priv, DM9000_ISR);
	iow(priv, DM9000_ISR, int_status);

	/* 收到数据包 */
	if (int_status & ISR_PRS)
		dm9000_rx(ndev);

	/* 有数据包发送完成 */
	if (int_status & ISR_PTS)
		dm9000_tx_done(ndev);

	/* 网线状态变化 */
	if (int_status & ISR_LNKCHNG) {
		dm9000_carrier_change(ndev);
	}

	/* 重新使能DM9000中断 */
	iow(priv, DM9000_IMR, priv->imr_all);

	writeb(reg_save, priv->io_addr);
	spin_unlock(&priv->lock);

	return IRQ_HANDLED;
}


#ifdef CONFIG_NET_POLL_CONTROLLER

/******************************
 * net_device_ops->ndo_poll_controller
 * 由netconsole使用
 * 在关闭DM9000中断的情况下，手工调用中断处理函数
 ******************************/
static void dm9000_poll_controller(struct net_device *ndev)
{
	disable_irq(ndev->irq);
	dm9000_interrupt(ndev->irq, ndev);
	enable_irq(ndev->irq);
}
#endif


/*******************************
 * net_device_ops->ndo_open
 * 对应ifconfig和ifup
 *******************************/
static int
dm9000_open(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	unsigned long irqflags = IRQF_SHARED | IRQF_TRIGGER_HIGH;
	int ret;

	/* power up PHY */
	iow(priv, DM9000_GPR, 0);
	mdelay(1);

	/* 复位并初始化DM9000寄存器 */
	dm9000_reset(priv);
	dm9000_init_regs(ndev);

	/* 注册中断处理函数 */
	ret = request_irq(ndev->irq, dm9000_interrupt, irqflags, ndev->name, ndev);
	if (ret) {
		printk("Cannot request irq %d\n", ndev->irq);
		return -EAGAIN;
	}

	/* 第一个1：允许打印网线连接状态
	 * 第二个1：允许修改duplex mode */
	mii_check_media(&priv->mii, 1, 1);

	/* 使能发送队列 */
	netif_start_queue(ndev);

	return 0;
}


/***********************
 * mii_if_info->mdio_read
 * 读PHY寄存器
 ***********************/
static int
dm9000_phy_read(struct net_device *ndev, int phy_id, int reg)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	unsigned long flags;
	unsigned int reg_save;
	int ret;

	spin_lock_irqsave(&priv->lock, flags);
	reg_save = readb(priv->io_addr);

	/* 将要访问的PHY寄存器偏移写入PHY地址寄存器 */
	iow(priv, DM9000_EPAR, DM9000_PHY | reg);

	/* 发出读命令并等待完成 */
	iow(priv, DM9000_EPCR, EPCR_ERPRR | EPCR_EPOS);
	mdelay(1);

	/* 清除读命令 */
	iow(priv, DM9000_EPCR, 0x0);

	/* 读出PHY寄存器的内容 */
	ret = (ior(priv, DM9000_EPDRH) << 8) | 
	      (ior(priv, DM9000_EPDRL));

	writeb(reg_save, priv->io_addr);
	spin_unlock_irqrestore(&priv->lock, flags);

	printk("phy_read[%02x] -> %04x\n", reg, ret);
	return ret;
}


/**************************
 * mii_if_info->mdio_write
 * 写PHY寄存器
 **************************/
static void
dm9000_phy_write(struct net_device *ndev, int phy_id, int reg, int value)
{
	dm9k_priv_t *priv = netdev_priv(ndev);
	unsigned long flags;
	unsigned long reg_save;

	printk("phy_write[%02x] = %04x\n", reg, value);

	spin_lock_irqsave(&priv->lock,flags);
	reg_save = readb(priv->io_addr);

	/* 将要访问的寄存器偏移写入PHY地址寄存器 */
	iow(priv, DM9000_EPAR, DM9000_PHY | reg);

	/* 写PHY数据寄存器 */
	iow(priv, DM9000_EPDRL, value);
	iow(priv, DM9000_EPDRH, value >> 8);

	/* 发出写命令并等待完成 */
	iow(priv, DM9000_EPCR, EPCR_EPOS | EPCR_ERPRW);
	mdelay(1);

	/* 清除写命令 */
	iow(priv, DM9000_EPCR, 0x0);

	writeb(reg_save, priv->io_addr);
	spin_unlock_irqrestore(&priv->lock, flags);
}


/***************************
 * 关闭DM9000网卡硬件
 ***************************/
static void
dm9000_shutdown(struct net_device *ndev)
{
	dm9k_priv_t *priv = netdev_priv(ndev);

	//dm9000_phy_write(ndev, 0, MII_BMCR, BMCR_RESET); /* PHY RESET */
	iow(priv, DM9000_GPR, 0x01);	/* Power-Down PHY */
	iow(priv, DM9000_IMR, IMR_PAR);	/* Disable all interrupt */
	iow(priv, DM9000_RCR, 0x00);	/* Disable RX */
}


/****************************
 * net_device_ops->ndo_stop
 * 停止网卡，对应ifdown
 ****************************/
static int
dm9000_stop(struct net_device *ndev)
{
	netif_stop_queue(ndev);
	netif_carrier_off(ndev);

	free_irq(ndev->irq, ndev);
	dm9000_shutdown(ndev);

	return 0;
}


static struct net_device_ops dm9000_netdev_ops = {
	.ndo_open	= dm9000_open,
	.ndo_stop	= dm9000_stop,
	.ndo_start_xmit	= dm9000_start_xmit,
	.ndo_tx_timeout	= dm9000_timeout,
	.ndo_set_rx_mode= dm9000_hash_table,
	.ndo_do_ioctl	= dm9000_ioctl,
	.ndo_set_features= dm9000_set_features,
	.ndo_change_mtu	= eth_change_mtu,
	.ndo_validate_addr= eth_validate_addr,
	.ndo_set_mac_address= eth_mac_addr,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= dm9000_poll_controller,
#endif
};


/******************************
 * platform_driver->probe
 ******************************/
static int
dm9000_probe(struct platform_device *pdev)
{
	struct net_device *ndev;
	dm9k_priv_t *priv;
	struct resource *addr_res, *data_res, *irq_res;
	int i, iosize, ret = 0;
	u32 id_val;
	struct dm9000_plat_data *pdata = pdev->dev.platform_data;

	/* 1.为net_device和dm9k_priv结构体分配空间
	 * 2.通过调用ether_setup执行net_device的第一阶段初始化 */
	ndev = alloc_etherdev(sizeof(struct dm9k_priv));
	if (!ndev)
		return -ENOMEM;
	SET_NETDEV_DEV(ndev, &pdev->dev);

	/* 3.dm9k_priv的第一阶段初始化 */
	priv = netdev_priv(ndev);
	priv->dev = &pdev->dev;
	priv->ndev = ndev;
	spin_lock_init(&priv->lock);

	/* 4.从platform_device中获取网卡的端口地址和中断号 */
	addr_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	irq_res  = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	if (!addr_res || !data_res || !irq_res) {
		dev_err(priv->dev, "insufficient resources\n");
		ret = -ENOENT;
		goto out;
	}

	/* 5.为INDEX端口request_mem_region/ioremap */
	iosize = addr_res->end - addr_res->start + 1;
	priv->addr_req = request_mem_region(addr_res->start, 
			iosize, "INDEX");
	if (!priv->addr_req) {
		dev_err(priv->dev, "cannot request INDEX port\n");
		ret = -EIO;
		goto out;
	}

	priv->io_addr = ioremap(addr_res->start, iosize);
	if (!priv->io_addr) {
		dev_err(priv->dev, "failed to ioremap INDEX port\n");
		ret = -EINVAL;
		goto out;
	}

	/* 6.为DATA端口request_mem_region/ioremap */
	iosize = data_res->end - data_res->start + 1;
	priv->data_req = request_mem_region(data_res->start, 
			iosize, "DATA");
	if (!priv->data_req) {
		dev_err(priv->dev, "cannot request DATA port\n");
		ret = -EIO;
		goto out;
	}

	priv->io_data = ioremap(data_res->start, iosize);
	if (!priv->io_data) {
		dev_err(priv->dev, "failed to ioremap DATA port\n");
		ret = -EINVAL;
		goto out;
	}

	ndev->base_addr = (unsigned long)priv->io_addr;
	ndev->irq	= irq_res->start;

	/* 7.确定in/out/dump函数指针 */
	priv->inblk 	= dm9000_inblk_16bit;
	priv->outblk 	= dm9000_outblk_16bit;
	priv->dumpblk 	= dm9000_dumpblk_16bit;

	/* 8.软件复位DM9000 */
	dm9000_reset(priv);

	/* 9.读DM9000的ID，多试几次 */
	for (i = 0; i < 8; i++) {
		id_val  = ior(priv, DM9000_VIDL);
		id_val |= (u32)ior(priv, DM9000_VIDH) << 8;
		id_val |= (u32)ior(priv, DM9000_PIDL) << 16;
		id_val |= (u32)ior(priv, DM9000_PIDH) << 24;

		if (id_val == DM9000_ID)
			break;
	}

	if (id_val != DM9000_ID) {
		dev_err(priv->dev, "wrong id: 0x%08x\n", id_val);
		ret = -ENODEV;
		goto out;
	}

	/* 10.net_device的第二阶段初始化 */
	ndev->hw_features 	= NETIF_F_RXCSUM | NETIF_F_IP_CSUM;
	ndev->features 		|= ndev->hw_features;
	ndev->netdev_ops	= &dm9000_netdev_ops;
	ndev->watchdog_timeo	= msecs_to_jiffies(watchdog);

	/* 11.初始化私有结构体中和MII相关的内容，该内容用于访问PHY */
	priv->mii.phy_id_mask  = 0x1f;
	priv->mii.reg_num_mask = 0x1f;
	priv->mii.force_media  = 0;
	priv->mii.full_duplex  = 0;
	priv->mii.dev	     = ndev;
	priv->mii.mdio_read    = dm9000_phy_read;
	priv->mii.mdio_write   = dm9000_phy_write;

	/* 12.确定DM9000的MAC地址 */
	memcpy(ndev->dev_addr, pdata->dev_addr, 6);

	/* 13.注册net_device */
	ret = register_netdev(ndev);
	if (ret == 0)
		printk("%s: dm9000 at %p,%p IRQ %d MAC: %pM\n",
			ndev->name, priv->io_addr, priv->io_data, 
			ndev->irq, ndev->dev_addr);

	platform_set_drvdata(pdev, ndev);
	return 0;

out:
	dev_err(priv->dev, "not found (%d).\n", ret);
	dm9000_release_board(priv);
	free_netdev(ndev);

	return ret;
}


/***************************
 * platform_driver->remove
 ***************************/
static int 
dm9000_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	dm9k_priv_t *priv = netdev_priv(ndev);

	platform_set_drvdata(pdev, NULL);

	unregister_netdev(ndev);
	dm9000_release_board(priv);
	free_netdev(ndev);

	dev_dbg(priv->dev, "released and freed device\n");
	return 0;
}


/**************************
 * dev_pm_ops->suspend
 **************************/
static int
dm9000_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);

	if (!ndev || !netif_running(ndev))
		return 0;

	netif_device_detach(ndev);
	dm9000_shutdown(ndev);

	return 0;
}


/**************************
 * dev_pm_ops->resume
 **************************/
static int
dm9000_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);
	dm9k_priv_t *priv = netdev_priv(ndev);

	if (!ndev)
		return 0;

	if (netif_running(ndev)) {
		dm9000_reset(priv);
		dm9000_init_regs(ndev);
		netif_device_attach(ndev);
	}

	return 0;
}

static const struct dev_pm_ops dm9000_pm_ops = {
	.suspend	= dm9000_suspend,
	.resume		= dm9000_resume,
};


static struct platform_driver dm9000_driver = {
	.driver	= {
		.name    = "shrek-dm9k",
		.owner	 = THIS_MODULE,
		.pm	 = &dm9000_pm_ops,
	},
	.probe   = dm9000_probe,
	.remove  = dm9000_remove,
};


static int __init dm9000_init(void)
{
	printk("%s Ethernet Driver, V%s\n", CARDNAME, DRV_VERSION);
	return platform_driver_register(&dm9000_driver);
}

static void __exit dm9000_exit(void)
{
	platform_driver_unregister(&dm9000_driver);
}

module_init(dm9000_init);
module_exit(dm9000_exit);

MODULE_AUTHOR("ZHT");
MODULE_DESCRIPTION("Davicom DM9000 network driver");
MODULE_LICENSE("GPL");

