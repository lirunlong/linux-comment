/*
 * Wrapper functions for accessing the file_struct fd array.
 */

#ifndef __LINUX_FILE_H
#define __LINUX_FILE_H

#include <asm/atomic.h>
#include <linux/posix_types.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>

/*
 * The default fd array needs to be at least BITS_PER_LONG,
 * as this is the granularity returned by copy_fdset().
 */
#define NR_OPEN_DEFAULT BITS_PER_LONG

/*
 * Open file table structure
 */
struct files_struct {
	/*共享这个表的进程数*/
        atomic_t count;
        spinlock_t file_lock;     /* Protects all the below members.  Nests inside tsk->alloc_lock */
		/*初始设置为NR_OPEN_DEFAULT 一般为32,指fd指针数组的大小，通过expand_fd_array扩展，如果打开的文件数超过32*/
        int max_fds;
		/*对应open_fds能表示的文件的个数*/
        int max_fdset;
        int next_fd;
		/*默认指向fd_array字段，如果超过NR_OPEN_DEFAULT 则重现分配*/
        struct file ** fd;      /* current fd array */
		/*对应close_on_exec_init,和open_fds的描述相同，大小和open_fds相等*/
        fd_set *close_on_exec;
		/*用位图表示打开的文件，对应位1，表示相应的fd有效，初始指向open_fds_init,如果不够用，则重新alloc(通过expand_fdset函数)*/
        fd_set *open_fds;
        fd_set close_on_exec_init;
        fd_set open_fds_init;
        struct file * fd_array[NR_OPEN_DEFAULT];
};

extern void FASTCALL(__fput(struct file *));
extern void FASTCALL(fput(struct file *));

static inline void fput_light(struct file *file, int fput_needed)
{
	if (unlikely(fput_needed))
		fput(file);
}

extern struct file * FASTCALL(fget(unsigned int fd));
extern struct file * FASTCALL(fget_light(unsigned int fd, int *fput_needed));
extern void FASTCALL(set_close_on_exec(unsigned int fd, int flag));
extern void put_filp(struct file *);
extern int get_unused_fd(void);
extern void FASTCALL(put_unused_fd(unsigned int fd));
struct kmem_cache_s;
extern void filp_ctor(void * objp, struct kmem_cache_s *cachep, unsigned long cflags);
extern void filp_dtor(void * objp, struct kmem_cache_s *cachep, unsigned long dflags);

extern struct file ** alloc_fd_array(int);
extern void free_fd_array(struct file **, int);

extern fd_set *alloc_fdset(int);
extern void free_fdset(fd_set *, int);

extern int expand_files(struct files_struct *, int nr);

static inline struct file * fcheck_files(struct files_struct *files, unsigned int fd)
{
	struct file * file = NULL;

	if (fd < files->max_fds)
		file = files->fd[fd];
	return file;
}

/*
 * Check whether the specified fd has an open file.
 */
#define fcheck(fd)	fcheck_files(current->files, fd)

extern void FASTCALL(fd_install(unsigned int fd, struct file * file));

struct task_struct;

struct files_struct *get_files_struct(struct task_struct *);
void FASTCALL(put_files_struct(struct files_struct *fs));

#endif /* __LINUX_FILE_H */
