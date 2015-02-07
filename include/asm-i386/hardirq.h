#ifndef __ASM_HARDIRQ_H
#define __ASM_HARDIRQ_H

#include <linux/config.h>
#include <linux/threads.h>
#include <linux/irq.h>

/*irq统计数据，每个cpu一个*/
typedef struct {
	/*表示挂起的软中断*/
	unsigned int __softirq_pending;
	/*cpu变为空闲的时间*/
	unsigned long idle_timestamp;
	/*非屏蔽终端发生的次数*/
	unsigned int __nmi_count;	/* arch dependent */
	/*本地apic时钟中断发生的次数*/
	unsigned int apic_timer_irqs;	/* arch dependent */
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

void ack_bad_irq(unsigned int irq);

#endif /* __ASM_HARDIRQ_H */
