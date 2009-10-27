obj-m = tikle.o 
tikle-objs := tikle_core.o tikle_hooks.o tikle_fh.o tikle_comm.o
KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
DEBUG = y

ifeq ($(DEBUG),y)
  DEBFLAGS = -DTIKLE_DEBUG
  EXTRA_CFLAGS += $(DEBFLAGS)
endif

all: module parser

clean: clean-module

cleanall: clean-module
	rm -f *~ *odule*

module:
	make -C $(MODCFLAGS) $(KDIR) M=$(PWD) modules

clean-module: clean-parser
	make -C $(KDIR) M=$(PWD) clean

docs:
	doxygen src/docs.mk && cd docs/latex && make && cd ../..

clean-docs:
	rm -rf docs

parser:
	cd src/parsing && make && cd ../..

clean-parser: clean-docs
	cd src/parsing && rm -f *~ *.o dispatcher scanner.c lemon parser.{c,h,out} && cd ../..

