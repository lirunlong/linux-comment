#ifndef _LINUX_PERSONALITY_H
#define _LINUX_PERSONALITY_H

/*
 * Handling of different ABIs (personalities).
 */

struct exec_domain;
struct pt_regs;

extern int		register_exec_domain(struct exec_domain *);
extern int		unregister_exec_domain(struct exec_domain *);
extern int		__set_personality(unsigned long);

/*
 * Flags for bug emulation.
 *
 * These occupy the top three bytes.
 */
enum {
	FDPIC_FUNCPTRS =	0x0080000,	/* userspace function ptrs point to descriptors
						 * (signal handling)
						 */
	MMAP_PAGE_ZERO =	0x0100000,
	ADDR_COMPAT_LAYOUT =	0x0200000,
	READ_IMPLIES_EXEC =	0x0400000,
	ADDR_LIMIT_32BIT =	0x0800000,
	SHORT_INODE =		0x1000000,
	WHOLE_SECONDS =		0x2000000,
	STICKY_TIMEOUTS	=	0x4000000,
	ADDR_LIMIT_3GB = 	0x8000000,
};

/*
 * Security-relevant compatibility flags that must be
 * cleared upon setuid or setgid exec:
 */
#define PER_CLEAR_ON_SETID (READ_IMPLIES_EXEC)

/*
 * Personality types.
 *
 * These go in the low byte.  Avoid using the top bit, it will
 * conflict with error returns.
 */
enum {
	/*标准执行域*/
	PER_LINUX =		0x0000,
	/*linux，64位结构中32位物理地址*/
	PER_LINUX_32BIT =	0x0000 | ADDR_LIMIT_32BIT,
	/*linux程序，格式为ELF FDPIC*/
	PER_LINUX_FDPIC =	0x0000 | FDPIC_FUNCPTRS,
	/*System V release 4*/
	PER_SVR4 =		0x0001 | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
	/*System V release 3*/
	PER_SVR3 =		0x0002 | STICKY_TIMEOUTS | SHORT_INODE,
	/*SCO Unix Version 3.2*/
	PER_SCOSVR3 =		0x0003 | STICKY_TIMEOUTS |
					 WHOLE_SECONDS | SHORT_INODE,
	/*SCO OpenServer Release 5*/
	PER_OSR5 =		0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS,
	/*Unix System V/386 Release 3.2.1*/
	PER_WYSEV386 =		0x0004 | STICKY_TIMEOUTS | SHORT_INODE,
	/*交互式Unix*/
	PER_ISCR4 =		0x0005 | STICKY_TIMEOUTS,
	/*BSD Unix*/
	PER_BSD =		0x0006,
	/*SunOS*/
	PER_SUNOS =		0x0006 | STICKY_TIMEOUTS,
	/*Xenix*/
	PER_XENIX =		0x0007 | STICKY_TIMEOUTS | SHORT_INODE,
	/*64位结构中32位Linux程序模拟(使用4GB用户态地址空间)*/
	PER_LINUX32 =		0x0008,
	/*64位结构中32位Linux程序模拟(使用3GB用户态地址空间)*/
	PER_LINUX32_3GB =	0x0008 | ADDR_LIMIT_3GB,
	/*SGI Irix-5 32位*/
	PER_IRIX32 =		0x0009 | STICKY_TIMEOUTS,/* IRIX5 32-bit */
	/*SGI Irix-6 32位*/
	PER_IRIXN32 =		0x000a | STICKY_TIMEOUTS,/* IRIX6 new 32-bit */
	/*SGI Irix-6 64位*/
	PER_IRIX64 =		0x000b | STICKY_TIMEOUTS,/* IRIX6 64-bit */
	/*RISC OS*/
	PER_RISCOS =		0x000c,
	/*Sun 的Solaris*/
	PER_SOLARIS =		0x000d | STICKY_TIMEOUTS,
	/*SCO（正式为Caldera)的Unix Ware7*/
	PER_UW7 =		0x000e | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
	/*Digital UNIX(Compad Tru64 UNIX)*/
	PER_OSF4 =		0x000f,			 /* OSF/1 v4 */
	/*HP的HP-UX*/
	PER_HPUX =		0x0010,
	PER_MASK =		0x00ff,
};


/*
 * Description of an execution domain.
 * 
 * The first two members are refernced from assembly source
 * and should stay where they are unless explicitly needed.
 */
typedef void (*handler_t)(int, struct pt_regs *);

struct exec_domain {
	const char		*name;		/* name of the execdomain */
	handler_t		handler;	/* handler for syscalls */
	unsigned char		pers_low;	/* lowest personality */
	unsigned char		pers_high;	/* highest personality */
	unsigned long		*signal_map;	/* signal mapping */
	unsigned long		*signal_invmap;	/* reverse signal mapping */
	struct map_segment	*err_map;	/* error mapping */
	struct map_segment	*socktype_map;	/* socket type mapping */
	struct map_segment	*sockopt_map;	/* socket option mapping */
	struct map_segment	*af_map;	/* address family mapping */
	struct module		*module;	/* module context of the ed. */
	struct exec_domain	*next;		/* linked list (internal) */
};

/*
 * Return the base personality without flags.
 */
#define personality(pers)	(pers & PER_MASK)

/*
 * Personality of the currently running process.
 */
#define get_personality		(current->personality)

/*
 * Change personality of the currently running process.
 */
#define set_personality(pers) \
	((current->personality == pers) ? 0 : __set_personality(pers))

#endif /* _LINUX_PERSONALITY_H */
