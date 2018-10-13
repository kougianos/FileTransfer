#include "MirrorServer.h"

void copyAdministrator(ContentServers cs)
{
	// Administrato to do the fileCopy (&Fetch)
	int sock;
	struct stat st = {0};
	struct sockaddr_in server;
	struct sockaddr *serverptr = (struct sockaddr *)&server;
	struct hostent *rem;	
	char* command = (char*) malloc( 128 * sizeof(char));

	// Open|Create Port - Socket 
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("socket - copyAdministrator");
		pthread_exit(NULL);
	}
	// Find server address 
	if( (rem = gethostbyname(cs.address)) == NULL)
	{
		perror("gethostbyname - copyAdministrator");
		pthread_exit(NULL);
	}
	server.sin_family = AF_INET;
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
	server.sin_port = htons(cs.port);
	// Initiate connection
	if( connect(sock, serverptr, sizeof(server)) < 0 )
	{
		perror("connect");
		pthread_exit(NULL);
	}

	printf("[Thread %ld] >> Connecting to Content_ Server [%s] on port [%d]...\n\n",(long)pthread_self(),cs.address,cs.port);

	// Send command Fetch
	strcpy(command, "FETCH ");
	command = strcat(command, cs.dirname);
	if( write_all(sock, command, strlen(command)+1) == -1 )
	{
		perror("write - copyAdministrator");
		pthread_exit(NULL);
	}
	
	// Fetch File
	fetchFile(sock, cs);

	close(sock);
	free(command);
}





int fetchFile(int sock, ContentServers cs)
{
	// Will Create & Fetch FIle
	clock_t start = clock();
	// Get instance to our content server for mutexex if needed
	ContentServerList* cs_instance = existContentServerList(cs.port,cs.address);

	// Write Properties for File
	mode_t mode = S_IRUSR | S_IWUSR |S_IXUSR| S_IRGRP | S_IROTH|S_IXGRP|S_IXOTH;
	struct stat st = {0};
	
	char path[256];
	memset(path, '\0', 256);
	strcpy(path,dirname);
	char temp[256];
	memset(temp, '\0', 256);
	sprintf(temp, "/%s_%d",cs.address,cs.port);
	strcat(path,temp);

	// Create contnet server's directory
	if( stat(path, &st) == -1 )
	{	// If Directory does not exists
		// Lock so that only I am creating the folder/file
		pthread_mutex_lock(&(cs_instance->dir_lock));	
		while( cs_instance->on_creation )
		{
			printf("[Thread %ld] >> Someone else is creating. \n",(long)pthread_self());
			pthread_cond_wait(&(cs_instance->dir_creating), &(cs_instance->dir_lock));
		}

		cs_instance->on_creation = 1;
		// Avoid first time raceCondition error
		if( stat(path, &st) == -1 )
			mkdir(path, 0755);	  // Create it
		
		// List unlock	& Broadcast	
		cs_instance->on_creation = 0;
		pthread_mutex_unlock(&(cs_instance->dir_lock));
		pthread_cond_broadcast(&(cs_instance->dir_creating));
	}
	
	// Go for the creation of the file. Create slowly the path (envelops)
	strcat(path,"/");
	memmove(cs.dirname,cs.dirname+2,strlen(cs.dirname)-2+1);	//delete first 2 chars (./)
	memset(temp, '\0', 256);
	strcpy(temp,cs.dirname);
	char* token = strtok(temp, "/");
	while( token != NULL )
	{	
		strcat(path,token);
		if( stat(path, &st) == -1 ){	// If Directory does not exists 
			// Lock so that only I am creating the folder/file
			pthread_mutex_lock(&(cs_instance->dir_lock));	
			while( cs_instance->on_creation )
			{
				printf("[Thread %ld] >> Someone else is creating. \n",(long)pthread_self());
				pthread_cond_wait(&(cs_instance->dir_creating), &(cs_instance->dir_lock));
			}

			cs_instance->on_creation = 1;
			//Avoid first time raceCondition error
			if( stat(path, &st) == -1 )
				mkdir(path, 0755);	  // Create it
			
			// List unlock	& Broadcast	
			cs_instance->on_creation = 0;
			pthread_mutex_unlock(&(cs_instance->dir_lock));
			pthread_cond_broadcast(&(cs_instance->dir_creating));
		}

		//Get next folder/File in path
		strcat(path,"/");
		token = strtok(NULL, "/");
	}

	//Delete last character
	path[strlen(path)-1] = '\0';
	
	// The Last one is File so change it from directory to file
	remove(path);
	int fd;
	if( (fd = open(path, O_WRONLY | O_CREAT, mode )) < 0 )
	{
		perror("open - fetch");
		close(sock);
		pthread_exit(NULL);
	}

	//Delay - sleep for time
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	if( (((float)cs.delay) - seconds) > 0)
		sleep(cs.delay - seconds);
	
	// Read Page size
	memset(temp, '\0', 256);
	read_all(sock,temp);
	int page_size = atoi(temp);
	write_all(sock, "OK", 3);
	char* file_bufer = (char *)malloc(page_size);

	// Write on file
	int recieve;
	unsigned long bytes_received = 0;
	while(1)
	{
		// Read Data of File
		if( (recieve = read_all(sock,file_bufer)) < 0 )
		{
			perror("read");
			exit(3);
		}
		bytes_received += recieve;
		write_all(sock, "OK", 3);

		//Test if file ended
		if( strcmp(file_bufer,"EOF") == 0 )				
			break;

		// Write on File
		if( write(fd,file_bufer, recieve) < 0 )
		{
			perror("write");
			exit(4);
		}
			
		memset(file_bufer, 0, page_size);						
	}

	// Inform list about Fetches
	pthread_mutex_lock(&list_lock);	
	while( accessed )
	{
		printf("[Thread %ld] >> List is already accessed. \n",(long)pthread_self());
		pthread_cond_wait(&list_accessed, &list_lock);
	}
	
	accessed = 1;
	cs_instance->files++;
	cs_instance->numOfFetches--;
	cs_instance->bytes += bytes_received;
	insertFileList(cs_instance,bytes_received);
	
	// List unlock & Broadcast	
	accessed = 0;
	pthread_mutex_unlock(&list_lock);
	pthread_cond_broadcast(&list_accessed);	

	close(fd);
	free(file_bufer);		
}

void sendStatistics(int sock)
{
	// Send the statistics about this loop back to initiator
	char* str_stats = (char*) malloc(256 * sizeof(char));
	ContentServerList* temp;
	temp = csl;
	
	memset(str_stats, '\0', 256* sizeof(char) );
	sprintf(str_stats, "%d", contentServersNum());
	write_all(sock, str_stats, strlen(str_stats)+1);
	read_all(sock,str_stats);

	// Send server based stats
	while( temp != NULL )
	{
		memset(str_stats, '\0', 256* sizeof(char) );
		getStats(str_stats,temp);
		write_all(sock, str_stats, strlen(str_stats)+1);
		read_all(sock,str_stats);
		temp = temp->next;
	}

	// Send average stats
	unsigned long bytes = getTotalBytes();
	int files = getTotalFiles();
	double average = 0;
	if( files != 0 )
		average = (double)bytes / (double)files;

	double variance = calculateVariance(average);
	if( files != 0 )
		variance = variance / (double)files;
	memset(str_stats, '\0', 256* sizeof(char) );
	sprintf(str_stats, "%g,%g", average, variance);
	write_all(sock, str_stats, strlen(str_stats)+1);
	read_all(sock,str_stats);

	free(str_stats);
}
