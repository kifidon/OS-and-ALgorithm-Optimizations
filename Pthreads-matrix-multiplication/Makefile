# Define the compiler
CC = gcc

# Define compiler flags
CFLAGS = -Wall -g

# Define the source files
SRCS = matrix_mult.c lab1_IO.c
TEST_SRCS = test.c lab1_IO.c

# Define the object files
OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_EXEC = test

# Define the executable
EXEC = main

# Libraries to link
LIBS = -lm -lpthread  # Link math and pthread libraries

# The default target (when you run `make`)
all: $(EXEC)

# Compile the executable from the object files
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LIBS)  # Link necessary libraries here

test: gen all $(TEST_OBJS)
	$(CC) $(TEST_OBJS) -o $(TEST_EXEC) $(LIBS)  # Link necessary libraries here
	rm -f timing_results.txt

test%.o: test%.c
	$(CC) $(CFLAGS) -c $< -o $@
# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

gen:
	$(CC) $(CFLAGS) matrixgen.c -o matrixgen
	./matrixgen

# Clean up generated files
clean:
	rm -f $(OBJS) $(EXEC) $(TEST_OBJS) $(TEST_EXEC)
	rm -f matrixgen data_input data_output timing_results.txt 
	rm -rf matrixgen.dSYM

# Run the program
run: $(EXEC) 
	./$(EXEC) 4

test_run: test
	clear
	./$(TEST_EXEC)