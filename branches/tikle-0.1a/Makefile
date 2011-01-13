CC = gcc
CFLAGS = -Wall -ggdb -I..

all: check_update

check_update: check_update.c
	${CC} -o $@ ${CFLAGS} $? -lcurl
clean:
	rm -f *~ *.o check_update
