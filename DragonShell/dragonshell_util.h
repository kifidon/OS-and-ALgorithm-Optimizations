#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> 
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include "time.h"

#define DEBUG 0


#define MAX_CMD_LENGTH 1024
#define MAX_ARG 10
#define PATH_MAX 1024
#define INVALID_ARGS  -2 
#define SUCCESS 0
#define FAILED -1
#define EXIT 0
#define INPUT 11
#define OUTPUT 12


#define CD 10
// #define LS 20 //Debugging 
#define PWD 30
#define EXECUTE 40

pid_t * childPID; // keeps track of every child process from launch
int childIndex = 0;
int maxChildren = MAX_ARG;


/**
 * @brief Prints the current local date and time to the standard output.
 * 
 * This function retrieves the current time, formats it as a string in 
 * the format "YYYY-MM-DD HH:MM:SS", and prints it to the standard output. 
 * The time is formatted using a static buffer to ensure that the memory 
 * is preserved between function calls.
 */
void timeNow() {
    static char time_string[100];  // Static buffer to hold the formatted time
    time_t current_time;
    struct tm *time_info;

    // Get the current time
    time(&current_time);

    // Convert to local time format
    time_info = localtime(&current_time);

    // Format time as a string
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    fprintf(stdout,"%s\n", time_string);  // Return the formatted time string
}

/**
 * @brief Searches for a target character in an array of strings.
 * 
 * This function iterates through an array of strings (args) and checks 
 * if any of the strings contain the specified target character. If found, 
 * the function sets that string's pointer to NULL to indicate it has been 
 * "removed" and returns the index of the first occurrence (1-based). 
 * If the target character is not found, the function returns 0.
 * 
 * @param args - An array of C strings to search.
 * @param len - The length of the args array.
 * @param target - The character to search for in the strings.
 * @return The 1-based index of the first string containing the target character, 
 * or 0 if not found.
 */
int contains(char* args[], int len, char target) {
    for (int i = 0; i < len; i++) {
        if (args[i] != NULL && strchr(args[i], target) != NULL) {
            args[i] = NULL; // remove target
            return i+1; //ensures it is non zero if found, returns index of file arg
        }
    }
    return 0; 
}

/**
 * @brief Resizes the array of child process IDs if the current capacity is exceeded.
 * 
 * This function checks if the current number of child processes (childCount) 
 * has reached the maximum capacity (maxChildren). If it has, the function 
 * doubles the size of the array to accommodate more child process IDs. It 
 * reallocates memory for the child process ID array and updates the pointer 
 * to the new memory. Note that error checking for memory allocation failure 
 * is performed, and in case of failure, the original array is freed.
 * 
 * @param childCount - A pointer to the current number of child processes.
 * @param maxChildren - A pointer to the maximum capacity of the child process ID array.
 * @param childPID - A pointer to the array of child process IDs.
 */
void resizeChildren(int *childCount, int *maxChildren, pid_t *childPID ){ //void, skipped error checking in cller function
    if (*childCount >= *maxChildren) {
        *maxChildren *= 2; // Double the size
        pid_t *newPIDs = realloc(childPID, *maxChildren * sizeof(pid_t));
        if (newPIDs == NULL) {
            perror("Failed to reallocate memory");
            free(childPID);
            return;
        }
        childPID = newPIDs; // Update pointer to new memory
    }
    return;
}

/**
 * @brief Redirects standard input or output to a specified file.
 * 
 * This function handles the redirection of standard input (STDIN) or 
 * standard output (STDOUT) based on the specified redirection type. 
 * If the redirection type is INPUT, the function opens the specified 
 * file for reading and redirects STDIN to this file. If the redirection 
 * type is OUTPUT, the function opens the specified file for writing (creating 
 * it if it doesn't exist) and redirects STDOUT to this file. The function 
 * also handles error checking for file opening and redirection failures, 
 * terminating the process if an error occurs.
 * 
 * @param args - An array of strings containing command-line arguments.
 * @param redirection - An integer indicating the type of redirection 
 *                      (INPUT or OUTPUT).
 * @param paramIndex - The index in the args array where the file name 
 *                     for redirection is located.
 */
void redirectIO(char* args[], int redirection, int paramIndex) {
    char *file; 
    int fd;
    
    if (redirection == INPUT) { 
        file = args[paramIndex]; 
        args[paramIndex] = NULL; 
        fd = open(file, O_RDONLY);
        if (fd == -1){
            perror("Failed to open input file");
            exit(EXIT_FAILURE);  
        }

        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("Failed to redirect input");
            exit(EXIT_FAILURE);  
        }
        close(fd);  
    }
    else if (redirection == OUTPUT) {
        file = args[paramIndex]; 
        args[paramIndex] = NULL; 
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1){
            perror("Failed to open output file");
            exit(EXIT_FAILURE);  
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect output");
            exit(EXIT_FAILURE);  
        }
        close(fd);  // Close the file descriptor since it's duplicated now
    }

}

/**
 * @brief Determines the type of system command based on the input string.
 * 
 * This function checks the provided command string against known commands 
 * and determines the corresponding action to take. It recognizes built-in 
 * commands such as "cd", "pwd", and "exit", as well as checks if a given 
 * command is executable using the `access` function. The function also 
 * includes a debug mode to print the command being checked. It returns 
 * specific integer constants representing each command type.
 * 
 * @param call - The command string to evaluate.
 * @return An integer code representing the command type:
 *         - CD for "cd"
 *         - LS for "ls" (if DEBUG mode is enabled)
 *         - PWD for "pwd"
 *         - EXIT for "exit"
 *         - EXECUTE if the command is executable
 *         - FAILED if the command is not recognized or not executable.
 */
int systemCodes(const char* call){
    if(DEBUG){
        printf("%s",call);
        int accessResult = access(call,F_OK);
        fflush(stdout);
    }
    if (!strcmp(call, "cd")){
        return CD;
    }
    else if (!strcmp(call, "pwd")){
        return PWD;
    }
    else if (!strcmp(call, "exit")){
        return EXIT;
    }
    else if (access(call,X_OK) == 0){
        return EXECUTE;
    }
    else {
return FAILED;}
}

/**
 * @brief Displays the command prompt with the current working directory.
 * 
 * This function retrieves the current time and 
 * formats the prompt accordingly. If debugging mode is enabled, it prints 
 * the current time before displaying the prompt. The function writes the 
 * prompt to the standard output.
 * 
 * @note If `getcwd` fails, an error message will be printed to standard error.
 * 
 * @return void
 */
void prompt(){
  if( DEBUG){ 
    char* output =  "<dragonshell> ";
    timeNow();
    write(STDOUT_FILENO, output, strlen(output));
    }
  else {write(STDOUT_FILENO, "dragonshell> ", 13);}; //write output 
}

/**
 * @brief Reads a command from standard input.
 * 
 * This function prompts the user for input, reads a command from the 
 * standard input, and stores it in the provided buffer. It also handles 
 * newline characters by replacing them with null terminators. If the 
 * input is empty, the function returns a value to indicate that the 
 * prompt should be displayed again.
 * 
 * @param command - A buffer to store the user input command. 
 * @param bytesRead - A variable to store the number of bytes read from input.
 * 
 * @return int - Returns SUCCESS (typically defined as 0) on successful input, 
 *               or 1 to indicate that the input was empty and the prompt 
 *               should be displayed again.
 */
int getInput(char * command, ssize_t bytesRead){
    prompt();
    bytesRead = read(STDIN_FILENO, command, MAX_CMD_LENGTH - 1);
    
    command[bytesRead] = '\0';
    if (bytesRead > 0 && command[bytesRead - 1] == '\n') {
      command[bytesRead - 1] = '\0';  
    }
    if(!strcmp(command, "")){
      return 1; // reprompt on empty input 
    }
    return SUCCESS;
}


/**
 * @brief Handle termination and stop signals for child processes.
 * 
 * This function is invoked when a signal (e.g., SIGINT or SIGTSTP) is received.
 * It iterates through an array of child process IDs and attempts to send the specified
 * signal to each active child process. If a process is no longer active, it is skipped.
 * If a SIGINT signal is sent and a child process terminates, the function reaps that process.
 * In the case of a SIGTSTP signal, it checks for the status of suspended or running subprocesses.
 * 
 * @param sig - The signal number that was received, which determines the action to be taken
 *              on the child processes.
 * 
 * @return void - This function does not return a value. 
 *                Debug information may be printed if DEBUG is enabled.
 */
void sighandler(int sig) {
  if(DEBUG) {
    // fprintf(stdout, "\nReceived signal %d\n", sig);
    fprintf(stdout, "\nChild Index - %d\n", childIndex);
  }

    for(int i=0; i <= childIndex; i++){
      if(DEBUG){
        fprintf(stdout, "Current Child Id - %d\n", childPID[i]);
      }
      if( childPID[i] > 0){
        if(kill(childPID[i], 0 ) != 0){
          if(DEBUG){fprintf(stdout, "\tThis Child is no longer active\n\n");}
          continue; // process is no longer active
        }
        kill(childPID[i], sig);
        if(DEBUG){
            fprintf(stdout, "\tKill signal sent - ");
            timeNow();
          }
        switch(sig){
          case SIGINT:
            if(waitpid(childPID[i], NULL, 0) > 0 ){ //reaps on terminated,
              if(DEBUG){
                fprintf(stdout, "\t\tKilled process status changed - ");
                timeNow();
              }
            } else {
              perror("Problem occured while waiting child process");
            }
            break;
          case SIGTSTP:
            if(waitpid(childPID[i], NULL, WNOHANG) != 0){
              perror("Cannot find a suspended or currently running supbrocess");
            }
        }

      }
    }
  
  if(DEBUG) {fprintf(stdout, "Returning\n");}
  fprintf(stdout, "\n");
  return;
  }