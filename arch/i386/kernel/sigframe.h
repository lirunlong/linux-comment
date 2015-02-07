struct sigframe
{
	/*信号处理函数的返回地址，它指向__kernel_sigreturn标志处的代码*/
	char *pretcode;
	/*信号编号*/
	int sig;
	/*包含正好切换到内核态前用户态进程的硬件上下文（从current的内核态堆栈中拷贝过来的)，还包含进程被阻塞的常规信号位数组*/
	struct sigcontext sc;
	/*可以用来存放用户太进程的浮点寄存器内容*/
	struct _fpstate fpstate;
	/*被阻塞的实时信号位数组*/
	unsigned long extramask[_NSIG_WORDS-1];
	/*发出sigreturn()系统调用的8字节代码。*/
	char retcode[8];
};

struct rt_sigframe
{
	char *pretcode;
	int sig;
	struct siginfo *pinfo;
	void *puc;
	struct siginfo info;
	struct ucontext uc;
	struct _fpstate fpstate;
	char retcode[8];
};
