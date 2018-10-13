OBJS	= MirrorServer.o List.o Communicate.o MainBuffer.o MirrorServerFunctions.o ThreadLib.o
SOURCE	= MirrorServer.c Communicate.c List.c MainBuffer.c MirrorServerFunctions.c ThreadLib.c
HEADER	= Communicate.h MirrorServer.h List.h MainBuffer.h ThreadLib.h
CC	= gcc
LFLAG	= -g -o mirror_server
FLAG	= -c -g

all: mirror_server

mirror_server: $(OBJS)
	$(CC) $(LFLAG) $(OBJS) -lpthread

MirrorServer.o: MirrorServer.c
	$(CC) $(FLAG) MirrorServer.c

Communicate.o: Communicate.c
	$(CC) $(FLAG) Communicate.c

List.o: List.c
	$(CC) $(FLAG) List.c

MainBuffer.o: MainBuffer.c
	$(CC) $(FLAG) MainBuffer.c

MirrorServerFunctions.o: MirrorServerFunctions.c
	$(CC) $(FLAG) MirrorServerFunctions.c

ThreadLib.o: ThreadLib.c
	$(CC) $(FLAG) ThreadLib.c

clean:
	rm -f $(OBJS)

count:
	wc $(SOURCE) $(HEADER) 
