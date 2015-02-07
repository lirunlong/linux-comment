#ifndef _LINUX_FS_H
#define _LINUX_FS_H

/*
 * This file has definitions for some important file table
 * structures etc.
 */

#include <linux/config.h>
#include <linux/limits.h>
#include <linux/ioctl.h>

/*
 * It's silly to have NR_OPEN bigger than NR_FILE, but you can change
 * the file limit at runtime and only root can increase the per-process
 * nr_file rlimit, so it's safe to set up a ridiculously high absolute
 * upper limit on files-per-process.
 *
 * Some programs (notably those using select()) may have to be 
 * recompiled to take full advantage of the new limits..  
 */

/* Fixed constants first: */
#undef NR_OPEN
#define NR_OPEN (1024*1024)	/* Absolute upper limit on fd num */
#define INR_OPEN 1024		/* Initial setting for nfile rlimits */

#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)

/* And dynamically-tunable limits and defaults: */
struct files_stat_struct {
	int nr_files;		/* read only */
	int nr_free_files;	/* read only */
	int max_files;		/* tunable */
};
extern struct files_stat_struct files_stat;

struct inodes_stat_t {
	int nr_inodes;
	int nr_unused;
	int dummy[5];
};
extern struct inodes_stat_t inodes_stat;

extern int leases_enable, lease_break_time;

#ifdef CONFIG_DNOTIFY
extern int dir_notify_enable;
#endif

#define NR_FILE  8192	/* this can well be larger on a larger system */

#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4
#define MAY_APPEND 8

#define FMODE_READ 1
#define FMODE_WRITE 2

/* Internal kernel extensions */
#define FMODE_LSEEK	4
#define FMODE_PREAD	8
#define FMODE_PWRITE	FMODE_PREAD	/* These go hand in hand */

#define RW_MASK		1
#define RWA_MASK	2
#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead  - don't block if no resources */
#define SPECIAL 4	/* For non-blockdevice requests in request queue */
#define READ_SYNC	(READ | (1 << BIO_RW_SYNC))
#define WRITE_SYNC	(WRITE | (1 << BIO_RW_SYNC))
#define WRITE_BARRIER	((1 << BIO_RW) | (1 << BIO_RW_BARRIER))

#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

/* public flags for file_system_type */
/*这种类型的任何文件系统必须位于物理磁盘卷设备上*/
#define FS_REQUIRES_DEV 1 
/*文件系统使用的2进制安装数据*/
#define FS_BINARY_MOUNTDATA 2
/*时钟在目录项告诉缓存中使用.或..路径重新生效(针对网络文件系统)*/
#define FS_REVAL_DOT	16384	/* Check the paths ".", ".." for staleness */
/*“重命名”操作就是“移动”操作(针对网络文件系统）*/
#define FS_ODD_RENAME	32768	/* Temporary stuff; will go away as soon
				  * as nfs_rename() will be cleaned up
				  */
/*
 * These are the fs-independent mount-flags: up to 32 flags are supported
 */
/*文件只能被读*/
#define MS_RDONLY	 1	/* Mount read-only */
/*禁止setuid和setgid标志*/
#define MS_NOSUID	 2	/* Ignore suid and sgid bits */
/*禁止访问设备文件*/
#define MS_NODEV	 4	/* Disallow access to device special files */
/*不允许程序执行*/
#define MS_NOEXEC	 8	/* Disallow program execution */
/*文件和目录上的写操作是即时的*/
#define MS_SYNCHRONOUS	16	/* Writes are synced at once */
/*重新安装改变来安装标志的文件系统*/
#define MS_REMOUNT	32	/* Alter flags of a mounted FS */
/*允许强制加锁*/
#define MS_MANDLOCK	64	/* Allow mandatory locks on an FS */
/*目录上的写操作是即时的*/
#define MS_DIRSYNC	128	/* Directory modifications are synchronous */
/*不更新文件访问时间*/
#define MS_NOATIME	1024	/* Do not update access times. */
/*不更新目录访问时间*/
#define MS_NODIRATIME	2048	/* Do not update directory access times */
/*创建一个“绑定安装”，这就使得一个文件或目录在系统目录树的另外一个点上可以看得见(mount 命令的__bind选项)*/
#define MS_BIND		4096
/*自动把一个已安装文件系统移动到另一个安装点(mount命令的__move选项)*/
#define MS_MOVE		8192
/*为目录子树递归地创建绑定安装*/
#define MS_REC		16384
/*在安装出错时产生内核消息*/
#define MS_VERBOSE	32768
#define MS_POSIXACL	(1<<16)	/* VFS does not apply the umask */
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)

/*
 * Superblock flags that can be altered by MS_REMOUNT
 */
#define MS_RMT_MASK	(MS_RDONLY|MS_SYNCHRONOUS|MS_MANDLOCK|MS_NOATIME|\
			 MS_NODIRATIME)

/*
 * Old magic mount flag and mask
 */
#define MS_MGC_VAL 0xC0ED0000
#define MS_MGC_MSK 0xffff0000

/* Inode flags - they have nothing to superblock flags now */

#define S_SYNC		1	/* Writes are synced at once */
#define S_NOATIME	2	/* Do not update access times */
#define S_APPEND	4	/* Append-only file */
#define S_IMMUTABLE	8	/* Immutable file */
#define S_DEAD		16	/* removed, but still open directory */
#define S_NOQUOTA	32	/* Inode is not counted to quota */
#define S_DIRSYNC	64	/* Directory modifications are synchronous */
#define S_NOCMTIME	128	/* Do not update file c/mtime */
#define S_SWAPFILE	256	/* Do not truncate: swapon got its bmaps */

/*
 * Note that nosuid etc flags are inode-specific: setting some file-system
 * flags just means all the inodes inherit those flags by default. It might be
 * possible to override it selectively if you really wanted to with some
 * ioctl() that is not currently implemented.
 *
 * Exception: MS_RDONLY is always applied to the entire file system.
 *
 * Unfortunately, it is possible to change a filesystems flags with it mounted
 * with files in use.  This means that all of the inodes will not have their
 * i_flags updated.  Hence, i_flags no longer inherit the superblock mount
 * flags, so these have to be checked separately. -- rmk@arm.uk.linux.org
 */
#define __IS_FLG(inode,flg) ((inode)->i_sb->s_flags & (flg))

#define IS_RDONLY(inode) ((inode)->i_sb->s_flags & MS_RDONLY)
#define IS_SYNC(inode)		(__IS_FLG(inode, MS_SYNCHRONOUS) || \
					((inode)->i_flags & S_SYNC))
#define IS_DIRSYNC(inode)	(__IS_FLG(inode, MS_SYNCHRONOUS|MS_DIRSYNC) || \
					((inode)->i_flags & (S_SYNC|S_DIRSYNC)))
#define IS_MANDLOCK(inode)	__IS_FLG(inode, MS_MANDLOCK)

#define IS_NOQUOTA(inode)	((inode)->i_flags & S_NOQUOTA)
#define IS_APPEND(inode)	((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode)	((inode)->i_flags & S_IMMUTABLE)
#define IS_NOATIME(inode)	(__IS_FLG(inode, MS_NOATIME) || ((inode)->i_flags & S_NOATIME))
#define IS_NODIRATIME(inode)	__IS_FLG(inode, MS_NODIRATIME)
#define IS_POSIXACL(inode)	__IS_FLG(inode, MS_POSIXACL)

#define IS_DEADDIR(inode)	((inode)->i_flags & S_DEAD)
#define IS_NOCMTIME(inode)	((inode)->i_flags & S_NOCMTIME)
#define IS_SWAPFILE(inode)	((inode)->i_flags & S_SWAPFILE)

/* the read-only stuff doesn't really belong here, but any other place is
   probably as bad and I don't want to create yet another include file. */

#define BLKROSET   _IO(0x12,93)	/* set device read-only (0 = read-write) */
#define BLKROGET   _IO(0x12,94)	/* get read-only status (0 = read_write) */
#define BLKRRPART  _IO(0x12,95)	/* re-read partition table */
#define BLKGETSIZE _IO(0x12,96)	/* return device size /512 (long *arg) */
#define BLKFLSBUF  _IO(0x12,97)	/* flush buffer cache */
#define BLKRASET   _IO(0x12,98)	/* set read ahead for block device */
#define BLKRAGET   _IO(0x12,99)	/* get current read ahead setting */
#define BLKFRASET  _IO(0x12,100)/* set filesystem (mm/filemap.c) read-ahead */
#define BLKFRAGET  _IO(0x12,101)/* get filesystem (mm/filemap.c) read-ahead */
#define BLKSECTSET _IO(0x12,102)/* set max sectors per request (ll_rw_blk.c) */
#define BLKSECTGET _IO(0x12,103)/* get max sectors per request (ll_rw_blk.c) */
#define BLKSSZGET  _IO(0x12,104)/* get block device sector size */
#if 0
#define BLKPG      _IO(0x12,105)/* See blkpg.h */

/* Some people are morons.  Do not use sizeof! */

#define BLKELVGET  _IOR(0x12,106,size_t)/* elevator get */
#define BLKELVSET  _IOW(0x12,107,size_t)/* elevator set */
/* This was here just to show that the number is taken -
   probably all these _IO(0x12,*) ioctls should be moved to blkpg.h. */
#endif
/* A jump here: 108-111 have been used for various private purposes. */
#define BLKBSZGET  _IOR(0x12,112,size_t)
#define BLKBSZSET  _IOW(0x12,113,size_t)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)	/* return device size in bytes (u64 *arg) */

#define BMAP_IOCTL 1		/* obsolete - kept for compatibility */
#define FIBMAP	   _IO(0x00,1)	/* bmap access */
#define FIGETBSZ   _IO(0x00,2)	/* get the block size used for bmap */

#ifdef __KERNEL__

#include <linux/linkage.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/dcache.h>
#include <linux/stat.h>
#include <linux/cache.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/radix-tree.h>
#include <linux/prio_tree.h>
#include <linux/audit.h>
#include <linux/init.h>

#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <asm/byteorder.h>

struct iovec;
struct nameidata;
struct pipe_inode_info;
struct poll_table_struct;
struct kstatfs;
struct vm_area_struct;
struct vfsmount;

/* Used to be a macro which just called the function, now just a function */
extern void update_atime (struct inode *);

extern void __init inode_init(unsigned long);
extern void __init inode_init_early(void);
extern void __init mnt_init(unsigned long);
extern void __init files_init(unsigned long);

struct buffer_head;
typedef int (get_block_t)(struct inode *inode, sector_t iblock,
			struct buffer_head *bh_result, int create);
typedef int (get_blocks_t)(struct inode *inode, sector_t iblock,
			unsigned long max_blocks,
			struct buffer_head *bh_result, int create);
typedef void (dio_iodone_t)(struct inode *inode, loff_t offset,
			ssize_t bytes, void *private);

/*
 * Attribute flags.  These should be or-ed together to figure out what
 * has been changed!
 */
#define ATTR_MODE	1
#define ATTR_UID	2
#define ATTR_GID	4
#define ATTR_SIZE	8
#define ATTR_ATIME	16
#define ATTR_MTIME	32
#define ATTR_CTIME	64
#define ATTR_ATIME_SET	128
#define ATTR_MTIME_SET	256
#define ATTR_FORCE	512	/* Not a change, but a change it */
#define ATTR_ATTR_FLAG	1024
#define ATTR_KILL_SUID	2048
#define ATTR_KILL_SGID	4096

/*
 * This is the Inode Attributes structure, used for notify_change().  It
 * uses the above definitions as flags, to know which values have changed.
 * Also, in this manner, a Filesystem can look at only the values it cares
 * about.  Basically, these are the attributes that the VFS layer can
 * request to change from the FS layer.
 *
 * Derek Atkins <warlord@MIT.EDU> 94-10-20
 */
struct iattr {
	unsigned int	ia_valid;
	umode_t		ia_mode;
	uid_t		ia_uid;
	gid_t		ia_gid;
	loff_t		ia_size;
	struct timespec	ia_atime;
	struct timespec	ia_mtime;
	struct timespec	ia_ctime;
	unsigned int	ia_attr_flags;
};

/*
 * This is the inode attributes flag definitions
 */
#define ATTR_FLAG_SYNCRONOUS	1 	/* Syncronous write */
#define ATTR_FLAG_NOATIME	2 	/* Don't update atime */
#define ATTR_FLAG_APPEND	4 	/* Append-only file */
#define ATTR_FLAG_IMMUTABLE	8 	/* Immutable file */
#define ATTR_FLAG_NODIRATIME	16 	/* Don't update atime for directory */

/*
 * Includes for diskquotas.
 */
#include <linux/quota.h>

/*
 * oh the beauties of C type declarations.
 */
struct page;
struct address_space;
struct writeback_control;
struct kiocb;

struct address_space_operations {
	int (*writepage)(struct page *page, struct writeback_control *wbc);
	int (*readpage)(struct file *, struct page *);
	int (*sync_page)(struct page *);

	/* Write back some dirty pages from this mapping. */
	int (*writepages)(struct address_space *, struct writeback_control *);

	/* Set a page dirty */
	int (*set_page_dirty)(struct page *page);

	int (*readpages)(struct file *filp, struct address_space *mapping,
			struct list_head *pages, unsigned nr_pages);

	/*
	 * ext3 requires that a successful prepare_write() call be followed
	 * by a commit_write() call - they must be balanced
	 */
	int (*prepare_write)(struct file *, struct page *, unsigned, unsigned);
	int (*commit_write)(struct file *, struct page *, unsigned, unsigned);
	/* Unfortunately this kludge is needed for FIBMAP. Don't use it */
	sector_t (*bmap)(struct address_space *, sector_t);
	int (*invalidatepage) (struct page *, unsigned long);
	int (*releasepage) (struct page *, int);
	ssize_t (*direct_IO)(int, struct kiocb *, const struct iovec *iov,
			loff_t offset, unsigned long nr_segs);
};

struct backing_dev_info;
struct address_space {
	/*指向拥有该对象的索引结点的指针*/
	struct inode		*host;		/* owner: inode, block_device */
	/*表示拥有者页的基数的根*/
	struct radix_tree_root	page_tree;	/* radix tree of all pages */
	/*保护基树的自旋锁*/
	spinlock_t		tree_lock;	/* and spinlock protecting it */
	/*地址空间中共享内存映射的个数*/
	unsigned int		i_mmap_writable;/* count VM_SHARED mappings */
	/*radix优先搜索树的根*/
	struct prio_tree_root	i_mmap;		/* tree of private and shared mappings */
	/*地址空间中非线性内存区的链表*/
	struct list_head	i_mmap_nonlinear;/*list VM_NONLINEAR mappings */
	/*保护radix优先搜索树的自旋锁*/
	spinlock_t		i_mmap_lock;	/* protect tree, count, list */
	/*截断文件时使用的顺序计数器*/
	unsigned int		truncate_count;	/* Cover race condition with truncate */
	/*所有者的页总数*/
	unsigned long		nrpages;	/* number of total pages */
	/*最后一次回写操作所作用的页的索引*/
	pgoff_t			writeback_index;/* writeback starts here */
	/*对所有者页进行操作的方法*/
	struct address_space_operations *a_ops;	/* methods */
	/*错误位和内存分配器的标志*/
	unsigned long		flags;		/* error bits/gfp mask */
	/*指向拥有者所有者数据的块设备的backing_dev_info的指针*/
	struct backing_dev_info *backing_dev_info; /* device readahead, etc */
	/*通常是管理private_list链表时使用的自旋锁*/
	spinlock_t		private_lock;	/* for use by the address_space */
	/*通常是与索引节点相关的间接块的脏缓冲区的链表*/
	struct list_head	private_list;	/* ditto */
	/*通常是指向间接块所在块设备的address_space对象的指针*/
	struct address_space	*assoc_mapping;	/* ditto */
} __attribute__((aligned(sizeof(long))));
	/*
	 * On most architectures that alignment is already the case; but
	 * must be enforced here for CRIS, to let the least signficant bit
	 * of struct page's "mapping" pointer be used for PAGE_MAPPING_ANON.
	 */

struct block_device {
	dev_t			bd_dev;  /* not a kdev_t - it's a search key */
	struct inode *		bd_inode;	/* will die */
	int			bd_openers;
	struct semaphore	bd_sem;	/* open/close mutex */
	struct semaphore	bd_mount_sem;	/* mount mutex */
	struct list_head	bd_inodes;
	void *			bd_holder;
	int			bd_holders;
	struct block_device *	bd_contains;
	unsigned		bd_block_size;
	struct hd_struct *	bd_part;
	/* number of times partitions within this device have been opened. */
	unsigned		bd_part_count;
	int			bd_invalidated;
	struct gendisk *	bd_disk;
	struct list_head	bd_list;
	struct backing_dev_info *bd_inode_backing_dev_info;
	/*
	 * Private data.  You must have bd_claim'ed the block_device
	 * to use this.  NOTE:  bd_claim allows an owner to claim
	 * the same device multiple times, the owner must take special
	 * care to not mess up bd_private for that case.
	 */
	unsigned long		bd_private;
};

/*
 * Radix-tree tags, for tagging dirty and writeback pages within the pagecache
 * radix trees
 */
#define PAGECACHE_TAG_DIRTY	0
#define PAGECACHE_TAG_WRITEBACK	1

int mapping_tagged(struct address_space *mapping, int tag);

/*
 * Might pages of this file be mapped into userspace?
 */
static inline int mapping_mapped(struct address_space *mapping)
{
	return	!prio_tree_empty(&mapping->i_mmap) ||
		!list_empty(&mapping->i_mmap_nonlinear);
}

/*
 * Might pages of this file have been modified in userspace?
 * Note that i_mmap_writable counts all VM_SHARED vmas: do_mmap_pgoff
 * marks vma as VM_SHARED if it is shared, and the file was opened for
 * writing i.e. vma may be mprotected writable even if now readonly.
 */
static inline int mapping_writably_mapped(struct address_space *mapping)
{
	return mapping->i_mmap_writable != 0;
}

/*
 * Use sequence counter to get consistent i_size on 32-bit processors.
 */
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
#include <linux/seqlock.h>
#define __NEED_I_SIZE_ORDERED
#define i_size_ordered_init(inode) seqcount_init(&inode->i_size_seqcount)
#else
#define i_size_ordered_init(inode) do { } while (0)
#endif

struct inode {

	/*用于散列链表的指针*/
	struct hlist_node	i_hash;
	/*用于描述索引节点当前状态的链表的指针*/
	struct list_head	i_list;
	/*用于超级块的索引节点的链表的指针*/
	struct list_head	i_sb_list;
	/*引用索引节点的目录项对象链表的头*/
	struct list_head	i_dentry;
	/*索引节点号*/
	unsigned long		i_ino;
	/*引用计数器*/
	atomic_t		i_count;
	/*文件类型与访问权限*/
	umode_t			i_mode;
	/*硬链接数目*/
	unsigned int		i_nlink;
	/*所有者标识符*/
	uid_t			i_uid;
	/*组标识符*/
	gid_t			i_gid;
	/*实设备标识符*/
	dev_t			i_rdev;
	/*文件的字节数*/
	loff_t			i_size;
	/*上次访问文件的时间*/
	struct timespec		i_atime;
	/*上次写文件的时间*/
	struct timespec		i_mtime;
	/*上次修改索引节点的时间*/
	struct timespec		i_ctime;
	/*块的位数*/
	unsigned int		i_blkbits;
	/*块的字节数*/
	unsigned long		i_blksize;
	/*版本号(每次使用后自动递增)*/
	unsigned long		i_version;
	/*文件的块数*/
	unsigned long		i_blocks;
	/*文件中最后一个块的字节数*/
	unsigned short          i_bytes;
	/*如果文件是一个套接字则为非0*/
	unsigned char		i_sock;
	/*保护索引节点的一些字段的自旋锁*/
	spinlock_t		i_lock;	/* i_blocks, i_bytes, maybe i_size */
	/*索引节点信号量*/
	struct semaphore	i_sem;
	/*在直接I/O文件操作中避免出现竞争条件的读/写信号量*/
	struct rw_semaphore	i_alloc_sem;
	/*索引节点的操作*/
	struct inode_operations	*i_op;
	/*缺省文件的操作*/
	struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	/*指向超级块对象的指针*/
	struct super_block	*i_sb;
	/*指向文件锁链表的指针*/
	struct file_lock	*i_flock;
	/*指向address_space对象的指针*/
	struct address_space	*i_mapping;
	/*文件的address_space对象*/
	struct address_space	i_data;
#ifdef CONFIG_QUOTA
	/*索引节点磁盘配额*/
	struct dquot		*i_dquot[MAXQUOTAS];
#endif
	/* These three should probably be a union */
	/*用于具体的字符或块设备索引节点链表的指针*/
	struct list_head	i_devices;
	/*如果文件是一个管道则使用它*/
	struct pipe_inode_info	*i_pipe;
	/*指向块设备驱动程序的指针*/
	struct block_device	*i_bdev;
	/*指向字符设备驱动程序的指针*/
	struct cdev		*i_cdev;
	/*指向一组次设备号的设备文件的索引*/
	int			i_cindex;

	/*索引节点版本号(由某些文件系统使用)*/
	__u32			i_generation;

#ifdef CONFIG_DNOTIFY
	/*目录通知时间的位掩码*/
	unsigned long		i_dnotify_mask; /* Directory notify events */
	/*用于目录通知*/
	struct dnotify_struct	*i_dnotify; /* for directory notifications */
#endif

	/*索引节点的状态标志*/
	unsigned long		i_state;
	/*索引节点的弄脏时间*/
	unsigned long		dirtied_when;	/* jiffies of first dirtying */

	/*文件系统的安装标志*/
	unsigned int		i_flags;

	/*用于写进程的引用计数器*/
	atomic_t		i_writecount;
	/*指向索引节点安全结构的指针*/
	void			*i_security;
	union {
		/*指向私有数据结构的指针*/
		void		*generic_ip;
	} u;
#ifdef __NEED_I_SIZE_ORDERED
	/*SMP系统为i_size字段获取一致值时使用的顺序计数器*/
	seqcount_t		i_size_seqcount;
#endif
};

/*
 * NOTE: in a 32bit arch with a preemptable kernel and
 * an UP compile the i_size_read/write must be atomic
 * with respect to the local cpu (unlike with preempt disabled),
 * but they don't need to be atomic with respect to other cpus like in
 * true SMP (so they need either to either locally disable irq around
 * the read or for example on x86 they can be still implemented as a
 * cmpxchg8b without the need of the lock prefix). For SMP compiles
 * and 64bit archs it makes no difference if preempt is enabled or not.
 */
static inline loff_t i_size_read(struct inode *inode)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	loff_t i_size;
	unsigned int seq;

	do {
		seq = read_seqcount_begin(&inode->i_size_seqcount);
		i_size = inode->i_size;
	} while (read_seqcount_retry(&inode->i_size_seqcount, seq));
	return i_size;
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	loff_t i_size;

	preempt_disable();
	i_size = inode->i_size;
	preempt_enable();
	return i_size;
#else
	return inode->i_size;
#endif
}


static inline void i_size_write(struct inode *inode, loff_t i_size)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	write_seqcount_begin(&inode->i_size_seqcount);
	inode->i_size = i_size;
	write_seqcount_end(&inode->i_size_seqcount);
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	preempt_disable();
	inode->i_size = i_size;
	preempt_enable();
#else
	inode->i_size = i_size;
#endif
}

static inline unsigned iminor(struct inode *inode)
{
	return MINOR(inode->i_rdev);
}

static inline unsigned imajor(struct inode *inode)
{
	return MAJOR(inode->i_rdev);
}

extern struct block_device *I_BDEV(struct inode *inode);

struct fown_struct {
	rwlock_t lock;          /* protects pid, uid, euid fields */
	int pid;		/* pid or -pgrp where SIGIO should be sent */
	uid_t uid, euid;	/* uid/euid of process setting the owner */
	void *security;
	int signum;		/* posix.1b rt signal to be delivered on IO */
};

/*
 * Track a single file's readahead state
 */
struct file_ra_state {
	unsigned long start;		/* Current window */
	unsigned long size;
	unsigned long flags;		/* ra flags RA_FLAG_xxx*/
	unsigned long cache_hit;	/* cache hit count*/
	unsigned long prev_page;	/* Cache last read() position */
	unsigned long ahead_start;	/* Ahead window */
	unsigned long ahead_size;
	unsigned long ra_pages;		/* Maximum readahead window */
	unsigned long mmap_hit;		/* Cache hit stat for mmap accesses */
	unsigned long mmap_miss;	/* Cache miss stat for mmap accesses */
};
#define RA_FLAG_MISS 0x01	/* a cache miss occured against this file */
#define RA_FLAG_INCACHE 0x02	/* file is already in cache */

struct file {
	/*用于通用文件对象链表的指针*/
	struct list_head	f_list;
	/*与文件相关的目录项对象*/
	struct dentry		*f_dentry;
	/*含有该文件的已安装文件系统*/
	struct vfsmount         *f_vfsmnt;
	/*指向文件操作表的指针*/
	struct file_operations	*f_op;
	/*文件对象的引用计数*/
	atomic_t		f_count;
	/*当打开文件时所指定的标志*/
	unsigned int 		f_flags;
	/*进程的访问模式*/
	mode_t			f_mode;
	/*网络写操作的错误码*/
	int			f_error;
	/*当前的文件位移量*/
	loff_t			f_pos;
	/*通过信号进行I/O时间通知的数据*/
	struct fown_struct	f_owner;
	/*用户的uid，gid*/
	unsigned int		f_uid, f_gid;
	/*文件预读状态*/
	struct file_ra_state	f_ra;

	/*一次单一的操作所能读或写的最大字节数*/
	size_t			f_maxcount;
	/*版本号，每次使用后自动递增*/
	unsigned long		f_version;
	/*指向文件对象的安全结构的指针*/
	void			*f_security;

	/* needed for tty driver, and maybe others */
	/*指向特定文件系统或设备驱动程序所需的数据的指针*/
	void			*private_data;

#ifdef CONFIG_EPOLL
	/* Used by fs/eventpoll.c to link all the hooks to this file */
	/*文件的时间轮询等待者链表的头*/
	struct list_head	f_ep_links;
	/*保护f_ep_links链表的自旋锁*/
	spinlock_t		f_ep_lock;
#endif /* #ifdef CONFIG_EPOLL */
	/*指向文件地址空间对象的指针*/
	struct address_space	*f_mapping;
};
extern spinlock_t files_lock;
#define file_list_lock() spin_lock(&files_lock);
#define file_list_unlock() spin_unlock(&files_lock);

#define get_file(x)	atomic_inc(&(x)->f_count)
#define file_count(x)	atomic_read(&(x)->f_count)

#define	MAX_NON_LFS	((1UL<<31) - 1)

/* Page cache limit. The filesystems should put that into their s_maxbytes 
   limits, otherwise bad things can happen in VM. */ 
#if BITS_PER_LONG==32
#define MAX_LFS_FILESIZE	(((u64)PAGE_CACHE_SIZE << (BITS_PER_LONG-1))-1) 
#elif BITS_PER_LONG==64
#define MAX_LFS_FILESIZE 	0x7fffffffffffffffUL
#endif

#define FL_POSIX	1
#define FL_FLOCK	2
#define FL_ACCESS	8	/* not trying to lock, just looking */
#define FL_LOCKD	16	/* lock held by rpc.lockd */
#define FL_LEASE	32	/* lease held on this file */
#define FL_SLEEP	128	/* A blocking lock */

/*
 * The POSIX file lock owner is determined by
 * the "struct files_struct" in the thread group
 * (or NULL for no owner - BSD locks).
 *
 * Lockd stuffs a "host" pointer into this.
 */
typedef struct files_struct *fl_owner_t;

struct file_lock_operations {
	void (*fl_insert)(struct file_lock *);	/* lock insertion callback */
	void (*fl_remove)(struct file_lock *);	/* lock removal callback */
	void (*fl_copy_lock)(struct file_lock *, struct file_lock *);
	void (*fl_release_private)(struct file_lock *);
};

struct lock_manager_operations {
	int (*fl_compare_owner)(struct file_lock *, struct file_lock *);
	void (*fl_notify)(struct file_lock *);	/* unblock callback */
	void (*fl_copy_lock)(struct file_lock *, struct file_lock *);
	void (*fl_release_private)(struct file_lock *);
	void (*fl_break)(struct file_lock *);
};

/* that will die - we need it for nfs_lock_info */
#include <linux/nfs_fs_i.h>

struct file_lock {
	/*与索引节点关联的锁链表中的下一个元素*/
	struct file_lock *fl_next;	/* singly linked list for this inode  */
	/*用于活动或阻塞链表的指针*/
	struct list_head fl_link;	/* doubly linked list of all locks */
	/*用于锁的等待者链表的指针*/
	struct list_head fl_block;	/* circular list of blocked processes */
	/*文件所有者的files_struct*/
	fl_owner_t fl_owner;
	/*进程拥有者的pid*/
	unsigned int fl_pid;
	/*阻塞进程的等待队列*/
	wait_queue_head_t fl_wait;
	/*指向文件对象的指针*/
	struct file *fl_file;
	/*锁标志*/
	unsigned char fl_flags;
	/*锁类型*/
	unsigned char fl_type;
	/*被锁区字段的起始偏移量*/
	loff_t fl_start;
	/*被锁区字段的末尾偏移量*/
	loff_t fl_end;

	/*用于租借锁中断通知*/
	struct fasync_struct *	fl_fasync; /* for lease break notifications */
	/*租借结束前的剩余时间*/
	unsigned long fl_break_time;	/* for nonblocking lease breaks */

	/*指向文件锁操作的指针*/
	struct file_lock_operations *fl_ops;	/* Callbacks for filesystems */
	/*指向锁管理操作的指针*/
	struct lock_manager_operations *fl_lmops;	/* Callbacks for lockmanagers */
	/*具体文件系统的信息*/
	union {
		struct nfs_lock_info	nfs_fl;
	} fl_u;
};

/* The following constant reflects the upper bound of the file/locking space */
#ifndef OFFSET_MAX
#define INT_LIMIT(x)	(~((x)1 << (sizeof(x)*8 - 1)))
#define OFFSET_MAX	INT_LIMIT(loff_t)
#define OFFT_OFFSET_MAX	INT_LIMIT(off_t)
#endif

extern struct list_head file_lock_list;

#include <linux/fcntl.h>

extern int fcntl_getlk(struct file *, struct flock __user *);
extern int fcntl_setlk(struct file *, unsigned int, struct flock __user *);

#if BITS_PER_LONG == 32
extern int fcntl_getlk64(struct file *, struct flock64 __user *);
extern int fcntl_setlk64(struct file *, unsigned int, struct flock64 __user *);
#endif

extern void send_sigio(struct fown_struct *fown, int fd, int band);
extern int fcntl_setlease(unsigned int fd, struct file *filp, long arg);
extern int fcntl_getlease(struct file *filp);

/* fs/locks.c */
extern void locks_init_lock(struct file_lock *);
extern void locks_copy_lock(struct file_lock *, struct file_lock *);
extern void locks_remove_posix(struct file *, fl_owner_t);
extern void locks_remove_flock(struct file *);
extern struct file_lock *posix_test_lock(struct file *, struct file_lock *);
extern int posix_lock_file(struct file *, struct file_lock *);
extern int posix_lock_file_wait(struct file *, struct file_lock *);
extern void posix_block_lock(struct file_lock *, struct file_lock *);
extern void posix_unblock_lock(struct file *, struct file_lock *);
extern int posix_locks_deadlock(struct file_lock *, struct file_lock *);
extern int flock_lock_file_wait(struct file *filp, struct file_lock *fl);
extern int __break_lease(struct inode *inode, unsigned int flags);
extern void lease_get_mtime(struct inode *, struct timespec *time);
extern int setlease(struct file *, long, struct file_lock **);
extern void remove_lease(struct file_lock *);
extern int lock_may_read(struct inode *, loff_t start, unsigned long count);
extern int lock_may_write(struct inode *, loff_t start, unsigned long count);
extern void steal_locks(fl_owner_t from);

struct fasync_struct {
	int	magic;
	int	fa_fd;
	struct	fasync_struct	*fa_next; /* singly linked list */
	struct	file 		*fa_file;
};

#define FASYNC_MAGIC 0x4601

/* SMP safe fasync helpers: */
extern int fasync_helper(int, struct file *, int, struct fasync_struct **);
/* can be called from interrupts */
extern void kill_fasync(struct fasync_struct **, int, int);
/* only for net: no internal synchronization */
extern void __kill_fasync(struct fasync_struct *, int, int);

extern int f_setown(struct file *filp, unsigned long arg, int force);
extern void f_delown(struct file *filp);
extern int send_sigurg(struct fown_struct *fown);

/*
 *	Umount options
 */

#define MNT_FORCE	0x00000001	/* Attempt to forcibily umount */
#define MNT_DETACH	0x00000002	/* Just detach from the tree */
#define MNT_EXPIRE	0x00000004	/* Mark for expiry */

/*所有超级块对象的链表头*/
extern struct list_head super_blocks;
/*保护超级块链表的自旋锁*/
extern spinlock_t sb_lock;

#define sb_entry(list)	list_entry((list), struct super_block, s_list)
#define S_BIAS (1<<30)
struct super_block {
	/*指向超级块链表的指针*/
	struct list_head	s_list;		/* Keep this first */
	/*设备标识符*/
	dev_t			s_dev;		/* search index; _not_ kdev_t */
	/*以字节为单位的块大小*/
	unsigned long		s_blocksize;
	/*基本块设备驱动程序中提到的以字节为单位的块大小*/
	unsigned long		s_old_blocksize;
	/*以位为单位的块大小*/
	unsigned char		s_blocksize_bits;
	/*修改标志*/
	unsigned char		s_dirt;
	/*文件的最长长度*/
	unsigned long long	s_maxbytes;	/* Max file size */
	/*文件系统类型*/
	struct file_system_type	*s_type;
	/*超级块方法*/
	struct super_operations	*s_op;
	/*磁盘限额处理方法*/
	struct dquot_operations	*dq_op;
	/*磁盘限额管理方法*/
 	struct quotactl_ops	*s_qcop;
	/*网络文件系统使用的输出操作*/
	struct export_operations *s_export_op;
	/*安装标志*/
	unsigned long		s_flags;
	/*文件系统的魔数*/
	unsigned long		s_magic;
	/*文件系统根目录的目录项对象*/
	struct dentry		*s_root;
	/*卸载所用的信号量*/
	struct rw_semaphore	s_umount;
	/*超级块信号量*/
	struct semaphore	s_lock;
	/*引用计数器*/
	int			s_count;
	/*表示对超级块的索引节点进行同步的标志*/
	int			s_syncing;
	/*对超级块的已安装文件系统进行同步的标志*/
	int			s_need_sync_fs;
	/*次级引用计数器*/
	atomic_t		s_active;
	/*指向超级块安全数据结构的指针*/
	void                    *s_security;
	/*指向超级块扩展属性结构的指针*/
	struct xattr_handler	**s_xattr;

	/*所有索引节点的链表 inode: i_sb_list*/
	struct list_head	s_inodes;	/* all inodes */
	/*改进型索引节点的链表*/
	struct list_head	s_dirty;	/* dirty inodes */
	/*等待被写入磁盘的索引节点的链表*/
	struct list_head	s_io;		/* parked for writeback */
	/*用于处理远程网络文件系统的匿名目录项的链表*/
	struct hlist_head	s_anon;		/* anonymous dentries for (nfs) exporting */
	/*文件对象的链表*/
	struct list_head	s_files;

	/*指向块设备驱动程序描述符的指针*/
	struct block_device	*s_bdev;
	/*用于给定文件系统类型的超级块对象链表的指针*/
	struct list_head	s_instances;
	/*磁盘限额的描述符*/
	struct quota_info	s_dquot;	/* Diskquota specific options */

	/*冻结文件系统时使用的标志*/
	int			s_frozen;
	/*进程挂起的等待队列，知道文件系统被解冻*/
	wait_queue_head_t	s_wait_unfrozen;

	/*包含超级块的块设备的名称*/
	char s_id[32];				/* Informational name */

	/*指向特定文件系统的超级块信息的指针*/
	void 			*s_fs_info;	/* Filesystem private info */

	/*
	 * The next field is for VFS *only*. No filesystems have any business
	 * even looking at it. You had been warned.
	 */
	/*当vfs通过目录重命名文件时使用的信号量*/
	struct semaphore s_vfs_rename_sem;	/* Kludge */

	/* Granuality of c/m/atime in ns.
	   Cannot be worse than a second */
	/*时间戳的粒度(纳秒级)*/
	u32		   s_time_gran;
};

extern struct timespec current_fs_time(struct super_block *sb);

/*
 * Snapshotting support.
 */
enum {
	SB_UNFROZEN = 0,
	SB_FREEZE_WRITE	= 1,
	SB_FREEZE_TRANS = 2,
};

#define vfs_check_frozen(sb, level) \
	wait_event((sb)->s_wait_unfrozen, ((sb)->s_frozen < (level)))

/*
 * Superblock locking.
 */
static inline void lock_super(struct super_block * sb)
{
	down(&sb->s_lock);
}

static inline void unlock_super(struct super_block * sb)
{
	up(&sb->s_lock);
}

/*
 * VFS helper functions..
 */
extern int vfs_create(struct inode *, struct dentry *, int, struct nameidata *);
extern int vfs_mkdir(struct inode *, struct dentry *, int);
extern int vfs_mknod(struct inode *, struct dentry *, int, dev_t);
extern int vfs_symlink(struct inode *, struct dentry *, const char *, int);
extern int vfs_link(struct dentry *, struct inode *, struct dentry *);
extern int vfs_rmdir(struct inode *, struct dentry *);
extern int vfs_unlink(struct inode *, struct dentry *);
extern int vfs_rename(struct inode *, struct dentry *, struct inode *, struct dentry *);

/*
 * VFS dentry helper functions.
 */
extern void dentry_unhash(struct dentry *dentry);

/*
 * File types
 *
 * NOTE! These match bits 12..15 of stat.st_mode
 * (ie "(i_mode >> 12) & 15").
 */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

#define OSYNC_METADATA	(1<<0)
#define OSYNC_DATA	(1<<1)
#define OSYNC_INODE	(1<<2)
int generic_osync_inode(struct inode *, struct address_space *, int);

/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */
typedef int (*filldir_t)(void *, const char *, int, loff_t, ino_t, unsigned);

struct block_device_operations {
	/*打开块设备文件*/
	int (*open) (struct inode *, struct file *);
	/*关闭对块设备文件的最后一个引用*/
	int (*release) (struct inode *, struct file *);
	/*在块设备文件上发出ioctl()系统调用(使用大内核锁)*/
	int (*ioctl) (struct inode *, struct file *, unsigned, unsigned long);
	/*在块设备文件上发出ioctl()系统调用(不使用大内核锁)*/
	long (*compat_ioctl) (struct file *, unsigned, unsigned long);
	/*检查可移动介质是否已经变化(例如软盘)*/
	int (*media_changed) (struct gendisk *);
	/*检查块设备是否持有有效数据*/
	int (*revalidate_disk) (struct gendisk *);
	struct module *owner;
};

/*
 * "descriptor" for what we're up to with a read for sendfile().
 * This allows us to use the same read code yet
 * have multiple different users of the data that
 * we read from a file.
 *
 * The simplest case just copies the data to user
 * mode.
 */
typedef struct {
	size_t written;
	size_t count;
	union {
		char __user * buf;
		void *data;
	} arg;
	int error;
} read_descriptor_t;

typedef int (*read_actor_t)(read_descriptor_t *, struct page *, unsigned long, unsigned long);

/* These macros are for out of kernel modules to test that
 * the kernel supports the unlocked_ioctl and compat_ioctl
 * fields in struct file_operations. */
#define HAVE_COMPAT_IOCTL 1
#define HAVE_UNLOCKED_IOCTL 1

/*
 * NOTE:
 * read, write, poll, fsync, readv, writev, unlocked_ioctl and compat_ioctl
 * can be called without the big kernel lock held in all filesystems.
 */
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*aio_read) (struct kiocb *, char __user *, size_t, loff_t);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*aio_write) (struct kiocb *, const char __user *, size_t, loff_t);
	int (*readdir) (struct file *, void *, filldir_t);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, struct dentry *, int datasync);
	int (*aio_fsync) (struct kiocb *, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*readv) (struct file *, const struct iovec *, unsigned long, loff_t *);
	ssize_t (*writev) (struct file *, const struct iovec *, unsigned long, loff_t *);
	ssize_t (*sendfile) (struct file *, loff_t *, size_t, read_actor_t, void *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*dir_notify)(struct file *filp, unsigned long arg);
	int (*flock) (struct file *, int, struct file_lock *);
};

struct inode_operations {
	/*在某一个目录下，为与目录项对象相关的普通文件创建一个新的磁盘索引节点*/
	int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
	/*为包含在一个目录项对象中的文件名对应的索引节点查找目录*/
	struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);
	/*创建一个新的名为new_dentry(第3个参数）的硬链接，它指向dir目录下(第2个参数)名为old_dentry(第1个参数)的文件*/
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	/*从一个目录中删除目录项对象所指定文件的硬链接*/
	int (*unlink) (struct inode *,struct dentry *);
	/*在某个目录下，为与目录项对象相关的符号链接创建一个新的索引节点*/
	int (*symlink) (struct inode *,struct dentry *,const char *);
	/*在某个目录下，为与目录项对象相关的目录创建一个新的索引节点*/
	int (*mkdir) (struct inode *,struct dentry *,int);
	/*从一个目录删除子目录，子目录的名称包含在目录项对象中*/
	int (*rmdir) (struct inode *,struct dentry *);
	/*在某个目录中，为与目录项对象相关的特定文件创建一个新的磁盘索引节点。其中int 参数和dev_t参数表示文件的类型和设备的主次设备号*/
	int (*mknod) (struct inode *,struct dentry *,int,dev_t);
	int (*rename) (struct inode *, struct dentry *,
			struct inode *, struct dentry *);
	/*将目录项所指定的符号链接中对应的文件路径名拷贝到用户态内存区*/
	int (*readlink) (struct dentry *, char __user *,int);
	/*解析索引节点对象所指定的符号链接；如果该符号链接是一个相对路径，则从第2个参数所指定的目录开始查找*/
	int (*follow_link) (struct dentry *, struct nameidata *);
	/*释放由follow_link方法分配的用于解析符号链接的临时数据结构*/
	void (*put_link) (struct dentry *, struct nameidata *);
	/*修改与索引节点相关的文件的长度，在调用该方法之前，必须将inode对象的i_size字段设置为需要的新长度值*/
	void (*truncate) (struct inode *);
	/*检查是否允许对索引节点所指的文件进行指定模式的访问*/
	int (*permission) (struct inode *, int, struct nameidata *);
	/*在触及索引节点属性后 通知一个“修改事件”*/
	int (*setattr) (struct dentry *, struct iattr *);
	/*由一些文件系统用于读取索引节点属性*/
	int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
	/*为索引节点设置“扩展属性”（扩展属性存放在任何索引节点之外的硬盘块中）*/
	int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
	/*获取索引节点的扩展属性*/
	ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
	/*获取扩展属性名称的整个链表*/
	ssize_t (*listxattr) (struct dentry *, char *, size_t);
	/*删除索引节点的扩展属性*/
	int (*removexattr) (struct dentry *, const char *);
};

struct seq_file;

extern ssize_t vfs_read(struct file *, char __user *, size_t, loff_t *);
extern ssize_t vfs_write(struct file *, const char __user *, size_t, loff_t *);
extern ssize_t vfs_readv(struct file *, const struct iovec __user *,
		unsigned long, loff_t *);
extern ssize_t vfs_writev(struct file *, const struct iovec __user *,
		unsigned long, loff_t *);

/*
 * NOTE: write_inode, delete_inode, clear_inode, put_inode can be called
 * without the big kernel lock held in all filesystems.
 */
struct super_operations {
	/*为索引节点对象分配空间，包括具体文件系统的数据所需要的空间*/
   	struct inode *(*alloc_inode)(struct super_block *sb);
	/*撤销索引节点对象，包括具体文件系统的数据*/
	void (*destroy_inode)(struct inode *);

	/*用磁盘上的数据填充以参数传递过来的索引节点对象字段，索引节点对象的i_ino字段标识从磁盘要读取的具体文件系统的索引节点*/
	void (*read_inode) (struct inode *);
  
	/*当索引节点标记为修改时调用。ReiserFS, Ext3这样的文件系统用它来更新磁盘上的文件系统日志*/
   	void (*dirty_inode) (struct inode *);
	/*用通过传递参数指定的索引节点对象的内容更新一个文件系统的索引节点，索引节点对象的i_ino字段
	 * 标识所涉及磁盘上文件系统的索引节点，int参数标识I/O操作是否应该同步*/
	int (*write_inode) (struct inode *, int);
	/*释放索引节点时调用(减少该节点引用计数器值)以执行具体文件系统的操作*/
	void (*put_inode) (struct inode *);
	/*在即将撤销索引节点时调用--也就是说，当最后一个用户释放该索引节点时；实现该方法的文件系统通常使用generic_drop_inode()函数。
	 * 该函数从vfs数据结构中移走对索引节点的每一个引用，如果索引节点不再出现在任何目录中，则调用超级块方法delete_inode将它从文件
	 * 系统中删除*/
	void (*drop_inode) (struct inode *);
	/*在必须撤销索引节点时调用。删除内存中的VFS索引节点和磁盘上的文件数据及元数据*/
	void (*delete_inode) (struct inode *);
	/*释放通过传递的参数指定的超级块对象(因为相应的文件系统被卸载)*/
	void (*put_super) (struct super_block *);
	/*用指定对象的内容 更新文件系统的超级块*/
	void (*write_super) (struct super_block *);
	/*在清除文件系统来更新磁盘上的具体文件系统数据结构时调用(由日志文件系统使用)*/
	int (*sync_fs)(struct super_block *sb, int wait);
	/*阻塞对文件系统的修改并用指定对象的内容更新超级块。当文件系统被冻结时调用该方法，例如，由逻辑卷管理器驱动程序(lvm)调用*/
	void (*write_super_lockfs) (struct super_block *);
	/*取消由write_super_lockfs()超级块方法实现的对文件系统更新的阻塞*/
	void (*unlockfs) (struct super_block *);
	/*将文件系统的统计信息返回，填写在kstatfs*缓冲区中*/
	int (*statfs) (struct super_block *, struct kstatfs *);
	/*用新的选项重新安装文件系统*/
	int (*remount_fs) (struct super_block *, int *, char *);
	/*当撤销磁盘索引节点执行具体文件系统操作时调用*/
	void (*clear_inode) (struct inode *);
	/*中断一个安装操作，因为相应的卸载操作已经开始(只在网络文件系统中使用)*/
	void (*umount_begin) (struct super_block *);

	/*用来显示特定文件系统的选项*/
	int (*show_options)(struct seq_file *, struct vfsmount *);

	/*限额系统使用该方法从文件中读取数据，该文件详细说明了所在文件系统的限制*/
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	/*限额系统使用该方法将数据写入文件中，该文件详细说明来所在文件系统的限制*/
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
};

/* Inode state bits.  Protected by inode_lock. */
#define I_DIRTY_SYNC		1 /* Not dirty enough for O_DATASYNC */
#define I_DIRTY_DATASYNC	2 /* Data-related inode changes pending */
#define I_DIRTY_PAGES		4 /* Data-related inode changes pending */
#define __I_LOCK		3
/*涉及的索引节点对象处于I/O传送中*/
#define I_LOCK			(1 << __I_LOCK)
/*索引节点对象正在被释放*/
#define I_FREEING		16
/*索引节点对象的内容不再有意义*/
#define I_CLEAR			32
/*索引节点对象已经分配但还没有从磁盘索引节点读取来的数据填充*/
#define I_NEW			64

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)

extern void __mark_inode_dirty(struct inode *, int);
static inline void mark_inode_dirty(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY);
}

static inline void mark_inode_dirty_sync(struct inode *inode)
{
	__mark_inode_dirty(inode, I_DIRTY_SYNC);
}

static inline void touch_atime(struct vfsmount *mnt, struct dentry *dentry)
{
	/* per-mountpoint checks will go here */
	update_atime(dentry->d_inode);
}

static inline void file_accessed(struct file *file)
{
	if (!(file->f_flags & O_NOATIME))
		touch_atime(file->f_vfsmnt, file->f_dentry);
}

int sync_inode(struct inode *inode, struct writeback_control *wbc);

/**
 * &export_operations - for nfsd to communicate with file systems
 * decode_fh:      decode a file handle fragment and return a &struct dentry
 * encode_fh:      encode a file handle fragment from a dentry
 * get_name:       find the name for a given inode in a given directory
 * get_parent:     find the parent of a given directory
 * get_dentry:     find a dentry for the inode given a file handle sub-fragment
 *
 * Description:
 *    The export_operations structure provides a means for nfsd to communicate
 *    with a particular exported file system  - particularly enabling nfsd and
 *    the filesystem to co-operate when dealing with file handles.
 *
 *    export_operations contains two basic operation for dealing with file handles,
 *    decode_fh() and encode_fh(), and allows for some other operations to be defined
 *    which standard helper routines use to get specific information from the
 *    filesystem.
 *
 *    nfsd encodes information use to determine which filesystem a filehandle
 *    applies to in the initial part of the file handle.  The remainder, termed a
 *    file handle fragment, is controlled completely by the filesystem.
 *    The standard helper routines assume that this fragment will contain one or two
 *    sub-fragments, one which identifies the file, and one which may be used to
 *    identify the (a) directory containing the file.
 *
 *    In some situations, nfsd needs to get a dentry which is connected into a
 *    specific part of the file tree.  To allow for this, it passes the function
 *    acceptable() together with a @context which can be used to see if the dentry
 *    is acceptable.  As there can be multiple dentrys for a given file, the filesystem
 *    should check each one for acceptability before looking for the next.  As soon
 *    as an acceptable one is found, it should be returned.
 *
 * decode_fh:
 *    @decode_fh is given a &struct super_block (@sb), a file handle fragment (@fh, @fh_len)
 *    and an acceptability testing function (@acceptable, @context).  It should return
 *    a &struct dentry which refers to the same file that the file handle fragment refers
 *    to,  and which passes the acceptability test.  If it cannot, it should return
 *    a %NULL pointer if the file was found but no acceptable &dentries were available, or
 *    a %ERR_PTR error code indicating why it couldn't be found (e.g. %ENOENT or %ENOMEM).
 *
 * encode_fh:
 *    @encode_fh should store in the file handle fragment @fh (using at most @max_len bytes)
 *    information that can be used by @decode_fh to recover the file refered to by the
 *    &struct dentry @de.  If the @connectable flag is set, the encode_fh() should store
 *    sufficient information so that a good attempt can be made to find not only
 *    the file but also it's place in the filesystem.   This typically means storing
 *    a reference to de->d_parent in the filehandle fragment.
 *    encode_fh() should return the number of bytes stored or a negative error code
 *    such as %-ENOSPC
 *
 * get_name:
 *    @get_name should find a name for the given @child in the given @parent directory.
 *    The name should be stored in the @name (with the understanding that it is already
 *    pointing to a a %NAME_MAX+1 sized buffer.   get_name() should return %0 on success,
 *    a negative error code or error.
 *    @get_name will be called without @parent->i_sem held.
 *
 * get_parent:
 *    @get_parent should find the parent directory for the given @child which is also
 *    a directory.  In the event that it cannot be found, or storage space cannot be
 *    allocated, a %ERR_PTR should be returned.
 *
 * get_dentry:
 *    Given a &super_block (@sb) and a pointer to a file-system specific inode identifier,
 *    possibly an inode number, (@inump) get_dentry() should find the identified inode and
 *    return a dentry for that inode.
 *    Any suitable dentry can be returned including, if necessary, a new dentry created
 *    with d_alloc_root.  The caller can then find any other extant dentrys by following the
 *    d_alias links.  If a new dentry was created using d_alloc_root, DCACHE_NFSD_DISCONNECTED
 *    should be set, and the dentry should be d_rehash()ed.
 *
 *    If the inode cannot be found, either a %NULL pointer or an %ERR_PTR code can be returned.
 *    The @inump will be whatever was passed to nfsd_find_fh_dentry() in either the
 *    @obj or @parent parameters.
 *
 * Locking rules:
 *  get_parent is called with child->d_inode->i_sem down
 *  get_name is not (which is possibly inconsistent)
 */

struct export_operations {
	struct dentry *(*decode_fh)(struct super_block *sb, __u32 *fh, int fh_len, int fh_type,
			 int (*acceptable)(void *context, struct dentry *de),
			 void *context);
	int (*encode_fh)(struct dentry *de, __u32 *fh, int *max_len,
			 int connectable);

	/* the following are only called from the filesystem itself */
	int (*get_name)(struct dentry *parent, char *name,
			struct dentry *child);
	struct dentry * (*get_parent)(struct dentry *child);
	struct dentry * (*get_dentry)(struct super_block *sb, void *inump);

	/* This is set by the exporting module to a standard helper */
	struct dentry * (*find_exported_dentry)(
		struct super_block *sb, void *obj, void *parent,
		int (*acceptable)(void *context, struct dentry *de),
		void *context);


};

extern struct dentry *
find_exported_dentry(struct super_block *sb, void *obj, void *parent,
		     int (*acceptable)(void *context, struct dentry *de),
		     void *context);

struct file_system_type {
	/*文件系统名*/
	const char *name;
	/*文件系统类型标志*/
	int fs_flags;
	/*读超级块的方法*/
	struct super_block *(*get_sb) (struct file_system_type *, int,
				       const char *, void *);
	/*删除超级块的方法*/
	void (*kill_sb) (struct super_block *);
	/*指向实现文件系统的模块的指针*/
	struct module *owner;
	/*指向文件系统类型链表中下一个元素的指针*/
	struct file_system_type * next;
	/*具有相同文件系统类型的超级块对象链表的头*/
	struct list_head fs_supers;
};

struct super_block *get_sb_bdev(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data,
	int (*fill_super)(struct super_block *, void *, int));
struct super_block *get_sb_single(struct file_system_type *fs_type,
	int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int));
struct super_block *get_sb_nodev(struct file_system_type *fs_type,
	int flags, void *data,
	int (*fill_super)(struct super_block *, void *, int));
void generic_shutdown_super(struct super_block *sb);
void kill_block_super(struct super_block *sb);
void kill_anon_super(struct super_block *sb);
void kill_litter_super(struct super_block *sb);
void deactivate_super(struct super_block *sb);
int set_anon_super(struct super_block *s, void *data);
struct super_block *sget(struct file_system_type *type,
			int (*test)(struct super_block *,void *),
			int (*set)(struct super_block *,void *),
			void *data);
struct super_block *get_sb_pseudo(struct file_system_type *, char *,
			struct super_operations *ops, unsigned long);
int __put_super(struct super_block *sb);
int __put_super_and_need_restart(struct super_block *sb);
void unnamed_dev_init(void);

/* Alas, no aliases. Too much hassle with bringing module.h everywhere */
#define fops_get(fops) \
	(((fops) && try_module_get((fops)->owner) ? (fops) : NULL))
#define fops_put(fops) \
	do { if (fops) module_put((fops)->owner); } while(0)

extern int register_filesystem(struct file_system_type *);
extern int unregister_filesystem(struct file_system_type *);
extern struct vfsmount *kern_mount(struct file_system_type *);
extern int may_umount_tree(struct vfsmount *);
extern int may_umount(struct vfsmount *);
extern long do_mount(char *, char *, char *, unsigned long, void *);

extern int vfs_statfs(struct super_block *, struct kstatfs *);

#define FLOCK_VERIFY_READ  1
#define FLOCK_VERIFY_WRITE 2

extern int locks_mandatory_locked(struct inode *);
extern int locks_mandatory_area(int, struct inode *, struct file *, loff_t, size_t);

/*
 * Candidates for mandatory locking have the setgid bit set
 * but no group execute bit -  an otherwise meaningless combination.
 */
#define MANDATORY_LOCK(inode) \
	(IS_MANDLOCK(inode) && ((inode)->i_mode & (S_ISGID | S_IXGRP)) == S_ISGID)

static inline int locks_verify_locked(struct inode *inode)
{
	if (MANDATORY_LOCK(inode))
		return locks_mandatory_locked(inode);
	return 0;
}

extern int rw_verify_area(int, struct file *, loff_t *, size_t);

static inline int locks_verify_truncate(struct inode *inode,
				    struct file *filp,
				    loff_t size)
{
	if (inode->i_flock && MANDATORY_LOCK(inode))
		return locks_mandatory_area(
			FLOCK_VERIFY_WRITE, inode, filp,
			size < inode->i_size ? size : inode->i_size,
			(size < inode->i_size ? inode->i_size - size
			 : size - inode->i_size)
		);
	return 0;
}

static inline int break_lease(struct inode *inode, unsigned int mode)
{
	if (inode->i_flock)
		return __break_lease(inode, mode);
	return 0;
}

/* fs/open.c */

extern int do_truncate(struct dentry *, loff_t start);
extern struct file *filp_open(const char *, int, int);
extern struct file * dentry_open(struct dentry *, struct vfsmount *, int);
extern int filp_close(struct file *, fl_owner_t id);
extern char * getname(const char __user *);

/* fs/dcache.c */
extern void __init vfs_caches_init_early(void);
extern void __init vfs_caches_init(unsigned long);

#define __getname()	kmem_cache_alloc(names_cachep, SLAB_KERNEL)
#define __putname(name) kmem_cache_free(names_cachep, (void *)(name))
#ifndef CONFIG_AUDITSYSCALL
#define putname(name)   __putname(name)
#else
#define putname(name)							\
	do {								\
		if (unlikely(current->audit_context))			\
			audit_putname(name);				\
		else							\
			__putname(name);				\
	} while (0)
#endif

extern int register_blkdev(unsigned int, const char *);
extern int unregister_blkdev(unsigned int, const char *);
extern struct block_device *bdget(dev_t);
extern void bd_set_size(struct block_device *, loff_t size);
extern void bd_forget(struct inode *inode);
extern void bdput(struct block_device *);
extern struct block_device *open_by_devnum(dev_t, unsigned);
extern struct file_operations def_blk_fops;
extern struct address_space_operations def_blk_aops;
extern struct file_operations def_chr_fops;
extern struct file_operations bad_sock_fops;
extern struct file_operations def_fifo_fops;
extern int ioctl_by_bdev(struct block_device *, unsigned, unsigned long);
extern int blkdev_ioctl(struct inode *, struct file *, unsigned, unsigned long);
extern long compat_blkdev_ioctl(struct file *, unsigned, unsigned long);
extern int blkdev_get(struct block_device *, mode_t, unsigned);
extern int blkdev_put(struct block_device *);
extern int bd_claim(struct block_device *, void *);
extern void bd_release(struct block_device *);

/* fs/char_dev.c */
extern int alloc_chrdev_region(dev_t *, unsigned, unsigned, const char *);
extern int register_chrdev_region(dev_t, unsigned, const char *);
extern int register_chrdev(unsigned int, const char *,
			   struct file_operations *);
extern int unregister_chrdev(unsigned int, const char *);
extern void unregister_chrdev_region(dev_t, unsigned);
extern int chrdev_open(struct inode *, struct file *);

/* fs/block_dev.c */
#define BDEVNAME_SIZE	32	/* Largest string for a blockdev identifier */
extern const char *__bdevname(dev_t, char *buffer);
extern const char *bdevname(struct block_device *bdev, char *buffer);
extern struct block_device *lookup_bdev(const char *);
extern struct block_device *open_bdev_excl(const char *, int, void *);
extern void close_bdev_excl(struct block_device *);

extern void init_special_inode(struct inode *, umode_t, dev_t);

/* Invalid inode operations -- fs/bad_inode.c */
extern void make_bad_inode(struct inode *);
extern int is_bad_inode(struct inode *);

extern struct file_operations read_fifo_fops;
extern struct file_operations write_fifo_fops;
extern struct file_operations rdwr_fifo_fops;
extern struct file_operations read_pipe_fops;
extern struct file_operations write_pipe_fops;
extern struct file_operations rdwr_pipe_fops;

extern int fs_may_remount_ro(struct super_block *);

/*
 * return READ, READA, or WRITE
 */
#define bio_rw(bio)		((bio)->bi_rw & (RW_MASK | RWA_MASK))

/*
 * return data direction, READ or WRITE
 */
#define bio_data_dir(bio)	((bio)->bi_rw & 1)

extern int check_disk_change(struct block_device *);
extern int invalidate_inodes(struct super_block *);
extern int __invalidate_device(struct block_device *, int);
extern int invalidate_partition(struct gendisk *, int);
unsigned long invalidate_mapping_pages(struct address_space *mapping,
					pgoff_t start, pgoff_t end);
unsigned long invalidate_inode_pages(struct address_space *mapping);
static inline void invalidate_remote_inode(struct inode *inode)
{
	if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	    S_ISLNK(inode->i_mode))
		invalidate_inode_pages(inode->i_mapping);
}
extern int invalidate_inode_pages2(struct address_space *mapping);
extern int write_inode_now(struct inode *, int);
extern int filemap_fdatawrite(struct address_space *);
extern int filemap_flush(struct address_space *);
extern int filemap_fdatawait(struct address_space *);
extern int filemap_write_and_wait(struct address_space *mapping);
extern void sync_supers(void);
extern void sync_filesystems(int wait);
extern void emergency_sync(void);
extern void emergency_remount(void);
extern int do_remount_sb(struct super_block *sb, int flags,
			 void *data, int force);
extern sector_t bmap(struct inode *, sector_t);
extern int setattr_mask(unsigned int);
extern int notify_change(struct dentry *, struct iattr *);
extern int permission(struct inode *, int, struct nameidata *);
extern int generic_permission(struct inode *, int,
		int (*check_acl)(struct inode *, int));

extern int get_write_access(struct inode *);
extern int deny_write_access(struct file *);
static inline void put_write_access(struct inode * inode)
{
	atomic_dec(&inode->i_writecount);
}
static inline void allow_write_access(struct file *file)
{
	if (file)
		atomic_inc(&file->f_dentry->d_inode->i_writecount);
}
extern int do_pipe(int *);

extern int open_namei(const char *, int, int, struct nameidata *);
extern int may_open(struct nameidata *, int, int);

extern int kernel_read(struct file *, unsigned long, char *, unsigned long);
extern struct file * open_exec(const char *);
 
/* fs/dcache.c -- generic fs support functions */
extern int is_subdir(struct dentry *, struct dentry *);
extern ino_t find_inode_number(struct dentry *, struct qstr *);

#include <linux/err.h>

/* needed for stackable file system support */
extern loff_t default_llseek(struct file *file, loff_t offset, int origin);

extern loff_t vfs_llseek(struct file *file, loff_t offset, int origin);

extern void inode_init_once(struct inode *);
extern void iput(struct inode *);
extern struct inode * igrab(struct inode *);
extern ino_t iunique(struct super_block *, ino_t);
extern int inode_needs_sync(struct inode *inode);
extern void generic_delete_inode(struct inode *inode);

extern struct inode *ilookup5(struct super_block *sb, unsigned long hashval,
		int (*test)(struct inode *, void *), void *data);
extern struct inode *ilookup(struct super_block *sb, unsigned long ino);

extern struct inode * iget5_locked(struct super_block *, unsigned long, int (*test)(struct inode *, void *), int (*set)(struct inode *, void *), void *);
extern struct inode * iget_locked(struct super_block *, unsigned long);
extern void unlock_new_inode(struct inode *);

static inline struct inode *iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode = iget_locked(sb, ino);
	
	if (inode && (inode->i_state & I_NEW)) {
		sb->s_op->read_inode(inode);
		unlock_new_inode(inode);
	}

	return inode;
}

extern void __iget(struct inode * inode);
extern void clear_inode(struct inode *);
extern void destroy_inode(struct inode *);
extern struct inode *new_inode(struct super_block *);
extern int remove_suid(struct dentry *);
extern void remove_dquot_ref(struct super_block *, int, struct list_head *);
extern struct semaphore iprune_sem;

extern void __insert_inode_hash(struct inode *, unsigned long hashval);
extern void remove_inode_hash(struct inode *);
static inline void insert_inode_hash(struct inode *inode) {
	__insert_inode_hash(inode, inode->i_ino);
}

extern struct file * get_empty_filp(void);
extern void file_move(struct file *f, struct list_head *list);
extern void file_kill(struct file *f);
struct bio;
extern void submit_bio(int, struct bio *);
extern int bdev_read_only(struct block_device *);
extern int set_blocksize(struct block_device *, int);
extern int sb_set_blocksize(struct super_block *, int);
extern int sb_min_blocksize(struct super_block *, int);

extern int generic_file_mmap(struct file *, struct vm_area_struct *);
extern int generic_file_readonly_mmap(struct file *, struct vm_area_struct *);
extern int file_read_actor(read_descriptor_t * desc, struct page *page, unsigned long offset, unsigned long size);
extern int file_send_actor(read_descriptor_t * desc, struct page *page, unsigned long offset, unsigned long size);
extern ssize_t generic_file_read(struct file *, char __user *, size_t, loff_t *);
int generic_write_checks(struct file *file, loff_t *pos, size_t *count, int isblk);
extern ssize_t generic_file_write(struct file *, const char __user *, size_t, loff_t *);
extern ssize_t generic_file_aio_read(struct kiocb *, char __user *, size_t, loff_t);
extern ssize_t __generic_file_aio_read(struct kiocb *, const struct iovec *, unsigned long, loff_t *);
extern ssize_t generic_file_aio_write(struct kiocb *, const char __user *, size_t, loff_t);
extern ssize_t generic_file_aio_write_nolock(struct kiocb *, const struct iovec *,
		unsigned long, loff_t *);
extern ssize_t generic_file_direct_write(struct kiocb *, const struct iovec *,
		unsigned long *, loff_t, loff_t *, size_t, size_t);
extern ssize_t generic_file_buffered_write(struct kiocb *, const struct iovec *,
		unsigned long, loff_t, loff_t *, size_t, ssize_t);
extern ssize_t do_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);
extern ssize_t do_sync_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);
ssize_t generic_file_write_nolock(struct file *file, const struct iovec *iov,
				unsigned long nr_segs, loff_t *ppos);
extern ssize_t generic_file_sendfile(struct file *, loff_t *, size_t, read_actor_t, void *);
extern void do_generic_mapping_read(struct address_space *mapping,
				    struct file_ra_state *, struct file *,
				    loff_t *, read_descriptor_t *, read_actor_t);
extern void
file_ra_state_init(struct file_ra_state *ra, struct address_space *mapping);
extern ssize_t generic_file_direct_IO(int rw, struct kiocb *iocb,
	const struct iovec *iov, loff_t offset, unsigned long nr_segs);
extern ssize_t generic_file_readv(struct file *filp, const struct iovec *iov, 
	unsigned long nr_segs, loff_t *ppos);
ssize_t generic_file_writev(struct file *filp, const struct iovec *iov, 
			unsigned long nr_segs, loff_t *ppos);
extern loff_t no_llseek(struct file *file, loff_t offset, int origin);
extern loff_t generic_file_llseek(struct file *file, loff_t offset, int origin);
extern loff_t remote_llseek(struct file *file, loff_t offset, int origin);
extern int generic_file_open(struct inode * inode, struct file * filp);
extern int nonseekable_open(struct inode * inode, struct file * filp);

static inline void do_generic_file_read(struct file * filp, loff_t *ppos,
					read_descriptor_t * desc,
					read_actor_t actor)
{
	do_generic_mapping_read(filp->f_mapping,
				&filp->f_ra,
				filp,
				ppos,
				desc,
				actor);
}

ssize_t __blockdev_direct_IO(int rw, struct kiocb *iocb, struct inode *inode,
	struct block_device *bdev, const struct iovec *iov, loff_t offset,
	unsigned long nr_segs, get_blocks_t get_blocks, dio_iodone_t end_io,
	int lock_type);

enum {
	DIO_LOCKING = 1, /* need locking between buffered and direct access */
	DIO_NO_LOCKING,  /* bdev; no locking at all between buffered/direct */
	DIO_OWN_LOCKING, /* filesystem locks buffered and direct internally */
};

static inline ssize_t blockdev_direct_IO(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_blocks_t get_blocks,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_blocks, end_io, DIO_LOCKING);
}

static inline ssize_t blockdev_direct_IO_no_locking(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_blocks_t get_blocks,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_blocks, end_io, DIO_NO_LOCKING);
}

static inline ssize_t blockdev_direct_IO_own_locking(int rw, struct kiocb *iocb,
	struct inode *inode, struct block_device *bdev, const struct iovec *iov,
	loff_t offset, unsigned long nr_segs, get_blocks_t get_blocks,
	dio_iodone_t end_io)
{
	return __blockdev_direct_IO(rw, iocb, inode, bdev, iov, offset,
				nr_segs, get_blocks, end_io, DIO_OWN_LOCKING);
}

extern struct file_operations generic_ro_fops;

#define special_file(m) (S_ISCHR(m)||S_ISBLK(m)||S_ISFIFO(m)||S_ISSOCK(m))

extern int vfs_readlink(struct dentry *, char __user *, int, const char *);
extern int vfs_follow_link(struct nameidata *, const char *);
extern int page_readlink(struct dentry *, char __user *, int);
extern int page_follow_link_light(struct dentry *, struct nameidata *);
extern void page_put_link(struct dentry *, struct nameidata *);
extern int page_symlink(struct inode *inode, const char *symname, int len);
extern struct inode_operations page_symlink_inode_operations;
extern int generic_readlink(struct dentry *, char __user *, int);
extern void generic_fillattr(struct inode *, struct kstat *);
extern int vfs_getattr(struct vfsmount *, struct dentry *, struct kstat *);
void inode_add_bytes(struct inode *inode, loff_t bytes);
void inode_sub_bytes(struct inode *inode, loff_t bytes);
loff_t inode_get_bytes(struct inode *inode);
void inode_set_bytes(struct inode *inode, loff_t bytes);

extern int vfs_readdir(struct file *, filldir_t, void *);

extern int vfs_stat(char __user *, struct kstat *);
extern int vfs_lstat(char __user *, struct kstat *);
extern int vfs_fstat(unsigned int, struct kstat *);

extern int vfs_ioctl(struct file *, unsigned int, unsigned int, unsigned long);

extern struct file_system_type *get_fs_type(const char *name);
extern struct super_block *get_super(struct block_device *);
extern struct super_block *user_get_super(dev_t);
extern void drop_super(struct super_block *sb);

extern int dcache_dir_open(struct inode *, struct file *);
extern int dcache_dir_close(struct inode *, struct file *);
extern loff_t dcache_dir_lseek(struct file *, loff_t, int);
extern int dcache_readdir(struct file *, void *, filldir_t);
extern int simple_getattr(struct vfsmount *, struct dentry *, struct kstat *);
extern int simple_statfs(struct super_block *, struct kstatfs *);
extern int simple_link(struct dentry *, struct inode *, struct dentry *);
extern int simple_unlink(struct inode *, struct dentry *);
extern int simple_rmdir(struct inode *, struct dentry *);
extern int simple_rename(struct inode *, struct dentry *, struct inode *, struct dentry *);
extern int simple_sync_file(struct file *, struct dentry *, int);
extern int simple_empty(struct dentry *);
extern int simple_readpage(struct file *file, struct page *page);
extern int simple_prepare_write(struct file *file, struct page *page,
			unsigned offset, unsigned to);
extern int simple_commit_write(struct file *file, struct page *page,
				unsigned offset, unsigned to);

extern struct dentry *simple_lookup(struct inode *, struct dentry *, struct nameidata *);
extern ssize_t generic_read_dir(struct file *, char __user *, size_t, loff_t *);
extern struct file_operations simple_dir_operations;
extern struct inode_operations simple_dir_inode_operations;
struct tree_descr { char *name; struct file_operations *ops; int mode; };
struct dentry *d_alloc_name(struct dentry *, const char *);
extern int simple_fill_super(struct super_block *, int, struct tree_descr *);
extern int simple_pin_fs(char *name, struct vfsmount **mount, int *count);
extern void simple_release_fs(struct vfsmount **mount, int *count);

extern ssize_t simple_read_from_buffer(void __user *, size_t, loff_t *, const void *, size_t);

extern int inode_change_ok(struct inode *, struct iattr *);
extern int __must_check inode_setattr(struct inode *, struct iattr *);

extern void inode_update_time(struct inode *inode, int ctime_too);

static inline ino_t parent_ino(struct dentry *dentry)
{
	ino_t res;

	spin_lock(&dentry->d_lock);
	res = dentry->d_parent->d_inode->i_ino;
	spin_unlock(&dentry->d_lock);
	return res;
}

/* kernel/fork.c */
extern int unshare_files(void);

/* Transaction based IO helpers */

/*
 * An argresp is stored in an allocated page and holds the
 * size of the argument or response, along with its content
 */
struct simple_transaction_argresp {
	ssize_t size;
	char data[0];
};

#define SIMPLE_TRANSACTION_LIMIT (PAGE_SIZE - sizeof(struct simple_transaction_argresp))

char *simple_transaction_get(struct file *file, const char __user *buf,
				size_t size);
ssize_t simple_transaction_read(struct file *file, char __user *buf,
				size_t size, loff_t *pos);
int simple_transaction_release(struct inode *inode, struct file *file);

static inline void simple_transaction_set(struct file *file, size_t n)
{
	struct simple_transaction_argresp *ar = file->private_data;

	BUG_ON(n > SIMPLE_TRANSACTION_LIMIT);

	/*
	 * The barrier ensures that ar->size will really remain zero until
	 * ar->data is ready for reading.
	 */
	smp_mb();
	ar->size = n;
}

#ifdef CONFIG_SECURITY
static inline char *alloc_secdata(void)
{
	return (char *)get_zeroed_page(GFP_KERNEL);
}

static inline void free_secdata(void *secdata)
{
	free_page((unsigned long)secdata);
}
#else
static inline char *alloc_secdata(void)
{
	return (char *)1;
}

static inline void free_secdata(void *secdata)
{ }
#endif	/* CONFIG_SECURITY */

#endif /* __KERNEL__ */
#endif /* _LINUX_FS_H */
