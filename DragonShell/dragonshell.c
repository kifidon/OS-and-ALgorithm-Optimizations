#include "dragonshell_util.h"

/**
 * @brief Tokenize a C string 
 * 
 * @param str - The C string to tokenize 
 * @param delim - The C string containing delimiter character(s) 
 * @param argv - A char* array that will contain the tokenized strings
 * Make sure that you allocate enough space for the array.
 */
void tokenize(char* str, const char* delim, char ** argv){
  char* token;
  token = strtok(str, delim);
  for(size_t i = 0; token != NULL; ++i){
    argv[i] = token;
  token = strtok(NULL, delim);
  }
}

/**
 * @brief Change the current working directory.
 * 
 * This function changes the current working directory of the calling process to the specified path.
 * If the path contains double quotes (filename has spaces), it tokenizes the string to extract the enclosed path.
 * 
 * @param path - The C string representing the path to change to. 
 *               If NULL, the function returns INVALID_ARGS.
 * 
 * @return int - Returns SUCCESS (0) if the directory change is successful,
 *               or FAILED (-1) if there is an error (e.g., if the path is invalid or access is denied).
 *               In case of an error, an appropriate error message is printed to stderr.
 */
int cd (char* path){
  if(path == NULL){
    return INVALID_ARGS;
  }
  if(strstr(path, "\"")){
    char * arg[MAX_ARG];
    tokenize(path, "\"", arg); // find path inside enclosed ""
    path = arg[0];
  }
  int result = chdir(path);
  switch(result) {
    case SUCCESS: return SUCCESS;
    case FAILED:  
      printf("dragonshell: %s\n", strerror(errno));
      fflush(stdout);
      return FAILED;
    default: return FAILED;
  }

}

/**
 * @brief Print the current working directory.
 * 
 * This function retrieves the current working directory of the calling process
 * and prints it to standard output. If the current directory cannot be retrieved,
 * an error message is printed to standard error.
 * 
 * @return void - This function does not return a value. 
 *                If an error occurs while getting the current directory, 
 *                an error message is printed using perror().
 */
void pwd(){
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL){
    perror(strerror(errno));
    return;
  }
  else{
    write(STDOUT_FILENO, cwd, strlen(cwd));
    printf("\n");
    fflush(stdout);
    return;
  }

}

/**
 * @brief Terminate all child processes and print resource usage statistics.
 * 
 * This function iterates through an array of child process IDs and attempts to
 * terminate each active child process using the SIGTERM signal. It handles cases
 * where a child process may have already been reaped and prints an error message
 * if termination fails. After terminating the processes, it retrieves and prints
 * the total user and system time consumed by the terminated child processes.
 * 
 * @return void - This function does not return a value. 
 *                After terminating all child processes, it exits the program.
 */
void terminateProcesses() {
  struct rusage usage;
  long totalUserTime = 0;
  long totalSysTime = 0;
  for(int i = 0; i < childIndex; i ++){
    if(childPID[i] > 0){
      if(kill(childPID[0] , 0) != 0){
        continue; // child has already been reaped
      }
      if(kill(childPID[i], SIGTERM) == -1) {
        perror("Failed to kill child process: ");
        continue;
      }
      wait3(NULL, 0, &usage); // reaps any child processes that were currently running and just killed by the previous step 
      
    }
  }
  getrusage(RUSAGE_CHILDREN, &usage);
  totalUserTime += usage.ru_utime.tv_sec;
  totalSysTime += usage.ru_stime.tv_sec;


  printf("Total user time: %ld seconds\n", totalUserTime);
  printf("Total system time: %ld seconds\n", totalSysTime);

  exit(0);
}


/**
 * @brief Combine an array of strings into a single C string.
 * 
 * This function concatenates strings from an array of arguments starting from a given
 * index into a single dynamically allocated string, separating each argument with a space.
 * It calculates the total length needed for the combined string and ensures proper memory
 * allocation. If allocation fails, the program exits with an error message.
 * 
 * @param args - An array of C strings (char* array) that will be combined.
 * @param start_index - The index in the array to start combining strings from.
 * 
 * @return char* - A pointer to the newly allocated string containing the combined arguments.
 *                 The caller is responsible for freeing the allocated memory.
 */
char* combineArgs(char* args[], int start_index) {
    size_t total_length = 0;
    for (int i = start_index; args[i] != NULL; ++i) {
        total_length += strlen(args[i]) + 1; // +1 for space or null terminator
    }
    char *combined = malloc(total_length);
    if (combined == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    combined[0] = '\0'; // Start with an empty string for strcat
    for (int i = start_index; args[i] != NULL; ++i) {
        strcat(combined, args[i]);
        if (args[i + 1] != NULL) {
            strcat(combined, " "); 
        }
    }

    return combined;
}

/**
 * @brief Create a pipeline between two commands using a pipe.
 * 
 * This function takes an array of command-line arguments, creates a pipe, 
 * and forks two child processes. The first child executes the command 
 * corresponding to the arguments before the pipe index, redirecting its output 
 * to the write end of the pipe. The second child executes the command 
 * corresponding to the arguments after the pipe index, reading its input from 
 * the read end of the pipe. The function handles errors in pipe and fork creation 
 * and ensures proper cleanup by closing pipe file descriptors and reaping 
 * child processes.
 * 
 * @param args - An array of C strings (char* array) representing command-line arguments.
 * @param pipeIndex - The index in the `args` array that separates the two commands.
 * 
 * @return void - This function does not return a value.
 */
void pipeProcesses(char* args[], int pipeIndex){
    resizeChildren(&childIndex, &maxChildren, childPID);
    char* cmd1Args[MAX_ARG];
    char* cmd2Args[MAX_ARG];
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("Failed to create pipe");
        return;
    }

    // First child (cmd1)
    // refactor this section of code into a function to allow for multiple levels of piping and loop on contains '|'
    if((childPID[childIndex] = fork()) < 0) {
        perror("Failed to create Child");
        return;
    } else if(childPID[childIndex] == 0) { // Child process
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("Failed to redirect output");
            exit(1);
        }
        close(pipefd[0]);  // Close read end since it is not needed in this process 
        close(pipefd[1]); 
        for(int i = 0; i < pipeIndex; i++) { // read the first command from input 
            cmd1Args[i] = args[i];
        }
        cmd1Args[pipeIndex] = NULL; // Terminate the argument list
        if(DEBUG){
          printf("\ncmd1Args areee:\n");
          for (int i = 0; cmd1Args[i] != NULL; i++) {
              printf("arg[%d]: %s\n", i, cmd1Args[i]);
          }
        }
        execve(cmd1Args[0], cmd1Args, NULL);
        perror("exec failed");
        exit(1); // Exit child process if exec fails
    }

    childIndex++;
    resizeChildren(&childIndex, &maxChildren, childPID);

    // Second child (cmd2)
    if((childPID[childIndex] = fork()) < 0) {
        perror("Failed to create Child");
        return;
    } else if(childPID[childIndex] == 0) { // Child process
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("Failed to redirect input");
            exit(1);
        }
        close(pipefd[0]);  // Close read end after dup
        close(pipefd[1]);  // Close write end

        for(int i = pipeIndex ; i < MAX_ARG && args[i] != NULL; i++) {
            cmd2Args[i - pipeIndex] = args[i];
            if(DEBUG){
              printf("%s", args[i] );
            }
        }
        cmd2Args[MAX_ARG - pipeIndex ] = NULL; // Terminate the argument list
        if(DEBUG){
          printf("\ncmd2Args are :\n");
          for (int i = 0; cmd2Args[i] != NULL; i++) {
              printf("arg[%d]: %s\n", i, cmd2Args[i]);
          }
        }
        execve(cmd2Args[0], cmd2Args, NULL);
        perror("exec failed");
        exit(1); // Exit child process if exec fails
    }


    // Parent process
    close(pipefd[0]);  // Close read end
    close(pipefd[1]);  // Close write end

    // Wait for the first child to finish
    if (waitpid(-1, NULL, 0) < 0 && errno !=EINTR) {
      perror("1. Problem occured while reaping child process");
    }
    // Wait for the second child to finish
    if (waitpid(-1, NULL, 0) < 0 && errno !=EINTR) {
      perror("2. Problem occured while reaping child process");
    }
    resizeChildren(&childIndex, &maxChildren, childPID);
    childIndex++; // childPID points to next availble position on call 
}

/**
 * @brief Execute a command with optional background execution and I/O redirection.
 * 
 * This function forks a new process to execute a command specified in the 
 * `args` array. It checks for background execution indicated by an '&' 
 * character and handles input/output redirection based on specified 
 * delimiters ('>' for output and '<' for input). If executed in the background,
 * the function redirects standard output, standard error, and standard input 
 * to `/dev/null`. The parent process either reaps the child process immediately 
 * or prints a message indicating the child's background execution.
 * 
 * @param args - An array of C strings (char* array) representing the command 
 *               and its arguments.
 * 
 * @return void - This function does not return a value. If the command execution
 *                fails, an error message is printed, and the child process exits.
 */
void executeAny(char * args[]){
  int background = 0;
  int backIndex;
  if ((backIndex = contains(args, MAX_ARG, '&'))){ 
          background = 1;
        }
  if ((childPID[childIndex] = fork()) < 0){
    perror("Failed to create Child");
    return;
  } 
  else if (childPID[childIndex] == 0) {
        // Child process
        
        if(DEBUG){
          printf("Child Starts at - ");
          timeNow();
        }
        int outIndex;
        int inIndex;
        if ((outIndex = contains(args, MAX_ARG, '>'))){
          redirectIO(args,OUTPUT, outIndex);
        } else if (background){
          // If there's no output redirection, redirect stdout and stderr to /dev/null
          freopen("/dev/null", "w", stdout);
          freopen("/dev/null", "w", stderr);
        }
        if ((inIndex = contains(args, MAX_ARG, '<')) ){
          redirectIO(args,INPUT, inIndex); // 0 indicates one way redirection
        } else if (background){
          freopen("/dev/null", "w", stdin); 
        }
        childIndex++;
        if(DEBUG){
          printf("Child Starts execution - ");
          timeNow();
        }
        execve(args[0], args, NULL);
 
        perror("exec failed");
        exit(1); // Exit child process if exec fails
  } else {
        // Parent process
        if (!background){
          if(wait(NULL) > 0){ // reap immediatly 
            if(DEBUG){
              printf("Parent reaps child - ");
              timeNow();
            }
          } 
        } else {
          fprintf(stdout, "PID %i is sent to background\n", childPID[childIndex]);
        }
        resizeChildren(&childIndex, &maxChildren, childPID);
        childIndex ++;
    }
}

/**
 * @brief Execute a command or pipeline based on input arguments.
 * 
 * This function checks for the presence of a pipe ('|') in the provided 
 * `args` array. If a pipe is found, it calls `pipeProcesses` to execute 
 * the commands in a pipeline. If no pipe is present, it calls `executeAny` 
 * to execute the single command specified in the arguments. The function 
 * also manages the resizing of the child process array to accommodate 
 * new child processes. Further implemenatation of shell subprocesses could
 * be implimented in this function
 * 
 * @param args - An array of C strings (char* array) representing the command 
 *               and its arguments.
 * 
 * @return void - This function does not return a value.
 */
void subProcess(char* args[] ){
  resizeChildren(&childIndex, &maxChildren, childPID);
  int pipeIndex;
  if ((pipeIndex = contains(args, MAX_ARG, '|')) ){
    pipeProcesses(args, pipeIndex);
  }
  else { // can be expanded to include cases for other formated functions
    executeAny(args);
  }

}

/**
 * @brief Entry point for the Dragon Shell program.
 * 
 * This function initializes the shell, sets up signal handling, and enters 
 * the main command loop. It allocates memory for child process IDs and 
 * handles user input. The command entered by the user is tokenized, and 
 * the corresponding action is taken based on the command provided. 
 * Supported commands include changing the directory (cd), printing the 
 * current working directory (pwd), terminating processes, and executing 
 * subprocesses. The shell continues running until the exit command is 
 * received, at which point it cleans up resources and exits.
 * 
 * @param argc - The number of command-line arguments.
 * @param argv - An array of C strings containing the command-line arguments.
 * 
 * @return int - Returns 0 on successful execution or an error code on failure.
 */
int main(int argc, char *argv[]){
  pid_t shell_pid = getpid();
  // max children at launch at one time is the number of args that can be passed at oce
  childPID = malloc(MAX_ARG * sizeof(pid_t)); 
  if (childPID == NULL) {
        perror("Failed to allocate memory");
        exit(1);
    }
  if (DEBUG){
    printf("%i\n", shell_pid);
    fflush(stdout);
  }
  write(STDOUT_FILENO, "Welcome to Dragon Shell!\n\n", strlen("Welcome to Dragon Shell!\n\n"));
  struct sigaction  sa;
  sa.sa_handler = sighandler;
  sigaction(SIGTSTP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);


  char command[MAX_CMD_LENGTH];
  char* args[MAX_ARG];
  
  while(1){
    memset(command, 0, sizeof(command));  // Set command to 0/
    memset(args, 0, sizeof(args));         // Set args to 0/
    ssize_t bytesRead;
    
    if(getInput(command, bytesRead)){
      continue;
    }
    tokenize(command, " ", args);
    int result;

    
    //each system call 
    if(DEBUG){
      printf("\nSystem Code - %i\n",systemCodes(args[0]));
      fflush(stdout);
    }
    switch(systemCodes(args[0])) {
      case CD: 
        if(strstr(combineArgs(args,1), "\"")){ // joins parameters together for file names that include spaces, enclosed with quote ""
          result = cd(combineArgs(args,1));
        }
        else{
          result = cd(args[1]);
        }
        break;
      case PWD: 
        pwd();
        break;
      case EXIT:
        terminateProcesses();
        exit(0);
        break;
      case EXECUTE: // Execute all subproceses from here 
        subProcess(args); //Forks subprocess 
        
        break;
      default: 
        printf("Command not found\n");
        fflush(stdout);
    }
  }
  memset(command, 0, sizeof(command));  
  memset(args, 0, sizeof(args));  
  free(childPID);   
  return 0;
}

