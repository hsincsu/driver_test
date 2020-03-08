# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Binxin network device drivers.
#
# insomnia@2020/1/28 15:21:57

KERNELDIR:=/lib/modules/5.0.5-1.el7.elrepo.x86_64/build                 
PWD:=$(shell pwd)

obj-m := rnic.o
rnic-objs :=	bx_rnic.o \
                bx_rnic_com.o \
				bx_mac_net.o \
				bx_mac_desc.o \
				bx_mac_hw.o \
				bx_mac_ethtool.o \
				bx_mac_cfg.o \
                bx_mac.o \
				bx_roce.o\
				bx_mpu.o \
				bx_pcs.o \
				bx_mpp.o \
				bx_pcie.o \
				bx_mpb.o \
				bx_cm.o \
				bx_phd.o \
				bx_pbu.o \
				bx_ieu.o
				

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
