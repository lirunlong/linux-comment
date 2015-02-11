obj-m :=  plat_dev2.o plat_drv2.o

KERNEL := /lib/modules/`uname -r`/build/

all:
	make -C $(KERNEL) M=`pwd` modules
install:
	make -C $(KERNEL) M=`pwd` modules_install
	depmod -A
clean:
	make -C $(KERNEL) M=`pwd` clean
	rm -f Module.* modules.* .*.cmd

