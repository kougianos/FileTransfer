#include "List.h"

// List Mutexes
void List_Mutex_Initialize()
{
	pthread_mutex_init(&list_lock, 0);
	pthread_cond_init(&list_accessed, 0);
	accessed = 0;
}
void List_Mutex_Destroy()
{
	pthread_cond_destroy(&list_accessed);
	pthread_mutex_destroy(&list_lock);
}


// List functions
void initialize_List()
{
	id = 0;
	csl = NULL;
}

void destroy_List()
{
	while(csl != NULL ){
		ContentServerList* temp = csl;
		csl = csl->next;	
		pthread_cond_destroy(&(temp->dir_creating));
		pthread_mutex_destroy(&(temp->dir_lock));
		free(temp->address);
		free(temp->dirName);
		if( temp->fl != NULL )
			free(temp->fl);
		free(temp);
	}
}


// Content Servers & List
int contentServerExist(int port, char* address)
{
	//Return if the server exists based on address and port
	ContentServerList* temp = csl;

	while( temp != NULL ){
		if( temp->port == port && strcmp(temp->address, address) == 0 ) 
			return 1;
		temp = temp->next;
	}
	return 0;
}

int addContentServerList(char* address, int port, int delay )
{
	//Add new content server
	ContentServerList* temp;
	if( csl == NULL ){
		csl = (ContentServerList*)malloc(sizeof(ContentServerList));
		csl->next = NULL;
		temp = csl;
	}
	else
	{
		temp = csl;
		while( temp->next != NULL )
		{
			if( (temp->port == port ) &&  strcmp(temp->address, address) == 0 ) // content server exists
				return temp->id;
			temp = temp->next;
		}

		temp->next = (ContentServerList*)malloc(sizeof(ContentServerList));
		temp = temp->next;
		temp->next = NULL;
	}

	//Initialize data about server 
	temp->serverOn = 1;
	temp->port = port;
	temp->delay = delay;
	temp->id = id++;
	temp->address = (char*) malloc(256*sizeof(char));
	temp->dirName = (char*) malloc(256*sizeof(char));
	temp->dirName[0] = '\0';
	strcpy(temp->address, address);
	
	temp->bytes = 0;
	temp->files = 0;
	temp->numOfFetches = 0;
	temp->updating = 0;

	pthread_mutex_init(&(temp->dir_lock), 0);
	pthread_cond_init(&(temp->dir_creating), 0);
	temp->on_creation = 0;

	temp->fl = NULL;
	
	return temp->id;
}

ContentServerList* existContentServerList(int port, char* address)
{
	//Return the server's instance exists based on address and port
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		if( temp->port == port && strcmp(temp->address, address) == 0 ) // content server exists
			return temp;
		temp = temp->next;
	}
	return NULL;
}


// About the Content Servers

int getId(char* address, int port)
{
	//Return the server's id based on address and port
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		if( temp->port == port && strcmp(temp->address, address) == 0 ) // content server exists
			return temp->id;
		temp = temp->next;
	}
	return -1;
}

void insertFileList(ContentServerList* cs, unsigned long bytes)
{	
	//Insert the bytes of a file to the FilesList of this server
	if( cs->fl == NULL )
	{
		cs->fl = (FileInfoLise*) malloc(sizeof(FileInfoLise));
		cs->fl->next = NULL;
		cs->fl->bytes = bytes;
	}
	else
	{
		FileInfoLise* temp = cs->fl;
		while( temp->next != NULL )
			temp = temp->next;
		temp->next = (FileInfoLise*) malloc(sizeof(FileInfoLise));
		temp = temp->next;
		temp->next = NULL;
		temp->bytes = bytes;
	}	
}



int getDelay(char* address, int port)
{
	//Return the server's delay based on address and port
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		if( temp->port == port && strcmp(temp->address, address) == 0 ) // content server exists
			return temp->delay;
		temp = temp->next;
	}
	return 0;
}

unsigned long getTotalBytes()
{
	//Return total bytes sent
	unsigned long bytes = 0;
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		bytes += temp->bytes;
		temp = temp->next;
	}
	return bytes;
}

int getTotalFiles()
{
	//Will return the Total Files of all fetches
	int files = 0;
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		files += temp->files;
		temp = temp->next;
	}
	return files;
}


int onProgress(ContentServerList* cs)
{
	//Will return if contnet server is busy
	return cs->updating;
}

int contentServersNum()
{
	int count = 0;
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		count++;
		temp = temp->next;
	}
	return count;
}

//Contnet Servers & Threads
int allProducersDone()
{
	//Decide if all Producers are done
	ContentServerList* temp;
	temp = csl;
	while( temp != NULL )
	{
		if( onProgress(temp) )
			return 0;
		temp = temp->next;
	}
	return 1;
}

int decideAllDone()
{
	//Decide if all Consumers & Producers are done
	ContentServerList* temp;
	temp = csl;

	while( temp != NULL )
	{	
		if( onProgress(temp) || temp->numOfFetches != 0 )
			return 0;
		temp = temp->next;
	}
	return 1;
}


int contentServersDone()
{
	//Will return the ammount of doneWorking content servers
	int count = 0;
	ContentServerList* temp;
	temp = csl;

	while( temp != NULL )
	{
		if( onProgress(temp) == 0 && temp->numOfFetches == 0 )
			count++;
		temp = temp->next;
	}
	return count;
}


//Content Servers & Stats
void getStats(char* str,ContentServerList* cs)
{
	//Will write on str the format: ip,port,files,bytes,connected
	sprintf(str, "%s,%d,%d,%lu,%d", cs->address, cs->port, cs->files, cs->bytes, cs->serverOn );
}


double calculateVariance(double average)
{
	//Will reply the variance
	double variance = 0;
	ContentServerList* temp;
	temp = csl;

	while( temp != NULL )
	{
		FileInfoLise* tempFl = temp->fl;
		while( tempFl != NULL )
		{
			variance += ( average - tempFl->bytes ) * ( average - tempFl->bytes );
			tempFl = tempFl->next;
		}
		temp = temp->next;
	}
	return variance;
}



