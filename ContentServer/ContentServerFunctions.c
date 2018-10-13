#include "ContentServerFunctions.h"
#include "Communicate.h"


void commandList(int newsock, char* command, char* directory)
{
	int   files       = 0;
	char* wbuf        = (char*) malloc(4096*sizeof(char));
	char* t_directory = (char*) malloc(256*sizeof(char));
	char buf[10];
	strcpy(wbuf, directory);
	strcat(wbuf, "/");

	// Send the dirName we are looking to 
	if( write_all(newsock, directory, strlen(directory)+1) < 0 )	
	{
		perror("write");
		exit(-1);
	}
	//Wait Ack
	read_all(newsock,buf);

	// Read and Send File - hierarchy recursively
	strcpy(t_directory,directory);
	Read_Directories(newsock, &t_directory,&wbuf,&files);
	
	// Inform that there are no more available Files
	if( write_all(newsock, "EOF", 4) < 0 )	
	{
		perror("write");
		exit(-1);
	}
	
	printf("Sent directories:\n%s\n",wbuf);
	free(t_directory);
	free(wbuf);
}


int commandFetch(int sock, char* command, char* directory)
{
	//Delay the send 
	sleep(delay);
	fileSender(sock,command,directory);
	
}

int fileSender(int sock, char* command, char* directory)
{
	int   page_size = sysconf(_SC_PAGESIZE);
	int   fd;
	int   count = 0;
	char  fbufer[page_size];
	char  buf[10];
	char* fileName = (char*) malloc(256 * sizeof(char));	
	char* token;
	
	token = strtok(command," ");
	token = strtok(NULL," ");
	strcpy(fileName, token);

	// Send page Size
	sprintf(buf,"%d",page_size);
	if( write_all(sock, buf, 10) < 0 )
	{	
		perror("write");
		return -1;
	}
	//WAit Ack
	read_all(sock,buf);
	memset(buf, '\0', 10);

	/* Open file */
	if( (fd = open(fileName, O_RDONLY)) <0 )
	{
		perror("open");
		return(1);
	}
	

	// Send files. Each time send page_size or less 
	while( (count = read(fd,fbufer,page_size)) > 0 )
	{
		//Sent the ammount(=count) i read
		if( write_all(sock, fbufer, count) < 0 )
		{	
			perror("write");
			return -1;
		}	
		//Wait Ack
		if( read_all(sock,buf) == -1 )
		{
			perror("read");
			return -1;
		}
		memset(fbufer, 0, page_size);
	}

	//Inform File Ended
	if( write_all(sock, "EOF", 4) < 0 )	
	{
		perror("write");
		return -1;
	}
	//Wait Ack
	if( read_all(sock,buf) == -1 )
	{
		perror("read");
		return -1;
	}

	close(fd);
	free(fileName);
	close(sock);
	return 0;	
}




//READ DIRECTORIES

void Read_Directories(int sock, char** directory, char** wbuf, int* files)
{
	
	int isDir;
	DIR *dir;	
	struct dirent pDirent;
	struct dirent *result;
	struct stat s;
	char newpath[100];
	char spath[100];

	dir = opendir(*directory);

	if( dir != NULL )
	{
		// For all inside-hierarchy 
		readdir_r(dir,&pDirent,&result);
		while( result !=NULL )
		{
			// Ignore "." and ".." 
			if( strcmp(pDirent.d_name,".") == 0 ||  strcmp(pDirent.d_name,"..") == 0 )
			{
				readdir_r(dir,&pDirent,&result);
				continue;
			}

			// Make new path 
			strcpy(newpath,*directory);
			strcat(newpath,"/");
			strcat(newpath,pDirent.d_name);

			// File/ Directory Case 
			if( stat(newpath,&s) == 0 )
			{
				if( s.st_mode & S_IFDIR )
				{	
					// Directory. Recursive call! 
					strcpy(spath,*directory);	// save current path 
					strcpy(*directory,newpath);
					Read_Directories(sock, directory,wbuf, files);
					strcpy(*directory,spath);	// retrive current path
				}
				else if( s.st_mode & S_IFREG )
				{
					// ADD File to Queue 
					// Ignore those 'ghost' files 
					if( pDirent.d_name[strlen(pDirent.d_name)-1] == '~' )
					{	readdir_r(dir,&pDirent,&result);	
						continue;
					}
					
					if( (*files) == -1 ) // Real Recursive call. Insert 
					{
						strcpy(spath,*directory);	// save current path 
						strcpy(*directory, newpath);

						strcpy(*directory,spath);	// retrive current path

					}
					else
					{ 
						(*files)++; /* Increase counter */ 

						strcat(*wbuf,",");

						//NEW

						//INstant Write back
						if( write_all(sock, newpath, strlen(newpath)+1) < 0 )
						{	
							perror("write");
							exit(-1);
						}
						char buf[256];
						read_all(sock,buf);

						strcat(*wbuf,newpath);
					}
				}	
				else
				{
					/* Uknown */
					readdir_r(dir,&pDirent,&result);
					continue;					
				}
			}
			else
			{ 
				perror("stat"); 
			}

			readdir_r(dir,&pDirent,&result);	
		}		
	}

	closedir(dir);	
}


