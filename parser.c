#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "parser.h"


#define BUFLEN 1024

void tokenizeBothCommands(char* inputCommands, char *args[], char *pipeArgs[]) {
    char* token = strtok(inputCommands, " ");
    int argIndex = 0;
    int tokenArgIndex = 0;
    int isPipe = 0;
    while (token != NULL && argIndex < BUFLEN - 1) {
        if (!strcmp(token, "|")) {
            isPipe = 1;
            token = strtok(NULL, " ");
        }
        if (isPipe) {
            pipeArgs[tokenArgIndex++] = token;
        } else {
            args[argIndex++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[argIndex] = NULL; // Null-terminate the argument array
    pipeArgs[tokenArgIndex] = NULL;
    return;
}

int executeExecutable(char *command, char *args[]) {
    if (command[0] == '/') {
        if(execve(args[0],args,NULL) == -1)
        {
            fprintf(stderr, "Error running command in execve\n");
            return -100;
        }
    } else {
        return checkWorkingPath(command, args);
    }

}

void runNextPipe() {
    int pipefd[2]; // File descriptors for the pipe
    pid_t child_pid1, child_pid2;

    child_pid2 = fork();
    if (child_pid2 == 0) {
        dup2(child_pid2, STDIN_FILENO); // Redirect stdin to the read end of the pipe
        close(child_pid2); // Close the read end of the pipe
        execlp("grep", "grep", "parser", NULL);
        perror("execlp");

    } else {
        wait(NULL);
    }

    return;
}



// Function looks for the executable 
// In both the local Directory aswell as the PATH directories
int checkWorkingPath(char *command, char *args[]) {

    // Check working directory first
    args[0] = concatWorkingDir(command);
    if(execve(args[0],args,NULL) != -1)
        {
            if (args[0] != NULL) {
                printf("this os not workjing");
                //free(args[0]);
            }
            return 1;
        }

    // Check all different directories in the path
    char *path = getenv("PATH");
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable not found.\n");
        return 1;
    }

    char *path_copy = strdup(path); // Duplicate path for strtok modification
    if (path_copy == NULL) {
        perror("strdup");
        return 1;
    }

    char *dir = strtok(path_copy, ":"); // Split path using ':' delimiter

    while (dir != NULL) {
        // Construct the full path to the command in this directory
        char full_path[1024]; // Adjust the buffer size as needed
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        args[0] = full_path;
        // Exit if command is found in PATH
        if(execve(args[0],args,NULL) != -1)
            {
                if (path_copy != NULL) {
                    free(path_copy);
                }
                return 1;
            }
        dir = strtok(NULL, ":");
    }

    fprintf(stderr, "Error running command in execve\n");

    if (path_copy != NULL) {
        free(path_copy);
    }

    return -100;
}


// Gets the location of the current working dir
char * concatWorkingDir(char *nextPath) {
    char *buffer;
    size_t size;

    // Determine the size of the buffer needed
    size = pathconf(".", _PC_PATH_MAX) + strlen(nextPath) + 2;

    if ((buffer = (char *)malloc((size_t)size)) != NULL) {
        if (getcwd(buffer, (size_t)size) != NULL) {
            strcat(buffer, "/");
            strcat(buffer, nextPath);
        } else {
            perror("getcwd");
        }
    } else {
        perror("malloc");
    }

    return buffer;
}

//Command to trim whitespace and ASCII control characters from buffer
//[Input] char* inputbuffer - input string to trim
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char* outputbuffer - output string after trimming 
//[Return] size_t - size of output string after trimming
size_t trimstring(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{   
    memcpy(outputbuffer, inputbuffer, bufferlen*sizeof(char));

    for(size_t ii = strlen(outputbuffer)-1; ii >=0; ii--){
        if(outputbuffer[ii] < '!') //In ASCII '!' is the first printable (non-control) character
        {
            outputbuffer[ii] = 0;
        }else{
            break;
        }    
    }

    return strlen(outputbuffer);
}

//Command to trim the input command to just be the first word
//[Input] char* inputbuffer - input string to trim
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char* outputbuffer - output string after trimming 
//[Return] size_t - size of output string after trimming
size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{
    //TO DO: Implement this function
    return 0;
}

//Command to test that string only contains valid ascii characters (non-control and not extended)
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] bool - true if no invalid ASCII characters present
bool isvalidascii(const char* inputbuffer, size_t bufferlen)
{
    //TO DO: Correct this function so that the second test string fails
    size_t testlen = bufferlen;
    size_t stringlength = strlen(inputbuffer);
    if(strlen(inputbuffer) < bufferlen){
        testlen = stringlength;
    }

    bool isValid = true;
    for(size_t ii = 0; ii < testlen; ii++)
    {
        isValid &= ((unsigned char) inputbuffer[ii] <= '~'); //In (lower) ASCII '~' is the last printable character
    }

    return isValid;
}

//Command to find location of pipe character in input string
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] int - location in the string of the pipe character, or -1 pipe character not found
int findpipe(const char* inputbuffer, size_t bufferlen){
    int index = -1; // Initialize index to -1 to indicate that '|' was not found
    int i = 0;

    while (inputbuffer[i] != '\0') {
        if (inputbuffer[i] == '|') {
            index = i; // Set the index if '|' is found
            break;     // Exit the loop once '|' is found
        }
        i++;
    }

    return index;
}


//Command to test whether the input string ends with "&" and
//thus represents a command that should be run in background
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] bool - true if string ends with "&"
bool runinbackground(const char* inputbuffer, size_t bufferlen){
    //TO DO: Implement this function

    return false;
}
