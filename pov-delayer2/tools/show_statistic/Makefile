LDFLAGS=-lncurses -lpq
CFLAGS=-c -Wall -std=gnu99
IFLAGS=-I ./ -I ../../../pov-database/src/c/ -I ../../../pov-common/ -I /usr/include/postgresql/
CC=gcc

all: show_statistic

show_statistic: show_statistic.o dbcommon.o dbstreaminterfaces.o dbdelayers.o dbfile.o
	$(CC) show_statistic.o dbcommon.o dbstreaminterfaces.o dbdelayers.o dbfile.o -o show_statistic $(LDFLAGS)

show_statistic.o: show_statistic.c
	$(CC) $(CFLAGS) $(IFLAGS) show_statistic.c

dbcommon.o: ../../../pov-database/src/c/dbcommon.c
	$(CC) $(CFLAGS) $(IFLAGS) ../../../pov-database/src/c/dbcommon.c -o dbcommon.o

dbstreaminterfaces.o: ../../../pov-database/src/c/dbstreaminterfaces.c
	$(CC) $(CFLAGS) $(IFLAGS) ../../../pov-database/src/c/dbstreaminterfaces.c -o dbstreaminterfaces.o

dbdelayers.o: ../../../pov-database/src/c/dbdelayers.c
	$(CC) $(CFLAGS) $(IFLAGS) ../../../pov-database/src/c/dbdelayers.c -o dbdelayers.o

dbfile.o: ../../../pov-database/src/c/dbfile.c
	$(CC) $(CFLAGS) $(IFLAGS) ../../../pov-database/src/c/dbfile.c -o dbfile.o


clean:
	rm *.o show_statistic
