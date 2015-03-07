/*
File Name: FS.c
Destination Program: FS
Execute command: ./FS localhost 10356 12356
This program simulate the Inode File System, to operate on this 
program. You must first execute BDS_for_FS as its sever and 
execute FC as its client. You input commands through FC and 
observe return answer via FC. 
FS has the following operations:
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
#define BUFFER_SIZE 256
#define INODE_SIZE 64

// ****************MAIN_OPERATIONS**************
int Format_File_System (int);
int Make_File (int, const char *);
int Make_Dir (int, const char *);
int Remove_File (int, const char *);
int Change_Dir (int, char *);
int List (int, const char *);
int Write_Filename_L_Data (int, const char *, int, const char *);
int Append_Filename_L_Data (int, const char *, int, const char *);
int Catch_File (int, const char *);
int Remove_Dir (int, const char *);

// **************BDS_RELATED_OPERATIONS*********
void Memcpy_from_C_S (int, int, int, char *);
void Memcpy_to_C_S (int, int, int, const char *);
void Update_Super_Block (int);
void Get_Super_Block (int);
void Update_Bitmap (int, int, bool, bool);
void Read_Block (int, int, char *, bool);
void Write_Block (int, int, const char *, bool);
void Memcpy_from_File_Content (int, int);
void Get_Npart_of_Single_Dir (int, int, int, char *);
void Get_Npart_of_Inode (int, int, int, char *);
int Get_File_Size (int, int);
void Write_Npart_of_Inode (int, int, int, const char *);
int Get_Num_of_Sub_FileDir (int, int);
int Check_Same_Name (const char *, const char *, int, bool);
void Memcpy_File_Content_to_File (int, int, const char *);
void Update_FileDir_Num_of_Current_Dir (int, int, int) ;
int Get_Empty_Block (int, bool);
void Strench_File_to_New_Size (int, int, int);
void Write_Npart_of_Single_Dir (int, int, int, const char *);
void Initial_Dir (int, int, int);
void Remove_Dir_Recursion (int, int);

// ******************TOOLS***********************
int Parse_Command_Line (char *, char **);
int Parse_Slash_Line (char *, char **);
int Parse_Space_Line (char *, char *);
void Read_Socket(int, char *);
void Write_Socket(int, const char *); 

// ****************SOCKETS DEFINITION**************
int BDS_Port;
int FS_Port;
int BDS_Socket;
int FC_Socket;
int FS_Socket;
sockaddr_in FS_addr;

// ****************SUPERBLOCK CONTENTS************
int Inode_Num;
int Inode_Empty_Num;
int Inode_Block_Num;
int Bitmap_Block_Num;
int Bitmap_Start;
int Data_Num;
int Data_Empty_Num;

// ****************VARIBLES DEFINATION*************
int CYLINDER, SECTOR;
int Current_Dir = 0;
int Root_Dir_Inode = 0;
char Full_File_Content[100000];// Store the full content of a file 
char List_Show_Content[100000];//function List() output
char Catch_File_Show_Content[100000];//catch() output
char Bitmap_Content[100000];//bitmap

// *****************FUNCTION IMPLEMENTATION***********************
int main (int argc, char *argv[]) {
	char buffer[1024];
    	int Argument_Num;
    	char *command[10];
	int Ret_Val;

	//Server_Host of FS
    	int FS_Port = atoi (argv[3]);
    	int FS_Socket, FC_Socket;
    	socklen_t Client_Len;
    	struct sockaddr_in Serv_FS_Addr, Client_Addr;
 	FS_Socket = socket (AF_INET, SOCK_STREAM, 0);
	if (FS_Socket < 0){
		printf("ERROR: Opening FS Socket!\n");
		exit(-1);
	}
	bzero ((char*) &Serv_FS_Addr, sizeof (Serv_FS_Addr));
	Serv_FS_Addr.sin_family = AF_INET;
	Serv_FS_Addr.sin_addr.s_addr = INADDR_ANY;
	Serv_FS_Addr.sin_port = htons(FS_Port);
	if (bind (FS_Socket, (struct sockaddr *) &Serv_FS_Addr, sizeof(Serv_FS_Addr)) < 0){
		printf("ERROR: Binding FS Socket!\n");
		exit(-1);
	}
	listen (FS_Socket, 5);
	Client_Len = (sizeof (Client_Addr));
	FC_Socket = accept (FS_Socket, (struct sockaddr*) &Client_Addr, &Client_Len);
	if (FC_Socket < 0) {
		printf("ERROR: Accepting Client!\n");
		exit(-1);
	}
	   
	//client of BDS
	int BDS_Socket, n;
	struct sockaddr_in Serv_BDS_Addr;
	struct hostent *Server_Host;
	int BDS_Port = atoi (argv[2]);
	BDS_Socket = socket (AF_INET, SOCK_STREAM, 0);
	if (BDS_Socket < 0){
		printf("ERROR: Opening BDS Socket!\n");
		exit(-1);
	}
	Server_Host = gethostbyname (argv[1]);
	if (Server_Host == NULL){
		printf("ERROR: Getting host name!\n");
		exit(-1);
	}
	bzero ((char*) &Serv_BDS_Addr, sizeof (Serv_BDS_Addr));
	Serv_BDS_Addr.sin_family = AF_INET;
	bcopy ((char*)Server_Host -> h_addr, (char*) &Serv_BDS_Addr.sin_addr.s_addr, Server_Host -> h_length);
	Serv_BDS_Addr.sin_port = htons(BDS_Port);
	if (connect (BDS_Socket, (struct sockaddr*) &Serv_BDS_Addr, sizeof(Serv_BDS_Addr)) < 0){
		printf("ERROR: Connecting to BDS Server!\n");
		exit(-1);
	}

	Write_Socket (BDS_Socket, "I\n");
	Read_Socket (BDS_Socket, buffer);
	Parse_Command_Line (buffer, command);
	CYLINDER = atoi (command[0]);
	SECTOR = atoi (command[1]);
	Format_File_System(BDS_Socket);  
	Get_Super_Block(BDS_Socket);
	// bool Exit_Flag = true;
	while (true) {
		// printf ("curdir = %d\n", Current_Dir);
		Read_Socket (FC_Socket, buffer);
		Argument_Num = Parse_Command_Line (buffer, command);
		if (!strcmp (command[0], "f") && Argument_Num == 1) {
			if (Format_File_System (BDS_Socket)) Write_Socket (FC_Socket, "Success! Format.");
			else Write_Socket (FC_Socket, "Fail! Format.");
		}
		else if (!strcmp(command[0], "mk") && Argument_Num == 2) {
			Ret_Val = Make_File (BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Create file.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A file of same name already exists.");
			else if (Ret_Val == 2) Write_Socket (FC_Socket, "Fail! Lack spare Inode or data block.");
		}
		else if (!strcmp (command[0], "mkdir") && Argument_Num == 2) {
			Ret_Val = Make_Dir (BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Create directory.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A directory of same name already exists.");
			else if (Ret_Val == 2) Write_Socket (FC_Socket, "Fail! Lack spare Inode or data block.");
		}
		else if (!strcmp (command[0], "rm") && Argument_Num == 2) {
			Ret_Val = Remove_File (BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Remove file.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A file of same name not exists.");
		}
		else if (!strcmp (command[0], "cd") && Argument_Num == 2) {
			Ret_Val = Change_Dir (BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Change to new directory.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! No such directory exists.");
		}
		else if (!strcmp (command[0], "rmdir") && Argument_Num == 2) {
			Ret_Val = Remove_Dir (BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Remove directory.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A directory of same name exists.");
		}
		else if (!strcmp (command[0], "ls") && Argument_Num == 2) {
			Ret_Val = List(BDS_Socket, command[1]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, List_Show_Content);
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! Wrong command argument.");
		}
		else if (!strcmp (command[0], "w") && Argument_Num == 4) {
			Ret_Val = Write_Filename_L_Data (BDS_Socket, command[1], atoi(command[2]), command[3]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Rewrite file content.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A file of same name not exists.");
			else if (Ret_Val == 2) Write_Socket (FC_Socket, "Fail! Data length not proper.");
			else if (Ret_Val == 3) Write_Socket (FC_Socket, "Fail! Lack spare Inode or data block.");
		}
		else if (!strcmp (command[0], "a") && Argument_Num == 4) {
			Ret_Val = Append_Filename_L_Data (BDS_Socket, command[1], atoi(command[2]), command[3]);
			if (Ret_Val == 0) Write_Socket (FC_Socket, "Success! Append file content.");
			else if (Ret_Val == 1) Write_Socket (FC_Socket, "Fail! A file of same name not exists.");
			else if (Ret_Val == 2) Write_Socket (FC_Socket, "Fail! Data length not proper.");
			else if (Ret_Val == 3) Write_Socket (FC_Socket, "Fail! Lack spare Inode or data block.");
		}
		else if (!strcmp (command[0], "cat") && Argument_Num == 2) {
			Ret_Val = Catch_File (BDS_Socket, command[1]);
			if (Ret_Val == 0) { strcpy(Catch_File_Show_Content, "Success! Catch file.\n"); strcat(Catch_File_Show_Content, Full_File_Content); }
			else if (Ret_Val == 1) strcpy(Catch_File_Show_Content, "Fail! A file of same name not exists.");
			Write_Socket (FC_Socket, Catch_File_Show_Content);
		}
		
		else if (!strcmp(command[0], "exit") && Argument_Num == 1) {
			Write_Socket (FC_Socket, "Success! Exit file system.");
			break;
		}
		else {
			Write_Socket (FC_Socket, "Fail! Error command.");		
		}
	}	
	close (FC_Socket);
	close (BDS_Socket);
	close (FS_Socket);
	return 0;
}

int Parse_Command_Line (char *line, char *array[]) {    // Parse Command Line
	char *p;
	int count = 0;
	line = strtok (line, "\n");
	p = strtok (line, " ");
	while (p) {
		array[count] = p;
 		count ++;
		p = strtok (NULL, " ");
	}
	return count;
}

int Parse_Space_Line (char *line, char *array[]) {    //Parse Lines with " "
	char *p;
	int count = 0;
	//line = strtok (line, "\n");
	p = strtok (line, " ");
	while (p) {
		array[count] = p;
		count ++;
		p = strtok (NULL, " ");
	}
	return count;
}

int Parse_Slash_Line (char *line, char *array[]) {    // Parse Lines with "/"
	char *p;
	int count = 0;
	line = strtok (line, "\n");
	p = strtok (line, "/");
	while (p) {
		array[count] = p;
		count ++;
		p = strtok (NULL, "/");
	}
	return count;
}

void Read_Socket(int Socket_fd, char *buffer) {	// Read from Socket_fd
	int nread;
	bzero (buffer, 1024);
	nread = read (Socket_fd, buffer, 1023);
	if (nread < 0) {
		printf("ERROR: Reading from socket.\n");
		exit(-1);
	}
	return;
}

void Write_Socket(int Socket_fd, const char *buffer) {	// Write to Socket_fd
	int nwrite;
	nwrite = write (Socket_fd, buffer, strlen(buffer));
	if (nwrite < 0) {
		printf("ERROR: Writing to socket.\n");
		exit(-1);
	}
	return;
}

void Memcpy_from_C_S (int Socket_fd, int c, int s, char *buf) {	// Get the content from Cylinder c and Sector s
	char buffer[1024];
	char *command[10];
	int count;
	bzero (buffer, 1024);
	sprintf (buffer, "R %d %d", c, s);
	Write_Socket (Socket_fd, buffer);
	Read_Socket (Socket_fd, buffer);
	count = Parse_Space_Line (buffer, command);
	if (count != 1) strcpy (buf, command[1]);
	else buf[0] = '\0';
}

void Memcpy_to_C_S (int Socket_fd, int c, int s, const char *buf) {	// Save content to Cylinder c and Sector s
	char buffer[1024];
	int len = strlen (buf);
	bzero (buffer, 1024);
	sprintf (buffer, "W %d %d %d %s", c, s, len, buf);
	Write_Socket (Socket_fd, buffer); 
	Read_Socket (Socket_fd, buffer);
}

void Read_Block (int Socket_fd, int Node_Order, char *buf, bool Inode_or_Data_Block) {	// Read through Block Number instead of Cylinder and Sector,
	int c, s;										// Seperated by "Inode_or_Data_Block"
	int Real_Order;
	char buffer[256];
	int i, j;
	if (Inode_or_Data_Block) 
		Real_Order = Node_Order / 4;
	else 
		Real_Order = Inode_Block_Num + Node_Order;
	c = Real_Order / SECTOR;
	s = Real_Order % SECTOR;
	Memcpy_from_C_S (Socket_fd, c, s, buffer);
	if (Inode_or_Data_Block) {
		for (i = (Node_Order % 4) * 64, j = 0; j < 64; ++i, ++j)
			buf[j] = buffer[i];
	}
	else {
		for (i = 0; i < 256; ++i)
			buf[i] = buffer[i];
		buf[i] = '\0';
	}
}

void Write_Block (int Socket_fd, int Node_Order, const char *buf, bool Inode_or_Data_Block) {	// Write through Block Number instead of Cylinder and Sector,
	int c, s;											// Seperated by "Inode_or_Data_Block"
	int Real_Order;
	char buffer[256];
	int i, j;
	if (Inode_or_Data_Block) Real_Order = Node_Order / 4;
	else Real_Order = Inode_Block_Num + Node_Order;
	   
	c = Real_Order / SECTOR;
	s = Real_Order % SECTOR;
	if (Inode_or_Data_Block) {
		Memcpy_from_C_S (Socket_fd, c, s, buffer);
		for (i = (Node_Order % 4) * 64, j = 0; j < 64; ++i, ++j)
			buffer[i] = buf[j];
		Memcpy_to_C_S (Socket_fd, c, s, buffer);
	}
	else Memcpy_to_C_S (Socket_fd, c, s, buf);
}

void Update_Super_Block (int Socket_fd) { //Update Super block value
	char buffer[1024];
	sprintf (buffer, "%d/%d/%d/%d/%d/%d/%d/", Inode_Num, Inode_Empty_Num, Inode_Block_Num, Bitmap_Block_Num, Bitmap_Start, Data_Num, Data_Empty_Num);
	Memcpy_to_C_S (Socket_fd, CYLINDER - 1, SECTOR - 1, buffer);
}


void Get_Super_Block (int Socket_fd) { //Get values from super block
	char buffer[1024];
	char *Super_Info[10];
	Memcpy_from_C_S (Socket_fd, CYLINDER - 1, SECTOR - 1, buffer);
	Parse_Slash_Line (buffer, Super_Info);
	Inode_Num = atoi(Super_Info[0]); 
	Inode_Empty_Num = atoi(Super_Info[1]); 
	Inode_Block_Num = atoi(Super_Info[2]);
	Bitmap_Block_Num = atoi (Super_Info[3]); 
	Bitmap_Start = atoi (Super_Info[4]);
	Data_Num = atoi(Super_Info[5]); 
	Data_Empty_Num = atoi (Super_Info[6]);
}

void Update_Bitmap (int Socket_fd, int n, bool Used_or_Not, bool Inode_or_Data_Block) { //Used_or_Not = 1, ->full, Used_or_Not = 0, ->empty; Inode_or_Data_Block
	int Bitmap_Bias;
	int Byte_Bias;
	char buf[1024];
	if (Inode_or_Data_Block) {
		Bitmap_Bias = Bitmap_Start + ( n / 256 );
		Byte_Bias = n % 256;
	}
	else { 
		Bitmap_Bias = Bitmap_Start + (Inode_Num + n) / 256;
		Byte_Bias = (n + Inode_Num) % 256;
	}
	Memcpy_from_C_S (Socket_fd, Bitmap_Bias/SECTOR, Bitmap_Bias%SECTOR, buf);
		
	if (Used_or_Not) buf[Byte_Bias] = '1';
	else buf[Byte_Bias] = '0';
	Memcpy_to_C_S (Socket_fd, Bitmap_Bias/SECTOR, Bitmap_Bias%SECTOR, buf);
	if (Inode_or_Data_Block && Used_or_Not) Inode_Empty_Num--;
	else if (Inode_or_Data_Block && !Used_or_Not) Inode_Empty_Num++;
	else if (!Inode_or_Data_Block && Used_or_Not) Data_Empty_Num--;
	else if (!Inode_or_Data_Block && !Used_or_Not) Data_Empty_Num++;
}

int Get_Empty_Block (int Socket_fd, bool Inode_or_Data_Block) {  //Inode_or_Data_Block = 1, inode; Inode_or_Data_Block = 0, data
	int i, j, k;	
	char Update_Num[257];
	Bitmap_Content[0] = '\0';
	for (i = Bitmap_Start; i < CYLINDER * SECTOR - 1; ++i) {
		Memcpy_from_C_S (Socket_fd, i/SECTOR, i%SECTOR, Update_Num);
		strcat (Bitmap_Content, Update_Num); 	
	}
	if (!Inode_or_Data_Block)  { j = Inode_Num; k = Bitmap_Start - 1; }
	else  { j = 0; k = Inode_Num - 1; }
	for (i = j; i <= k; ++i) {
		if (Bitmap_Content[i] == '0') break;
	}
	if (Inode_or_Data_Block) return i;
	else return (i - Inode_Num);
}

void Update_FileDir_Num_of_Current_Dir (int Socket_fd, int size, int File_Number) {	// Add file count of the current directory
	Update_Super_Block (Socket_fd);
	char Update_Num[8];
	sprintf (Update_Num, "%08d", File_Number + 1);
	Write_Npart_of_Inode (Socket_fd, Current_Dir, 1, Update_Num);
}

void Strench_File_to_New_Size (int Socket_fd, int Inode_Num, int New_Size) {	// Allocate new blocks or delete needless blocks of the rewrite or append operation
	int Origin_Size = Get_File_Size (Socket_fd, Inode_Num);
	int Origin_Block, New_Block;
	int i;
	int Empty_Block;
	int Single_Inode;
	char Update_Num[9];
	char Direct_Pointer[9];
	sprintf (Update_Num, "%08d", New_Size);
	Write_Npart_of_Inode (Socket_fd, Inode_Num, 0, Update_Num); //store new size
	if (Origin_Size == 0) Origin_Block = 0;
	else Origin_Block = (Origin_Size - 1) / 256 + 1;
	if (New_Size == 0) New_Block = 0;
	else New_Block = (New_Size - 1) / 256 + 1;
	if (New_Block == Origin_Block) return;
	if (New_Block > Origin_Block) { // MORE THAN CONDITION
		// Direct
		if (New_Block <= 4) {
			for (i = Origin_Block; i < New_Block; ++i) {
				Empty_Block = Get_Empty_Block (Socket_fd, 0);
				sprintf (Update_Num, "%08d\0", Empty_Block);
				Write_Npart_of_Inode (Socket_fd, Inode_Num, i + 2, Update_Num);
				Update_Bitmap (Socket_fd, Empty_Block, 1, 0);
			}
		}

		// Single_Indirect
		else if (New_Block <= 36) {
			if (Origin_Block <= 4) {
				for (i = Origin_Block; i < 5; ++i) {
					Empty_Block = Get_Empty_Block (Socket_fd, 0);
					sprintf (Update_Num, "%08d\0", Empty_Block);
					Write_Npart_of_Inode (Socket_fd, Inode_Num, i + 2, Update_Num);
					Update_Bitmap (Socket_fd, Empty_Block, 1, 0);
				}
				Single_Inode = Empty_Block;
				Empty_Block = Get_Empty_Block (Socket_fd, 0);
				sprintf (Update_Num, "%08d\0", Empty_Block);
				Write_Npart_of_Single_Dir (Socket_fd, Single_Inode, 0, Update_Num);
				Update_Bitmap (Socket_fd, Empty_Block, 1, 0);
				for (i = 5; i < New_Block; ++i) {
					Empty_Block = Get_Empty_Block (Socket_fd, 0);
					sprintf (Update_Num, "%08d\0", Empty_Block);	
					Write_Npart_of_Single_Dir (Socket_fd, Single_Inode, i - 4, Update_Num);
					Update_Bitmap (Socket_fd, Empty_Block, 1, 0);
	  			}
			}
			else {
				Get_Npart_of_Inode (Socket_fd, Inode_Num, 6, Direct_Pointer);
				for (i = Origin_Block; i < New_Block; ++i) {
					Empty_Block = Get_Empty_Block (Socket_fd, 0);
					sprintf (Update_Num, "%08d\0", Empty_Block);	
					Write_Npart_of_Single_Dir (Socket_fd, atoi (Direct_Pointer), i - 4, Update_Num);
					Update_Bitmap (Socket_fd, Empty_Block, 1, 0);
	  			}
			}
		}
	} 

	// LESS THAN CONDITION
	if (New_Block < Origin_Block) {
		if (Origin_Block <= 4) { // Direct
			for (i = Origin_Block; i > New_Block; --i) {
				Get_Npart_of_Inode (Socket_fd, Inode_Num, i + 1, Update_Num);
				Update_Bitmap (Socket_fd, atoi(Update_Num), 0, 0);
			}
		}
		else if (Origin_Block == 5) { 
			Get_Npart_of_Inode (Socket_fd, Inode_Num, 6, Update_Num);
			Get_Npart_of_Single_Dir (Socket_fd, atoi(Update_Num), 0, Direct_Pointer);
			Update_Bitmap (Socket_fd, atoi(Update_Num), 0, 0);
			Update_Bitmap (Socket_fd, atoi(Direct_Pointer), 0, 0);
			for (i = Origin_Block - 1; i > New_Block; --i) {
				Get_Npart_of_Inode (Socket_fd, Inode_Num, i + 1, Update_Num);
				Update_Bitmap (Socket_fd, atoi(Update_Num), 0, 0);
			}
		}
		else if(Origin_Block <= 36) { // Single_Indirect
			Get_Npart_of_Inode (Socket_fd, Inode_Num, 6, Update_Num);
		
			if (New_Block >= 5) {
				for (i = Origin_Block; i > New_Block; --i) {
					Get_Npart_of_Single_Dir (Socket_fd, atoi(Update_Num), i - 5, Direct_Pointer);

					Update_Bitmap (Socket_fd, atoi(Direct_Pointer), 0, 0);
				}
			}
			else {
				for (i = Origin_Block; i > 5; --i) {

					Get_Npart_of_Single_Dir (Socket_fd, atoi(Update_Num), i - 5, Direct_Pointer);

					Update_Bitmap (Socket_fd, atoi(Direct_Pointer), 0, 0);
				}
				Update_Bitmap (Socket_fd, atoi(Update_Num), 0, 0);
				for (i = 4; i > New_Block; --i) {
					Get_Npart_of_Inode (Socket_fd, Inode_Num, i + 1, Update_Num);

					Update_Bitmap(Socket_fd, atoi(Update_Num), 0, 0);
				}			
			}		
		}
		else {} // Double_indirect
	}
}

void Memcpy_File_Content_to_File (int Socket_fd, int Inode_Num, const char *content) {	// Memcpy the new file content to the allocated file space
	int size = strlen (content);
	char Direct_Pointer[8];
	char Single_Pointer[8];
	char Double_Pointer[8];
	int i, j, h = 0, k;
	char buf[257];
	
	Strench_File_to_New_Size (Socket_fd, Inode_Num, size); 

	if ( (size - 1) / 256 < 4) { // Direct
		for (i = 2; i < (size - 1) / 256 + 2; ++i) {
			Get_Npart_of_Inode(Socket_fd, Inode_Num, i, Direct_Pointer);
			for (j = 0, h = (i - 2) * 256; j < 256; ++j, ++h)
				buf[j] = content[h];
			buf[j] = '\0';
			Write_Block (Socket_fd, atoi(Direct_Pointer), buf, 0);					
		}
		for (j = 0; h < size; ++j, ++h) buf[j] = content[h];
		buf[j] = '\0';
		Get_Npart_of_Inode (Socket_fd, Inode_Num, i, Direct_Pointer);
		Write_Block (Socket_fd, atoi(Direct_Pointer), buf, 0);
	}


	else if ((size - 1) / 256 < 36) { // Single_Indirect
		for (i = 2; i <= 5; ++i) {
			Get_Npart_of_Inode (Socket_fd, Inode_Num, i, Direct_Pointer);	
			for (j = 0, h = (i - 2) * 256; j < 256; ++j, ++h)
				buf[j] = content[h];
			buf[j] = '\0';
			Write_Block (Socket_fd, atoi(Direct_Pointer), buf, 0);
		}	
		Get_Npart_of_Inode (Socket_fd, Inode_Num, 6, Direct_Pointer);
		for (i = 0; i < (size - 1) / 256 - 4; ++i) {
			Get_Npart_of_Single_Dir(Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
			for (j = 0, h = (i + 4) * 256; j < 256; ++j, ++h)
				buf[j] = content[h];
			buf[j] = '\0';
			Write_Block (Socket_fd, atoi(Single_Pointer), buf, 0);
		}
		Get_Npart_of_Single_Dir (Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
		h = (i + 4) * 256;		
		for (j = 0; h < size; ++j, ++h) buf[j] = content[h];
		buf[j] = '\0';
		Write_Block (Socket_fd, atoi(Single_Pointer), buf, 0);
	}
	else { // Double_indirect
		for (i = 2; i <= 5; ++i) {
			Get_Npart_of_Inode (Socket_fd, Inode_Num, i, Direct_Pointer);	
			for (j = 0, h = (i - 2) * 256; j < 256; ++j, ++h)
				buf[j] = content[h];
			buf[j] = '\0';
			Write_Block (Socket_fd, atoi(Direct_Pointer), buf, 0);
		}
		Get_Npart_of_Inode (Socket_fd, Inode_Num, 6, Direct_Pointer);
		for (i = 0; i < 32; ++i) {
			Get_Npart_of_Single_Dir(Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
			for (j = 0, h = (i + 4) * 256; j < 256; ++j, ++h)
				buf[j] = content[h];
			buf[j] = '\0';
			Write_Block (Socket_fd, atoi(Single_Pointer), buf, 0);
		}
		Get_Npart_of_Inode (Socket_fd, Inode_Num, 7, Direct_Pointer);
		for (i = 0; i < ((size - 1) / 256 - 36) / 32; ++i)
			for (j = 0; j < 32; ++j) {
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Single_Pointer), j, Double_Pointer);
				for (j = 0, h = (32 * i + j + 36) * 256; j < 256; ++j, ++h)
					buf[j] = content[h];
				buf[j] = '\0';
				Write_Block (Socket_fd, atoi(Double_Pointer), buf, 0); 	
			}
		Get_Npart_of_Single_Dir (Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
		for (j = 0; j <= ( (size - 1) / 256 - 36 ) % 32; ++j) {		
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Single_Pointer), j, Double_Pointer);
				for (k = 0, h = ( (size - 1) / 256 - 36 - ( (size - 1) / 256 - 36 ) % 32 + j) * 256; k < 256; ++k, ++h)
					buf[k] = content[h];
				buf[k] = '\0';
				Write_Block (Socket_fd, atoi(Double_Pointer), buf, 0);
		}			
	}
}

void Memcpy_from_File_Content (int Socket_fd, int Inode_No) {	// Get whole content of the file with Inode_No
	int size = Get_File_Size (Socket_fd, Inode_No);
	int i, j;
	char Direct_Pointer[8];
	char Single_Pointer[8];
	char Double_Pointer[8];
	char content[257];
	char Single_Indirect[256];
	Full_File_Content[0] = '\0';
	if (size == 0) return;
	if ( (size - 1) / 256 < 4) { // Direct
		for (i = 2; i <= (size - 1)/256 + 2; ++i) {
			Get_Npart_of_Inode (Socket_fd, Inode_No, i, Direct_Pointer);
			Read_Block (Socket_fd, atoi(Direct_Pointer), content, 0);
			strcat (Full_File_Content, content);
		}
	}
	else if ( ((size - 1) / 256) < 36) { // Single_Indirect
		for (i = 2; i <= 5; ++i) {
			Get_Npart_of_Inode (Socket_fd, Inode_No, i, Direct_Pointer);
			Read_Block (Socket_fd, atoi(Direct_Pointer), content, 0);
			strcat (Full_File_Content, content);
		}
		Get_Npart_of_Inode (Socket_fd, Inode_No, 6, Direct_Pointer);
		for (i = 0; i <= (size - 1) / 256 - 4; ++i) {
			Get_Npart_of_Single_Dir(Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
			Read_Block (Socket_fd, atoi(Single_Pointer), content, 0);
			strcat (Full_File_Content, content);
		}
	}
	else {	// Double_Indirect
		for (i = 2; i <= 5; ++i) {
				Get_Npart_of_Inode (Socket_fd, Inode_No, i, Direct_Pointer);
				Read_Block (Socket_fd, atoi(Direct_Pointer), content, 0);
				if (i == 2) strcpy (Full_File_Content, content);
				else strcat (Full_File_Content, content);
		}
		Get_Npart_of_Inode (Socket_fd, Inode_No, 6, Direct_Pointer);
		for (i = 0; i < 32; ++i) {
			Get_Npart_of_Single_Dir(Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
			Read_Block (Socket_fd, atoi(Single_Pointer), content, 0);
			strcat (Full_File_Content, content);
		}
		Get_Npart_of_Inode (Socket_fd, Inode_No, 7, Direct_Pointer);
		for (i = 0; i < ((size - 1) / 256 - 36) / 32; ++i)
			for (j = 0; j < 32; ++j) {
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Single_Pointer), j, Double_Pointer);
				Read_Block (Socket_fd, atoi(Double_Pointer), content, 0); 	
				strcat (Full_File_Content, content);
			}
		Get_Npart_of_Single_Dir (Socket_fd, atoi(Direct_Pointer), i, Single_Pointer);
		for (j = 0; j <= ( (size - 1)/256 - 36 ) % 32; ++j) {		
				Get_Npart_of_Single_Dir (Socket_fd, atoi(Single_Pointer), j, Double_Pointer);
				Read_Block (Socket_fd, atoi(Double_Pointer), content, 0); 	
				strcat (Full_File_Content, content);
		}	
	}
}

void Get_Npart_of_Single_Dir (int Socket_fd, int Data_Block_No, int n, char *content) {	// Get the certain part n of a Single Indirect Block
	char Get_Num[9];
	char buf[257];
	int i, j;
	Read_Block (Socket_fd, Data_Block_No, buf, 0);
	for (i = n * 8, j = 0; j < 8; ++i, ++j) Get_Num[j] = buf[i];
	Get_Num[j] = '\0';
	strcpy (content, Get_Num);
}

void Get_Npart_of_Inode (int Socket_fd, int Inode_No, int n, char *content) { 	// Get the certain part n of a Inode
	char buf[64];
	char Get_Num[8];
	int i, j;
	Read_Block (Socket_fd, Inode_No, buf, 1);
	for (i = n * 8, j = 0; j < 8; ++i, ++j)
		Get_Num[j] = buf[i];
	strcpy(content, Get_Num);
}

void Write_Npart_of_Inode (int Socket_fd, int Inode_No, int n, const char *content) {	// Write to the certain part n of a Inode
	char buf[65];
	int i, j;
	Read_Block (Socket_fd, Inode_No, buf, 1);
	buf[64] = '\0';
	for (i = n * 8, j = 0; j < 8; ++i, ++j) 
		buf[i] = content[j];
	Write_Block (Socket_fd, Inode_No, buf, 1);
}

void Write_Npart_of_Single_Dir (int Socket_fd, int Data_Block_No, int n, const char *content) {	// Write to the part N of a Single Indirect Block
	char buf[257];
	int i, j;
	if (n == 0) Write_Block (Socket_fd, Data_Block_No, "0", 0);
	Read_Block (Socket_fd, Data_Block_No, buf, 0);
	for (i = n * 8, j = 0; j < 8; ++i, ++j) buf[i] = content[j];
	Write_Block (Socket_fd, Data_Block_No, buf, 0);	
}

int Get_File_Size (int Socket_fd, int Inode_No) {	// Get the file_size of a file with Inode_No
	char Get_Num[8];
	int i, j;
	Get_Npart_of_Inode (Socket_fd, Inode_No, 0, Get_Num);
	return atoi(Get_Num); 	
}

int Get_Num_of_Sub_FileDir (int Socket_fd, int Inode_No) { 	// Get the sub file/directory num of a directory with Inode_No	
	int i, j;
	char Get_Num[8];
	Get_Npart_of_Inode (Socket_fd, Inode_No, 1, Get_Num);
	return atoi(Get_Num); 	
}


void Initial_Dir (int Socket_fd, int Inode_Num, int pre_num) {	// Create a new directory node and fill it with need content and save to file
	char buf[65];
	sprintf (buf, "0.*000000000000000000000%08d0..*00000000000000000000%08d", Inode_Num, pre_num);
	buf[64] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, Inode_Num, buf);
	char Update_Num[8];
	sprintf (Update_Num, "%08d", 2);
	Write_Npart_of_Inode (Socket_fd, Inode_Num, 1, Update_Num);
}

int Format_File_System (int Socket_fd) { 	// MAIN_FUNC: format the file system
	int i;
	char buf[1024];
	bzero (buf, 1024);
	Inode_Num = Inode_Empty_Num = (CYLINDER * SECTOR / 5) - ( ( CYLINDER * SECTOR / 5 ) % 4 ); 
	Inode_Block_Num = Inode_Num / 4;
	Bitmap_Block_Num = (Inode_Num * 4 + CYLINDER * SECTOR - Inode_Block_Num) / 256 + 1 ;
	Bitmap_Start = CYLINDER * SECTOR - 1 - Bitmap_Block_Num;
	Data_Num = Data_Empty_Num = CYLINDER * SECTOR - Bitmap_Block_Num - Inode_Num;
	Update_Super_Block (Socket_fd);
	for (i = Bitmap_Start; i < CYLINDER * SECTOR - 1; ++i) Memcpy_to_C_S (Socket_fd, i/SECTOR, i%SECTOR, "0");
	Update_Bitmap (Socket_fd, 0, 1, 1);//Root_Dir_Inode 'SECTOR inode
	Update_Bitmap (Socket_fd, 0, 1, 0);//Root_Dir_Inode 'SECTOR dir
	//initialize dir 
	Current_Dir = 0;
	for (i = 0; i < Inode_Block_Num; ++i) Memcpy_to_C_S (Socket_fd, i/SECTOR, i%SECTOR, "0");
	Write_Npart_of_Inode (Socket_fd, 0, 0, "00000064");
	Write_Npart_of_Inode (Socket_fd, 0, 1, "00000002");
	Write_Block (Socket_fd, 0, "0.*000000000000000000000000000000..*0000000000000000000000000000", 0);
	return 1;            
}

int Make_File (int Socket_fd, const char *File_Name) { 	// MAIN_FUNC: make a new file in current directory
	int i, j, k;
	if (Inode_Empty_Num == 0 || Data_Empty_Num == 0) return 2;
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	Memcpy_from_File_Content(Socket_fd, Current_Dir);
	if (Check_Same_Name (Full_File_Content, File_Name, File_Number, 1) != -1) return 1;
	int New_Inode_Pos = Get_Empty_Block(Socket_fd, 1);
	Write_Block (Socket_fd, New_Inode_Pos, "0", 1);
	int size = Get_File_Size (Socket_fd, Current_Dir);
	char Direct_Pointer[8];
	sprintf (Direct_Pointer,"%08d",New_Inode_Pos);
	i = File_Number * 32;
	Full_File_Content[i] = '1';
	for (k = i + 1, j = 0; j < strlen(File_Name); ++k, ++j)
		Full_File_Content[k] = File_Name[j];
	Full_File_Content[k] = '*';
	for (k = i + 24, j = 0; j < 8; ++j, ++k)
		Full_File_Content[k] = Direct_Pointer[j];
	Full_File_Content[k] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, Current_Dir, Full_File_Content);
	Update_Bitmap (Socket_fd, New_Inode_Pos, 1, 1);
	Update_FileDir_Num_of_Current_Dir(Socket_fd, size, File_Number);
	return 0;
}

int Make_Dir (int Socket_fd, const char *Dir_Name) {	// MAIN_FUNC: make a new directory in current directory
	int i, j, k;
	if (Inode_Empty_Num == 0 || Data_Empty_Num == 0) return 2;
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	if (Check_Same_Name (Full_File_Content, Dir_Name, File_Number, 0) != -1) return 1;
	int New_Inode_Pos = Get_Empty_Block(Socket_fd, 1);
	Write_Block (Socket_fd, New_Inode_Pos, "0", 1);
	int size = Get_File_Size (Socket_fd, Current_Dir);
	char Direct_Pointer[8];
	sprintf (Direct_Pointer,"%08d",New_Inode_Pos);
	i = File_Number * 32;
	Full_File_Content[i] = '0';
	for (k = i + 1, j = 0; j < strlen(Dir_Name); ++k, ++j)
		Full_File_Content[k] = Dir_Name[j];
	Full_File_Content[k] = '*';
	for (k = i + 24, j = 0; j < 8; ++j, ++k)
		Full_File_Content[k] = Direct_Pointer[j];
	Full_File_Content[k] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, Current_Dir, Full_File_Content);
	Update_Bitmap (Socket_fd, New_Inode_Pos, 1, 1);
	Update_FileDir_Num_of_Current_Dir(Socket_fd, size, File_Number);
	Initial_Dir (Socket_fd, New_Inode_Pos, Current_Dir);
	return 0;
}

int Remove_File (int Socket_fd, const char *File_Name) {	// MAIN_FUNC: del a file name File_Name in the current directory
	int i, j;
	char Direct_Pointer[9];
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	int Start_Pos = Check_Same_Name(Full_File_Content, File_Name, File_Number, 1);
	int size = Get_File_Size (Socket_fd, Current_Dir);
	if (Start_Pos == -1) return 1;
	for (i = Start_Pos + 24, j = 0; j < 8; ++i, ++j)
		Direct_Pointer[j] = Full_File_Content[i];
	Direct_Pointer[j] = '\0';
	for (i = Start_Pos, j = size - 32; j < size; ++i, ++j) {
		Full_File_Content[i] = Full_File_Content[j];
	}
	Full_File_Content[size - 32] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, Current_Dir, Full_File_Content);
	Memcpy_File_Content_to_File (Socket_fd, atoi(Direct_Pointer), "\0");
	Update_Bitmap (Socket_fd, atoi(Direct_Pointer), 0, 1);
	Update_Super_Block (Socket_fd);
	char Update_Num[9];
	sprintf (Update_Num, "%08d", File_Number - 1);
	Write_Npart_of_Inode (Socket_fd, Current_Dir, 1, Update_Num);
	return 0;
}

int Check_Same_Name (const char *Dir_Content, const char *File_Name, int File_Number, bool Inode_or_Data_Block) {	// check whether a same file/directory name already exists in current directory
	int count = 0;
	char Name_Copy[24];
	char *Get_Name;
	int i, j;
	for (i = 0; count < File_Number; i = i + 32, ++count) { 
		if ( ( Inode_or_Data_Block && ( Dir_Content[i] == '1') ) || ( !Inode_or_Data_Block && ( Dir_Content[i] == '0') ) ) {
			for (j = 0; j < 23; ++j)
				Name_Copy[j] = Dir_Content[i + 1 + j];
			Name_Copy[23] = '\0';
			Get_Name = strtok (Name_Copy, "*");
			if(!strcmp(Get_Name, File_Name)) return i;			
		}
	}
	return -1;
}

int Change_Dir (int Socket_fd, char *CD_Path) {	// MAIN_FUNC: Change current directory
	char Direct_Pointer[9];
	char *p[100];	
	int i, j, k, Check_Id, File_Number;
	if (CD_Path[0] != '/') {	// Relative path
		int count = Parse_Slash_Line(CD_Path, p);
		int New_Dir = Current_Dir;
		for (i = 0; i < count; ++i) {
			Memcpy_from_File_Content (Socket_fd, New_Dir);
			File_Number = Get_Num_of_Sub_FileDir (Socket_fd, New_Dir);
			Check_Id = Check_Same_Name (Full_File_Content, p[i], File_Number, 0);
			if (Check_Id == -1) return 1;
			for (k = Check_Id + 24, j = 0; j < 8; ++k, ++j) Direct_Pointer[j] = Full_File_Content[k];
			Direct_Pointer[j] = '\0';
			New_Dir = atoi(Direct_Pointer);
		}
		Current_Dir = New_Dir;
	}
	else {	// Absolute path
		int count = Parse_Slash_Line(CD_Path + 1, p);
		int New_Dir = 0;
		for (i = 0; i < count; ++i) {
			Memcpy_from_File_Content (Socket_fd, New_Dir);
			File_Number = Get_Num_of_Sub_FileDir (Socket_fd, New_Dir);
			Check_Id = Check_Same_Name (Full_File_Content, p[i], File_Number, 0);
			if (Check_Id == -1) return 1;
			for (k = Check_Id + 24, j = 0; j < 8; ++k, ++j) Direct_Pointer[j] = Full_File_Content[k];
			Direct_Pointer[j] = '\0';
			New_Dir = atoi(Direct_Pointer);
		}
		Current_Dir = New_Dir;
	}
	return 0;
}

int List(int Socket_fd, const char *Inode_Id) {	// MAIN_FUNC: List files and directories in the current category
	int i, j;
	List_Show_Content[0] = '\0';
	char Get_Name[30];	
	char File_Name[30];
	char Direct_Pointer[9];
	char size[9];
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);	
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	if (!strcmp  (Inode_Id, "0")) {
		strcpy (List_Show_Content, "Type\t\tName\n");
		for (i = 0; i < File_Number; ++i) {
			if (Full_File_Content[i * 32] == '0')  
				strcat (List_Show_Content, "Directory\t");
			else 
				strcat (List_Show_Content, "File\t\t");
			for (j = 0; j < 24; ++j) {
				Get_Name[j] = Full_File_Content[i * 32 + j + 1];
			}
			strcat (List_Show_Content, strtok(Get_Name, "*"));
			strcat (List_Show_Content, "\n");
		}	
	}
	else if (!strcmp( Inode_Id, "1") ){
		strcpy (List_Show_Content, "Type\t\tName\tFile_Num/File_Size\n");
		for (i = 0; i < File_Number; ++i) {
			if (Full_File_Content[i * 32] == '0') {
				strcat(List_Show_Content, "Directory\t");
				for (j = 0; j < 24; ++j) {
					Get_Name[j] = Full_File_Content[i * 32 + j + 1];
				}
				strcat (List_Show_Content, strtok(Get_Name, "*"));
				for (j = 0; j < 8; ++j) Direct_Pointer[j] = Full_File_Content[i * 32 + 24 + j];
				Direct_Pointer[j] = '\0';
				Get_Npart_of_Inode (Socket_fd, atoi(Direct_Pointer), 1, size);
				sprintf (size, "%d files", atoi(size) - 2);
				strcat (List_Show_Content, "\t");
				strcat (List_Show_Content, size);
				strcat (List_Show_Content, "\n");
			}
			else {
				strcat (List_Show_Content, "File\t\t");
				for (j = 0; j < 24; ++j) {
					Get_Name[j] = Full_File_Content[i * 32 + j + 1];
				}
				strcat (List_Show_Content, strtok(Get_Name, "*"));
				for (j = 0; j < 8; ++j) Direct_Pointer[j] = Full_File_Content[i * 32 + 24 + j];
				Direct_Pointer[j] = '\0';
				Get_Npart_of_Inode (Socket_fd, atoi(Direct_Pointer), 0, size);
				sprintf (size, "%d bytes", atoi(size));
				strcat (List_Show_Content, "\t");
				strcat (List_Show_Content, size);
				strcat (List_Show_Content, "\n");
			}
		}
	}
	else
		return 1;
	return 0;
}

int Write_Filename_L_Data (int Socket_fd, const char *File_Name, int length, const char *content) {	// MAIN_FUNC: Rewrite file "File_Name" with length given characters
	if (length != strlen(content)) return 2;	
	Memcpy_from_File_Content (Socket_fd, Current_Dir);	
	int size;
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	int d = Check_Same_Name (Full_File_Content, File_Name, File_Number, 1);
	char Direct_Pointer [9];
	int i;
	if (d == -1) return 1;
	for (i = 0; i < 8; ++i) 
		Direct_Pointer[i] = Full_File_Content[d + i + 24];
	Direct_Pointer[i] = '\0';
	size = Get_File_Size (Socket_fd, atoi(Direct_Pointer));
	if ( (length - size - 1) / 256 + 1 > Data_Empty_Num) return 3;
	Memcpy_File_Content_to_File (Socket_fd, atoi(Direct_Pointer), content);
}

int Append_Filename_L_Data (int Socket_fd, const char *File_Name, int length, const char *content) {	// MAIN_FUNC: Append "File_Name" with length given characters at the end of file
	if (length != strlen(content)) return 2;
	if ( (length - 1) / 256 + 1 > Data_Empty_Num) return 3;
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	int size; 
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	int Check_Id = Check_Same_Name (Full_File_Content, File_Name, File_Number, 1);
	char Direct_Pointer[9];
	int i, j;
	if (Check_Id == -1) return 1;
	for (i = 0; i < 8; ++i)
		Direct_Pointer[i] = Full_File_Content[Check_Id + i + 24];
	Direct_Pointer[i] = '\0';
	size = Get_File_Size (Socket_fd, atoi(Direct_Pointer));
	Memcpy_from_File_Content (Socket_fd, atoi(Direct_Pointer));
	for (i = 0, j = size; i < length; ++i, ++j) Full_File_Content[j] = content[i];
	Full_File_Content[j] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, atoi(Direct_Pointer), Full_File_Content);
}

int Catch_File (int Socket_fd, const char *File_Name) {	// MAIN_FUNC: Get all the content of "File_Name"
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	int size;
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	int Check_Id = Check_Same_Name (Full_File_Content, File_Name, File_Number, 1);
	char Direct_Pointer[9];
	int i, j;
	if (Check_Id == -1) return 1;
	for (i = 0; i < 8; ++i) 
		Direct_Pointer[i] = Full_File_Content[Check_Id + 24 + i];
	Direct_Pointer[i] = '\0';	
	Memcpy_from_File_Content (Socket_fd, atoi(Direct_Pointer));
	size = Get_File_Size (Socket_fd, atoi(Direct_Pointer));
	Full_File_Content[size] = '\0';	
	return 0;
}

int Remove_Dir (int Socket_fd, const char *Dir_Name) {	// MAIN_FUNC: Delete a given directory
	char Direct_Pointer[9];
	int i, j;
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Current_Dir);
	Memcpy_from_File_Content (Socket_fd, Current_Dir);
	int size = Get_File_Size (Socket_fd, Current_Dir);
	int Start_Pos = Check_Same_Name (Full_File_Content, Dir_Name, File_Number, 0);
	if (Start_Pos == -1) return 1;
	for (i = Start_Pos + 24, j = 0; j < 8; ++i, ++j) Direct_Pointer[j] = Full_File_Content[i];
	Direct_Pointer[j] = '\0';
	for (i = Start_Pos, j = size - 32; j < size; ++i, ++j) Full_File_Content[i] = Full_File_Content[j];
	Full_File_Content[size - 32] = '\0';
	Memcpy_File_Content_to_File (Socket_fd, Current_Dir, Full_File_Content);
	Remove_Dir_Recursion (Socket_fd, atoi(Direct_Pointer));
	Update_Super_Block (Socket_fd);
	sprintf (Direct_Pointer, "%08d", File_Number - 1);
	Write_Npart_of_Inode (Socket_fd, Current_Dir, 1, Direct_Pointer);
	return 0;
}

void Remove_Dir_Recursion (int Socket_fd, int Inode_Num) {	// Used to recursively delete the subdirectorys and files of the deleted directory
	int i, j;
	char Update_Num[9];
	Memcpy_from_File_Content (Socket_fd, Inode_Num);
	int File_Number = Get_Num_of_Sub_FileDir (Socket_fd, Inode_Num);
	for (i = 2; i < File_Number; ++i) {
		if (Full_File_Content[i * 32] == '1') {
			for (j = 0; j < 8; ++j) Update_Num[j] = Full_File_Content[i * 32 + 24 + j];
			Update_Num[j] = '\0';
			Memcpy_File_Content_to_File(Socket_fd, atoi(Update_Num), "\0");
			Update_Bitmap(Socket_fd, atoi(Update_Num), 0, 1);
		}
		else if (Full_File_Content[i * 32] == '0') {
			for (j = 0; j < 8; ++j) Update_Num[j] = Full_File_Content[i * 32 + 24 + j];
			Update_Num[j] = '\0';
			Remove_Dir_Recursion(Socket_fd, atoi(Update_Num));
		}
	}
	Memcpy_File_Content_to_File(Socket_fd, Inode_Num, "\0"); 
	Update_Bitmap (Socket_fd, Inode_Num, 0, 1);
}


