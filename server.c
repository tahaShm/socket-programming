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
#define MAX 80 
#define PORT_Y 8080
#define DOWNLOAD "d"
#define UPLOAD "u"
#define NOTYPE "n"
#define NOTFOUND 'x'
#define SA struct sockaddr 
#define QUEUELIMIT 10
int sockX;
struct sockaddr_in servAddrX;

void dieWithError(char* message){
    write(0, message, strlen(message));
	write(0, "\n", 1);
    exit(0);
}

void heartBeatHandler() {
    sendto(sockX, "8080", strlen("8080"), 0, (struct sockaddr *)&servAddrX, sizeof(servAddrX));
    alarm(1);
    // write(0, "heartbeat\n", 10);
}
 
// Driver function 
int main(int argc, char* argv[]) 
{ 
    unsigned short portX;
    int opt1 = 1, opt2 = 1, opt3 = 1;
    int sockY, connfd, len; 
    struct sockaddr_in servAddrY, cli;

    if (argc != 2) { 
        write(0, "usage: ", 7);
		write(0, argv[0], strlen(argv[0]));
		write(0, " <port_x>\n", 10);
        exit(1);
    }

    portX = atoi(argv[1]);

    /////////////////////////////////////////////////////////////// stablish broadcast on port_x
    
    if ((sockX = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("socket X failed");
    else
        write(0, "socket X successfully created...\n", 33);
    
    if (setsockopt(sockX, SOL_SOCKET, SO_REUSEADDR, (char*)&opt1, sizeof(opt1)) < 0)
		dieWithError("error while setsockopt ->  SO_REUSEADDR");
	if (setsockopt(sockX, SOL_SOCKET, SO_BROADCAST, (char*)&opt2, sizeof(opt2)) < 0)
		dieWithError("error while setsockopt ->  SO_BROADCAST");
	if (setsockopt(sockX, SOL_SOCKET, SO_REUSEPORT, (char*)&opt3, sizeof(opt3)) < 0)
		dieWithError("error while setsockopt ->  SO_REUSEPORT");


    bzero(&servAddrX, sizeof(servAddrX));

    // assign IP, PORT 
    servAddrX.sin_family = AF_INET; 
    servAddrX.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    servAddrX.sin_port = htons(portX); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockX, (SA*)&servAddrX, sizeof(servAddrX))) != 0) { 
        dieWithError("socket X bind failed ...");
    }
    else
        write(0, "socket X successfully binded\n", 29);
    signal(SIGALRM, heartBeatHandler);
    alarm(1);

    /////////////////////////////////////////////////////////////// connect to port_y

    // socket create and verification 
    if ((sockY = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        dieWithError("socket Y failed");
    else
        write(0, "socket Y successfully created...\n", 33);
     
    bzero(&servAddrY, sizeof(servAddrY)); 
  
    // assign IP, PORT 
    servAddrY.sin_family = AF_INET; 
    servAddrY.sin_addr.s_addr = htonl(INADDR_ANY); 
    servAddrY.sin_port = htons(PORT_Y); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockY, (SA*)&servAddrY, sizeof(servAddrY))) != 0) { 
        dieWithError("socket Y bind failed ...");
    } 
    else
        write(0, "Socket Y successfully binded..\n", 31); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockY, QUEUELIMIT)) != 0) { 
        write(0, "Listen on port Y failed...\n", 27); 
        exit(0); 
    } 
    else
        write(0, "Server is listening on port Y...\n", 33); 


    int addrlen = sizeof(servAddrY);   
    write(0, "Waiting for connections on port Y ...\n", 38);   

    //set of socket descriptors for select  
    fd_set readfds;  
    int max_clients = 10;
    int max_sd, sd, activity, new_socket, valread;
    int client_socket[10];
    char* message = "Hello\r\n";
    char buffer[1025];  //data buffer of 1K 
    char fileContent[16384];
    for (int i = 0; i < max_clients; i++)   
        client_socket[i] = 0;   

    while(1)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add sockY to set  
        FD_SET(sockY, &readfds);   
        max_sd = sockY;   
             
        //add child sockets to set  
        for (int i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET(sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if (errno!=EINTR)   
            write(0, "select error", 12);
        if (activity < 0)
            continue;
             
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(sockY, &readfds))   
        {   
            if ((new_socket = accept(sockY,  
                    (struct sockaddr *)&servAddrY, (socklen_t*)&addrlen))<0)   
            {   
                write(0, "accept", 6);   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(servAddrY.sin_addr) , ntohs(servAddrY.sin_port));   
           
            //send new connection greeting message 
                 
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);    
                    break;   
                }   
            }   
        }   
        //else its some IO operation on some other socket 
        for (int i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message 
                valread = 0; 
                if ((valread = read( sd , buffer, 1024)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&servAddrY ,  (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  inet_ntoa(servAddrY.sin_addr) , ntohs(servAddrY.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_socket[i] = 0;   
                }     
                //Echo back the message that came in  
                else 
                {   
                    //set the string terminating NULL byte on the end  
                    //of the data read  
                    buffer[valread] = '\0';   
                    if (buffer[0] == DOWNLOAD[0]){
                        for (int i = 0;i < 1025; i++)
                            buffer[i] = '\0';
                        read(sd , buffer, 1024);
                        char* filePath = malloc((strlen("./_server/") + strlen(buffer)) + 1);
                        memcpy(filePath, "./_server/", strlen("./_server/") + 1);
                        memcpy(filePath + (strlen("./_server/")/sizeof(char)), buffer, strlen(buffer) + 1);
                        int fd = open(filePath, O_RDONLY);
                        if(fd < 0){
                            char empty[1];
                            empty[0] = NOTFOUND;
                            send(sd, empty, 1, 0);
                        }
                        else 
                        {
                            int readLen;
                            readLen = read(fd, fileContent, sizeof(fileContent));
                            fileContent[readLen] = '\0';
                            if(send(sd, fileContent, readLen + 1, 0) != readLen + 1)      
				                dieWithError("error while sending requested file...");
                            write(0, "requested file sent to client on port ", 38);
                            write(0, ntohs(servAddrY.sin_port), sizeof(ntohs(servAddrY.sin_port)));
                            write(0, "...\n", 4);
                        }                    
                    } else if (buffer[0] == UPLOAD[0]){
                        for (int i = 0;i < 1025; i++)
                            buffer[i] = '\0';
                        int readLen = read(sd , buffer, 1024);
                        char* filePath = malloc((strlen("./_server/") + strlen(buffer)) + 1);
                        memcpy(filePath, "./_server/", strlen("./_server/") + 1);
                        memcpy(filePath + (strlen("./_server/")/sizeof(char)), buffer, strlen(buffer) + 1);
                        write(0, "filepath: ", 10);
                        write(0, filePath, strlen(filePath));
                        write(0, "\n", 1);
                        int fd = open(filePath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                        int contentLen = read(sd , fileContent, sizeof(fileContent));
                        fileContent[contentLen] = '\0';
				        write(fd, fileContent, contentLen - 1);
				        close(fd);                    
                    }
                }
            }   
        }   
    }
  
    // After chatting close the socket 
    close(sockX);
    close(sockY);
}