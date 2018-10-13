#include "initiator.h"

void printList(ContentServerList** head)
{
	// print the Content Server List  (created by -s argument)
	printf("===ContentServerList===\n");
	ContentServerList* temp = *head;
	while(temp!=NULL)
	{
		printf("[*] %s\n",temp->cs);
		temp = temp->next;
	}
	printf("===End Of List===\n");
}

void decode_statistics(char* buff){	
	// decodes and prints the statistics
	char* ip        = strtok(buff,",");
	char* port      = strtok(NULL,",");
	char* files     = strtok(NULL,",");
	char* bytes     = strtok(NULL,",");
	int   connected = atoi(strtok(NULL,","));
	if(connected)
	{
		printf("[*][Content Server %s:%s] files: %s bytes: %s \n",ip,port,files,bytes);
	}
	else
	{
		printf("[*][Content Server %s:%s] Couldn't Establish Connection \n",ip,port);
	}

}


