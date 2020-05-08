#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

void executeShell(int timeout);

void writeToStdout(char* text);

void alarmHandler(int sig);

void timeOutReset();

void sigintHandler(int sig);

void replaceNewLine(char** command_ptr);

void removeLeadTrailSpaces(char** command_ptr);

char* processCommand(char** command_ptr);

char* getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

int main(int argc, char** argv)
{
    int timeout = 0;
    registerSignalHandlers();

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
void killChildProcess()
{
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig)
{
    char catchPhrase[] = "Bwahaha ... tonight I dine on turtle soup\n";
    writeToStdout(catchPhrase);
    killChildProcess();
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig)
{
    if (childPid != 0) {
        killChildProcess();
    }
}

/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers()
{
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
void executeShell(int timeout)
{
    char* command;
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

        //If alarm signal received, send to alarmHandler
        if (signal(SIGALRM, alarmHandler) == SIG_ERR) {
            perror("Error in time out");
            exit(EXIT_FAILURE);
        }

        //set alarm
        alarm(timeout);

        //Code run by child process
        if (childPid == 0) {
            char* const envVariables[] = { NULL };
            char* const args[] = { command, NULL };
            
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
    //reset any pending alarms
    alarm(0); 
    free(command);
}

/* Writes particular text to standard output */
void writeToStdout(char* text)
{
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Finds '\n' and replaces with null-terminator */
void replaceNewLine(char** command_ptr)
{
    //ascii code for new line
    int asciiNewLine = 10;
    char* newLinePtr = strrchr((const char*) *command_ptr, asciiNewLine);
    *newLinePtr = '\0';
}

/* Removes leading and trailing spaces in the command string */
void removeLeadTrailSpaces(char** command_ptr)
{
    //Ascii code for space
    int asciiSpace = 32;
    //Advance command_ptr to remove leading spaces
    while (**command_ptr == asciiSpace) {
        (*command_ptr)++;
    }

    //Find last occurence of space, set it to null-terminator
    char* firstTrailSpace = index((*command_ptr), 32);
    if (firstTrailSpace != NULL) {
        *firstTrailSpace = '\0';
    }
}

/* 'Cleans' the command string receieved from the user input */
char* processCommand(char** command_ptr)
{
    char* cleanCommand = calloc(INPUT_SIZE, sizeof(char));
    replaceNewLine(command_ptr);
    removeLeadTrailSpaces(command_ptr);
    //Copy processed string to calloc'd memory
    stpcpy(cleanCommand, *command_ptr);
    return cleanCommand;
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char* getCommandFromInput()
{
    int returnedEOF = 0;
    char command[INPUT_SIZE] = {0};
    char* command_ptr = command;
    int ret = read(STDIN_FILENO, command, INPUT_SIZE);

    //Exit if CTRL+D
    if (ret == returnedEOF) {
        char lineBreak = '\n';
        writeToStdout(&lineBreak);
        exit(EXIT_SUCCESS);
    }

    command_ptr = processCommand(&command_ptr);

    //Repeat loop if no command is typed
    if (strlen(command_ptr) <= 1) {
        return NULL;
    }

    return command_ptr;
}
