# Makefile for distwc program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -pthread 

# Executable and source files
TARGET = distwc
SRC = distwc.c
HEADERS = mapreduce.h threadpool.h

# Directory for sample input files
INPUT_DIR = sample_inputs
INPUT_FILES = $(wildcard $(INPUT_DIR)/*.txt)
INPUT_FILES_test = sample_inputs/sample1.txt sample_inputs/sample2.txt sample_inputs/sample3.txt sample_inputs/sample4.txt

# Compile the target
$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Run the program with all input files
run: $(TARGET)
	./$(TARGET) $(INPUT_FILES)
test:
	./$(TARGET) $(INPUT_FILES_test)
# Clean up the compiled files
clean:
	rm -f $(TARGET)
	rm -f *.txt
	rm -f distwc.dSYM
