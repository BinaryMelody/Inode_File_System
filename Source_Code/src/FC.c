/*
File Name: FC.c
Destination Program: FC
Execute command: ./FC localhost 12356
This program is the client of FS. You can operate on FS through this
program. To operate on FS, you should execute in the order of
BDS_for_FS, FS, FC. It has implemented the following commands:
f
mk filename
mkdir dirname
rm filename
cd path(either related or absolute)
ls bool(0 for simplified and 1 for detailed)
w f l d
a f l d
cat filename
rmdir dirname
exit
When first operate on a new file, you should use "f" command 
to format the file system.
*/
#include "headers.h"
#define DEFAULT_PORT 12356
#define BUFFER_SIZE 10000

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
	bool flag = true;
	while(flag){
		bzero(buf, BUFFER_SIZE);
		nwrite = read(STDIN_FILENO, buf, BUFFER_SIZE);
		buf[nwrite - 1] = '\0';
		nwrite = write(sockfd, buf, nwrite);
		if(strcmp(buf, "exit") == 0) flag = false;
		nwrite = read(sockfd, buf, BUFFER_SIZE);
		printf("%s\n", buf);
	}
	write(sockfd, "\n", 1);
	read(sockfd, buf, BUFFER_SIZE);
	write(sockfd, "\n", 1);
	read(sockfd, buf, BUFFER_SIZE);
	close(sockfd);
}
