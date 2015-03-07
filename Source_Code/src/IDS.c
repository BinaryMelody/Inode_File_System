/*
File Name: IDS.c
Destination Program: IDS
Execute command: ./IDS filename cylinders sectors tracktotrackdelay 11356 n
This program can simulate three different ways of operating on raw blocks:
FCFS, SSTF, CLOOK. It will cache at most n operations a time to operate via
different schedule algorithms. If you set n to 1, it will work as the BDS. Also, 
when n = 1, FCFS, SSTF, CLOOK delay will be the same. You can use this to 
compare the speed of three schedule algorithms.
*/
#include "headers.h"
#define BLOCKSIZE 256
#define BASIC_SERVER_PORT 11356
#define BUFFER_SIZE 1024
int n;
int N;
int Request_Cylinders[10000];
int SSTF_Cylinders[10000];
int CLOOK_Cylinders[10000];
int CYLINDERS, SECTORS, trackToTrackDelay;

int FCFS_delay;
int SSTF_delay;
int CLOOK_delay;

void Calculate_SSTF_delay(){
	int prev = 0;
	int queue[1000];
	int Real_in_que;
	int i = 0;
	int Compare_val;
	int x = 0;
	while(i<N){
		for(Real_in_que = 0; Real_in_que < n && i < N; ++Real_in_que, ++i)
			queue[Real_in_que] = Request_Cylinders[i];
		for(int j = 0; j < Real_in_que; ++j){
			int min_dis = 2147483647;
			int min_pos = 0;
			for(int k = 0; k < Real_in_que; ++k){
				if(queue[k] == -1) continue;
				Compare_val = abs(prev - queue[k]);
				if(Compare_val < min_dis){
					min_dis = Compare_val;
					min_pos = k;
				}
			}
			prev = queue[min_pos];
			SSTF_Cylinders[x++] = queue[min_pos];
			queue[min_pos] = -1;
		}
	}
	prev = 0;
	SSTF_delay = 0;
	for(i = 0; i<N; ++i){
		SSTF_delay += abs(SSTF_Cylinders[i] - prev);
		prev = SSTF_Cylinders[i];
	}
	SSTF_delay = SSTF_delay * trackToTrackDelay;
	printf("SSTF_delay %d\n", SSTF_delay);
	for(i=0; i<N; ++i)
		printf("%d ", SSTF_Cylinders[i]);
	printf("\n");
}
void Calculate_CLOOK_delay(){
	int prev = 0;
	int queue[1000];
	int Real_in_que;
	int i = 0;
	int Compare_val;
	int x = 0;
	while(i<N){
		for(Real_in_que = 0; Real_in_que < n && i < N; ++Real_in_que, ++i)
			queue[Real_in_que] = Request_Cylinders[i];
		int min_dis = 2147483647;
		int min_pos = 0;
		int new_prev = 0;
		for(int k = 0; k < Real_in_que; ++k){
			if(queue[k] < min_dis){
				min_dis = queue[k];
				new_prev = k;
			}
		}
		for(int j = 0; j < Real_in_que; ++j){
			min_dis = 2147483647;
			min_pos = 0;
			for(int k = 0; k < Real_in_que; ++k){
				if(queue[k] == -1) continue;
				Compare_val = queue[k] - prev;
				if(Compare_val < 0) continue;
				if(Compare_val < min_dis){
					min_dis = Compare_val;
					min_pos = k;
				}
			}
			if(min_dis == 2147483647)
				min_pos = new_prev;
			prev = queue[min_pos];
			CLOOK_Cylinders[x++] = queue[min_pos];
			queue[min_pos] = -1;
		}
	}
	prev = 0;
	CLOOK_delay = 0;
	for(i = 0; i<N; ++i){
		CLOOK_delay += abs(CLOOK_Cylinders[i] - prev);
		prev = CLOOK_Cylinders[i];
	}
	CLOOK_delay = CLOOK_delay * trackToTrackDelay;
	printf("CLOOK_delay %d\n", CLOOK_delay);
	for(i=0; i<N; ++i)
		printf("%d ", CLOOK_Cylinders[i]);
	printf("\n");
}
int main(int argc, char *argv[]){
	char *filename;

	// Copy argv to arguments and check their validity.
	if(argc != 7){
		printf("ERROR: Wrong number of arguments!\n");
		exit(-1);
	}
	filename = argv[1];
	CYLINDERS = atoi(argv[2]);
	SECTORS = atoi(argv[3]);
	trackToTrackDelay = atoi(argv[4]);
	n = atoi(argv[6]);

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
	N = 0;
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
				Request_Cylinders[N] = c;
				N = N + 1;
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
				Request_Cylinders[N] = c;
				N = N + 1;
				Response_length = strlen(buf);
				break;
			default:
				sprintf(buf, "Wrong command format!");
				break;
		}
		write(client_socketfd, buf, Response_length + 1);
	}
	printf("FCFS_delay %d\n", FCFS_delay);
	for(int i=0; i<N; ++i)
		printf("%d ", Request_Cylinders[i]);
	printf("\n");
	Calculate_SSTF_delay();
	Calculate_CLOOK_delay();
	close(client_socketfd);
	close(sockfd);
	close(fd);

	return 0;
}