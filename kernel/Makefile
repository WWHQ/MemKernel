ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
	# 直接指定为模块，避免依赖 CONFIG_MEMKERNEL 配置项
	obj-m := memkernel.o
	memkernel-y := memory.o process.o entry.o
else
# normal makefile
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif
