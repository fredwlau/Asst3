#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>
#include "libnetfiles.h"

#define NUM_CLIENT 5
static int buf_size = BUF_SIZE;
static int FDES[10];
static int FMODES[10];
static int FFLAGS[10];
static int connections = 0;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}
int server_open(char** tokens, const int num_tokens, char* msg){
	printf("Opening file path %s\n", tokens[2]);
	char * pathname = tokens[2];
	int flag = atoi(tokens[3]);
	int fd = -1;
	int mode = atoi(tokens[1]);
    int canOperate = 0;
	
	fd = open(pathname, flag);
	
	/*assert(strcmp(tokens[0], "open") == 0);*/
	//mode = atoi(tokens[1]);
	if(mode < 1 || mode > 3){
		errno = INVALID_FILE_MODE;
	}
	else{
        if(connections == 0){
            canOperate = 1;
        }
        else{
        	int i;
			for(i=0; i<connections; i++){
				if(FDES [i] == fd){
					if(mode == 3){
						errno = EACCESS;
						canOperate = 0;
						close(fd);
					}
					if(FMODES[i] == 1 && mode == 1){
						//continue with normal open operations
						canOperate = 1;
					}
					if(FMODES[i] == 1 && mode == 2 && flag == O_RDONLY){
						//continue with normal open operations
						canOperate = 1;
					}
					if(FMODES[i] == 1 && mode == 2 && (FFLAGS[i] == O_WRONLY || FFLAGS[i] == O_RDWR) && (flag == O_WRONLY || flag == O_RDWR)){
						//set errno permission denied
						errno = EACCESS;
						canOperate = 0;
						close(fd);
					}
					if((FMODES[i] == 2 && FFLAGS[i] == O_RDONLY) && mode != 3 && flag == O_RDONLY){
						//continue with normal open operations
						canOperate = 1;
					}
					if((FMODES[i] == 2 && (FFLAGS[i] == O_WRONLY || FFLAGS[i] == O_RDWR)) && (flag == O_WRONLY || flag == O_RDWR)){
						//set errno permission denied
						errno = EACCESS;
						canOperate = 0;
						close(fd);
					}
					if((FMODES[i] == 3)){
						//set errno permission denied
						errno = EACCESS;
						canOperate = 0;
						close(fd);
					}
				}
			}
		}
	}

	/*else{
		//path = tokens[2];
		//flag = atoi(tokens[3]);
		fd = open(pathname, flag);
	}*/
	if(canOperate == 1){
		fd = open(pathname, flag);
	
	    if(fd < 0){
			printf("%d",errno);
		    sprintf(msg, "%d\x1F%d", FAILURE_RET, errno);
	    }
	    else{
		    sprintf(msg, "%d\x1F%d", SUCCESS_RET, -fd);
		    FDES[connections] = -fd;
		    FMODES[connections] = mode;
		    FFLAGS[connections] = flag;			
		    connections++;
	    }
    }
    else if(canOperate == 0){
        sprintf(msg, "%d\x1F%d", FAILURE_RET, errno);
    }
	return 0;
}
int server_read(char** tokens, const int num_tokens, char* msg){
	printf("Reading...\n");
	int fd = atoi(tokens[2]);
	size_t nbytes;
	ssize_t bytesRead = 0;
	char* data;
	int i;
	int valid = 0;
	
	/*assert(strcmp(tokens[0], "read") == 0);*/
	//fd = atoi(tokens[2]);
	for(i = 0; i < connections; ++i){
        //printf("Got here\n");
		if(FDES[i] == fd && FFLAGS[i] != O_WRONLY){
            //printf("File exists...\n");
			valid = 1;
			break;
		}
	}
	if(!valid){
        //printf("Error here\n");
		errno = EBADF;
	}
	else{
        //printf("Got here2\n");
		nbytes = (size_t)atoi(tokens[3]);
		data = malloc(nbytes);
		bytesRead = read(-fd, (void*)data, nbytes);	
	}
	if(bytesRead == -1){
		sprintf(msg, "%d\x1F%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d\x1F%ld\x1F%s", SUCCESS_RET, bytesRead, data);
	}
	free(data);
	return 0;
}
int server_write(char** tokens, const int num_tokens, char* msg){
	printf("Writing...\n");
	int fd = atoi(tokens[2]);
	size_t nbytes;
	ssize_t bytesWritten = 0;
	int i;
	int valid = 0;
	
	/*assert(strcmp(tokens[0], "write") == 0);
	assert(num_tokens == 5);*/
	//fd = atoi(tokens[2]);
	for(i = 0; i < connections; ++i){
		if(FDES[i] == fd && FFLAGS[i] != O_RDONLY){
			valid = 1;
			break;
		}
	}
	if(!valid){
		errno = EBADF;
	}
	else{
		nbytes = (size_t)atoi(tokens[3]);
		bytesWritten = write(-fd, (void*)(tokens[4]), nbytes);	
	}
	if(bytesWritten == -1){
		sprintf(msg, "%d\x1F%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d\x1F%ld", SUCCESS_RET, bytesWritten);
	}
	return 0;
}
int server_close(char** tokens, const int num_tokens, char* msg){
	printf("Closing...\n");
	int fd = atoi(tokens[2]);
	int status = -1;
	int i;
	int valid = 0;
	
	/*assert(strcmp(tokens[0], "close") == 0);
	assert(num_tokens == 3);*/
	//fd = atoi(tokens[2]);
	for(i = 0; i < connections; i++){
		if(FDES[i] == fd){
			valid = 1;
			FDES[i] = 0;
			FMODES[i] = 0;
			FFLAGS[i] = 0;
			break;
		}
	}
	if(!valid){
		errno = EBADF;
	}
	else{
		status = close(-fd);
	}
	
	if(status == -1){
		sprintf(msg, "%d\x1F%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d", SUCCESS_RET);
	}
	connections--;
	return 0;
}
void* threaded(void* fd) 
{
	int newsockfd; 
	int status, num_tokens;
	char buffer[buf_size];
	char msg[buf_size];
	char** tokens;

	newsockfd = *(int*)fd;
    bzero(buffer, buf_size);
	status = recv(newsockfd, buffer, buf_size, 0);
    if (status < 0) error("ERROR reading from socket");
	/* Tokenize massage */
    num_tokens = count_tokens(buffer, 31);
    tokens = get_tokens(buffer, 31);
	//tokens = tokenize(buffer, ',', &num_tokens); 
	if(strcmp(tokens[0], "open")==0){
		printf("Processing open request...\n");
		server_open(tokens, num_tokens, msg);	
	}
	else
	if(strcmp(tokens[0], "read")==0){
		printf("Processing read request...\n");
		server_read(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "write")==0){
		printf("Processing write request...\n");
		server_write(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "close")==0){
		printf("Processing close request...\n");
		server_close(tokens, num_tokens, msg);
	}
	else{
		errno = INVALID_OPERATION_MODE;
		fprintf(stderr, "Invalid request type: %s\n", tokens[0]);	
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	bzero(buffer, buf_size);
	strcpy(buffer, msg);		
   	status = send(newsockfd, buffer, buf_size, 0);
   	if (status < 0) error("ERROR writing to socket");
	free(tokens);
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd;
	long portno = PORTNUM;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

	pthread_t client_thread;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(sockfd,5);
    printf("Server Port: %hu\n", serv_addr.sin_port);
	clilen = sizeof(cli_addr);
	while( newsockfd = accept(sockfd, 
		   (struct sockaddr *)&cli_addr, &clilen)){
		puts("Connection Success\n");
		printf("Server Port: %hu\n", serv_addr.sin_port);
		printf("Client Port: %hu\n", cli_addr.sin_port);
		if(pthread_create(&client_thread, 
		   NULL, threaded, (void*)&newsockfd) < 0){
			perror("Thread creation failed");
			return -1;
		}	
	}
	pthread_exit(NULL);
    return 0; 
}
