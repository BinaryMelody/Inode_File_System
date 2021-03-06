/*
File Name: BDC_random.c
Destination Program: BDC_random
Execute command: ./BDC_random localhost 10356 N
This program provide you with a random client of BDS server with exactly N 
operations. Actually, it is more suitable for IDS to compare different seek 
method among FCFS, SSTF and CLOOK. It will only operate on W and R 
operations. Intrestingly, if you don't close the terminal. Each time you 
re-execute BDC_random, it will produce exactly the same commands.
Pay attention that if you want to operate on IDS, according to requirement,
you should change 10356 to 11356. Although it is OK if you change IDS port
to 10356 instead of change BDC_random port.
*/
#include "headers.h"
#define DEFAULT_PORT 10356

int main(int argc, char * argv[]){
	// Arguments check
	if(argc != 4){
		printf("ERROR: Wrong num of arguments!\n");
		exit(-1);
	}
	// Get port and number
	int BASIC_SERVER_PORT = atoi(argv[2]);
	int RequestNum = atoi(argv[3]);
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
	// Get cylinders and sectors
	char buffer[1024];
	int cylinder, sector;
	write(sockfd, "I", 2);
	read(sockfd, buffer, 1024);
	sscanf(buffer, "%d %d", &cylinder, &sector);

	// Random commands produce
	int c, s;
	int bias;
	char *p;
	char test[1024];
	for(int i=0; i < RequestNum; ++i){
		c = random() % cylinder;
		s = random() % sector;
		if(random() % 2 == 1){
			bias = sprintf(buffer, "W %d %d 256 ", c, s);
			p=buffer+bias;
			for(int j=0; j<256; ++j, ++p)
				*p = (char)(random() % 26 + 'a');
			*p = 0;
			write(sockfd, buffer, bias+257);
			printf("%d W\n", i + 1);
		}
		else{
			bias = sprintf(buffer, "R %d %d ", c, s);
			write(sockfd, buffer, bias);
			printf("%d R\n", i + 1);
		}
		read(sockfd, test, 1024);
		// printf("%s\n", test);
	}
	close(sockfd);
}
