#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>
#include <netdb.h> 
#include "libnetfiles.h"

static struct hostent *server = NULL;
static struct sockaddr_in serv_addr;
static int f_mode = -1;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}
int netserverinit(char* hostname, int filemode){
	long portno = PORTNUM;

	server = gethostbyname(hostname);
	if(server == NULL){
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Set file mode */
	f_mode = filemode;
	return 0;
}

int start_session(int * connected){
	int socket_fildes = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fildes < 0){
		*connected = -1;
		error("ERROR opening socket");
	}
	if(server == NULL){
		*connected = -1;
		error("Host not set\n");
	}
	if (connect(socket_fildes,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
		*connected = -1;
		error("ERROR connecting");
	}
	printf("Socket connection established!\n");
	*connected = 0;
	return socket_fildes;
}
int end_session(int sockfd){
	printf("Socket connection closed!\n");
	close(sockfd);
	return 0;
}
int netopen(const char *pathname, int flags){
	char buffer[BUF_SIZE];
	int sfd; //socket fildes after we start a session
	int not_connected;
	int fd = -1;
	
	sfd = start_session(&not_connected);
	if(not_connected){ //if we cannot connect to the socket
		error("Socket ERROR");
	}
	if(server == NULL){ //if server is not initialized properly
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	
	/*PRELIMINARY CHECKS COMPLETED*/
	
	char **tokens;
	//char count; to test the returned messages from the server for recv
	
	memset(buffer, 0, sizeof(buffer)); //zero out the buffer to make behavior predictable
	snprintf(buffer, sizeof(buffer), "open\x1F%d\x1F%s\x1F%d", f_mode, pathname, flags); //command to send to socket
	send(sfd, buffer, BUF_SIZE, 0); //send command to socket
	
	memset(buffer, 0, sizeof(buffer)); //zero out buffer to receive message
	recv(sfd, buffer, BUF_SIZE, 0);
	tokens = get_tokens(buffer, 31); //tokenize rturned message, comes in the form [-1/0],[errno/fd]
	
	if(atoi(tokens[0])){ //if tokens[0] is nonzero, the second token is sent as the errno
		errno = atoi(tokens[1]);
	}
	else{ //if tokens[0] is zero, the second token will be the fildes
		fd = atoi(tokens[1]);
	}
	end_session(sfd);
	return fd;
}
ssize_t netread(int fildes, void *buf, size_t nbyte){
	char buffer[BUF_SIZE];
	int sfd; //socket fildes after we start a session
	int not_connected;
	int fd = fildes; //file descriptor of the opened file we wish to read
	
	sfd = start_session(&not_connected);
	if(not_connected){ //if we cannot connect to the socket
		error("Socket ERROR");
	}
	if(server == NULL){ //if server is not initialized properly
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	
	/*PRELIMINARY CHECKS COMPLETED*/
	
	char **tokens;
	ssize_t bytes_read = -1; //if errors have occurred, -1 is returned
	//char count; to test the returned messages from the server for recv
	
	memset(buffer, 0, sizeof(buffer)); //zero out the buffer to make behavior predictable
	snprintf(buffer, sizeof(buffer), "read\x1F%d\x1F%d\x1F%ld", f_mode, fd, nbyte); //command to send to socket
	
	send(sfd, buffer, BUF_SIZE, 0); //send command to socket
	
	memset(buffer, 0, sizeof(buffer)); //zero out buffer to receive message
	recv(sfd, buffer, BUF_SIZE, 0);
	
	tokens = get_tokens(buffer, 31); //tokenize returned message, comes in the form [-1/0],[errno/nbytes_read],[\0,data]
	if(atoi(tokens[0])){ //if tokens[0] is nonzero, the second token is sent as the errno
		errno = atoi(tokens[1]);
	}
	else{ //if tokens[0] is zero, the second token will be the number of bytes read, the third will be the read data
		bytes_read = atoi(tokens[1]);
		strcpy(buf, tokens[2]);
	}
	end_session(sfd);
	return bytes_read;
}
ssize_t netwrite(int fildes, const void *buf, size_t nbyte){
	char buffer[BUF_SIZE];
	int sfd; //socket fildes after we start a session
	int not_connected;
	int fd = fildes; //file descriptor of the opened file we wish to read
	
	sfd = start_session(&not_connected);
	if(not_connected){ //if we cannot connect to the socket
		error("Socket ERROR");
	}
	if(server == NULL){ //if server is not initialized properly
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	
	/*PRELIMINARY CHECKS COMPLETED*/
	
	char **tokens;
	ssize_t bytes_written = -1; //if errors have occurred, -1 is returned
	//char count; to test the returned messages from the server for recv
	
	memset(buffer, 0, sizeof(buffer)); //zero out the buffer to make behavior predictable
	snprintf(buffer, sizeof(buffer), "write\x1F%d\x1F%d\x1F%ld\x1F%s", f_mode, fd, nbyte, (char*)buf); //command to send to socket
	
	send(sfd, buffer, BUF_SIZE, 0); //send command to socket
	
	memset(buffer, 0, sizeof(buffer)); //zero out buffer to receive message
	recv(sfd, buffer, BUF_SIZE, 0);
	
	tokens = get_tokens(buffer, 31); //tokenize returned message, comes in the form [-1/0],[errno/bytes_written]
	if(atoi(tokens[0])){ //if tokens[0] is nonzero, the second token is sent as the errno
		errno = atoi(tokens[1]);
	}
	else{ //if tokens[0] is zero, the second token will be the number of bytes read, the third will be the read data
		bytes_written = atoi(tokens[1]);
	}
	end_session(sfd);
	return bytes_written;
}

int netclose(int fd){
	char buffer[BUF_SIZE];
	int sfd; //socket fildes after we start a session
	int not_connected;
	
	sfd = start_session(&not_connected);
	if(not_connected){ //if we cannot connect to the socket
		error("Socket ERROR");
	}
	if(server == NULL){ //if server is not initialized properly
		h_errno = HOST_NOT_FOUND;
		return -1;
	}
	
	/*PRELIMINARY CHECKS COMPLETED*/
	
	char **tokens;
	//char count; to test the returned messages from the server for recv
	
	memset(buffer, 0, sizeof(buffer)); //zero out the buffer to make behavior predictable
	snprintf(buffer, sizeof(buffer), "close\x1F%d\x1F%d", f_mode, fd); //command to send to socket
	
	send(sfd, buffer, BUF_SIZE, 0); //send command to socket
	
	memset(buffer, 0, sizeof(buffer)); //zero out buffer to receive message
	recv(sfd, buffer, BUF_SIZE, 0);
	
	tokens = get_tokens(buffer, 31); //tokenize returned message, comes in the form [-1/0],[errno/NULL]
	if(atoi(tokens[0])){ //if tokens[0] is nonzero, the second token is sent as the errno
		errno = atoi(tokens[1]);
	}
	
	end_session(sfd);
	return atoi(tokens[0]);
}

int main(int argc, char *argv[])
{
	int fd, status;
	char buffer[BUF_SIZE];
	size_t nbytes = 100;
	ssize_t nbytes_read, nbytes_written;

	if(argc < 2){
		error("Usage: client [hostname]\n");
	}
	if(netserverinit(argv[1], UNRES) == -1){
		herror("Init Error");
		exit(-1);
	}		
	/* Open a file */
	fd = netopen(argv[2], O_RDWR);
	if(fd == -1){
		error("netopen error");
	}
	else{
		printf("netopen file descriptor: %d\n", fd);
	}
	
	/* Read a file */
	bzero(buffer, sizeof(buffer));
	nbytes_read = netread(fd, buffer, nbytes);
	if(nbytes_read == -1){
		error("netread error");
	}
	else{
		printf("netread returned: %ld bytes read\n", nbytes_read);
		printf("netread data: %s\n", buffer); 
	}
	
	/* Write to a file */
	bzero(buffer, sizeof(buffer));
	sprintf(buffer, "THIS IS THE DATA WRITTEN TO FILE\n"); 
	nbytes_written = netwrite(fd, buffer, 33);
	if(nbytes_written == -1){
		error("netwrite error");
	}
	else{
		printf("netwrite returned: %ld bytes written\n", nbytes_written);
	}
	/* Close a file */
	status = netclose(fd);
	if(status == -1){
		error("netclose error");
	}
	else{
		printf("netclose returned with success\n");
	}
	
	return 0;
}
