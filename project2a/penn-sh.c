#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tokenizer.h"
#include <sys/wait.h>
#include <signal.h>
#include<fcntl.h>

pid_t childPid = 0;

void executeShell();
void writeToStdout(char* text);
void sigintHandler(int sig);
char** getCommandFromInput();
void registerSignalHandlers();
void killChildProcess();
int countPipes (char** args);
int getTokCount(char** args);
void printPennShell();
int handleRedirects(char** args);
void freeToks(char** args);

/**
 * Main program execution
 */
int main( int argc, char *argv[] ) {
  registerSignalHandlers();
  while (1) {
    executeShell();
  }
  return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}
/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}

void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

void printPennShell() {
    char minishell[] = "penn-sh> ";
    writeToStdout(minishell);
}

void executeShell() {
    int status;
    childPid = 0;
    char** args = getCommandFromInput();
    
    if (args == NULL) return;

    // for (int i = 0; i < countPipes(args); i++) {
    //     int fd[2 * ]
    // }

    childPid = fork();

    if (childPid < 0) {
        perror("Error in creating child process");
        exit(EXIT_FAILURE);
    }

    //Code run by child process
    if (childPid == 0) {
        
        if (handleRedirects(args) < 0) {
            exit(EXIT_FAILURE);
        }

        if (execvp(args[0], args) == -1) {
            perror("Error in execve");
            exit(EXIT_FAILURE);
        }

    //Code run by parent process
    } else {
        do {
            if (wait(&status) == -1) {
                perror("Error in child process termination");
                exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    freeToks(args);
}

int handleRedirects(char** args) {
    int newStdOut, newStdIn;
    int argsCount;
    if ((argsCount = getTokCount(args)) < 0 ) {
        writeToStdout("Invalid redirection\n");
        return -1;
    }

    for (int i = 0; i < argsCount; i ++) {
        if (*args[i] == '>' ) {
            if ((newStdOut = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
                perror("Invailid output redirect");
                return -1;
            }
            dup2(newStdOut, STDOUT_FILENO);
            close(newStdOut);
            args[i] = NULL;
        } else if (*args[i] == '<' ) {
            if ((newStdIn = open(args[i+1], O_RDONLY)) < 0 ) {
                perror("Invalid input redirect");
                return -1;
            }
            dup2(newStdIn, STDIN_FILENO);
            close(newStdIn);
            args[i] = NULL;
            }
        }
    return 1;
}



/* Count number of tokens and check for proper
 * number of redirects */
int getTokCount(char** args) {
    int count = 0, outputRedirects = 0, inputRedirects = 0;
    char* file1;
    char* file2;
    while (args[count] != NULL) {
        if (*args[count] == '<' ) {
            if (inputRedirects == 0) {
                file1 = args[count + 1];
            } else {
                file2 = args[count + 1];
            }
            inputRedirects++;
        }
        if (*args[count] == '>') {
            if (outputRedirects == 0) {
                file1 = args[count + 1];
            } else {
                file2 = args[count + 1];
            }
            outputRedirects++;
        }
        count++;
    }

    //Check for 2 of the same redirects
    if (inputRedirects > 1 || outputRedirects > 1) {
        if (strcmp(file1, file2) != 0) {  //allow if same file names
            return -1;
        }
    }
    return count;
}

int countPipes(char** args) {
    int count = 0;
    int i = 0;
    while (args[i] != NULL) {
        if (*args[i] == '|') {
            count ++;
        }
        i++;
    }
    return count;
}

char** getCommandFromInput() {
  TOKENIZER *tokenizer;
  char string[256] = "";
  int br;
  int argsCnt = 0;

  printPennShell();
  
  string[255] = '\0';   /* ensure that string is always null-terminated */
  while ((br = read( STDIN_FILENO, string, 255 )) > 0) {
    if (br <= 1) {
       return NULL;
    }
    string[br-1] = '\0';   /* remove trailing \n */
    tokenizer = init_tokenizer( string );
    char** args = calloc(10, sizeof(char*));
    char* tok;
    while((tok = get_next_token( tokenizer )) != NULL ) {
        args[argsCnt] = tok;
        argsCnt++;
    }
    args[argsCnt] = NULL;
    free_tokenizer( tokenizer );
    return args;
  }

  //Exit if CTRL+D
  writeToStdout("\n");
  exit(EXIT_SUCCESS);
  
  return NULL;
}

void writeToStdout(char* text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

void freeToks(char** args) {
    for (int i = 0; i < 10; i++) {
        free(args[i]);
    }
    free(args);
}