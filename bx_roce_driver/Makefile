obj-m += bxroce.o

bxroce-y := bxroce_main.o bxroce_verbs.o bxroce_ah.o bxroce_hw.o bxroce_pool.o

KERNELDIR:=/lib/modules/5.0.5-1.el7.elrepo.x86_64/build
PWD:=$(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

