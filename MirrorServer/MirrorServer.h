#ifndef MIRRORSERVER_H
#define MIRRORSERVER_H

#define MAX_THREADS 50
#include "ThreadLib.h"
#include <time.h>

//Files & commands
void copyAdministrator(ContentServers );			//Administrator about the copy files & those need fetching
int fetchFile(int, ContentServers);				//Fetch correctly and create / write to file

//Statistics
void sendStatistics(int);					//Return the statistics of this loop				

#endif
