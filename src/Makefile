IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)
OPTIM=-O3 -ggdb

ODIR=obj
LDIR =../lib

LIBS=-ljson-c -lcurl -lpthread
STATIC_LIBS=../lib/libtelebot_static.a

main: std-display.o std-bot.o asciiLetters.o main.c
	$(CC) -o $@ $^ $(STATIC_LIBS) $(CFLAGS) $(LIBS) $(OPTIM)

asciiLetters.o: asciiLetters.c
	$(CC) -c -o $@ $< $(CFLAGS) $(OPTIM)

std-display.o: std-display.c
	$(CC) -c -o $@ $< $(CFLAGS) $(OPTIM)

std-bot.o: std-bot.c
	$(CC) -c -o $@ $< $(CFLAGS) $(OPTIM)

.PHONY: clean

clean: 
	rm *.o main
