#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

void executeShell(int timeout);

void writeToStdout(char *text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char *getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

int main(int argc, char **argv) {
    registerSignalHandlers();

    int timeout = 0;
    if (argc == 2) {
        timeout = atoi(argv[1]);
    }

    if (timeout < 0) {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1) {
        executeShell(timeout);
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

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig) {
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}


/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout) {
    char *command;
    int status;
    char minishell[] = "penn-shredder# ";
    writeToStdout(minishell);

    command = getCommandFromInput();

    if (command != NULL) {
        childPid = fork();

        if (childPid < 0) {
            perror("Error in creating child process");
            exit(EXIT_FAILURE);
        }

        //Code run by child process
        if (childPid == 0) {
            char *const envVariables[] = {NULL};
            char *const args[] = {command, NULL};
            if (execve(command, args, envVariables) == -1) {
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
    } 
    free(command);
}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}


void replaceNewLine(char** command_ptr) {
    //find \n and replace with null-term
    char* newLinePtr = strrchr((const char*) *command_ptr,10);
    *newLinePtr = '\0';  
}


void removeLeadTrailSpaces(char** command_ptr) {
    //remove leading spaces
    while(**command_ptr == 32) {
        (*command_ptr) ++;
    }

    //remove trailing spaces
    char* firstTrailSpace = index((*command_ptr),32);
    if (firstTrailSpace != NULL) {
        *firstTrailSpace = '\0';
    } 
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char *getCommandFromInput() {
    int charLen = 1023;
    char command[charLen];
    char* command_ptr = command;
    char* cleanCommand = calloc(charLen, sizeof(char));
    int ret = read(STDIN_FILENO, command, charLen);
    
    //exit if ctrl+d
    if (ret == 0) {
        char lineBreak = '\n';
        writeToStdout(&lineBreak);
        free(cleanCommand);
        exit(EXIT_SUCCESS);
    }

    replaceNewLine(&command_ptr);
    removeLeadTrailSpaces(&command_ptr);
    
    //copy cleanCommand
    stpcpy(cleanCommand, command_ptr);
    
    //repeat loop if no command is typed
    if (strlen(cleanCommand) <= 1) {
        return NULL;
    }

    return cleanCommand;
}


