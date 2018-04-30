#ifndef LIB_H
#define LIB_H
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORTNUM 2e14
#define BUF_SIZE 4096
/* Return value */
#define SUCCESS_RET 0
#define FAILURE_RET (-1)
/* Error code */
#define INVALID_FILE_MODE 5
#define INVALID_OPERATION_MODE 6
/* File mode */
#define UNRES 1
#define EXCLU 2
#define TRANS 3 
/* Permission mode */

int netserverinit(char* hostname, int filemode);
int netopen(const char* pathname, int flags);
ssize_t netread(int fildes, void *buf, size_t nbytes);
ssize_t netwrite(int fildes, const void* buf, size_t nbytes);
int netclose(int fd);


// How this is different from Just a strtok loop:
// Uses pointer to num_tokens and str since it maintains these values to read
char** tokenize(char* str, const char delim, int* num_tokens){
	int num;
	char _delim[2];
	char *tmp = str;
	char *token = NULL;
	char *saveptr1;
	char **tokens = NULL;

	if(!str){
		*num_tokens = 0;
		return NULL;
	}

	num = 0;
	_delim[1] = '\0';
	_delim[0] = delim;
	while(*tmp){
		if(*(tmp++) == delim){
			num++;
		}	
	}
	num += 1;
	tokens = malloc(sizeof(char*) * num);
	if(tokens){
		size_t idx = 0;
		token = strtok_r(str, _delim, &saveptr1); 	
		while(token){
			*(tokens + idx++) = token; 
			token = strtok_r(NULL, _delim, &saveptr1);
		}
	}	
	*num_tokens = num;
	return tokens;
}

int count_tokens(char *string, const char delim){
	char *temp = string;
	int n = 0;
	
	printf("COUNTING :%s\n",string);
	
	while(*temp){
		if(*temp == delim){
			n++;
		}
		temp++;
	}
	return n+1;
}

char** get_tokens(char *string, const char delimiter){
	int num_tokens;
	char *token = NULL;
	char * saveptr;
	char **token_array = NULL;
	
	num_tokens =count_tokens(string, delimiter);
	if (num_tokens == 0){
		return NULL;
	}
	token_array = malloc(sizeof(char*) * num_tokens);
	size_t ind = 0;
	token = strtok_r(string, &delimiter, &saveptr);
	while(token){
		printf("%s\n",token);
		token_array[ind] = token;
		token = strtok_r(NULL, &delimiter, &saveptr);
		ind++;
	}
	return token_array;
}

#endif
