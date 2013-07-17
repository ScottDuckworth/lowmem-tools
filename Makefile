CC=gcc
CFLAGS=-Wall

all: ls-lowmem rm-lowmem

clean:
	rm -f ls-lowmem rm-lowmem *.o

ls-lowmem: ls-lowmem.o
	$(CC) -o $@ $^

rm-lowmem: rm-lowmem.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $^
