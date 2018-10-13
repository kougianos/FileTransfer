#ifndef THREADLIB_H
#define THREADLIB_H

#include "Communicate.h"
#include "MainBuffer.h"
#include "List.h"
#include <pthread.h>

// Mutexes && Condition Variables
void Mutex_Initialize();
void Mutex_Destroy();

// Producers Threads
void *Producer(void *argp);	

// Consumers Threads
void *Consumer(void *argp);

// Write Destination file Function
char dirname[26];

// Mutexes && Condition Variables about Statistics
void Statistics_Mutex_Initialize();
void Statistics_Mutex_Destroy();

// Mutexes
pthread_mutex_t devices_lock; 
pthread_cond_t allDone;

#endif


