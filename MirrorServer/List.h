#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <fcntl.h>

//List about bytes of each file
struct fileInfoLise{
	unsigned long bytes;
	struct fileInfoLise* next;
};
typedef struct fileInfoLise FileInfoLise;

//List about COntent server's data
struct contentServerList{
	int port;
	int id;
	int delay;
	char* address;
	char* dirName;
	
	//Server's files data
	unsigned long bytes;
	int files;
	int numOfFetches;	//keep how much need to receive more (++/--)
	int updating;		//variable to inform if there is an update going on this server
	int serverOn;		//Server exists (=0 if connection error)
	
	FileInfoLise* fl;

	/* Mutexes */
	pthread_mutex_t dir_lock; 
	pthread_cond_t dir_creating;
	int on_creation;

	struct contentServerList* next;
};
typedef struct contentServerList ContentServerList;


// Mutexes about ContentServer List
pthread_mutex_t list_lock; 
pthread_cond_t list_accessed;
int accessed;

//ContentServer List & id's
ContentServerList* csl;
int id; 

//Mutexe's functions
void List_Mutex_Initialize();
void List_Mutex_Destroy();

//ContentServer List functions
void initialize_List();
void destroy_List();


//General ContentServer List functions

int contentServerExist(int , char* );			//check if Content server exists. Returns the id, based on addres & port
int addContentServerList(char* , int , int );		//Add new content server
ContentServerList* existContentServerList(int, char*);	//check if Content server exists. Returns the instance of the struct, based on addres & port
int getId(char*, int);					//return the id of a content server, based on addres & port
void insertFileList(ContentServerList*, unsigned long);	//Add bytes of the file

//About content servers
int getDelay(char*, int);				//returns the delay set for this server, based on addres & port
unsigned long getTotalBytes();				//returns total bytes
int getTotalFiles();					//returns total files
int onProgress(ContentServerList* );			//returns if the current server is in progress
int contentServersNum();				//Count coententServers

//About producers & content servers
int allProducersDone();					//returns if all producers are done
int decideAllDone();					//returns if all producers & consumers are done

int contentServersDone();				//returns how many contentServers are done working

//Stats
void getStats(char*, ContentServerList* );		//return the strng about statistics
double calculateVariance(double);			//returns the variance


#endif
