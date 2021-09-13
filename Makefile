obj-m += r0memdump.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

start:
	sudo dmesg -C
	sudo insmod r0memdump.ko
	dmesg

stop:
	sudo dmesg -C
	sudo rmmod r0memdump.ko
	dmesg
