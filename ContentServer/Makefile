OBJS	= ContentServer.o Communicate.o ContentServerFunctions.o
SOURCE	= ContentServer.c Communicate.c ContentServerFunctions.c
HEADER	= Communicate.h ContentServerFunctions.h
CC	= gcc
LFLAG	= -g -o content_server
FLAG	= -c -g

all: content_server

content_server: $(OBJS)
	$(CC) $(LFLAG) $(OBJS) -lpthread

ContentServer.o: ContentServer.c
	$(CC) $(FLAG) ContentServer.c

Communicate.o: Communicate.c
	$(CC) $(FLAG) Communicate.c

ContentServerFunctions.o: ContentServerFunctions.c
	$(CC) $(FLAG) ContentServerFunctions.c

clean:
	rm -f $(OBJS)

count:
	wc $(SOURCE) $(HEADER) 
