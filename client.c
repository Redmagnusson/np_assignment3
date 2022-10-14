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

bool testString(char* string, bool isNickname){
	char *expression="^[A-Za-z0-9_]+$";
    regex_t regularexpression;
    int reti;
  
    reti=regcomp(&regularexpression, expression,REG_EXTENDED);
    if(reti){
    	fprintf(stderr, "Could not compile regex.\n");
    	exit(1);
    }
	int matches;
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
  
    //Get ipv4 and port from arg TODO ADD IPV6 DETECTION PLEASE
    char delim[]=": ";
    char *Desthost=strtok(argv[1],delim);
    char *Destport=strtok(NULL,delim);
    char *nickname = argv[2];
    int port=atoi(Destport);
    
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
    } //else printf(server_message);
	
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
	int r, w, rc;
	fd_set reading, writing, ready_reading_sockets, ready_writing_sockets;
	FD_ZERO(&reading);
	FD_ZERO(&writing);
	FD_SET(socketDesc, &reading);
	FD_SET(socketDesc, &writing);
	
	while(true){
		memset(client_message, 0, CAP);
		
		//SELECT Setup
		ready_writing_sockets = writing;
		ready_reading_sockets = reading;
		struct timeval timeout;
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 2;
		timeout.tv_usec = 75000;
		w = select(2, NULL, &ready_writing_sockets, NULL, &timeout);
		r = select(2, &ready_reading_sockets, NULL, NULL, &timeout);
		
		
		//Check writes
		if(w >-1){
			
			gets(client_message);
			char* str = (char*)malloc(CAP);
			str = strdup(client_message);
			char* command = strtok(client_message, " ");
			char* text = strtok(NULL, "");
			if(strcmp(command, "NICK") == 0){
			
				//Dont forget to check if nickname is valid
				printf("Nickname: %s\n", text);
				if(testString(text, true)){
					
					sprintf(str, "%s %s\n", "NICK", text);
					nickname = strdup(text);
				}
				else{
					//printf("Error: Nickname invalid\n");
					continue;
				}
				if(send(socketDesc, str, strlen(str), 0) < 0){
					#ifdef DEBUG
					printf("Error sending message: %s\n", strerror(errno));
					#endif
				} //else printf("You sent:%s", str);
			}
			else if(strcmp(command, "CONNECT") == 0){
				//Close connection
				if(close(socketDesc) < 0){
					#ifdef DEBUG
					printf("Error closing socket: %s\n", strerror(errno));
					continue;
					#endif
				} else printf("Socket closed\n");
				
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
				printf("Nickname:%s", client_message);
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
				//else exit(1);
			}		
			else {
				//Send regular msg
				sprintf(str, "%s %s\n", "MSG", client_message);
				if(strlen(client_message) > 255){
					printf("Your message is too long. Max characters: 255\n");
				}
				else{
					if(send(socketDesc, str, strlen(str), 0) < 0){
					#ifdef DEBUG
					printf("Error sending message: %s\n", strerror(errno));
					#endif
					} //else printf("You sent:%s", str);
				}

			}
			
		}
		//Check reads
		if(r>-1){
			memset(server_message, 0, CAP);
			if(recv(socketDesc, server_message, sizeof(server_message), 0) < 0){
  				#ifdef DEBUG
  				printf("Error receiving message: %s\n", strerror(errno));
  				#endif
    		} 
    		else{
    			//printf(server_message);
				char* command = strtok(server_message, " ");
				char* name = strtok(NULL, " ");
				char* message = strtok(NULL, "");
				if(strcmp(name, nickname) != 0){
					printf("%s:%s\n", name, message);
				}

    		}
		}
	
	
	}
	exit(0);
	
	
	
	
	
	
}
