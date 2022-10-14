#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <regex.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/time.h>
int CAP = 2000;
int CLIENTS = 5;


#define DEBUG
int main(int argc, char *argv[]){
  
    //Get ipv4 and port from arg TODO ADD IPV6 DETECTION PLEASE
    char delim[]=": ";
    char *Desthost=strtok(argv[1],delim);
    char *Destport=strtok(NULL,delim);
    int port=atoi(Destport);
    int serverfd;
    int clientfd[CLIENTS];
    memset(clientfd, '\0', sizeof(clientfd));
    char server_message[CAP], client_message[CAP];
	struct sockaddr_in clients[CLIENTS];
	int len, readSize;
	printf("Host %s, and port %d.\n",Desthost,port);
	//Create Socket
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd < 0){
		#ifdef DEBUG
		printf("Error creating server socket: %s\n", strerror(errno));
		#endif
		exit(-1);
	} else printf("Server socket created\n");
	
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
  	servaddr.sin_addr.s_addr = inet_addr(Desthost);
  	servaddr.sin_port = htons(port);
  	
  	//Bind socket
  	if((bind(serverfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0){
  		#ifdef DEBUG
  		printf("Socket bind failed: %s\n", strerror(errno));
  		#endif
  		exit(-1);
  	}
  	else printf("Socket successfully bound\n");

  	//Time for SELECT
  	int r;
  	int w;
  	fd_set reading;
  	fd_set writing;
  	fd_set ready_reading_sockets, ready_writing_sockets, current_sockets;
  	
  	FD_ZERO(&reading);
  	FD_ZERO(&writing);
  	FD_ZERO(&current_sockets);
  	FD_SET(serverfd, &reading);
  	FD_SET(serverfd, &writing);
  	
  	while(true){


  		struct timeval timeout;
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 1;
		timeout.tv_usec = 1;
  			
  		//Check reads
  		ready_reading_sockets = reading;
  		if(select(CLIENTS, &ready_reading_sockets, &ready_writing_sockets, NULL, &timeout) < 0){
  			//Error
  			printf("Select error: %s\n", strerror(errno));
  		}
  		//printf("r[i] == %d\n", r[i]);

		for(int i = 0;i<CLIENTS;i++){
			if(FD_ISSET(i, &ready_reading_sockets)){
				if(i == serverfd){
  					//Listen
					if((listen(serverfd, CLIENTS)) < 0){
						#ifdef DEBUG
						printf("Failed to listen: %s\n", strerror(errno));
						#endif
					} //else printf("Listening\n");			
	  				clientfd[i] = accept(serverfd, (struct sockaddr*)&clients[i],(socklen_t*)&len);
	  				FD_SET(clientfd[i], &ready_reading_sockets);
  					if(clientfd[i] < 0){
  						#ifdef DEBUG
  						printf("Server accept failed: %s\n", strerror(errno));
  						#endif
  					} else printf("Socket %d has accepted a connection\n", i);
  				}
  				else{
  				printf("Reading\n");
  				  	readSize = recv(clientfd[3], &client_message, sizeof(client_message), 0);
  					if(readSize > 0){
  						printf("Msg: %s", client_message);
  					}
  				
  					}
				//FD_CLR(i, &ready_reading_sockets);
			}

  			//printf("Hello?\n");
  			//Check for message
  			//if(FD_ISSET(i, &ready_writing_sockets)){


  			//}
  			
  		}
  		
  		
  		
  		
  		
  		
  		
  		
  	}
  	
}
