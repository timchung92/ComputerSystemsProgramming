#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LINE 1024
#define NUM_REQUESTS 100

void addSocketsToReadFd(fd_set* read_fds_ptr, int sockets[], int max_fd_i) {
    int closedStatus = 0;
    for(int i = 0; i <= max_fd_i; i++)
    {
        if(sockets[i] == closedStatus)
            continue;
        fcntl(sockets[i], F_SETFL, O_NONBLOCK); // changes socket to non-blocking
        FD_SET(sockets[i], read_fds_ptr);  // add socket at i to read file descriptors set
    }
}

void printBuffer(char* buf) {
    buf[strlen(buf)] = '\0';
    printf("%s\n", buf);
}

void constructHelloString(char* buf, char* yStr, int X) {
    sprintf(yStr, "%d", X + 1);
    strcpy(buf, "HELLO "); 
    strcat(buf, yStr);
}

void closeClearSocket(int sockets[], int client_state[], int i, fd_set* read_fds) {
    close(sockets[i]);
    FD_CLR(sockets[i], read_fds);
    sockets[i] = 0;
    client_state[i] = 0;
}


int main(int argc, char* argv[]) 
{
    char* host_addr = "127.0.0.1";
    int port = atoi(argv[1]);
    int sockets[NUM_REQUESTS];
    char buf[MAX_LINE];
    int listen_sock;
    int max_fd;
    int max_fd_i = 0;
    struct sockaddr_in sin;
    int client_state[NUM_REQUESTS];
    int firstHandShake = -1;
    int closedStatus = 0;

    // createListeningSocket(&listen_sock, host_addr, port, &sin);
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) <0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    /* Config the server address */
    sin.sin_family = AF_INET; 
    sin.sin_addr.s_addr = inet_addr(host_addr);
    sin.sin_port = htons(port);
    // Set all bits of the padding field to 0
    memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

    /* Bind the socket to the address */
    if((bind(listen_sock, (struct sockaddr*)&sin, sizeof(sin)))<0) {
        perror("simplex-talk: bind");
        exit(1);
    }
    listen(listen_sock, NUM_REQUESTS);
    sockets[max_fd_i] = listen_sock;
    max_fd = listen_sock;
    

    while(1)
    {
	fd_set read_fds; // read file descriptors. Need to be initialized everytime we call p/select
	FD_ZERO(&read_fds); //removes all read file descriptors
        bzero(buf, MAX_LINE);
	
        addSocketsToReadFd(&read_fds, sockets, max_fd_i);

	int res_select = pselect(max_fd+1, &read_fds, NULL, NULL, NULL, NULL); // number of read file descriptors ready to be read. Removes any that are not
	if(res_select < 0) 
	{
		perror("pselect");
		exit(1);
	}
	else if(res_select == 0)
		continue;
		
        for(int i = 0; i <= max_fd_i; i++) {         
		if(FD_ISSET(sockets[i], &read_fds))  {  // Is fd ready to be read. I.e. is it still in read_fds?
                int new_s;
                int bytes;
                socklen_t len = sizeof(sin);
		if (sockets[i] == listen_sock) { // accept new client request
		    if((new_s = accept(listen_sock, (struct sockaddr *)&sin, &len)) <0) {
			perror("simplex-talk: accept");
			exit(1);
		    }
                    max_fd_i++;
                    max_fd = new_s;  // new socket is now new max_fd
                    sockets[max_fd_i] = new_s;  // add new socket to next avail pos
                    client_state[max_fd_i] = firstHandShake;  // update client state

                } else if (client_state[i] == firstHandShake) {      //handle first client handshake 
                    if ((bytes = recv(sockets[i], buf, sizeof buf, 0)) > 0) {
                        printBuffer(buf);
                        int X = atoi(buf + 6);
                        char yStr[MAX_LINE];
                        constructHelloString(buf, yStr, X);
                        send(sockets[i], buf, bytes, 0);
                        client_state[i] = X + 1;  // update client state with Y value
                    }

                } else if (client_state[i] > firstHandShake && client_state[i] != closedStatus) {  // handle third client handshake
                    if ((bytes = recv(sockets[i], buf, sizeof buf, 0)) > 0) {
                        int Z = atoi(buf + 6);
                        if (Z != client_state[i] + 1) {
                            perror("ERROR: Wrong Z");
                            closeClearSocket(sockets, client_state, i, &read_fds);
                        }
                        printBuffer(buf);
                        closeClearSocket(sockets, client_state, i, &read_fds);
                    }
		}
			}
		}
	}
}




