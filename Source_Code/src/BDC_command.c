/*
File Name: BDC_command.c
Destination Program: BDC_command
Execute command: ./BDC_command localhost 10356
This program provides you with a client to operate on the raw blocks
of the file when BDS is its sever and a way to check FS problems.
BDC_command has following operations
I
W Cylinder Sector L content
R Cylinder Sector
*/
#include "headers.h"
#define DEFAULT_PORT 10356
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
	// Arguments check
	if(argc < 2 || argc > 3){
		printf("ERROR: Wrong num of arguments.\n");
		exit(-1);
	}
	// Get port
	int BASIC_SERVER_PORT = (argc == 3 ? (atoi(argv[2])) : DEFAULT_PORT);
	// Connecting a client to a server
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("ERROR: Opening socket.\n");
		exit(-1);
	}
	struct sockaddr_in serv_addr;
	struct hostent *host;
	serv_addr.sin_family = AF_INET;
	host = gethostbyname(argv[1]);
	if(!host) {
		printf("ERROR: Getting hostname.\n");
		exit(-1);
	}
	memcpy(&serv_addr.sin_addr.s_addr, host->h_addr, host->h_length);
	serv_addr.sin_port = htons(BASIC_SERVER_PORT);
	if(connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("ERROR: Connecting server.\n");
		exit(-1);
	}
	ssize_t nwrite = 0;
	char buf[BUFFER_SIZE];
	while(true){
		nwrite = read(STDIN_FILENO, buf, BUFFER_SIZE);
		buf[nwrite - 1] = '\0';
		if(strcmp(buf, "exit") == 0) break;
		nwrite = write(sockfd, buf, nwrite);
		nwrite = read(sockfd, buf, BUFFER_SIZE);
		printf("%s\n", buf);
	}
	close(sockfd);
}