#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


void commandList(int  , char* , char*);               // When Receiving Command LIST  Fork and call this
int  commandFetch(int , char* , char* );              // When Receiving Command FETCH Fork and call this
int  fileSender(int   , char*, char* );               // A function Responsible for sending Files
void Read_Directories(int , char** , char** , int* ); // Traverses through the Directories
int  delay;                                           // delay is global so tha it is inherited from alla children ans it is easilly accesible
