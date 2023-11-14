ifndef KERNEL
KERNEL = /lib/modules/$(shell uname -r)/build
endif
ifndef OUTPUT
OUTPUT = ./amifldrv_mod.o
endif

EXTRA_CFLAGS		:= -Wall -Wstrict-prototypes -Wno-int-to-pointer-cast -O2 -fno-strict-aliasing -DBUILD_AMIFLDRV_MOD
obj-m				+= amifldrv_mod.o
amifldrv_mod-objs	:= amifldrv.o amiwrap.o

.PHONY : default clean

default:
	@if [ -d $(KERNEL) ]; then \
		make -C $(KERNEL) M=$(PWD) modules; \
		mv amifldrv_mod.ko $(OUTPUT); \
	fi

clean:
	rm -f *.*~
	rm -f test
	rm -f .*cmd
	rm -f *.o
	rm -f *mod*
	rm -f *.ko
	rm -rf .tmp_versions/
	rm -f Module.symvers
