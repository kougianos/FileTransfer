#ifndef INITIATOR_H
#define INITIATOR_H

#include "Communicate.h"

// Initiator creates a List of ContentServers
// and sends them to the MirrorServer one after the other
struct contentServerList{
	char* cs;
	struct contentServerList* next;
};
typedef struct contentServerList ContentServerList;

#endif
