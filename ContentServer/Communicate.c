#include "Communicate.h"


int write_all(int socket, char *buff, int size)
{
	int sent, n;
	char data[10];
	memset(data,'0',10);
	sprintf(data,"%d",size);

	if( write(socket, data, 10 ) < 0 )
		return -1;	

	for( sent = 0; sent < size; sent+=n )
	{
		if( (n = write(socket, buff+sent, size-sent)) == -1 )
			return -1;
	}

	
	return sent;	
}



int read_all(int socket, char *buff)
{
	int max,recieve, n = 0;
	char data[10];
	
	if( read(socket,data, 10) < 0 )
		return -1;

	/* Msg Did not recieve. Try again*/
	while( strcmp(data,"") == 0 )
		if( read(socket,data, 10) < 0 )
			return -1;

	max = atoi(data);
	
	while( n < max )	
	{
		if( (n += read(socket, buff, max - n)) < 0  )
			return -1;
	}
	
	return n;	
}
