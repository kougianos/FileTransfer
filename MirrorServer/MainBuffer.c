#include "MainBuffer.h"

//Main Buffer (Queue) As long String
//Base Buffer's Functions
int initiate_Buffer()
{
	main_buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));
	if( main_buffer == NULL )
		return -1;
	memset(main_buffer,'\0',BUFFER_SIZE);
	return 0;
}


int Is_Empty()
{
	if( strlen(main_buffer) == 0 )
		return 1;
	return 0;
}



int Is_Full(char* str)
{
	//If current len and the next string's len > BUFFER_SIZE supposed to be full
	if( ( strlen(main_buffer) + strlen(str) ) > BUFFER_SIZE )
		return 1;
	return 0;
}

void Buffer_Destroy()
{
	free(main_buffer);
}


void writeToBuffer(char* str)
{
	//Write the string: str to our MainBuffer
	char* copyStr = (char*) malloc(strlen(main_buffer)+strlen(str)+1);
	strcpy(copyStr,main_buffer);
	copyStr = strcat(copyStr,str);
	strcpy(main_buffer,copyStr);
	free(copyStr);
}


void removeSubstring(char *s,const char *toremove)
{
	//Remove string: toremove from string: s
    	memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
}


void Extract_Data(ContentServers* data)
{
	//Will get like FIFO the first available command to be executed.
	//Format: <dirorfilename, ContentServerAddress, ContentServerPort>
	char* temp_buffer = (char*) malloc( 256 * sizeof(char));
	char* pt = temp_buffer;
	strcpy(temp_buffer,main_buffer);

	//Extract 1rst command from Buffer
	//token = <......
	char* token;
	token = strtok(temp_buffer,">");
	removeSubstring(main_buffer,token);
	removeSubstring(main_buffer,">");
	//main_buffer = <......><......> ...
	//token = <...
	//temp = ......  only clear data
	temp_buffer = strtok(token,"<");
	
	//Save data from the extracted point
	token = strtok(temp_buffer,",");		
	strcpy(data->dirname,token);
	token = strtok(NULL,",");	
	strcpy(data->address,token);
	token = strtok(NULL,",");	
	data->port = atoi(token);

	//Get Delay
	data->delay = getDelay(data->address,data->port);
	
	free(pt);
}

// File Fetching Functions & responses
int fileNeedFetching(char* response, char* fetchFile, char* headerFolder)
{
	//Check fetchFIle, if it is incuded in the start of response
	char* substr = (char*) malloc((strlen(response)+1)*sizeof(char));
	char* ptr = substr;	
	strcpy(substr, response);
	removeSubstring(substr,headerFolder);
	removeSubstring(substr,"/");

	int fetch_len = strlen(fetchFile);

	if( strlen(substr) < fetch_len ) // No Need of that
		return 0;
	else
	{
		//Break response and keep only strating fetch_len characters
		char* str = (char*) malloc( (fetch_len+1) * sizeof(char));
		strncpy(str, substr,fetch_len);
		str[fetch_len] = 0; //null terminate destination

		//Check if it is the same path. Check wether it is the same file or inside the requested envelop
		if(  strcmp(str, fetchFile) == 0 && fetchFile[strlen(fetchFile)-1] == '/'){	
			//It is bellow the requested envelop	
			free(ptr);
			free(str);
			return 1;
		}
		else if( strcmp(str, fetchFile) == 0 && ( strcmp(str,substr) == 0 || substr[strlen(str)]=='/')){
			//Same file
			free(str);
			free(ptr);
			return 1;
		}
		free(str);
	}
	free(ptr);
	return 0;
}

void fixResponse(char* str, char* file, int port, char* address)
{
	//Change response to format: <dirorfilename, ContentServerAddress, ContentServerPort>
	strcpy(str, "<");
	strcat(str,file);
	strcat(str,",");
	strcat(str,address);
	strcat(str,",");
	char temp[10];
	memset(temp, '\0', 10);
	sprintf(temp, "%d", port);
	strcat(str,temp);
	strcat(str,">");
}
