ifneq ($(KERNELRELEASE), )
  obj-m := CharSnake.o
else
  KERN_DIR := /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)
  OBJ_DIR := $(PWD)/../obj
  MOD_DIR := $(PWD)/../mod

all:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) modules
	mkdir -p $(OBJ_DIR) $(MOD_DIR)
	mv *.o *.mod *.mod.c *.order *.symvers $(OBJ_DIR)
	mv *.ko $(MOD_DIR)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(MOD_DIR)

endif
