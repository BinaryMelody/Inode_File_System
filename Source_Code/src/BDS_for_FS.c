/*
File Name: BDS_for_FS.c
Destination Program: BDS_for_FS
Execute command: ./BDS_for_FS filename cylinders sectors tracktotrackdelay 10356
Because my FS have to operate on BDS for specificated format, and I realized 
it in BDS, thus producing this extra file. It will append all zeros to the current
written block rest spaces. So it is not suitble for BDC_command since it will 
return a lot of useless information.
When operating on FS, you should first execute BDS_for_FS and then
FS and last FC.
*/
#include "headers.h"

int parseLine (char *line, char *command_array[]); // Parse command line
void error (const char *msg); // Print Writing Error Message.

int main (int argc, char* argv[]) { 
    	// Copy argv to arguments and check their validity.
	if (argc < 6) {
		printf ("ERROR: Wrong num of arguments.\n");
		exit(1);
	}

	char *filename = argv[1];
	int CYLINDERS = atoi (argv[2]);
	int SECTORS = atoi (argv[3]);
	int trackToTrackDelay = atoi (argv[4]);
	int portno = atoi (argv[5]);
	int n;
	// Open a file
	int fd = open (filename, O_RDWR | O_CREAT, 0777);
	if (fd < 0) {
		printf ("ERROR: Could not open file '%s'.\n",filename);
		exit(-1);
	}
	// Stretch the file size to the size of the simulated disk
	long FILESIZE = 256 * CYLINDERS * SECTORS;
	int result = lseek (fd, FILESIZE - 1, SEEK_SET);
	if (result == -1) {
		printf("ERROR: Calling lseek() to 'stretch' the file.\n");
		close(fd);
		exit (-1);
	}
	// Write something at the end of the file to ensure the file actually have the new size.
	result = write(fd, "", 1);
	result = write (fd, "", 1);
	if (result != 1) {
		printf("ERROR: Writing last byte of the file.\n");
		close(fd);
		exit (-1);
	}
	// Map the file
	char *diskfile;
	diskfile = (char*) mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (diskfile == MAP_FAILED) {
		close(fd);
		printf("ERROR: Could not map file.\n");
		exit (-1);       
	}
	// Creatng socket
	int sockfd, client_socketfd;
	socklen_t clilen;
	char buffer[1024];
	char *command[10];
	struct sockaddr_in serv_addr, cli_addr;
	int cur_cylinder = 0, next_cylinder;
	int next_sector;    
	int delay_time;
	char read_char[257];
	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		printf("ERROR: Opening socket.\n");
		exit(-1);
	}
	bzero( (char*) &serv_addr, sizeof(serv_addr));
	// Binding socket
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
		printf("ERROR: Binding socket.\n");
		exit(-1);
	}
	// Listening and Accepting Connection
	listen (sockfd, 5);
	clilen = sizeof (cli_addr);
	client_socketfd = accept (sockfd, (struct sockaddr*) &cli_addr, &clilen);
	if (client_socketfd < 0){
		printf("ERROR: Accepting client.\n");
		exit(-1);
	}
	// Reading and Writing
	while (true) {
		bzero (buffer, 1024);
		n = read(client_socketfd, buffer, 1023);
		if (n < 0) 
			error ("Error reading from socket");
		  
		int argu_num = parseLine (buffer, command);

		if (!strcmp(command[0], "exit"))   {	
			n = write (client_socketfd, "Server finished!", 12);
			if (n < 0) 
				error ("Error writing to socket");	
			break;		
		}
		else if (!strcmp(command[0], "I") && argu_num == 1) {
			bzero(buffer, 1024);
			strcpy(buffer, argv[2]);
			strcat(buffer, " ");
			strcat(buffer, argv[3]);
			n = write (client_socketfd, buffer, strlen(buffer));
			if (n < 0) error ("Error writing to socket");
		}
		else if (!strcmp(command[0], "R")) {
			if (argu_num != 3) {
			n = write (client_socketfd, "Wrong argument number!", 22);
			if (n < 0) error ("Error writing to socket");
			continue;
			}
			next_cylinder = atoi (command[1]);
			next_sector = atoi (command[2]);
			delay_time = trackToTrackDelay * abs(next_cylinder - cur_cylinder);
			cur_cylinder = next_cylinder;           
			if (next_cylinder >= CYLINDERS || next_sector >= SECTORS) {
				n = write (client_socketfd, "No", 2);
				if (n < 0) error ("ERROR: Writing to socket");
			}

			else {
				bzero (buffer, 1024);                
				memcpy (read_char, &diskfile[256 * (next_cylinder * SECTORS + next_sector)], 256);
								
				read_char[256] = '\0';
				strcpy (buffer, "Yes ");
				strcat (buffer, read_char);
				                
				n = write (client_socketfd, buffer, strlen(buffer));
				if (n < 0) error ("ERROR: Writing to socket");
			} 
		}
		else if (!strcmp(command[0], "W")) {
			if (argu_num != 5) {
				printf ("%d", argu_num);
				n = write (client_socketfd, "Wrong argument number!", 22);
				if (n < 0) error ("ERROR: Writing to socket");
				continue;
			}
			next_cylinder = atoi(command[1]);			
			next_sector = atoi(command[2]);
			int i;
			int len = atoi(command[3]);
			if (next_cylinder >= CYLINDERS || next_sector >= SECTORS || len > 256) {
				n = write (client_socketfd, "No", 2);
				if (n < 0) error ("ERROR: Writing to socket");   
			}

			else {
				delay_time = trackToTrackDelay * abs(next_cylinder - cur_cylinder);
				cur_cylinder = next_cylinder;
				for (i = 0; i < len; ++i)
				buffer[i] = command[4][i];
				for (;i < 256; ++i) 
				buffer[i] = '0';
				memcpy(&diskfile[256 * (next_cylinder * SECTORS + next_sector)], buffer, 256);
				n = write (client_socketfd, "Yes", 3);
				if (n < 0) error ("ERROR: Writing to socket");
			}
					
		}
		else {
			n = write (client_socketfd, "Wrong command format!", 22);
			if (n < 0) error ("ERROR: Writing to socket"); 			
		}
	}
	close (fd);
	close (client_socketfd);
	close (sockfd);
	return 0;
}

int parseLine (char *line, char *command_array[]) {
	char *p;
	int count = 0;
	line = strtok (line, "\n");
	p = strtok (line, " ");
	while (p) {
		command_array[count] = p;
		count ++;
		p = strtok (NULL, " ");
	}
	return count;
}

void error (const char *msg) {
	perror (msg);
	exit(1);
}