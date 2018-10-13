#include "MirrorServer.h"

#define MaxClients 128

int exit_status = 0;
int times = 0;

// signal_handler to exit
void sig_handler(int signum)
{
	exit_status = 1;
	printf("\nMirror Server Exiting..\n");
}


void usage()
{
	printf("===MirrorServer usage===\n");
	printf("./mirrorserver \n  -p <port \n -m <dir> \n -w <threadNum>\n");
}

int main( int argc, char* argv[] )
{
	int i;
	int producers_num;
	int port,thread_num;
	char* command = (char*)malloc(256*sizeof(char)); 

	// Socket Vars - Internet
	int sock,newsock;
	struct sockaddr_in server, client;
	socklen_t clientlen;
	struct sockaddr *serverptr = (struct sockaddr *) &server;
	struct sockaddr *clientptr = (struct sockaddr *) &client;
	struct hostent *rem;

	// Arguments
	if( argc < 7 )
	{
		perror("Too few arguments\n");
		usage();
		exit(1);
	}

	// Read Port/ Thread Num/ Dir
	for( i = 1; i < argc; i+=2 )
	{
		
		if( strcmp(argv[i],"-p") == 0 )
			port = atoi(argv[i+1]);

		else if( strcmp(argv[i],"-w") == 0 )
			thread_num = atoi(argv[i+1]);
		
		else if( strcmp(argv[i],"-m") == 0 )
			strcpy(dirname,argv[i+1]);		
		else
		{
			printf("Error! Wrong Arguments!\n");
			exit(1);
		}
	}

	// Check Thread Size - Not too long
	if( thread_num > MAX_THREADS )
	{
		perror("Too many threads\n");
		exit(1);
	}

	printf("\nMirrorServer's parameters are:\n");
	printf("\tport: %d\n",port);
	printf("\tthread_num: %d\n",thread_num);
	printf("\tdirname: %s\n",dirname);

	// Thread variables. Create Table of pthread_t for consumers
	pthread_t* thr;
	pthread_t* thr_prod;
	thr = malloc( thread_num * sizeof(pthread_t) );
	if( thr == NULL)
	{
		perror("thread thr - malloc");
		exit(2);
	}
	pthread_t* thr_work;
	
	
	// Common Buffer
	if( initiate_Buffer() )
	{
		perror("main_buffer - malloc");
		exit(3);
	}

	// Enstablish signal Handler for kill
	signal(SIGINT, sig_handler);

	// Init all Mutexes
	Mutex_Initialize();
	List_Mutex_Initialize();
	Statistics_Mutex_Initialize();

	// Create Consumers
	for( i=0; i<thread_num; i++ )
	{
		if( pthread_create(&thr[i], NULL, Consumer, NULL) )
		{
			perror("pthread_create - Consumer");
			exit(3);
		}
	}

	// Open|Create Port - Socket
	printf("MirrorServer >> Creating Socket..\n");
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 )
	{
		perror("socket");
		exit(2);
	}
	server.sin_family = AF_INET; 			//Internet domain 
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port); 			// The given port
	// Bind socket to address
	if( bind(sock, serverptr, sizeof(server)) < 0 ) 
	{
		perror("bind"); 
		exit(2);
	}
	// Listen for connections
	if( listen(sock,MaxClients) < 0 )
	{
		perror("listen");
		exit(2);
	}
	
	printf("MirrorServer >> Server was succesfully initialized...\n");
	printf("MirrorServer >> Listening for connections to port %d\n",port);

	// While Loop - Connections
	i = 0; 
	while( !exit_status )
	{
		// Common Content_Server_List. Initialize for every run of initiator
		initialize_List();

		// Accept connection
		clientlen = sizeof(*clientptr);
		if( (newsock = accept(sock, clientptr, &clientlen)) < 0 )
		{	
			perror("accept");
			exit(2);
		}
		printf("MirrorServer >> Accepted connection on socket: %d\n",newsock);
		
		// Read Number of Commands
		memset(command, '\0', sizeof(command));
		if( read_all(newsock,command) == -1 )
		{
			perror("read");
			close(newsock);
			exit(3);
		}
		// Chase Exiting (SIGINT received)
		if( exit_status != 0 )
		{
			write_all(newsock, "EXITING", 8);
			close(newsock);
			break;
		}

		//Create producers 1 for each command i will receive
		printf("MirrorServer >> Number of commands: %s\n",command);
		int producers_num = atoi(command);
		thr_prod = ( pthread_t* ) malloc( producers_num * sizeof(pthread_t) );
		if(thr_prod == NULL)
		{
			perror("thread_producers - malloc \n");
			exit(4);
		}
		write_all(newsock, "OK", 3);
		
		// Read command for each producer
		for(i = 0; i < producers_num; i++)
		{
			// Read & Break Command to parts
			memset(command, '\0', sizeof(command));
			if( read_all(newsock,command) == -1 )
			{
				perror("read");
				close(newsock);
				exit(3);
			}
			printf("MirrorServer >> received command: %s\n",command);

			// Create & save the command in the struct
			ContentServers* cs = (ContentServers*) malloc(sizeof(ContentServers));
			if(cs == NULL)
			{
				perror("ContentServers - malloc\n");
				exit(5);
			}
			// Break Command
			char* token;
			token = strtok(command,":");	
			strcpy(cs->address,token);
			token = strtok(NULL,":");		
			cs->port = atoi(token);
			token = strtok(NULL,":");		
			strcpy(cs->dirname,token);
			token = strtok(NULL,":");
			cs->delay = atoi(token);
		
			// Create producer
			if( pthread_create(&thr_prod[i], NULL, Producer, (void*) cs) )
			{
				printf("ERORR PRODUCER CREATE\n");
				perror("pthread_create - Producer");
				exit(3);
			}
			printf("Created Succesfully thread[%d] \n",i);
			
			write_all(newsock, "OK", 3);
		}

		// Write Statistics
		// Lock & Sleep while workers are on Job
		pthread_mutex_lock(&devices_lock);	
		int curr_serversDone = 0;
		int prev_serversDone = -1;
		int totalServers = producers_num;
		printf("MirrorServer >> waiting untill all consumers are done.\n");
		do
		{
			curr_serversDone = contentServersDone();
			if( curr_serversDone != prev_serversDone )
				printf("MirrorServer >> %d / %d Content Servers finished\n",curr_serversDone,totalServers); 	
			prev_serversDone = curr_serversDone;
			pthread_cond_wait(&allDone, &devices_lock);
		}while( !decideAllDone() );

		printf("MirrorServer >> %d / %d Content Servers finished\n",totalServers,totalServers); 	

		// DeviceMutex unlock 
		pthread_mutex_unlock(&devices_lock);

		//Find & write all Statistics
		printf("MirrorServer >> Proceeding on Statistics.\n");
		sendStatistics(newsock);

		printf("========================================================================================\n");
		//Reinitialize list of contnet servers
		destroy_List();
		free(thr_prod);		
	}
	
	//Exit 
	Mutex_Destroy();
	List_Mutex_Destroy();
	Statistics_Mutex_Destroy();
	Buffer_Destroy();
	free(thr);
	destroy_List();
	printf("MirrorServer >> Terminating Normally\n");
	exit(0);

}

