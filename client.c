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
#include <string.h>
int CAP = 2000;

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
		    regfree(&regularexpression);
			return false;
		}
	}
    reti=regexec(&regularexpression, string, matches, &items,0);
    if(reti){
    	fprintf(stderr, "Error running regexec\n");
    }
    //printf("Passes regexec\n");
    if(!reti){
		printf("Nickname %s is accepted.\n",string);
		regfree(&regularexpression);
		return true;

    } else {
		printf("%s is not accepted.\n",string);
		regfree(&regularexpression);
		return false;
    }
 
    regfree(&regularexpression);
    printf("This is REALLY bad\n");
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
    char *nickname = argv[2];
    char *tempnick;
    int port=atoi(Destport);
    printf("Host: %s, Port: %d, Nickname: %s\n", Desthost, port, nickname);
    
    char server_message[CAP], client_message[CAP];
    struct sockaddr_in servAddr;

	//Check if nickname is valid.
	if(!testString(nickname, true)){
		exit(1);
	}
	
	//getaddrinfo
	struct addrinfo hints;
	struct addrinfo *servinfo = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(Desthost, Destport, &hints, &servinfo) < 0){
		printf("getaddrinfo error: %s\n", strerror(errno));
	}
	
	//Create TCP socket
	int socketDesc;
	struct sockaddr_in serverAddr;
	socketDesc = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
	if(socketDesc < 0){
		#ifdef DEBUG
		printf("Error creating socket: %s\n", strerror(errno));
		#endif
	} //else printf("Socket Created\n");
	
	//Create Socket Structure
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = inet_addr(Desthost);
	
	//Establish Connection, with getaddrinfo :)
	if(connect(socketDesc, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
		#ifdef DEBUG
		printf("Error establishing the connection: %s\n", strerror(errno));
		#endif
	} else printf("Connection Established\n");
	
	//Recieve Hello from Server
	memset(server_message, 0, CAP);
	if(recv(socketDesc, server_message, sizeof(server_message), 0) < 0){
  		#ifdef DEBUG
  		printf("Error receiving message: %s\n", strerror(errno));
  		#endif
    } else printf(server_message);
	
	//Send Nickname to Server
	memset(client_message, 0, CAP);
	sprintf(client_message, "%s %s\n", "NICK", nickname);
	if(send(socketDesc, client_message, strlen(client_message), 0) < 0){
		#ifdef DEBUG
		printf("Error sending message: %s\n", strerror(errno));
		#endif
	}
	
	//Recieve Response from Server
	memset(server_message, 0, CAP);
	if(recv(socketDesc, server_message, sizeof(server_message), 0) < 0){
  		#ifdef DEBUG
  		printf("Error receiving message: %s\n", strerror(errno));
  		#endif
    } //else printf(server_message);
	
	//Check if OK
	if(strcmp(server_message, "OK\n") == 0){
		printf("You're now connected to the chat server\n");
	}
	else exit(1);
	

	//Start the SELECT for recv and send/input
	int r, w, rc = socketDesc;
	fd_set reading, writing, ready_reading_sockets, ready_writing_sockets, master;
	FD_ZERO(&master);
	FD_ZERO(&reading);
	FD_SET(0, &master);
	FD_SET(socketDesc, &master);
	
	while(true){
	
		memset(client_message, 0, CAP);
		ready_reading_sockets = master;
		
		//SELECT Setup
		struct timeval timeout;
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		select(rc + 1, &ready_reading_sockets, NULL, NULL, NULL);

		if(FD_ISSET(0, &ready_reading_sockets)){
			
			//Retrieve client message from terminal
			scanf("%[^\n]%*c", client_message);
			
			char* str = (char*)malloc(CAP);
			str = strdup(client_message);
			char* command = strtok(client_message, " ");
			
			//Check if NICKNAME command
			if(strcmp(command, "NICK") == 0){
				char* text = strtok(NULL, "");
				//Dont forget to check if nickname is valid
				if(testString(text, true)){
					
					printf("Nick was valid\n");
					//sprintf(str, "%s %s\n", "NICK", text);
					//nickname = strdup(text);
					tempnick = strdup(text);
				}
				else{
					printf("Error: Nickname invalid\n");
					continue;
				}
				if(send(socketDesc, str, strlen(str), 0) < 0){
					#ifdef DEBUG
					printf("Error sending message: %s\n", strerror(errno));
					#endif
				} 
			}
			
			//Check if CONNECT command
			else if(strcmp(command, "CONNECT") == 0){
				//Close connection
				if(close(socketDesc) < 0){
					#ifdef DEBUG
					printf("Error closing socket: %s\n", strerror(errno));
					continue;
					#endif
				} else printf("Socket closed\n");
				char* text = strtok(NULL, "");
				//Get new info
				Desthost=strtok(text, ":");
   				Destport=strtok(NULL," ");
   				nickname = strtok(NULL, "");
   				nickname = strdup(nickname);
   				port=atoi(Destport);
   				
   				//Getaddrinfo
				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;
				if(getaddrinfo(Desthost, Destport, &hints, &servinfo) < 0){
					printf("getaddrinfo error: %s\n", strerror(errno));
				} else printf("Got addrinfo\n");
				
				socketDesc = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
				if(socketDesc < 0){
					#ifdef DEBUG
					printf("Error creating socket: %s\n", strerror(errno));
					#endif
				} //else printf("Socket Created\n");
				
				//Connect
				printf("Host: %s, Port: %d, Nickname: %s\n", Desthost, port, nickname);
				if(connect(socketDesc, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
					#ifdef DEBUG
					printf("Error establishing the connection: %s\n", strerror(errno));
					#endif
				} else printf("Connection Established\n");
				
				//Recieve Hello from Server
				memset(server_message, 0, CAP);
				if(recv(socketDesc, server_message, sizeof(server_message), 0) < 0){
  					#ifdef DEBUG
  					printf("Error receiving message: %s\n", strerror(errno));
  					#endif
   				} else printf(server_message);
	
				//Send Nickname to Server
				printf("Host: %s, Port: %d, Nickname: %s\n", Desthost, port, nickname);
				//memset(client_message, 0, CAP);
				sprintf(client_message, "%s %s\n", "NICK", nickname);
				//printf("Nickname:%s", nickname);
				if(send(socketDesc, client_message, strlen(client_message), 0) < 0){
					#ifdef DEBUG
					printf("Error sending message: %s\n", strerror(errno));
					#endif
				}
	
				//Recieve Response from Server
				memset(server_message, 0, CAP);
				if(recv(socketDesc, server_message, sizeof(server_message), 0) < 0){
  					#ifdef DEBUG
  					printf("Error receiving message: %s\n", strerror(errno));
  					#endif
    			} //else printf(server_message);
	
				//Check if OK
				if(strcmp(server_message, "OK\n") == 0){
					printf("You're now connected to the chat server\n");
				}
				else exit(1);
			}		
			
			//If no command, just send regular message
			else {
				
				char* sendStr = (char*)malloc(CAP);
				sprintf(sendStr, "%s %s\n", "MSG", str);
				if(strlen(sendStr) > 255){
					printf("Your message is too long. Max characters: 255\n");
				}
				else{
					if(send(socketDesc, sendStr, strlen(sendStr), 0) < 0){
						#ifdef DEBUG
						printf("Error sending message: %s\n", strerror(errno));
						#endif
					} //else printf("You sent:%s", str);
				}

			}
			
		}
		//Check reads
		if(FD_ISSET(rc, &ready_reading_sockets)){
			memset(server_message, 0, CAP);
			int n = recv(socketDesc, server_message, sizeof(server_message), 0);
			
			if(n < 0){
  				#ifdef DEBUG
  				printf("Error receiving message: %s\n", strerror(errno));
  				#endif
    	}
    	else if(n > 0){
    		if(strcmp(server_message, "OK\n") == 0){
    			printf("Nickname OK from server\n");
    			//Apply new nickname here?
    			nickname = tempnick;
    			//continue;
    		}
    		else if(strcmp(server_message, "ERR\n") == 0){
 					//Error recv
    			printf("Server denied your request\n");
    		}
    		else{
					char* command = strtok(server_message, " ");
					char* name = strtok(NULL, " ");
					char* message = strtok(NULL, "\n");
					if(strcmp(name, nickname) != 0){
						printf("%s: %s\n", name, message);
					}
				}
    	}
    			else if(n == 0){
    				//Drop connection
    				printf("Connection to server has been lost. Exiting...\n");
    				exit(0);
    			}
		}
	
		//printf("Did we loop?\n");
	}
	exit(0);
	
	
	
	
	
	
}
