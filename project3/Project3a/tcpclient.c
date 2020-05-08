#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 20

// int int extractSequence(char* buf);
// void extractXandIncrement(char* numString, char* buf);

int main (int argc, char *argv[]) {
  char* host_addr = argv[1];
  int port = atoi(argv[2]);
  char buf[MAX_LINE];
  int x = atoi(argv[3]);
  strcpy(buf, "HELLO "); 
  strcat(buf, argv[3]);

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0){
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

  /* Connect to the server */
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0){
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }

  /*main loop: get and send lines of text */ 
  int handShake = 0;
  char zString[MAX_LINE];
  
  while (1) {
    //Second send for Z
    if (handShake != 0) {
      bzero(buf, MAX_LINE);
      strcpy(buf, "HELLO "); 
      strcat(buf, zString);
      send(s, buf, strlen(buf), 0);
      break;
    } 

    send(s, buf, strlen(buf), 0);  //first send
    bzero(buf, MAX_LINE);          
    recv(s, buf, sizeof(buf), 0);

    int y = atoi(&(buf[6]));
    if (y != x + 1) {              
      printf("ERROR\n");
      break;
    } else {
      printf("%s\n", buf);
      sprintf(zString, "%d", y + 1);
    }
    handShake++;
  }
  
  close(s);
  return 0;

}
