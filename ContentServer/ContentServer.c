#include "ContentServerFunctions.h"

#define MaxClients 128

void usage()
{
	printf("===ContentServer usage===\n");
	printf("./contentserver \n  -p <port \n -d <dir> \n");
}

int exit_status = 0;
int sock;
// signal_handler to exit
void sig_handler(int signum)
{
	exit_status = 1;
	printf("\nSigHandler Server Exiting..\n");
}

// Main 
int main( int argc, char* argv[] )
{
	int  i;
	char c;
	// Problem's variables 
	long page_size;
	int  port;
	// Socket Vars - Internet 
	int             newsock;
	char*           directory = (char*)malloc(256*sizeof(char)); 
	char*           command = (char*)malloc(256*sizeof(char)); 
	char*           t_command = (char*)malloc(256*sizeof(char)); 
	struct          sockaddr_in server, client;
	socklen_t       clientlen;
	struct sockaddr *serverptr = (struct sockaddr *) &server;
	struct sockaddr *clientptr = (struct sockaddr *) &client;
	struct hostent  *rem;
	
	// Check Argument num 
	if( argc < 5 )
	{
		perror("Too few arguments\n");
		usage();
		exit(1);
	}
	
	// Parse Arguments 
	// Read Port/ Thread_Pool_Size/ Queue_Size 
	for( i = 1; i < argc; i+=2 )
	{
		
		if( strcmp(argv[i],"-p") == 0 )
			port = atoi(argv[i+1]);

		else if( strcmp(argv[i],"-d") == 0 )
			strcpy(directory,argv[i+1]);	
		else
		{
			perror("Error! Wrong Arguments!\n");
			usage();
			exit(1);
		}
	}

	printf("\nContent Server's [%d] parameters are :\n", getpid());
	printf("\tport:      %d\n",port);
	printf("\tdirectory: %s\n",directory);

	// signal Handler 
	signal(SIGINT, sig_handler);

	// Open|Create Port - Socket 
	printf("Creating Socket..\n");
	
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 )
	{
		perror("socket");
		exit(2);
	}

	//Create Server Struct
	server.sin_family      = AF_INET; // Internet domain 
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port); // The given port 
	// option to free socket if crash 
	int yes = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	
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
		

	printf("Content Server was succesfully initialized...\n");

	// Main Loop Listen For Connections And Fork 
	i = 0; 
	while( !exit_status )
	{
		i++;
		// Accept connection 
		printf("Listening for connections to port %d\n",port);
		clientlen = sizeof(*clientptr);
		// Wait until Connection Arrives and Create The new Connection Socket 
		if( (newsock = accept(sock, clientptr, &clientlen)) < 0 )
		{	
			perror("accept");
			exit(2);
		}
		if( exit_status )	/* Exit */
		{
			printf("Need to exit!\n");
			break;
		}

		printf("Accepted connection on socket: %d\n",newsock);
		
		//Read the command either LIST or FETCH
		memset(command, '\0', sizeof(command));
		if( read_all(newsock,command) == -1 )
		{
			perror("read");
			close(newsock);
			exit(3);
		}

		printf("Received Command: %s\n",command);
		if( strcmp(command, "EXIT") == 0 )
			break;
		strcpy(t_command, command);
		char* token;
		char command_header[10];
		token = strtok(t_command," ");
		strcpy(command_header, token);
		// if LIST we must save the delay to father so that the FETCH children
		// will inherit it's value
		if( strcmp(command_header,"LIST") == 0 )
		{
			token = strtok(NULL," ");
			token = strtok(NULL," ");
			delay = atoi(token);
			printf("Delay: %d\n",delay);
		}

		pid_t pid;
		//Content Server must Fork 
		//in order to be able to execute LIST and FETCH concurrently
		pid = fork();
		if(pid == 0){
			printf("[%d] Child Content Server\n",getpid());
			//child closes the socket that father listens to
			//so tha the sock isn't open by two procceses
			close(sock);			
			if( strcmp(command_header,"LIST") == 0 )
				commandList(newsock,command,directory);
			else if( strcmp(command_header,"FETCH") == 0 )
				commandFetch(newsock,command,directory);
			else
				printf("Uknown Command\n");

			//Child before Exit, free all resources
			close(newsock);
			free(command);
			free(t_command);
			free(directory);
			printf("child exiting\n");
			exit(0);
		}
		close(newsock);		
			
	}

	printf("Content Server Terminating normally\n");
	free(command);
	free(t_command);
	free(directory);
	close(sock);
	exit(0);

}

