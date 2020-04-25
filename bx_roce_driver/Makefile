obj-m += bxroce.o

bxroce-y := bxroce_main.o bxroce_verbs.o bxroce_ah.o bxroce_hw.o bxroce_pool.o bxroce_mpb_reg.o

KERNELDIR:=/lib/modules/5.0.5/build
PWD:=$(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

