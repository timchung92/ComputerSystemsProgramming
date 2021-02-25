#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 10
#define MAX_LINE 20

struct threadArgs {
  int new_s;
  char* buf;
  int* handShake;
};

void* threadListenServer(void* args);
void firstHandShake(char* buf, int new_s, char* yString, int* y, int* handShake);
void constructErrorString(char* buf, char* yString, int* y);
void packageArguments(struct threadArgs* args, int new_s, char* buf, int* handShake);
void printBuffer(char* buf);
int getYZ(char* buf);
void errorZ(int new_s);

int main(int argc, char *argv[]) {
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);
  pthread_t tids[MAX_PENDING];
  int i = 0;

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

  while(1) {
      bzero(buf, MAX_LINE);
      if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
        perror("simplex-talk: accept");
        exit(1);
      }
      int handShake = 0;
      struct threadArgs* args = malloc(sizeof(struct threadArgs));;
      packageArguments(args, new_s, buf, &handShake);
      pthread_create(&tids[i], NULL, threadListenServer, (void*)args);
      pthread_join(tids[i], NULL); i++; 
  }
  return 0;
}

void packageArguments(struct threadArgs* args, int new_s, char* buf, int* handShake) {
    args->buf = buf;
    args->new_s = new_s;
    args->handShake = handShake;
}

void* threadListenServer(void* args) {
  struct threadArgs* args2 = (struct threadArgs*)args;
  char yString[MAX_LINE];
  int y; 

  while(recv(args2->new_s, args2->buf, MAX_LINE, 0)) { 
    if (*args2->handShake == 0) {
      firstHandShake(args2->buf, args2->new_s, yString, &y, args2->handShake);
    } else {                             
        if (getYZ(args2->buf) != (y + 1)) {  
            errorZ(args2->new_s);
            close(args2->new_s);
            free(args);
            pthread_exit(NULL);
      } else {
            printBuffer(args2->buf);
            close(args2->new_s);
            free(args);
            pthread_exit(NULL);
      }
    }
  }
}

void firstHandShake(char* buf, int new_s, char* yString, int* y, int* handShake) {
    *y = getYZ(buf) + 1;
    printBuffer(buf);
    constructErrorString(buf, yString, y);
    send(new_s, buf, strlen(buf), 0);
    *handShake += 1;
}

void errorZ(int new_s) {
    printf("ERROR\n");
    close(new_s);
}

void constructErrorString(char* buf, char* yString, int* y) {
    sprintf(yString, "%d", *y);
    strcpy(buf, "HELLO "); 
    strcat(buf, yString);
}

void printBuffer(char* buf) {
    buf[strlen(buf)] = '\0';
    printf("%s\n", buf);
}

int getYZ(char* buf) {
  return atoi(buf + 6);
}



