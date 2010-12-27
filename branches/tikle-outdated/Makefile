obj-m = tikle.o
KVERSION := $(shell uname -r)

all: module parser

clean: clean-module

cleanall: clean-module
	rm -f *~ *odule*

module:
	make -C $(MODCFLAGS) /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean-module: clean-parser
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

docs:
	doxygen src/docs.mk && cd docs/latex && make && cd ../..

clean-docs:
	rm -rf docs

parser:
	cd src/parsing && make && cd ../..

clean-parser: clean-docs
	cd src/parsing && rm -f *~ *.o dispatcher scanner.c lemon parser.{c,h,out} && cd ../..

