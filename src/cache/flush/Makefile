obj-m += flush.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	sudo insmod flush.ko
	sudo rmmod flush

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
