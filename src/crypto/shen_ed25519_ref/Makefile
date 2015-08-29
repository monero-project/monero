
CC=gcc
CFLAGS=-O2 -Wall

OBJS= fe25519.o ge25519.o sc25519.o sha512-blocks.o sha512-hash.o ed25519.o randombytes.o verify.o
test: test.o $(OBJS)
	gcc -o $@ $^

clean:
	rm -f *.o test
