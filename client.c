#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdlib.h> 
#include <string.h> 
#include <signal.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define MAX 100 
#define PORT 8080 
#define DOWNLOAD "d"
#define UPLOAD "u"
#define NOTYPE "n"
#define NOTFOUND 'x'

#define SA struct sockaddr 
void dieWithError(char* message){
	write(0, message, strlen(message));
	write(0, "\n", 1);
    exit(0);
}

int fetchFromHeartBeatPort(int heartBeatPort) {
	int sockHb, opt1, opt2, opt3;
	struct sockaddr_in hbAddr;
	struct timeval hbTimeVal;
	hbTimeVal.tv_sec = 2;
	opt1 = 1;
	opt2 = 1;
	opt3 = 1;
	
	char portY[5];
	if ((sockHb = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		dieWithError("broadcast socket creation failed");
	if (setsockopt(sockHb, SOL_SOCKET, SO_REUSEADDR, (char*)&opt1, sizeof(opt1)) < 0)
		dieWithError("error while setsockopt ->  SO_REUSEADDR");
	if (setsockopt(sockHb, SOL_SOCKET, SO_RCVTIMEO, &hbTimeVal, sizeof(hbTimeVal)) < 0)
		dieWithError("error while setsockopt ->  SO_RCVTIMEO");
	if (setsockopt(sockHb, SOL_SOCKET, SO_BROADCAST, (char*)&opt2, sizeof(opt2)) < 0)
		dieWithError("error while setsockopt ->  SO_BROADCAST");
	if (setsockopt(sockHb, SOL_SOCKET, SO_REUSEPORT, (char*)&opt3, sizeof(opt3)) < 0)
		dieWithError("error while setsockopt ->  SO_REUSEPORT");
	hbAddr.sin_family = AF_INET; 
	hbAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
	hbAddr.sin_port = htons(heartBeatPort);
	if ((bind(sockHb, (SA*)&hbAddr, sizeof(hbAddr))) != 0) 
        dieWithError("bind to heartbeat socket failed ...");
	recvfrom(sockHb, portY, 5, 0, NULL, NULL);
	close(sockHb);
	return atoi(portY);


}

void fileSharing(int sockfd){
	char buff[MAX];
	char *type = NOTYPE;
	int n; 
	for (;;) {
		type = NOTYPE; 
		bzero(buff, sizeof(buff)); 
		write(0, "Enter your command : ", 21); 
		n = 0; 
		while ((buff[n++] = getchar()) != '\n'); 
		if (buff[8] == ' ' && buff[0] == 'd' && buff[1] == 'o' && buff[2] == 'w' && buff[3] == 'n' && buff[4] == 'l' && buff[5] == 'o' && buff[6] == 'a' && buff[7] == 'd')
			type = DOWNLOAD;
		else if (buff[6] == ' ' && buff[0] == 'u' && buff[1] == 'p' && buff[2] == 'l' && buff[3] == 'o' && buff[4] == 'a' && buff[5] == 'd')
			type = UPLOAD;
		if (type == DOWNLOAD) {
			int i = 9; //start of filename
			while (buff[i] != '\n'){
				i++;
			}
			int filenameSize = i - 9;
			char* filename = malloc(filenameSize * sizeof(char));
			i = 9; //start of filename
			while (buff[i] != '\n'){
				filename[i - 9] = buff[i];
				i++;
			}
			if(send(sockfd, type, strlen(type), 0) != strlen(type))      
				dieWithError("error while sending request type request...");
			sleep(1);
			if(send(sockfd, filename, strlen(filename), 0) != strlen(filename))      
				dieWithError("error while sending download request...");
			
			write(0, "download request successfully sent...\n", 38);
			char fileContent[16385];
			int recLen = recv(sockfd, fileContent, sizeof(fileContent) - 1, 0);
			if ((recLen == 1) && (fileContent[0] == NOTFOUND))
				write(0, "invalid file name...\n", 21);
			else {
				fileContent[recLen] = '\0';
				char* createFileName = malloc((strlen("./_client/") + strlen(filename)) + 1);
				memcpy(createFileName, "./_client/", strlen("./_client/") + 1);
				memcpy(createFileName + (strlen("./_client")/sizeof(char) + 1), filename, strlen(filename) + 1);
				int fd = open(createFileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				write(fd, fileContent, recLen - 1);
				close(fd);
				write(0, "required file downloded successfully :)\n", 40);
			}
		}else if (type == UPLOAD) {
			int i = 7; //start of filename
			while (buff[i] != '\n'){
				i++;
			}
			int filenameSize = i - 7;
			char* filename = malloc(filenameSize * sizeof(char));
			i = 7; //start of filename
			while (buff[i] != '\n'){
				filename[i - 7] = buff[i];
				i++;
			}

			char* filePath = malloc((strlen("./_client/") + strlen(filename)) + 1);
			memcpy(filePath, "./_client/", strlen("./_client/") + 1);
			memcpy(filePath + (strlen("./_client/")/sizeof(char)), filename, strlen(filename) + 1);
			int fd = open(filePath, O_RDONLY);
			if(fd < 0){
				write(0, "invalid filename...\n", 20);
				continue;
			}
			else 
			{
				int readLen;
				char fileContent[16385];
				readLen = read(fd, fileContent, sizeof(fileContent));
				fileContent[readLen] = '\0';

				if(send(sockfd, type, strlen(type), 0) != strlen(type))      
					dieWithError("error while sending request type request...");
				sleep(1);
				if(send(sockfd, filename, strlen(filename), 0) != strlen(filename))      
					dieWithError("error while sending download request...");
				sleep(1);
				if(send(sockfd, fileContent, readLen + 1, 0) != readLen + 1)      
					dieWithError("error while sending requested file...");
				write(0, "requested file sent to server...\n", 33);
			}
			close(fd);
			write(0, "required file uploaded successfully :)\n", 39);
		} else if (type == NOTYPE) {
			write(0, "invalid operand...\n", 19);
			continue;
		}

		if ((strncmp(buff, "exit", 4)) == 0) { 
			write(0, "Client Exit...\n", 15); 
			break; 
		} 
	}
}

int main(int argc, char* argv[]) { 
	int sockfd, connfd,portX; 
	struct sockaddr_in servaddr, cli; 
	char fileContent[1024]; 
	int heartBeatPort;
	if (argc != 4) { 
		write(0, "usage: ", 7);
		write(0, argv[0], strlen(argv[0]));
		write(0, " <port_x>\n", 9);
        exit(1);
    }

	heartBeatPort = atoi(argv[1]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		write(0, "socket creation failed...\n", 26); 
		exit(0); 
	} 
	else
		write(0, "Socket successfully created..\n", 30); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons((fetchFromHeartBeatPort(heartBeatPort))); 

	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		dieWithError("connection with the server failed...");
	} 
	else
		write(0, "connected to the server..\n", 26); 

	// function for chat 
	fileSharing(sockfd); 

	// close the socket 
	close(sockfd); 
} 
