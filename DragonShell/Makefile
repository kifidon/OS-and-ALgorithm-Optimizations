# Makefile for compiling and debugging dragonshell.c

# Variables
CC = gcc -I.
CFLAGS = -g
TARGET = dragonshell
SRC = dragonshell.c
OBJ = $(SRC:.c=.o)  # Converts .c files to .o files

# Default target
all: $(TARGET)

# Compile target
compile: $(OBJ)

# Linking target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule for compiling source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Combined target for compiling and running the program
run: 
	@echo "Compiling in run mode..."
	$(CC) -o $(TARGET) $(SRC) 
	@echo "Compilation complete. Running the program...\n\n\n\n\n\n" 
	./$(TARGET)
	@echo "\n\n\n\n\n\n"
	make clean

# Debug target (for debugging with LLDB)
db: 
	@echo "Compiling in debug mode..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo "Compilation complete. Starting LLDB...\n\n\n\n\n\n"
	lldb $(TARGET)
	@echo "\n\n\n\n\n\n"
	make clean

#compile and links test functions 
testrun:
	@echo "compiling test c..."
	$(CC) -o sub test-files/subprocess.c

# Clean target
clean:
	@echo "Cleaning up before exiting"
	rm -f $(TARGET)
	rm -rf dragonshell.dSYM
	rm -f sub
	rm -f test-files/factors.txt test.txt
	rm -f $(OBJ)

# resets the directory for execution
set: 
	make clean 
	make testrun 
	make dragonshell
