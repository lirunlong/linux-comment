obj-m := proc_test02.o 
KERNEL := /lib/modules/`uname -r`/source

all:
	make -C $(KERNEL) M=`pwd` modules
install:
	make -C $(KERNEL) M=`pwd` modules_install
	depmod -A

clean:
	make -C $(KERNEL) M=`pwd` clean
	rm -f modules.* *.~ Module.* 

