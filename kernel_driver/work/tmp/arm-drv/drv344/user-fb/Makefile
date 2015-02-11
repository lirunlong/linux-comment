DIR := /nfsboot/qt_root/usr/local/app_fb

all:
	arm-linux-gcc fblib.c test1.c -o fb_test1
	cp fb_test1 /nfsboot/qt_root/usr/bin
	arm-linux-gcc fblib.c test2.c -o fb_test2
	cp fb_test2 /nfsboot/qt_root/usr/bin
	rm -fr $(DIR)
	mkdir $(DIR)
	cp pic/* $(DIR)

clean:
	rm fb_test* -rf
