#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"

#define BUFLEN 1024


int runPipes(char *args[], char *pipeArgs[]) {
    int pipe_fd[2]; // File descriptors for the pipe
    pid_t child_pid1, child_pid2;

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the first child process to execute the first command (e.g., "ls")
    child_pid1 = fork();

    if (child_pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid1 == 0) {
        // Child process 1
        printf("Child Process 1 (PID: %d) executing 'ls'\n", getpid());
        close(pipe_fd[0]); // Close the read end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
        close(pipe_fd[1]); // Close the write end of the pipe
        executeExecutable(args[0], args);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        // Fork the second child process to execute the second command (e.g., "grep")
        child_pid2 = fork();

        if (child_pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid2 == 0) {
            // Child process 2
            printf("Child Process 2 (PID: %d) executing 'grep'\n", getpid());
            close(pipe_fd[1]); // Close the write end of the pipe
            dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
            close(pipe_fd[0]); // Close the read end of the pipe
            executeExecutable(pipeArgs[0], pipeArgs);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            close(pipe_fd[0]); // Close the read end of the pipe
            close(pipe_fd[1]); // Close the write end of the pipe

            // Wait for the second child to finish
            wait(NULL);
            printf("Both child processes have completed.\n");
        }
    }

    return 0;
}
//To Do: This base file has been provided to help you start the lab, you'll need to heavily modify it to implement all of the features

int main() {
    int pipe_fd[2]; 
    char buffer[1024];
    char* parsedinput;
    char* args[BUFLEN];
    char*pipeArgs[BUFLEN];
    char newline;

    printf("Welcome to the Group08 shell! Enter commands, enter 'quit' to exit\n");
    do {
        //Print the terminal prompt and get input
        printf("$ ");
        char* input = fgets(buffer, sizeof(buffer), stdin);
        if(!input)
        {
            fprintf(stderr, "Error reading input\n");
            return -1;
        }

        //Clean the input and put it into instructions
        parsedinput =  (char*) malloc(BUFLEN * sizeof(char));
        size_t parselength = trimstring(parsedinput, input, BUFLEN);
        tokenizeBothCommands(parsedinput, args, pipeArgs);

        //Sample shell logic implementation
        if ( strcmp(parsedinput, "quit") == 0 ) {
            printf("Bye!!\n");
            return 0;
        }

        // If needs to be piped will pipe
        if (pipeArgs[0] != NULL) {
            runPipes(args, pipeArgs);
            continue;
        }

        // Single exec if not piped 
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");
            exit(1);
        }

        if (child_pid == 0) {
            // This is the first child process
            // Redirect standard output to the write end of the pipe
            return executeExecutable(args[0], args);
        } else {
            waitpid(child_pid, NULL, 0);
    }

        //Remember to free any memory you allocate!
        if (parsedinput != NULL) {
            free(parsedinput);
        }

    } while ( 1 );

    return 0;
}