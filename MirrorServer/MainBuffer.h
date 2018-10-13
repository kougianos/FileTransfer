#ifndef MAINBUFFER_H
#define MAINBUFFER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUFFER_SIZE 256	


// Struct needed for transfering data to Threads
struct contentServers{
	char address[256];
	int port;
	char dirname[256];
	int delay;
};

typedef struct contentServers ContentServers;

char* main_buffer;				//Buffer is the main "Queue" of the server. Made with string

//General Function
int initiate_Buffer();
int Is_Empty();					//Return if there is command to execute in buffer (Empty or not)
int Is_Full(char*);				//Return if there is no space in buffer 
void Buffer_Destroy();

void Extract_Data(ContentServers*);		//Removes first command in buffer like FIFO
void removeSubstring(char*,const char*);	//Removes a substring from a string

//For testing
int fileNeedFetching(char*, char*, char*);	//Return if a file needs fetching. Determines by compining the command initiator gave us
void writeToBuffer(char*);			//Writes the file that needs fetching to buffer, so that consumers will execute
void fixResponse(char*, char*, int, char*);	//Make response from ContentServer to format: <dirorfilename, ContentServerAddress, ContentServerPort>
	

#endif
