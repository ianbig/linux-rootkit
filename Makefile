ifeq ($(KERNELRELEASE),)  

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)  

.PHONY: build clean process

build: sneaky_process
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
sneaky_process: sneaky_process.c
	gcc -Wall -Werror -pedantic --std=c99 $< -o $@

clean:
	rm -rf *.o *~ core .depend .*.cmd *.order *.symvers *.ko *.mod.c sneaky_process
else  

$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 
obj-m :=    sneaky_mod.o  

endif
