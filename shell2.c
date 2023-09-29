#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 100

void execute_command(char **command, int input_fd, int output_fd) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(command[0], command);
        perror("execvp");
        exit(1); // Exit child process on failure
    } else {
        // Parent process
        wait(NULL); // Wait for the child process to finish
    }
}

int main() {
    char input[MAX_INPUT_SIZE];

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Exit if Ctrl+D is pressed or an error occurs
        }

        // Remove trailing newline character
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        // Split input into commands based on pipe character "|"
        char *commands[MAX_ARG_COUNT];
        int num_commands = 0;
        char *token = strtok(input, "|");

        while (token != NULL && num_commands < MAX_ARG_COUNT) {
            commands[num_commands] = token;
            num_commands++;
            token = strtok(NULL, "|");
        }

        if (num_commands == 1) {
            // Single command without piping
            char *args[MAX_ARG_COUNT];
            int num_args = 0;
            token = strtok(commands[0], " ");

            while (token != NULL && num_args < MAX_ARG_COUNT) {
                args[num_args] = token;
                num_args++;
                token = strtok(NULL, " ");
            }
            args[num_args] = NULL;

            execute_command(args, STDIN_FILENO, STDOUT_FILENO);
        } else if (num_commands == 2) {
            // Two commands with piping
            char *args1[MAX_ARG_COUNT];
            char *args2[MAX_ARG_COUNT];
            int num_args1 = 0;
            int num_args2 = 0;

            token = strtok(commands[0], " ");
            while (token != NULL && num_args1 < MAX_ARG_COUNT) {
                args1[num_args1] = token;
                num_args1++;
                token = strtok(NULL, " ");
            }
            args1[num_args1] = NULL;

            token = strtok(commands[1], " ");
            while (token != NULL && num_args2 < MAX_ARG_COUNT) {
                args2[num_args2] = token;
                num_args2++;
                token = strtok(NULL, " ");
            }
            args2[num_args2] = NULL;

            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                exit(1);
            }

            execute_command(args1, STDIN_FILENO, pipe_fd[1]); // Redirect output of args1 to pipe
            execute_command(args2, pipe_fd[0], STDOUT_FILENO); // Redirect input of args2 from pipe

            close(pipe_fd[0]);
            close(pipe_fd[1]);
        } else {
            printf("Error: Too many commands or invalid input.\n");
        }
    }

    printf("Goodbye!\n");
    return 0;
}
