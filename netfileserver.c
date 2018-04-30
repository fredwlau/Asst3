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

#define PORTNUM 2e14
//#define NUM_CLIENT 5
static int buf_size = BUF_SIZE;
static int FDES[10];
static int FMODES[10];
static int FFLAGS[10];
static int connections = 0;

/*typedef struct multi_connection_conf{
	int connection_number;
	int port_number[]; 
} multi_connection_conf;


//Queue - Linked List implementation
struct Node {
	char* data;
	struct Node* next;
};
// Two glboal variables to store address of front and rear nodes. 
struct Node* front = NULL;
struct Node* rear = NULL;

// To Enqueue an operation
void Enqueue(char* x) {
	printf("enqueue\n");
	struct Node* temp = 
		(struct Node*)malloc(sizeof(struct Node));
	temp->data =x; 
	temp->next = NULL;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	rear->next = temp;
	rear = temp;
}

// To Dequeue an opertion
void Dequeue() {
	printf("dequeue\n");
	struct Node* temp = front;
	if(front == NULL) {
		printf("Queue is Empty\n");
		return;
	}
	if(front == rear) {
		front = rear = NULL;
	}
	else {
		front = front->next;
	}
	free(temp);
}
//put to front
void PutToFront(char* x){
	printf("puttofront\n");
	struct Node* temp = 
		(struct Node*)malloc(sizeof(struct Node));
	temp->data =x; 
	temp->next = front;
	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	front = temp;
}

struct file_metadata{
	char path[50];
	int  fds[5];
	int  modes[5];
	int  permission[5];
};*/

void error(const char * msg)
{
    perror(msg);
    exit(1);
}
int server_open(char ** tokens, const int num_tokens, char * msg){
	printf("Opening file path %s\n", tokens[2]);
	char * pathname = tokens[2];
	int flag = atoi(tokens[3]);
	int fd = open(path, flag);
	int mode;
	
	/*assert(strcmp(tokens[0], "open") == 0);*/
	mode = atoi(tokens[1]);
	if(mode < 1 || mode > 3){
		errno = INVALID_FILE_MODE;
	}
	/*else{
		path = tokens[2];
		flag = atoi(tokens[3]);
		fd = open(path, flag);
	}*/
	if(fd == -1){
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d\x1f%d", SUCCESS_RET, -fd);
		FDES[connections] = -fd;
		FMODES[connections] = mode;
		FFLAGS[connections] = flag;		
		connections++;
	}
	return 0;
}
int server_read(char ** tokens, const int num_tokens, char * msg){
	printf("Reading on thread of file descriptor: %s\n", tokens[2]);
	int fd = atoi(tokens[2]);
	int valid;
	size_t nbytes;
	ssize_t bytesRead = 0;
	char * data;
	
	/*assert(strcmp(tokens[0], "read") == 0);*/
	//fd = atoi(tokens[2]);
	int i = 0;
	while(i<connections){
		if(FDES[i] == fd && FFLAGS[i] != O_WRONLY){
			valid = 1;
			break;
		}
		i++;
	}
	/*for(i = 0; i < connections; ++i){
		if(FDES[i] == fd && FMODES[i] != O_WRONLY){
			valid = 1;
			break;
		}
	}*/
	if(!valid){
		errno = EBADF;
	}
	else{
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
	return 0;
}
int server_write(char ** tokens, const int num_tokens, char * msg){
	printf("Writing on thread of file descriptor: %s\n", tokens[2]);
	int fd = atoi(tokens[2]);
	size_t nbytes;
	ssize_t bytesWritten = 0;
	int valid;
	
	/*assert(strcmp(tokens[0], "write") == 0);
	assert(num_tokens == 5);*/
	//fd = atoi(tokens[2]);
	int i = 0;
	while(i<connections){
		if(FDES[i] == fd && FFLAGS[i] != O_RDONLY){
			valid = 1;
			break;
		}
		i++;
	}
	/*for(i = 0; i < num_files; ++i){
		if(fds[i] == fd && modes[i] != O_RDONLY){
			valid = 1;
			break;
		}
	}*/
	if(!valid){
		errno = EBADF;
	}
	else{
		nbytes = (size_t)atoi(tokens[3]);
		bytesWritten = write(-fd, (void*)(tokens[4]), nbytes);	
	}
	if(bytesWritten == -1){
		sprintf(msg, "%d\x1f%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d\x1f%ld", SUCCESS_RET, bytesWritten);
	}
	return 0;
}
int server_close(char ** tokens, const int num_tokens, char * msg){
	printf("Closing file opened on thread of file descriptor: %s\n", tokens[2]);
	int fd = atoi(tokens[2]);
	int status = -1;
	int valid;
	
	/*assert(strcmp(tokens[0], "close") == 0);
	assert(num_tokens == 3);*/
	//fd = atoi(tokens[2]);
	int i = 0;
	while(i<connections){
		if(FDES[i]==fd){
			valid = 1;
			FDES[i] = 0;
			FMODES[i] = 0;
			break;
		}
		i++;
	}
	/*for(i = 0; i < num_files; ++i){
		if(fds[i] == fd){
			valid = 1;
			fds[i] = 0;
			modes[i] = 0;
			break;
		}
	}*/
	if(!valid){
		errno = EBADF;
	}
	else{
		status = close(-fd);
	}
	
	if(status == -1){
		sprintf(msg, "%d\x1f%d", FAILURE_RET, errno);
	}
	else{
		sprintf(msg, "%d", SUCCESS_RET);
	}
	return 0;
}
void * threaded(void * fd) {
	int newsockfd; 
	int status; 
	int num_tokens;
	char buffer[buf_size];
	char msg[buf_size];
	char ** tokens;

	newsockfd = *(int*)fd;
    memset(buffer, 0, buf_size);
	status = recv(newsockfd, buffer, buf_size, 0);
    if (status <= 0) {
    	error("ERROR reading from socket");
    }
	/* Tokenize massage */
    num_tokens = count_tokens(buffer, '\x1f');
    tokens = get_tokens(buffer, '\x1f');
    char * op = tokens[0]
	//tokens = tokenize(buffer, ',', &num_tokens); 
	/*if(strcmp(tokens[0], "open")==0){
		printf("processing open request1\n");
		server_open(tokens, num_tokens, msg);	
	}
	else if(strcmp(tokens[0], "read")==0){
		printf("processing read request1\n");
		server_read(tokens, num_tokens, msg);
	}
	else if(strcmp(tokens[0], "write")==0){
		printf("processing write request1\n");
		server_write(tokens, num_tokens, msg);
	}
	else if(strcmp(tokens[0], "close")==0){
		printf("processing close request1\n");
		server_close(tokens, num_tokens, msg);
	}
	else{
		errno = INVALID_OPERATION_MODE;
		fprintf(stderr, "Invalid request type: %s\n", tokens[0]);	
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}*/
	int ExtAmode = atoi(tokens[1]);
	if(ExtAmode == 1){
		switch(op){
			case 'open':
				//anyone can open
				printf("Processing open...\n");
				server_open(tokens, num_tokens, msg);
			
			case 'read':
				//anyone can read
				printf("Processing read...\n");
				server_read(tokens, num_tokens, msg);
			case 'write':
				//anyone can write
				printf("Processing write...\n")
				server_write(tokens, num_tokens, msg);
			case 'close':
				//anyone can close
				printf("Processing close...\n")
				server_close(tokens, num_tokens, msg);
			default:
				errno = INVALID_OPERATION_MODE;
				fprintf(stderr, "Invalid request: %s\n", tokens[0]);
				sprintf(msg, "%d, %d", FAILURE_RET, errno);
		}
	}
	if(ExtAmode == 2){
		switch(op){
			case 'open':
				//anyone can open
				printf("Processing open...\n");
				server_open(tokens, num_tokens, msg);
			case 'read':
				//anyone can read
				printf("Processing read...\n");
				server_read(tokens, num_tokens, msg);
			case 'write':
				//some logic here to check if file is currently opened in write mode
				printf("Processing write...\n")
				server_write(tokens, num_tokens, msg);
			case 'close':
				//anyone can close
				printf("Processing close...\n")
				server_close(tokens, num_tokens, msg);
			default:
				errno = INVALID_OPERATION_MODE;
				fprintf(stderr, "Invalid request: %s\n", tokens[0]);
				sprintf(msg, "%d, %d", FAILURE_RET, errno);
		}
	}
	if(ExtAmode == 3){
		switch(op){
			case 'open':
				//check to see if file is opened by anyone else
				printf("Processing open...\n");
				server_open(tokens, num_tokens, msg);
			case 'read':
				//check to see if file is opened by anyone else
				printf("Processing read...\n");
				server_read(tokens, num_tokens, msg);
			case 'write':
				//check to see if file is opened by anyone else
				printf("Processing write...\n")
				server_write(tokens, num_tokens, msg);
			case 'close':
				//only one can close
				printf("Processing close...\n")
				server_close(tokens, num_tokens, msg);
			default:
				errno = INVALID_OPERATION_MODE;
				fprintf(stderr, "Invalid request: %s\n", tokens[0]);
				sprintf(msg, "%d, %d", FAILURE_RET, errno);
		}
	}
	memset(buffer, 0, buf_size);
	strcpy(buffer, msg);		
   	status = send(newsockfd, buffer, buf_size, 0);
   	if (status < 0){
   		error("ERROR writing to socket");	
   	}	
	return 0;
}

/*void* process_queue(void* fd) 
{
	printf("process_queue\n");
	int newsockfd; 
	int status, num_tokens;
	char buffer[buf_size];
	char msg[buf_size];
	char** tokens;

	newsockfd = *(int*)fd;
    bzero(buffer, buf_size);
	status = read(newsockfd, buffer, buf_size);
    if (status < 0) error("ERROR reading from socket");
	tokens = tokenize(buffer, ',', &num_tokens); 
	if(strcmp(tokens[0], "open") == 0){
		printf("processing open request2\n");
		f_open(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "read") == 0){
		printf("processing read request2\n");
		f_read(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "write") == 0){
		printf("processing write request2\n");
		f_write(tokens, num_tokens, msg);
	}
	else
	if(strcmp(tokens[0], "close") == 0){
		printf("processing close request2\n");
		f_close(tokens, num_tokens, msg);
	}
	else{
		errno = INVALID_OPERATION_MODE;
		fprintf(stderr, "Invalid request type: %s\n", tokens[0]);	
		sprintf(msg, "%d,%d", FAILURE_RET, errno);
	}
	bzero(buffer, buf_size);
	strcpy(buffer, msg);		
   	status = write(newsockfd, buffer, sizeof(buffer));
   	if (status < 0) error("ERROR writing to socket");		
	return 0;
}*/

int main(int argc, char * argv[]){
	int sockfd
	int newsockfd;
	long portnumber = PORTNUM;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

	pthread_t client_thread;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
       error("ERROR opening socket");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portnum);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    	error("ERROR on binding");
    }
    listen(sockfd, 10);
    printf("Server running on port: %hu\n", serv_addr.sin_port);
	clilen = sizeof(cli_addr);
	while(newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)){
		puts("Connection Successful\n");
		printf("Server Port: %hu\n", serv_addr.sin_port);
		printf("Client Port: %hu\n", cli_addr.sin_port);
		if(pthread_create(&client_thread, NULL, threaded, (void*)&newsockfd) < 0){
			perror("Thread creation failed");
			return -1;
		}	
	}
	pthread_exit(NULL);
    return 0; 
}
