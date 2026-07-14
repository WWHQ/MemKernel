ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
	obj-$(CONFIG_MEMKERNEL)  := memkernel.o
	memkernel-y := memory.o process.o entry.o hide_proc.o virtpipe-common-yyb.o\
                   inline_hook/p_lkrg_main.o \
                   inline_hook/p_hmem.o \
                   inline_hook/p_hook.o \
                   inline_hook/utils/p_memory.o \
                   inline_hook/p_transit_stubs.o

    EXTRA_CFLAGS += -O2 \
                -Wno-error=incompatible-pointer-types \
                -Wno-error=unused-function \
                -Wno-error=unused-variable \
                -Wno-error=unused-but-set-variable \
                -Wno-declaration-after-statement \
                -Wno-unused-variable
else
# normal makefile
default:
	# Assume kernel source is at KDIR or fail
	ifeq ($(KDIR),)
		$(error KDIR is not set. Please define KDIR to point to the kernel source directory.)
	endif
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers .*.cmd
endif
