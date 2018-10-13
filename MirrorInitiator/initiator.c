#include "initiator.h"

void usage(){
	printf("------Initiator Usage------\n");
	printf("./initiator \n-n <MirrorServerAddress> \n-p <MirrorServerPort> \n -s <ContentServerAddress1:ContentServerPort1:dirorfile1:delay1,ContentServerAddress2:ContentServerPort2:dirorfile2:delay2>\n");
}

int main(int argc, char *argv[]){
	printf("initiating INITIATOR\n");
	int    i;
	int    sockfd = 0, n = 0;
	char*  recvBuff = (char*)malloc(1024*sizeof(char));
	char*  sendBuff = (char*)malloc(1024*sizeof(char));
	char*  serverIP;
	int    serverPort;
	char*  css;
	struct sockaddr_in serv_addr;

	//Check num of arguments
	if(argc!=7)
	{
		usage();
		exit(2);
	}
	
	// parse Arguments
	for(i=1;i<argc;i+=2)
	{
		if(strcmp(argv[i],"-n")==0)
		{
			serverIP = argv[i+1];
		}
		else if(strcmp(argv[i],"-p")==0)
		{
			serverPort = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i],"-s")==0)
		{
			css = argv[i+1];	
		}
		else
		{
			usage();
			exit(2);
		}
	}

	printf("Initiator Args: \n");
	printf("IP:   %s \n",serverIP);
	printf("port: %d \n",serverPort);
	printf("css:  %s \n",css);

	// count_cs = num_cs but count_cs is INTEGER-format ans num_cs is STRING-format
	int count_cs = 0;
	char* num_cs = (char*)malloc(15*sizeof(char));

	//token = first ContentServer
	char* token  =  strtok(css,","); //
	count_cs++; // Counts contentServers

	// Create Content Server List 
	ContentServerList* head = (ContentServerList*)malloc(sizeof(ContentServerList));
	ContentServerList* temp;
	temp = head;

	// add the first Content Server to the first item of the List
	temp->cs = token;
	temp->next = NULL;
	token = strtok(NULL,",");
	
	// Parse and Count the rest Content Servers Given as arguments
	// and add them to the list
	while(token!=NULL)
	{

		temp->next       = (ContentServerList*)malloc(sizeof(ContentServerList));
		temp->next->next = NULL;
		temp->next->cs   = token;
		temp = temp->next;
		count_cs++;
		token = strtok(NULL,",");
	}
	
	printf("count_cs = %d\n",count_cs);
	printList(&head); // print the List of Content Servers
	//sent the total number of content Servers as String
	sprintf(num_cs,"%d",count_cs); 
	printf("num_cs = %s\n",num_cs);
	//OPEN SOCKET
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("\n Error : Could not create socket \n");
		return 1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverPort);

	// CONVERT IP FROM HUMAN READABLE TO BYTE, AND ADD TO STRUCT
	if(inet_aton(serverIP , &serv_addr.sin_addr)==0)
	{
		perror("\n inet_aton error occured\n Probably [INVALID IP]\n");
		return 1;
	} 

	//CONNECT TO SERVER TCP
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("\n Error : Connect Failed \n");
		return 1;
	} 

	
	printf("[*] sending num_cs[%s]\n",num_cs);
	write_all(sockfd,num_cs,strlen(num_cs)+1);
	printf("[*] Awaiting Ack\n"); 
	read_all(sockfd,recvBuff);
	printf("[*] Ack received about number: %s\n",recvBuff);
	if( strcmp(recvBuff,"EXITING") != 0 )
	{	
		temp = head;
		for(i=0;i<count_cs;i++)
		{
			printf("[*] sending[cs:%d]: %s   \n",i,temp->cs);
			write_all(sockfd,temp->cs,strlen(css)+1);
			printf("[*] sent\n");
			printf("[*] Awaiting Ack\n");
			read_all(sockfd,recvBuff);
			printf("[*] Ack received: %s \n",recvBuff);
			temp=temp->next;
		}
	
		//Read the number of different ContentServers
		read_all(sockfd,recvBuff);
		int totalDiferentServers = atoi(recvBuff);
		//Send Ack
		write_all(sockfd,"OK",strlen("OK")*sizeof(char));

		// read Stats for each different ContentServer
		for(i=0;i<totalDiferentServers;i++){
			read_all(sockfd,recvBuff);                        // Read Stats
			write_all(sockfd,"OK",strlen("OK")*sizeof(char)); // Write Ack
			decode_statistics(recvBuff);  // decode the statistics and print them in human-readable form
		}
		
		// read Average,Variance
		read_all(sockfd,recvBuff);
		// Write Ack
		write_all(sockfd,"OK",strlen("OK")*sizeof(char*));
		// Seperate Average from Variance
		char* token = strtok(recvBuff,",");
		printf("Average Bytes: %s\n",token);
		token = strtok(NULL,",");
		printf("Variance: %s\n",token);
	}



	printf("terminating INITIATOR\n");
	// free Content Server  list
	temp = head;
	while(head!=NULL)
	{
		head = head->next;
		free(temp);
		temp = head;
	}
	free(num_cs);
	free(recvBuff);
	free(sendBuff);
	close(sockfd);

}
