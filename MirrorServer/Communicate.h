#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>

int write_all(int, char *, int);
int read_all(int, char *);

#endif
