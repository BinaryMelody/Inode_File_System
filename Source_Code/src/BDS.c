/*
File Name: BDS.c
Destination Program: BDS
Execute command: ./BDS filename cylinders sectors tracktotrackdelay 10356
This program is the basic of our program and you can operate 
on raw blocks using this server. To operate on it, you should first execute BDS
and then BDC_command or BDC_random.
*/
#include "headers.h"
#define BLOCKSIZE 256
#define BASIC_SERVER_PORT 10356
#define BUFFER_SIZE 1024
int Request_Cylinders[10000];
int CYLINDERS, SECTORS, trackToTrackDelay;

int FCFS_delay;

int main(int argc, char *argv[]){
	char *filename;

	// Copy argv to arguments and check their validity.
	if(argc != 6){
		printf("ERROR: Wrong number of arguments!\n");
		exit(-1);
	}
	filename = argv[1];
	CYLINDERS = atoi(argv[2]);
	SECTORS = atoi(argv[3]);
	trackToTrackDelay = atoi(argv[4]);

	// Open a file
	int fd = open(filename, O_RDWR | O_CREAT, 0);
	if(fd < 0){
		printf("ERROR: Could not open file '%s'.\n", filename);
		exit(-1);
	}
	// Stretch the file size to the size of the simulated disk
	int FILESIZE = CYLINDERS * SECTORS *BLOCKSIZE;
	int result = lseek(fd, FILESIZE - 1, SEEK_SET);
	if(result == -1){
		printf("ERROR: Calling lseek() to 'stretch' the file.\n");
		close(fd);
		exit(-1);
	}

	// Write something at the end of the file to ensure the file actually have the new size.
	result = write(fd, "", 1);
	if(result != 1){
		printf("ERROR: Writing last byte of the file.\n");
		close(fd);
		exit(-1);
	}

	// Map the file
	char * diskfile;
	diskfile = (char *)mmap(0, FILESIZE,
				PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
	if(diskfile == MAP_FAILED){
		close(fd);
		printf("ERROR: Could not map file.\n");
		exit(-1);
	}

	// Creatng socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("ERROR: Opening socket.\n");
		exit(-1);
	}

	// Binding socket
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(BASIC_SERVER_PORT);
	if(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("ERROR: Binding socket.\n");
		exit(-1);
	}

	// Listening and Accepting Connection
	listen(sockfd, 5);
	struct sockaddr_in client_addr;
	socklen_t clientAddrSize = sizeof(client_addr);
	int client_socketfd = accept(sockfd, (sockaddr*) &client_addr, &clientAddrSize);
	if(client_socketfd < 0){
		printf("ERROR: Accepting client.\n");
		exit(-1);
	}

	// Reading and Writing
	char buf[BUFFER_SIZE];
	int nread;
	int c, s, l;
	char command[BLOCKSIZE];
	// char data[1024];
	int prevCylinder = 0;
	while(true){
		nread = read(client_socketfd, buf, BUFFER_SIZE);
		int Response_length;
		if(nread < 0){
			printf("ERROR: Reading failed.\n");
			continue;
		}
		else if(nread == 0){
			printf("Serve finished!\n");
			break;
		}
		switch(buf[0]){
			case 'I':
				sprintf(buf, "%d %d", CYLINDERS, SECTORS);
				// write(client_socketfd, buf, strlen(buf) + 1);
				Response_length = strlen(buf);
				break;
			case 'R':
				sscanf(buf, "%s %d %d", command, &c, &s);
				if(c >= CYLINDERS || c < 0 || s >= SECTORS || s < 0){
					sprintf(buf, "No");
					Response_length = strlen(buf);
				}
				else{
					usleep(abs(prevCylinder - c) * trackToTrackDelay);
					FCFS_delay += (abs(prevCylinder-c) * trackToTrackDelay);
					prevCylinder = c;
					sprintf(buf, "Yes ");
					memcpy(	buf + 4,
							&diskfile[BLOCKSIZE * (c * SECTORS + s)],
							BLOCKSIZE);
					// buf[strlen(buf)] = '\0';
					Response_length = 4 + BLOCKSIZE;
				}
				break;
			case 'W':
				int bias;
				sscanf(buf, "%s %d %d %d%n", command, &c, &s, &l, &bias);
				printf("%s %d %d %d %s\n", command, c, s, l, buf+bias);
				if(c >= CYLINDERS || c < 0 || s >= SECTORS || s < 0){
					sprintf(buf, "No");
				}
				else{
					usleep(abs(prevCylinder - c) * trackToTrackDelay);
					FCFS_delay += (abs(prevCylinder-c) * trackToTrackDelay);
					prevCylinder = c;
					memcpy(&diskfile[BLOCKSIZE * (c * SECTORS + s)], buf+bias+1, BLOCKSIZE);
					// l = strlen(buf + bias + 1); 
					memset(&diskfile[BLOCKSIZE * (c * SECTORS + s)] + l, 0, sizeof(char) * (BLOCKSIZE - l));
					sprintf(buf, "Yes");
				}
				Response_length = strlen(buf);
				break;
			default:
				sprintf(buf, "Wrong command format!");
				break;
		}
		write(client_socketfd, buf, Response_length + 1);
	}
	close(client_socketfd);
	close(sockfd);
	close(fd);

	return 0;
}