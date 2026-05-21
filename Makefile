obj-m := mpu9250.o
mpu9250-objs = main.o mpu9250_helper.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

#set KERN_DIR to linux source location (Beaglebone)
KERN_DIR = /home/raj-pc/RajDev/Linux_Device_Driver/linux

#set KERN_DIR to linux source location (Linux Ubuntu)
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

ccflags-y := -DMODULE_DEBUG


all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

dtc:
	dtc -@ -I dts -O dtb -o mpu9250.dtbo mpu9250.dts

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
	rm *.dtbo

clean_host:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean	

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help

host:
	make -C $(HOST_KERN_DIR) M=$(PWD)  modules

copy_ko:
	scp *.ko debian@192.168.7.2:/home/debian/drivers/02_i2c_Driver/main.ko

copy_dtbo:
	scp *.ko debian@192.168.7.2:/home/debian/drivers/02_i2c_Driver/mpu6050.dtbo

insert:
	sudo insmod main.ko

remove:
	sudo rmmod main.ko

get_info:
	file main.ko
	modinfo main.ko

show_logs:
	sudo dmesg -xT | tail -20

perm:
	sudo chmod 777 /dev/my_device

preprocess:
	make -C $(HOST_KERN_DIR) M=$(PWD)  main.i

user_app:
	gcc user_app.c -o user_app.o

clean_user:
	rm user_app.o

