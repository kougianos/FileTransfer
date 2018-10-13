#include "ThreadLib.h"

// Thread's Functions / Variables 

// Mutexes 
pthread_mutex_t buffer_lock; 
pthread_cond_t cond_empty;
pthread_cond_t cond_full;

void Mutex_Initialize()
{
	pthread_mutex_init(&buffer_lock, 0);
	pthread_cond_init(&cond_empty, 0);
	pthread_cond_init(&cond_full, 0);	
}

void Mutex_Destroy()
{
	pthread_cond_destroy(&cond_empty);
	pthread_cond_destroy(&cond_full);
	pthread_mutex_destroy(&buffer_lock);
}


void Statistics_Mutex_Initialize()
{	
	pthread_mutex_init(&devices_lock, 0);
	pthread_cond_init(&allDone, 0);
}

void Statistics_Mutex_Destroy()
{
	pthread_cond_destroy(&allDone);
	pthread_mutex_destroy(&devices_lock);
}

/* Producer */ 
void *Producer(void *argp)
{
	char* command = (char*) malloc(256* sizeof(char));

	// Detach Thread 
	if( pthread_detach(pthread_self()) )
	{
		perror("pthread_detach");
		free(command);
		pthread_exit(NULL);
	}

	ContentServers* cs = ( ContentServers* ) argp;
	int sockfd;	
	struct sockaddr_in content_serv_addr;
	
	printf("[Thread %ld] >> Producer Got bellow cs: \n",(long)pthread_self());
	printf("[Thread %ld] >> address: %s\n",(long)pthread_self(),cs->address);
	printf("[Thread %ld] >> port: %d\n",(long)pthread_self(),cs->port);
	printf("[Thread %ld] >> dirname: %s\n",(long)pthread_self(),cs->dirname);
	printf("[Thread %ld] >> delay: %d\n",(long)pthread_self(),cs->delay);

	//Lock Content_Server_List
	pthread_mutex_lock(&list_lock);	
	while( accessed )
	{
		printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
		pthread_cond_wait(&list_accessed, &list_lock);
	}

	accessed = 1;
	int content_server_id;
	// Add new Content_Server_id to List or get it's id
	if( existContentServerList(cs->port, cs->address) == NULL )
		content_server_id = addContentServerList(cs->address,cs->port,cs->delay);	
	else
		content_server_id = getId(cs->address, cs->port);
	//Get pointer to entry's struct
	ContentServerList* contentServerPtr = existContentServerList(cs->port, cs->address);

	// List unlock & Broadcast	
	accessed = 0;
	pthread_mutex_unlock(&list_lock);
	pthread_cond_broadcast(&list_accessed);	

	// OPEN SOCKET
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("\n Error : Could not create socket \n");
		free(command);
		pthread_exit(NULL);
	}

	content_serv_addr.sin_family = AF_INET;
	content_serv_addr.sin_port = htons(cs->port);

	printf("[Tread %ld] >> Address to connect: %s in port:%d\n",(long)pthread_self(),cs->address,cs->port);

	// CONVERT IP FROM HUMAN READABLE TO BYTE, AND ADD TO STRUCT
	if(inet_aton(cs->address , &content_serv_addr.sin_addr)==0)
	{
		perror("\n Error: inet_aton error occured\n Probably [INVALID IP]\n");
		pthread_exit(NULL);
	} 

	// CONNECT TO CONTENT_SERVER TCP 
	if( connect(sockfd, (struct sockaddr *)&content_serv_addr, sizeof(content_serv_addr)) < 0)
	{
		//Server does not exist
		perror("\n Error : Connect Failed \n");
		//Lock Content_Server_List
		pthread_mutex_lock(&list_lock);	
		while( accessed )
		{
			printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
			pthread_cond_wait(&list_accessed, &list_lock);
		}

		accessed = 1;
		// Change To No_Server Exist
		contentServerPtr->serverOn = 0;
		contentServerPtr->updating = 0;

		// List unlock		
		accessed = 0;
		pthread_mutex_unlock(&list_lock);
		pthread_cond_broadcast(&list_accessed);	// Broadcast to other producers
		pthread_cond_signal(&allDone);		// Broad cast to main cuase of race condition
		free(command);
		pthread_exit(NULL);
	} 

	//Send LIST command
	strcpy(command, "LIST ");
	char tstr[15];
	memset(tstr, '\0', 15);
	sprintf(tstr, "%d ", content_server_id);
	command = strcat(command, tstr);
	sprintf(tstr, "%d", cs->delay);
	command = strcat(command, tstr);

	if( write_all(sockfd, command, strlen(command)+1) == -1 )
	{
		perror("write - Producer_thread");
		pthread_exit(NULL);
	}
	
	// Read Header directory 
	char* response = (char*) malloc(1024*sizeof(char));
	memset(response, '\0', sizeof(response));
	if( read_all(sockfd,response) == -1 )
	{
		perror("read - Producer_thread");
		close(sockfd);
		pthread_exit(NULL);
	}
	printf("[Thread %ld] >> Got ContentServer's HeaderFile path:\n{%s}\n",(long)pthread_self(),response);
	write_all(sockfd, "OK", 3);
	
	// Lock Content_Server_List To update Data of server
	pthread_mutex_lock(&list_lock);	
	while( accessed )
	{
		printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
		pthread_cond_wait(&list_accessed, &list_lock);
	}

	accessed = 1;
	strcpy(contentServerPtr->dirName,response);
	contentServerPtr->updating = 1;

	// List unlock & Broadcast
	accessed = 0;
	pthread_mutex_unlock(&list_lock);
	pthread_cond_broadcast(&list_accessed);	

	// READ & append directories to List
	for( ; ; )
	{
		memset(response, '\0', sizeof(response));
		if( read_all(sockfd,response) == -1 )
		{
			perror("read - Producer_thread");
			close(sockfd);
			pthread_exit(NULL);
		}
		
		// End file
		if( strcmp(response, "EOF") == 0 )
			break;

		if( write_all(sockfd,"OK",3) == -1 )
		{
			perror("read - Producer_thread");
			close(sockfd);
			break;
		}
	
		// Insert List response (file) in Queue (mainBuffer) if it is needed
		if( fileNeedFetching(response,cs->dirname,contentServerPtr->dirName) )
		{
			char* add_str = malloc( 256*sizeof(char));
			fixResponse(add_str,response,cs->port,cs->address);
			//Lock main_buffer
			pthread_mutex_lock(&buffer_lock);
			while( Is_Full(add_str) )	// Condition Variable 
			{
				printf("[Thread %ld] >> Found Buffer Full \n",(long)pthread_self());
				pthread_cond_wait(&cond_full, &buffer_lock);
			}
			
			writeToBuffer(add_str);
			//Lock Content_Server_List To update the filesToFetch
			pthread_mutex_lock(&list_lock);	
			while( accessed )
			{
				printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
				pthread_cond_wait(&list_accessed, &list_lock);
			}
			accessed = 1;
			contentServerPtr->numOfFetches++;

			// List unlock 		
			accessed = 0;
			pthread_mutex_unlock(&list_lock);
			pthread_cond_broadcast(&list_accessed);	// Broadcast to other producers 

 			// Queue mutex unlock 
			pthread_mutex_unlock(&buffer_lock);	
			pthread_cond_broadcast(&cond_empty);	// broadcast to consumers

			free(add_str);
		}
	}

	// Lock Content_Server_List To tell that there is no more update about this one
	pthread_mutex_lock(&list_lock);	
	while( accessed )
	{
		printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
		pthread_cond_wait(&list_accessed, &list_lock);
	}

	accessed = 1;
	contentServerPtr->updating = 0;

	// List unlock 
	accessed = 0;
	pthread_mutex_unlock(&list_lock);
	pthread_cond_broadcast(&list_accessed);	// Broadcast to other producers 
	pthread_cond_signal(&cond_empty);


	printf("[Thread %ld] >> Exiting \n",(long)pthread_self());
	free(command);
	close(sockfd);
	free(response);
	free(cs);
	pthread_exit(NULL);
}


// Consumer 
void *Consumer(void *argp) 
{	
	int files;
	char buf[100];
	ContentServers data;
	
	// Detaching 
	if( pthread_detach(pthread_self()) )
	{
		perror("pthread_detach");
		pthread_exit(NULL);
	}

	// Remove from Queue - Do tasks 
	for(; ;)
	{	
		// Mutexes / Condition Variables needed here.Race Condition! Same mutex as insert. 
		// Queue lock 
		pthread_mutex_lock(&buffer_lock);	
		while( Is_Empty() )
		{
			printf("[Thread %ld] >> Found Buffer Empty \n",(long)pthread_self());
			pthread_cond_wait(&cond_empty, &buffer_lock);
			if( allProducersDone() && Is_Empty() )
				pthread_cond_signal(&allDone);
		}

		Extract_Data(&data);	
	
		// Queue unlock 
		pthread_mutex_unlock(&buffer_lock);	
		pthread_cond_broadcast(&cond_full);	// Broadcast to producers 
		
		printf("[Thread %ld] >> Got to do: ",(long)pthread_self());
		printf("<DirName:%s, ",data.dirname);
		printf("ContentServerAddress:%s, ",data.address);
		printf("ContentServerPort:%d,",data.port);
		printf("Delay:%d>\n",data.delay);
		copyAdministrator(data);

		printf("[Thread %ld] >> Consumer will restart\n",(long)pthread_self());

		//Buffer Empty && noProducers Then signal to main
		if( allProducersDone() &&  Is_Empty() )
			pthread_cond_signal(&allDone);
	}
	pthread_exit(NULL);
}

