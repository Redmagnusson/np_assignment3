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
#include <sys/time.h>'

int CAP = 2000;
int MAXCLIENTS = 10;
bool testString(char* string, bool isNickname){
	char *expression="^[A-Za-z0-9_]+$";
    regex_t regularexpression;
    int reti;
  
    reti=regcomp(&regularexpression, expression,REG_EXTENDED);
    if(reti){
    	fprintf(stderr, "Could not compile regex.\n");
    	exit(1);
    }
	int matches = 0;
    regmatch_t items;
  

	if(isNickname){
		if(strlen(string) > 12){
		    printf("%s is too long (%d vs 12 chars).\n", string, strlen(string));
			return false;
		}
	}

    reti=regexec(&regularexpression, string, matches, &items,0);
    if(!reti){
		printf("Nickname %s is accepted.\n",string);
		return true;

    } else {
		printf("%s is not accepted.\n",string);
		return false;
    }
    regfree(&regularexpression);
}
#define DEBUG
int main(int argc, char *argv[]){

    //Get ipv4 /ipv6 and port from arg
    char* splits[CAP];
    char* p = strtok(argv[1], ":");
    int counter = 0;
    while(p != NULL){
    	splits[counter++] = p;
    	p = strtok(NULL, ":");
    }
    char delim[]=": ";
    char *Desthost;//=strtok(argv[1],delim);
    char *Destport;//=strtok(NULL,delim);
    Destport = splits[--counter];
    Desthost = splits[0];
    for(int i = 1;i<counter;i++){
    	sprintf(Desthost, "%s:%s",Desthost, splits[i]);
    }
    int port=atoi(Destport);
    printf("Host %s, and port %d.\n",Desthost,port);
    int serverfd;
    int clientfd;
	int nrOfClients = 0;
	char* nicknames[MAXCLIENTS];
    char server_message[CAP], client_message[CAP];
    int bytesRecv;
	struct sockaddr_in client;
	int len, readSize;
	
	for(int i = 0;i<MAXCLIENTS;i++){
		nicknames[i] = "";
	}
	
	
	struct addrinfo hints, *serverinfo = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//Getaddrinfo
	if(getaddrinfo(Desthost, Destport, &hints, &serverinfo) < 0){
		printf("getaddrinfo error: %s\n", strerror(errno));
	}
	//Create Socket
	serverfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, 0);
	if(serverfd < 0){
		#ifdef DEBUG
		printf("Error creating server socket: %s\n", strerror(errno));
		#endif
		exit(-1);
	} else printf("Server socket created\n");
	

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	if(strcmp(Desthost, "0.0.0.0") == 0){
		servaddr.sin_addr.s_addr = INADDR_ANY;
	}
	else servaddr.sin_addr.s_addr = inet_addr(Desthost);
  	servaddr.sin_port = htons(port);
  	
  	//Bind socket
  	if((bind(serverfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0){
  		#ifdef DEBUG
  		printf("Socket bind failed: %s\n", strerror(errno));
  		#endif
  		exit(-1);
  	}
  	else printf("Socket successfully bound\n");
	
	//Listen
	if((listen(serverfd, MAXCLIENTS)) < 0){
		#ifdef DEBUG
		printf("Failed to listen: %s\n", strerror(errno));
		#endif
	} //else printf("Listening\n");	
	
  	//Time for SELECT
	fd_set master;
	FD_ZERO(&master);
	FD_SET(serverfd, &master);
  	int fdmax = serverfd;
  	
  	while(true){

		fd_set copy = master;
		
		if(select(fdmax +1, &copy, NULL, NULL, NULL) == -1){
			//Select failed
		}
		
		for(int i = 0;i<fdmax+1;i++){
			if(FD_ISSET(i, &copy)){
				if(i == serverfd){
					clientfd = accept(serverfd, (struct sockaddr*)&client, (socklen_t*)&len);
					if(clientfd < 0){
						printf("Error accepting client: %s\n", strerror(errno));
					}
					else{
						FD_SET(clientfd, &master);
						//nrOfClients++;
						if(clientfd > fdmax){
							fdmax = clientfd;
						}
						if(send(clientfd, "HELLO 1\n", strlen("HELLO 1\n"), 0) < 0){
							printf("Error sending message: %s\n", strerror(errno));
						}
					}
				}
				else{
					//Read msg
					memset(client_message, 0, CAP);
					bytesRecv = recv(i, client_message, CAP, 0);
					printf("INCOMING: %s\n", client_message);
					if(bytesRecv == 0){
						//Terminate client
						printf("Terminating: %s\n", nicknames[i]);
						//Idk how to handle the lists but maybe i reorder array?
						nicknames[i] = "";
						FD_CLR(i, &master);
					}
					
					else if(bytesRecv > 0){
					
					char* str = (char*)malloc(bytesRecv);
					str = strdup(client_message);
					char* command = strtok(client_message, " ");
					 char* text = strtok(NULL, "\n");
					
					//Check Msg length
					if(sizeof(text) > 255){
						//MSG too long, deny sending it.
						if(send(i, "ERR\n", strlen("ERR\n"), 0) < 0){
							printf("Error sending message: %s\n", strerror(errno));
						}
					}
					//Check for NICK
					else if(strcmp(command, "NICK") == 0){
					   
					    
					  //Check if nickname is occupied?
					  bool nickExists = false;
					  for(int i = 0;i<MAXCLIENTS;i++){
					  	if(strcmp(nicknames[i], text) == 0){
					  		nickExists = true;
					  	}
					  }
					  
						//Check if nickname is valid
						if(testString(text, true) && nickExists == false){
							nicknames[i] = strdup(text);
							//printf("Did we beat the regex?\n");
							if(send(i, "OK\n", strlen("OK\n"), 0) < 0){
								printf("Error sending message: %s\n", strerror(errno));
							}
						} else{
							if(send(i, "ERR\n", strlen("ERR\n"), 0) < 0){
								printf("Error sending message: %s\n", strerror(errno));
							}
						}
					}
					
					//Check for MSG
					else if(strcmp(command, "MSG") == 0){
					printf("Did we get here? MSG\n");
						//Echo to all clients
						//char* text = strtok(NULL, "\n");
						sprintf(server_message, "%s %s %s\n", command, nicknames[i], text);
						for(int j = 0;j<fdmax+1;j++){
						printf("Size: %d\n", fdmax);
							if(FD_ISSET(j, &master)){
								if(j != serverfd){ /*serverfd*/
								printf("Msg: %s", server_message);
									if(send(j, server_message, strlen(server_message), 0) < 0){
										printf("Error sending message: %s\n", strerror(errno));
									} else printf("Sent message to: %d\n", j);
								}
							}
						}
						
					}
					}
					
				}
			}
		}






  	
  			
  	}
  	
}
