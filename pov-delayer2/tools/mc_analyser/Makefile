LDFLAGS=
CFLAGS=-c -Wall -std=gnu99
IFLAGS=-I ./
CC=cc

all: mc_receiver

mc_receiver: receiver.o print_struct.o
	$(CC) $(LDFLAGS) receiver.o print_struct.o -o mc_receiver

receiver.o: receiver.c
	$(CC) $(CFLAGS) $(IFLAGS) receiver.c

print_struct.o: ../../../pov-common/print_struct.c
	$(CC) $(CFLAGS) $(IFLAGS) ../../../pov-common/print_struct.c

clean:
	rm *.o mc_receiver
