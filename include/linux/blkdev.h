#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H

#include <linux/config.h>
#include <linux/major.h>
#include <linux/genhd.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/wait.h>
#include <linux/mempool.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/stringify.h>

#include <asm/scatterlist.h>

struct request_queue;
typedef struct request_queue request_queue_t;
struct elevator_queue;
typedef struct elevator_queue elevator_t;
struct request_pm_state;

#define BLKDEV_MIN_RQ	4
#define BLKDEV_MAX_RQ	128	/* Default maximum */

/*
 * This is the per-process anticipatory I/O scheduler state.
 */
struct as_io_context {
	spinlock_t lock;

	void (*dtor)(struct as_io_context *aic); /* destructor */
	void (*exit)(struct as_io_context *aic); /* called on task exit */

	unsigned long state;
	atomic_t nr_queued; /* queued reads & sync writes */
	atomic_t nr_dispatched; /* number of requests gone to the drivers */

	/* IO History tracking */
	/* Thinktime */
	unsigned long last_end_request;
	unsigned long ttime_total;
	unsigned long ttime_samples;
	unsigned long ttime_mean;
	/* Layout pattern */
	unsigned int seek_samples;
	sector_t last_request_pos;
	u64 seek_total;
	sector_t seek_mean;
};

struct cfq_queue;
struct cfq_io_context {
	void (*dtor)(struct cfq_io_context *);
	void (*exit)(struct cfq_io_context *);

	struct io_context *ioc;

	/*
	 * circular list of cfq_io_contexts belonging to a process io context
	 */
	struct list_head list;
	struct cfq_queue *cfqq;
};

/*
 * This is the per-process I/O subsystem state.  It is refcounted and
 * kmalloc'ed. Currently all fields are modified in process io context
 * (apart from the atomic refcount), so require no locking.
 */
struct io_context {
	atomic_t refcount;
	pid_t pid;

	/*
	 * For request batching
	 */
	unsigned long last_waited; /* Time last woken after wait for request */
	int nr_batch_requests;     /* Number of requests left in the batch */

	spinlock_t lock;

	struct as_io_context *aic;
	struct cfq_io_context *cic;
};

void put_io_context(struct io_context *ioc);
void exit_io_context(void);
struct io_context *get_io_context(int gfp_flags);
void copy_io_context(struct io_context **pdst, struct io_context **psrc);
void swap_io_context(struct io_context **ioc1, struct io_context **ioc2);

struct request_list {
	/*两个计数器，分别用于记录分配给READ和WRITE请求的请求描述符数*/
	int count[2];
	/*两个标志，分别用于标记为读或写请求的分配是否失败*/
	int starved[2];
	/*一个指针，指向请求描述符的内存池*/
	mempool_t *rq_pool;
	/*两个等待队列，分别存放了为获得空闲的读和写请求描述符而睡眠的进程*/
	wait_queue_head_t wait[2];
	/*一个等待队列，存放等待一个请求队列被刷新(清空)的进程*/
	wait_queue_head_t drain;
};

#define BLK_MAX_CDB	16

/*
 * try to put the fields that are referenced together in the same cacheline
 */
struct request {
	/*请求队列链表的指针*/
	struct list_head queuelist; /* looking for ->queue? you must _not_
				     * access it directly, use
				     * blkdev_dequeue_request! */
	/*请求标志*/
	unsigned long flags;		/* see REQ_ bits below */

	/* Maintain bio traversal state for part by part I/O submission.
	 * hard_* are block layer internals, no driver should touch them!
	 */

	/*要传送的下一个扇区号*/
	sector_t sector;		/* next sector to submit */
	/*整个请求中要传送的扇区数*/
	unsigned long nr_sectors;	/* no. of sectors left to submit */
	/* no. of sectors left to submit in the current segment */
	/*当前bio的当前段中要传送的扇区数*/
	unsigned int current_nr_sectors;

	/*要传送的下一个扇区号*/
	sector_t hard_sector;		/* next sector to complete */
	/*整个请求中要传送的扇区数(由通用块层更新)*/
	unsigned long hard_nr_sectors;	/* no. of sectors left to complete */
	/* no. of sectors left to complete in the current segment */
	/*当前bio的当前段中要传送的扇区数(由通用块层更新)*/
	unsigned int hard_cur_sectors;

	/*请求中第一个没有完成传送操作的bio*/
	struct bio *bio;
	/*请求链表中末尾的bio*/
	struct bio *biotail;

	/*指向I/O调度程序私有数据的指针*/
	void *elevator_private;

	/*请求状态:实际上，或者是RQ_ACTIVE,或者是RQ_INACTIVE*/
	int rq_status;	/* should split this into a few status bits */
	/*请求所引用的磁盘描述符*/
	struct gendisk *rq_disk;
	/*用于记录当前传送中发生的I/O失败次数的计数器*/
	int errors;
	/*请求的起始时间jiffies*/
	unsigned long start_time;

	/* Number of scatter-gather DMA addr+len pairs after
	 * physical address coalescing is performed.
	 */
	/*请求的物理段数*/
	unsigned short nr_phys_segments;

	/* Number of scatter-gather addr+len pairs after
	 * physical and DMA remapping hardware coalescing is performed.
	 * This is the number of scatter-gather entries the driver
	 * will actually have to deal with after DMA mapping is done.
	 */
	/*请求的硬段数*/
	unsigned short nr_hw_segments;

	/*与请求相关的标记(只支持多次数据传送的硬件设备)*/
	int tag;
	/*指向当前数据传送的内存缓冲区的指针(如果缓冲区是高端内存区，则为NULL）*/
	char *buffer;

	/*请求的引用计数器*/
	int ref_count;
	/*指向包含请求的请求队列描述符的指针*/
	request_queue_t *q;
	/*指向request_list结构的指针*/
	struct request_list *rl;

	/*等待数据传送终止的completion结构*/
	struct completion *waiting;
	/*对硬件设备发出“特殊”命令的请求所使用的数据的指针*/
	void *special;

	/*
	 * when request is used as a packet command carrier
	 */
	/*cmd字段中命令的长度*/
	unsigned int cmd_len;
	/*由请求队列的prep_rq_fn方法准备好的预先内置命令所在的缓冲区*/
	unsigned char cmd[BLK_MAX_CDB];

	/*通常，由data字段指向的缓冲区中数据的长度*/
	unsigned int data_len;
	/*设备驱动程序为了跟踪所传送的数据而使用的指针*/
	void *data;

	/*由sense字段指向的缓冲区的长度(如果sense是NULL,则为0）*/
	unsigned int sense_len;
	/*指向输出sense命令的缓冲区的指针*/
	void *sense;

	/*请求的超时*/
	unsigned int timeout;

	/*
	 * For Power Management requests
	 */
	/*指向电源管理命令所使用的数据结构*/
	struct request_pm_state *pm;
};

/*
 * first three bits match BIO_RW* bits, important
 */
enum rq_flag_bits {
	__REQ_RW,		/* not set, read. set, write */
	__REQ_FAILFAST,		/* no low level driver retries */
	__REQ_SOFTBARRIER,	/* may not be passed by ioscheduler */
	__REQ_HARDBARRIER,	/* may not be passed by drive either */
	__REQ_CMD,		/* is a regular fs rw request */
	__REQ_NOMERGE,		/* don't touch this for merging */
	__REQ_STARTED,		/* drive already may have started this one */
	__REQ_DONTPREP,		/* don't call prep for this one */
	__REQ_QUEUED,		/* uses queueing */
	/*
	 * for ATA/ATAPI devices
	 */
	__REQ_PC,		/* packet command (special) */
	__REQ_BLOCK_PC,		/* queued down pc from block layer */
	__REQ_SENSE,		/* sense retrival */

	__REQ_FAILED,		/* set if the request failed */
	__REQ_QUIET,		/* don't worry about errors */
	__REQ_SPECIAL,		/* driver suplied command */
	__REQ_DRIVE_CMD,
	__REQ_DRIVE_TASK,
	__REQ_DRIVE_TASKFILE,
	__REQ_PREEMPT,		/* set for "ide_preempt" requests */
	__REQ_PM_SUSPEND,	/* suspend request */
	__REQ_PM_RESUME,	/* resume request */
	__REQ_PM_SHUTDOWN,	/* shutdown request */
	__REQ_BAR_PREFLUSH,	/* barrier pre-flush done */
	__REQ_BAR_POSTFLUSH,	/* barrier post-flush */
	__REQ_NR_BITS,		/* stops here */
};

/*数据传送的方向:READ(0) WRITE(1)*/
#define REQ_RW		(1 << __REQ_RW)
/*万一出错请求申明不再重试I/O操作*/
#define REQ_FAILFAST	(1 << __REQ_FAILFAST)
/*请求相当与I/O调度程序的屏障*/
#define REQ_SOFTBARRIER	(1 << __REQ_SOFTBARRIER)
/*请求相当与I/O调度程序和设备驱动程序的屏障--应当在旧请求与新请求之间处理该请求*/
#define REQ_HARDBARRIER	(1 << __REQ_HARDBARRIER)
/*包含一个标准的读或写I/O数据传送的请求*/
#define REQ_CMD		(1 << __REQ_CMD)
/*不孕需扩展或与其他请求合并的请求*/
#define REQ_NOMERGE	(1 << __REQ_NOMERGE)
/*正处理的请求*/
#define REQ_STARTED	(1 << __REQ_STARTED)
/*不调用请求队列中的prep_rq_fn方法预先准备把命令发送给硬件设备*/
#define REQ_DONTPREP	(1 << __REQ_DONTPREP)
/*请求被标记-也就是说，与该请求相关的硬件设备可以同时管理很多未完成数据的传送*/
#define REQ_QUEUED	(1 << __REQ_QUEUED)
/*请求包含发送给硬件设备的直接命令*/
#define REQ_PC		(1 << __REQ_PC)
/*与前一个标志功能相同，但发送的命令包含在bio结构中*/
#define REQ_BLOCK_PC	(1 << __REQ_BLOCK_PC)
/*请求包含一个sense请求命令(scsi和atapi设备使用)*/
#define REQ_SENSE	(1 << __REQ_SENSE)
/*当请求中的sense或direct命令的操作与预期的不一致时设置该标志*/
#define REQ_FAILED	(1 << __REQ_FAILED)
/*万一I/O操作出错请求申明不产生内核消息*/
#define REQ_QUIET	(1 << __REQ_QUIET)
/*请求包含对该硬件设备的特殊命令(例如，重设驱动器)*/
#define REQ_SPECIAL	(1 << __REQ_SPECIAL)
/*请求包含对IDE硬盘的特殊命令*/
#define REQ_DRIVE_CMD	(1 << __REQ_DRIVE_CMD)
/*请求包含对IDE硬盘的特殊命令*/
#define REQ_DRIVE_TASK	(1 << __REQ_DRIVE_TASK)
/*请求包含对IDE硬盘的特殊命令*/
#define REQ_DRIVE_TASKFILE	(1 << __REQ_DRIVE_TASKFILE)
/*请求取代位于请求队列前面的请求(仅对ide磁盘而言)*/
#define REQ_PREEMPT	(1 << __REQ_PREEMPT)
/*请求包含一个挂起硬件设备的电源管理的命令*/
#define REQ_PM_SUSPEND	(1 << __REQ_PM_SUSPEND)
/*请求包含一个唤醒硬件设备的电源管理命令*/
#define REQ_PM_RESUME	(1 << __REQ_PM_RESUME)
/*请求包含一个切断硬件设备的电源管理命令*/
#define REQ_PM_SHUTDOWN	(1 << __REQ_PM_SHUTDOWN)
/*请求包含一个要发送给磁盘控制器的“刷新队列”命令*/
#define REQ_BAR_PREFLUSH	(1 << __REQ_BAR_PREFLUSH)
/*请求包含一个已发送给磁盘控制器的“刷新队列”命令*/
#define REQ_BAR_POSTFLUSH	(1 << __REQ_BAR_POSTFLUSH)

/*
 * State information carried for REQ_PM_SUSPEND and REQ_PM_RESUME
 * requests. Some step values could eventually be made generic.
 */
struct request_pm_state
{
	/* PM state machine step value, currently driver specific */
	int	pm_step;
	/* requested PM state value (S1, S2, S3, S4, ...) */
	u32	pm_state;
	void*	data;		/* for driver use */
};

#include <linux/elevator.h>

typedef int (merge_request_fn) (request_queue_t *, struct request *,
				struct bio *);
typedef int (merge_requests_fn) (request_queue_t *, struct request *,
				 struct request *);
typedef void (request_fn_proc) (request_queue_t *q);
typedef int (make_request_fn) (request_queue_t *q, struct bio *bio);
typedef int (prep_rq_fn) (request_queue_t *, struct request *);
typedef void (unplug_fn) (request_queue_t *);

struct bio_vec;
typedef int (merge_bvec_fn) (request_queue_t *, struct bio *, struct bio_vec *);
typedef void (activity_fn) (void *data, int rw);
typedef int (issue_flush_fn) (request_queue_t *, struct gendisk *, sector_t *);

enum blk_queue_state {
	Queue_down,
	Queue_up,
};

#define BLK_TAGS_PER_LONG	(sizeof(unsigned long) * 8)
#define BLK_TAGS_MASK		(BLK_TAGS_PER_LONG - 1)

struct blk_queue_tag {
	struct request **tag_index;	/* map of busy tags */
	unsigned long *tag_map;		/* bit map of free/busy tags */
	struct list_head busy_list;	/* fifo list of busy tags */
	int busy;			/* current depth */
	int max_depth;			/* what we will send to device */
	int real_max_depth;		/* what the array can hold */
	atomic_t refcnt;		/* map can be shared */
};

struct request_queue
{
	/*
	 * Together with queue_head for cacheline sharing
	 */
	/*待处理请求的链表*/
	struct list_head	queue_head;
	/*指向队列中首先可能合并的请求的描述符*/
	struct request		*last_merge;
	/*指向elevator对象的指针*/
	elevator_t		*elevator;

	/*
	 * the queue request freelist, one for reads and one for writes
	 */
	/*为分配请求描述符所使用的数据结构*/
	struct request_list	rq;

	/*实现驱动程序的策略历程入口点的方法*/
	request_fn_proc		*request_fn;
	/*检查是否可能将bio合并到请求队列的最后一个请求中的方法*/
	merge_request_fn	*back_merge_fn;
	/*检查是否可能将bio合并到队列的第一个请求中的方法*/
	merge_request_fn	*front_merge_fn;
	/*试图合并请求队列中两个相邻请求的方法*/
	merge_requests_fn	*merge_requests_fn;
	/*将一个新请求插入请求队列时调用的方法*/
	make_request_fn		*make_request_fn;
	/*该方法把这个处理请求的命令发送给硬件设备*/
	prep_rq_fn		*prep_rq_fn;
	/*去掉块设备的方法*/
	unplug_fn		*unplug_fn;
	/*当增加一个新段时，该方法返回可插入到某个已存在的bio结构中的字节数*/
	merge_bvec_fn		*merge_bvec_fn;
	/*将某个请求加入请求队列时调用的方法*/
	activity_fn		*activity_fn;
	/*刷新请求队列时调用的方法(通过连续处理所有的请求清空队列)*/
	issue_flush_fn		*issue_flush_fn;

	/*
	 * Auto-unplugging state
	 */
	/*插入设备时使用的动态定时器*/
	struct timer_list	unplug_timer;
	/*如果请求队列中待处理请求数大于该值，将立即去掉请求设备(缺省值4)*/
	int			unplug_thresh;	/* After this many requests */
	/*去掉设备之前的时间延迟(缺省值是3ms)*/
	unsigned long		unplug_delay;	/* After this many jiffies */
	/*去掉设备时使用的操作队列*/
	struct work_struct	unplug_work;

	/*关于基本硬件块设备的I/O数据流量的信息。例如，保存量关于预读以及关于请求队列拥塞状态的信息*/
	struct backing_dev_info	backing_dev_info;

	/*
	 * The queue owner gets to use this for whatever they like.
	 * ll_rw_blk doesn't touch it.
	 */
	/*指向块设备驱动程序的私有数据结构的指针*/
	void			*queuedata;

	/*active_fn方法使用的私有数据*/
	void			*activity_data;

	/*
	 * queue needs bounce pages for pages above this limit
	 */
	/*在大于该页框号时必须使用的缓冲区回弹*/
	unsigned long		bounce_pfn;
	/*回弹缓冲区的内存分配标志*/
	int			bounce_gfp;

	/*
	 * various queue flags, see QUEUE_* below
	 */
	/*描述请求队列状态的标志*/
	unsigned long		queue_flags;

	/*
	 * protects queue structures from reentrancy
	 */
	/*指向请求队列锁的指针*/
	spinlock_t		*queue_lock;

	/*
	 * queue kobject
	 */
	/*请求队列的内嵌的kobject结构*/
	struct kobject kobj;

	/*
	 * queue settings
	 */
	/*请求队列中允许的最大请求数*/
	unsigned long		nr_requests;	/* Max # of requests */
	/*如果待处理请求数超出了该阀值，则认为该队列是拥挤的*/
	unsigned int		nr_congestion_on;
	/*如果待处理请求数在这个阀值的范围内，则认为该队列是不拥挤的*/
	unsigned int		nr_congestion_off;
	/*即使队列已满，仍可以由特殊进程batcher提交的待处理请求的最大值(通常为32)*/
	unsigned int		nr_batching;

	/*单个请求所能处理的最大扇区数(可调的)*/
	unsigned short		max_sectors;
	/*单个请求所能处理的最大扇区数(硬约束)*/
	unsigned short		max_hw_sectors;
	/*单个请求所能处理的最大物理段数*/
	unsigned short		max_phys_segments;
	/*单个请求所能处理的最大硬段数(分散-聚集DMA操作中的最大不同内存区数)*/
	unsigned short		max_hw_segments;
	/*扇区中以自己为单位的大小*/
	unsigned short		hardsect_size;
	/*物理段的最大长度(以自己为单位)*/
	unsigned int		max_segment_size;

	/*段合并的内存边界屏蔽字*/
	unsigned long		seg_boundary_mask;
	/*DMA缓冲区的起始地址和长度的对齐位图(缺省值为511)*/
	unsigned int		dma_alignment;

	/*空闲/忙标记的位图(用于带标记的请求)blk_queue_tag* */
	struct blk_queue_tag	*queue_tags;

	/*请求队列的引用计数器*/
	atomic_t		refcnt;

	/*请求队列中待处理请求数*/
	unsigned int		in_flight;

	/*
	 * sg stuff
	 */
	/*用户定义的命令超时(仅由scsi通用块设备使用)*/
	unsigned int		sg_timeout;
	/*基本上没有使用*/
	unsigned int		sg_reserved_size;

	/*临时延时的请求链表的首部，知道I/O调度程序被动态取代*/
	struct list_head	drain_list;
};

#define RQ_INACTIVE		(-1)
#define RQ_ACTIVE		1
#define RQ_SCSI_BUSY		0xffff
#define RQ_SCSI_DONE		0xfffe
#define RQ_SCSI_DISCONNECTING	0xffe0

#define QUEUE_FLAG_CLUSTER	0	/* cluster several segments into 1 */
#define QUEUE_FLAG_QUEUED	1	/* uses generic tag queueing */
#define QUEUE_FLAG_STOPPED	2	/* queue is stopped */
#define	QUEUE_FLAG_READFULL	3	/* write queue has been filled */
#define QUEUE_FLAG_WRITEFULL	4	/* read queue has been filled */
#define QUEUE_FLAG_DEAD		5	/* queue being torn down */
#define QUEUE_FLAG_REENTER	6	/* Re-entrancy avoidance */
#define QUEUE_FLAG_PLUGGED	7	/* queue is plugged */
#define QUEUE_FLAG_ORDERED	8	/* supports ordered writes */
#define QUEUE_FLAG_DRAIN	9	/* draining queue for sched switch */

#define blk_queue_plugged(q)	test_bit(QUEUE_FLAG_PLUGGED, &(q)->queue_flags)
#define blk_queue_tagged(q)	test_bit(QUEUE_FLAG_QUEUED, &(q)->queue_flags)
#define blk_queue_stopped(q)	test_bit(QUEUE_FLAG_STOPPED, &(q)->queue_flags)

#define blk_fs_request(rq)	((rq)->flags & REQ_CMD)
#define blk_pc_request(rq)	((rq)->flags & REQ_BLOCK_PC)
#define blk_noretry_request(rq)	((rq)->flags & REQ_FAILFAST)
#define blk_rq_started(rq)	((rq)->flags & REQ_STARTED)

#define blk_account_rq(rq)	(blk_rq_started(rq) && blk_fs_request(rq))

#define blk_pm_suspend_request(rq)	((rq)->flags & REQ_PM_SUSPEND)
#define blk_pm_resume_request(rq)	((rq)->flags & REQ_PM_RESUME)
#define blk_pm_request(rq)	\
	((rq)->flags & (REQ_PM_SUSPEND | REQ_PM_RESUME))

#define blk_barrier_rq(rq)	((rq)->flags & REQ_HARDBARRIER)
#define blk_barrier_preflush(rq)	((rq)->flags & REQ_BAR_PREFLUSH)
#define blk_barrier_postflush(rq)	((rq)->flags & REQ_BAR_POSTFLUSH)

#define list_entry_rq(ptr)	list_entry((ptr), struct request, queuelist)

#define rq_data_dir(rq)		((rq)->flags & 1)

static inline int blk_queue_full(struct request_queue *q, int rw)
{
	if (rw == READ)
		return test_bit(QUEUE_FLAG_READFULL, &q->queue_flags);
	return test_bit(QUEUE_FLAG_WRITEFULL, &q->queue_flags);
}

static inline void blk_set_queue_full(struct request_queue *q, int rw)
{
	if (rw == READ)
		set_bit(QUEUE_FLAG_READFULL, &q->queue_flags);
	else
		set_bit(QUEUE_FLAG_WRITEFULL, &q->queue_flags);
}

static inline void blk_clear_queue_full(struct request_queue *q, int rw)
{
	if (rw == READ)
		clear_bit(QUEUE_FLAG_READFULL, &q->queue_flags);
	else
		clear_bit(QUEUE_FLAG_WRITEFULL, &q->queue_flags);
}


/*
 * mergeable request must not have _NOMERGE or _BARRIER bit set, nor may
 * it already be started by driver.
 */
#define RQ_NOMERGE_FLAGS	\
	(REQ_NOMERGE | REQ_STARTED | REQ_HARDBARRIER | REQ_SOFTBARRIER)
#define rq_mergeable(rq)	\
	(!((rq)->flags & RQ_NOMERGE_FLAGS) && blk_fs_request((rq)))

/*
 * noop, requests are automagically marked as active/inactive by I/O
 * scheduler -- see elv_next_request
 */
#define blk_queue_headactive(q, head_active)

/*
 * q->prep_rq_fn return values
 */
#define BLKPREP_OK		0	/* serve it */
#define BLKPREP_KILL		1	/* fatal error, kill */
#define BLKPREP_DEFER		2	/* leave on queue */

extern unsigned long blk_max_low_pfn, blk_max_pfn;

/*
 * standard bounce addresses:
 *
 * BLK_BOUNCE_HIGH	: bounce all highmem pages
 * BLK_BOUNCE_ANY	: don't bounce anything
 * BLK_BOUNCE_ISA	: bounce pages above ISA DMA boundary
 */
#define BLK_BOUNCE_HIGH		((u64)blk_max_low_pfn << PAGE_SHIFT)
#define BLK_BOUNCE_ANY		((u64)blk_max_pfn << PAGE_SHIFT)
#define BLK_BOUNCE_ISA		(ISA_DMA_THRESHOLD)

#ifdef CONFIG_MMU
extern int init_emergency_isa_pool(void);
extern void blk_queue_bounce(request_queue_t *q, struct bio **bio);
#else
static inline int init_emergency_isa_pool(void)
{
	return 0;
}
static inline void blk_queue_bounce(request_queue_t *q, struct bio **bio)
{
}
#endif /* CONFIG_MMU */

#define rq_for_each_bio(_bio, rq)	\
	if ((rq->bio))			\
		for (_bio = (rq)->bio; _bio; _bio = _bio->bi_next)

struct sec_size {
	unsigned block_size;
	unsigned block_size_bits;
};

extern int blk_register_queue(struct gendisk *disk);
extern void blk_unregister_queue(struct gendisk *disk);
extern void register_disk(struct gendisk *dev);
extern void generic_make_request(struct bio *bio);
extern void blk_put_request(struct request *);
extern void blk_attempt_remerge(request_queue_t *, struct request *);
extern void __blk_attempt_remerge(request_queue_t *, struct request *);
extern struct request *blk_get_request(request_queue_t *, int, int);
extern void blk_put_request(struct request *);
extern void blk_insert_request(request_queue_t *, struct request *, int, void *, int);
extern void blk_requeue_request(request_queue_t *, struct request *);
extern void blk_plug_device(request_queue_t *);
extern int blk_remove_plug(request_queue_t *);
extern void blk_recount_segments(request_queue_t *, struct bio *);
extern int blk_phys_contig_segment(request_queue_t *q, struct bio *, struct bio *);
extern int blk_hw_contig_segment(request_queue_t *q, struct bio *, struct bio *);
extern int scsi_cmd_ioctl(struct file *, struct gendisk *, unsigned int, void __user *);
extern void blk_start_queue(request_queue_t *q);
extern void blk_stop_queue(request_queue_t *q);
extern void blk_sync_queue(struct request_queue *q);
extern void __blk_stop_queue(request_queue_t *q);
extern void blk_run_queue(request_queue_t *);
extern void blk_queue_activity_fn(request_queue_t *, activity_fn *, void *);
extern struct request *blk_rq_map_user(request_queue_t *, int, void __user *, unsigned int);
extern int blk_rq_unmap_user(struct request *, struct bio *, unsigned int);
extern int blk_execute_rq(request_queue_t *, struct gendisk *, struct request *);

static inline request_queue_t *bdev_get_queue(struct block_device *bdev)
{
	return bdev->bd_disk->queue;
}

static inline void blk_run_backing_dev(struct backing_dev_info *bdi,
				       struct page *page)
{
	if (bdi && bdi->unplug_io_fn)
		bdi->unplug_io_fn(bdi, page);
}

static inline void blk_run_address_space(struct address_space *mapping)
{
	if (mapping)
		blk_run_backing_dev(mapping->backing_dev_info, NULL);
}

/*
 * end_request() and friends. Must be called with the request queue spinlock
 * acquired. All functions called within end_request() _must_be_ atomic.
 *
 * Several drivers define their own end_request and call
 * end_that_request_first() and end_that_request_last()
 * for parts of the original function. This prevents
 * code duplication in drivers.
 */
extern int end_that_request_first(struct request *, int, int);
extern int end_that_request_chunk(struct request *, int, int);
extern void end_that_request_last(struct request *);
extern void end_request(struct request *req, int uptodate);

/*
 * end_that_request_first/chunk() takes an uptodate argument. we account
 * any value <= as an io error. 0 means -EIO for compatability reasons,
 * any other < 0 value is the direct error type. An uptodate value of
 * 1 indicates successful io completion
 */
#define end_io_error(uptodate)	(unlikely((uptodate) <= 0))

static inline void blkdev_dequeue_request(struct request *req)
{
	BUG_ON(list_empty(&req->queuelist));

	list_del_init(&req->queuelist);

	if (req->rl)
		elv_remove_request(req->q, req);
}

/*
 * Access functions for manipulating queue properties
 */
extern request_queue_t *blk_init_queue(request_fn_proc *, spinlock_t *);
extern void blk_cleanup_queue(request_queue_t *);
extern void blk_queue_make_request(request_queue_t *, make_request_fn *);
extern void blk_queue_bounce_limit(request_queue_t *, u64);
extern void blk_queue_max_sectors(request_queue_t *, unsigned short);
extern void blk_queue_max_phys_segments(request_queue_t *, unsigned short);
extern void blk_queue_max_hw_segments(request_queue_t *, unsigned short);
extern void blk_queue_max_segment_size(request_queue_t *, unsigned int);
extern void blk_queue_hardsect_size(request_queue_t *, unsigned short);
extern void blk_queue_stack_limits(request_queue_t *t, request_queue_t *b);
extern void blk_queue_segment_boundary(request_queue_t *, unsigned long);
extern void blk_queue_prep_rq(request_queue_t *, prep_rq_fn *pfn);
extern void blk_queue_merge_bvec(request_queue_t *, merge_bvec_fn *);
extern void blk_queue_dma_alignment(request_queue_t *, int);
extern struct backing_dev_info *blk_get_backing_dev_info(struct block_device *bdev);
extern void blk_queue_ordered(request_queue_t *, int);
extern void blk_queue_issue_flush_fn(request_queue_t *, issue_flush_fn *);
extern int blkdev_scsi_issue_flush_fn(request_queue_t *, struct gendisk *, sector_t *);

extern int blk_rq_map_sg(request_queue_t *, struct request *, struct scatterlist *);
extern void blk_dump_rq_flags(struct request *, char *);
extern void generic_unplug_device(request_queue_t *);
extern void __generic_unplug_device(request_queue_t *);
extern long nr_blockdev_pages(void);
extern void blk_wait_queue_drained(request_queue_t *, int);
extern void blk_finish_queue_drain(request_queue_t *);

int blk_get_queue(request_queue_t *);
request_queue_t *blk_alloc_queue(int);
#define blk_put_queue(q) blk_cleanup_queue((q))

/*
 * tag stuff
 */
#define blk_queue_tag_depth(q)		((q)->queue_tags->busy)
#define blk_queue_tag_queue(q)		((q)->queue_tags->busy < (q)->queue_tags->max_depth)
#define blk_rq_tagged(rq)		((rq)->flags & REQ_QUEUED)
extern int blk_queue_start_tag(request_queue_t *, struct request *);
extern struct request *blk_queue_find_tag(request_queue_t *, int);
extern void blk_queue_end_tag(request_queue_t *, struct request *);
extern int blk_queue_init_tags(request_queue_t *, int, struct blk_queue_tag *);
extern void blk_queue_free_tags(request_queue_t *);
extern int blk_queue_resize_tags(request_queue_t *, int);
extern void blk_queue_invalidate_tags(request_queue_t *);
extern long blk_congestion_wait(int rw, long timeout);

extern void blk_rq_bio_prep(request_queue_t *, struct request *, struct bio *);
extern int blkdev_issue_flush(struct block_device *, sector_t *);

#define MAX_PHYS_SEGMENTS 128
#define MAX_HW_SEGMENTS 128
#define MAX_SECTORS 255

#define MAX_SEGMENT_SIZE	65536

#define blkdev_entry_to_request(entry) list_entry((entry), struct request, queuelist)

extern void drive_stat_acct(struct request *, int, int);

static inline int queue_hardsect_size(request_queue_t *q)
{
	int retval = 512;

	if (q && q->hardsect_size)
		retval = q->hardsect_size;

	return retval;
}

static inline int bdev_hardsect_size(struct block_device *bdev)
{
	return queue_hardsect_size(bdev_get_queue(bdev));
}

static inline int queue_dma_alignment(request_queue_t *q)
{
	int retval = 511;

	if (q && q->dma_alignment)
		retval = q->dma_alignment;

	return retval;
}

static inline int bdev_dma_aligment(struct block_device *bdev)
{
	return queue_dma_alignment(bdev_get_queue(bdev));
}

#define blk_finished_io(nsects)	do { } while (0)
#define blk_started_io(nsects)	do { } while (0)

/* assumes size > 256 */
static inline unsigned int blksize_bits(unsigned int size)
{
	unsigned int bits = 8;
	do {
		bits++;
		size >>= 1;
	} while (size > 256);
	return bits;
}

extern inline unsigned int block_size(struct block_device *bdev)
{
	return bdev->bd_block_size;
}

typedef struct {struct page *v;} Sector;

unsigned char *read_dev_sector(struct block_device *, sector_t, Sector *);

static inline void put_dev_sector(Sector p)
{
	page_cache_release(p.v);
}

struct work_struct;
int kblockd_schedule_work(struct work_struct *work);
void kblockd_flush(void);

#ifdef CONFIG_LBD
# include <asm/div64.h>
# define sector_div(a, b) do_div(a, b)
#else
# define sector_div(n, b)( \
{ \
	int _res; \
	_res = (n) % (b); \
	(n) /= (b); \
	_res; \
} \
)
#endif 

#define MODULE_ALIAS_BLOCKDEV(major,minor) \
	MODULE_ALIAS("block-major-" __stringify(major) "-" __stringify(minor))
#define MODULE_ALIAS_BLOCKDEV_MAJOR(major) \
	MODULE_ALIAS("block-major-" __stringify(major) "-*")


#endif
