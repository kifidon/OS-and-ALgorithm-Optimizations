# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Timmy Ifidon
# SID : 1718228
# CCID : kifidon
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Dragon Shell

## Overview
Dragonshell is a simple unix based shell implementation for users to execute basic unix commands. This shell is a tool for understanding process management, command execution, and inpout/output redirection. Dragonshell supports built in commands such as cd, pwd, and exit, as well as external command execution, piping and redirection of standard I/O.

## Design Choices

### Modular Architecture

The Dragon Shell is designed with a modular architecture, which separates distinct functionalities into individual functions. This enhances the readability, maintainability, and testability of the code. Key modules include: [3]

- **Input Handling**: THe shell pormpts user for input and passes it to the main program. the main program handles empty inputs by reprompting.
  
- **Command Parsing**: The input command string is tokenized into an array of arguments using the `tokenize` function, allowing for dynamic handling of various command formats, including those with arguments and special characters. [3]

- **Execution Management**: The shell can differentiate between built-in commands and external commands. Built-in commands are executed directly, while external commands are executed in separate processes. [3]

### Error Handling and Debugging

Robust error handling is integrated throughout the shell's functions. Each function checks for potential errors, such as memory allocation failures, invalid file accesses, and unsuccessful command executions. Error messages are printed using `perror()` to provide context for the failure. A `DEBUG` mode is included, which provides additional runtime diagnostics, such as the current state of processes and detailed error messages, making it easier to trace issues. [3]

### System Calls Utilized

The implementation leverages several Unix system calls to achieve its functionality. The primary system calls employed in Dragon Shell include: [3] [3]

- **`fork()`**: Creates new child processes for executing commands. Each command is executed in its own process, enabling simultaneous execution and management of multiple commands.
  
- **`execve()`**: Replaces the current process image with a new process image for executing external commands. This system call is essential for running any executable that is not built-in.
  
- **`waitpid()`**: Waits for specific child processes to change state (e.g., exit), allowing the parent shell to reap zombie processes and manage resources effectively.
  
- **`chdir()`**: Changes the current working directory of the shell process, allowing users to navigate the file system.
  
- **`getcwd()`**: Retrieves the current working directory of the shell, enabling the `pwd` command functionality.
  
- **`pipe()`**: Creates a unidirectional data channel that can be used for inter-process communication, facilitating the implementation of piping between commands.
  
- **`dup2()`**: Duplicates file descriptors for redirecting input and output, essential for handling I/O redirection and piping when executing commands.
  
- **`open()`**: Opens files for reading or writing, necessary for input and output redirection. This system call is used to handle file descriptors.

### Key Features

1. **Built-in Commands**:
   - **`cd` (Change Directory)**: Allows users to change the current working directory. The function supports paths enclosed in quotes for those with spaces.
   - **`pwd` (Print Working Directory)**: Displays the current working directory path.
   - **`exit`**: Terminates the shell process and performs graceful cleanup.

2. **External Command Execution**:
   - The shell can execute external commands with varying degrees of complexity by passing its arguments to a subprocess and calling execve. This includes handling commands with multiple arguments and special characters.

3. **Input/Output Redirection**:
   - The shell supports input (`<`) and output (`>`) redirection, enabling users to specify files for reading from or writing to.
   - The `redirectIO` function manages file descriptor duplication for standard input and output, ensuring commands can interact with files seamlessly.

4. **Piping**:
   - 2 Commands can be chained together using the pipe operator (`|`). The output of one command can be passed directly as input to another command.


3. **Background Process Execution**:
   - Users can execute commands in the background by appending an ampersand (`&`) to the command line. The shell will display the process ID of the background task and continue accepting input after repromting.

## Testing Methodology

The testing strategy for Dragon Shell involved a comprehensive approach to ensure all features and edge cases functioned correctly:

- **Unit Testing**: Each function was tested in isolation to verify its behavior under various conditions. This included testing for valid and invalid inputs, ensuring error messages were correctly displayed.


- **Functional Testing**: Various commands were executed in the shell to ensure that both built-in and external commands functioned as expected. This included:
  - Simple commands: `ls`, `pwd`, and custom scripts.
  - Commands with arguments: `echo Hello World` and file manipulations.
  - Redirection commands: `cat < file.txt` and `echo Hello > output.txt`.
  - Piping commands: ` /usr/bin/find ./ | /usr/bin/sort -r`.
  - additional test cases are found in the breakpoitns.txt file

- **Edge Case Testing**: Special attention was given to edge cases, such as:
  - Invalid commands to confirm proper error handling.
  - File paths with spaces to validate parsing and execution.
  - Background execution of multiple commands to test concurrent process management.

- **Runtime Load Testing**: Simple and inefficient python and c files to monitor system and user time.
  - `subprocess.c` and `subprocess.py` located in the `test-files` directory list out all the factors `j ≤ i` for numbers `i < 100000` and  store it in the `factors.txt` file
  - Using the `exit` command will print out the sys and user time for these subprocesses.
  - Functions were also tested in the background and with I/O redirection

- **Debugging with `DEBUG` Mode**: The shell's debugging mode was utilized extensively to output detailed information about the execution flow, including timestamps and process states, aiding in identifying and fixing issues.

## Sources

1 Online resources such as the Linux man pages for system calls and their usage (`man 2 fork`, `man 2 execve`, etc.).
2 Various online forums and tutorials for command-line shell implementations.
3 ChatGPT: Used as a source for debugging syntax errors and summarizing/explaining man pages to ensure correct implementation of system calls and error handling. Chat GPT was also used to speed up the process for writing function descriptions by first explaining what the 
code function does, it's parameters, and its returns.