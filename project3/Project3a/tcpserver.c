#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 10
#define MAX_LINE 20


int main(int argc, char *argv[]) {
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);

  /*setup passive open*/
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0) {
    perror("simplex-talk: socket");
    exit(1);
  }

  /* Config the server address */
  struct sockaddr_in sin;
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = inet_addr(host_addr);
  sin.sin_port = htons(port);
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Bind the socket to the address */
  if((bind(s, (struct sockaddr*)&sin, sizeof(sin)))<0) {
    perror("simplex-talk: bind");
    exit(1);
  }

  // connections can be pending if many concurrent client requests
  listen(s, MAX_PENDING);  

  /* wait for connection, then receive and print text */
  int new_s;
  socklen_t len = sizeof(sin);
  char buf[MAX_LINE];
  char yString[MAX_LINE];
  int handShake = 0;
  int y;

  while(1) {
    if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
      perror("simplex-talk: accept");
      exit(1);
    }
    bzero(buf, MAX_LINE);
    handShake = 0;
    while(len = recv(new_s, buf, sizeof(buf), 0)) {      
        if (handShake != 0) {
          int z = atoi(&(buf[6]));
          if (z != y + 1) {
              printf("ERROR\n");
              close(new_s);
              break;
          } else {
            buf[len] = '\0';
            printf("%s\n", buf);
            break;
          }
        }
        
        buf[len] = '\0';
        printf("%s\n", buf);

        y = atoi(&(buf[6])) + 1;
        sprintf(yString, "%d", y);
        strcpy(buf, "HELLO "); 
        strcat(buf, yString);
        send(new_s, buf, strlen(buf), 0);

        handShake++;
      }
    close(new_s);
  }
  return 0;
}

