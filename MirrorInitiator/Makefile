OBJS	= initiator.o Communicate.o initiatorFunctions.o
SOURCE	= initiator.c Communicate.c initiatorFunctions.c
HEADER	= Communicate.h initiator.h
CC	= gcc
LFLAG	= -g -o initiator
FLAG	= -c -g

all: initiator

initiator: $(OBJS)
	$(CC) $(LFLAG) $(OBJS) -lpthread

initiator.o: initiator.c
	$(CC) $(FLAG) initiator.c

Communicate.o: Communicate.c
	$(CC) $(FLAG) Communicate.c

initiatorFunctions.o: initiatorFunctions.c
	$(CC) $(FLAG) initiatorFunctions.c

clean:
	rm -f $(OBJS)

count:
	wc $(SOURCE) $(HEADER) 
