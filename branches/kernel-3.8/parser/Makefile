CC = gcc
RE2C = re2c
CFLAGS = -Wall -ggdb -I.. -c
RE2C_FLAGS =
LEMON_FLAGS = -c -q
LEMON = ./lemon

dispatcher: parser.o scanner.o main.o
	$(CC) -Wall -o $@ $?  -Wunused-variable

all: parser

scanner.o: scanner.c
	$(CC) $(CFLAGS) $?
scanner.c: scanner.re
	$(RE2C) $(RE2C_FLAGS) -o $@ $?
main.o: main.c
	$(CC) $(CFLAGS) $?
parser.o: parser.c
	cat lemon_parser.c >> parser.c
	$(CC) $(CFLAGS) $? 
parser.c: lemon
	$(LEMON) $(LEMON_FLAGS) parser.y 
lemon: lemon.o
	$(CC) -Wall -o $@ $?
lemon.o:
	$(CC) -o $@ $(CFLAGS) lemon.c
clean:
	rm -f *~ *.o dispatcher scanner.c lemon parser.{c,h,out}
