obj-m = tikle.o 
tikle-objs := main.o config.o#tikle_core.o tikle_hooks.o tikle_fh.o tikle_comm.o
KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
DEBUG = y

ifeq ($(DEBUG),y)
  DEBFLAGS = -DTIKLE_DEBUG
  EXTRA_CFLAGS += $(DEBFLAGS)
endif

all: module

module:
	make -C $(MODCFLAGS) $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
