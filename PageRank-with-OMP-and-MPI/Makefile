CC = mpicc
CFLAGS = -Wno-unused-result -Wno-uninitialized -fopenmp  
LDFLAGS = -fopenmp  


SRCS = main.c Lab4_IO.c 
OBJS = $(SRCS:.c=.o)
EXEC = main

all: clean
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC) -lm $(LDFLAGS)  

clean: 
	rm -f $(OBJS) $(EXEC) 

debug: clean
	$(CC) -g $(CFLAGS) $(SRCS) -o $(EXEC) -lm $(LDFLAGS)

copy: all
	ssh node1 "mkdir -p /home/user_22/Lab4/Development_Kit_Lab4"
	ssh node2 "mkdir -p /home/user_22/Lab4/Development_Kit_Lab4"
	ssh node3 "mkdir -p /home/user_22/Lab4/Development_Kit_Lab4"
	scp -r * node1:/home/user_22/Lab4/Development_Kit_Lab4
	scp -r * node2:/home/user_22/Lab4/Development_Kit_Lab4
	scp -r * node3:/home/user_22/Lab4/Development_Kit_Lab4

ARGS = 4
run: all copy
	mpirun --hostfile hosts -np $(ARGS) --wdir /home/user_22/Lab4/Development_Kit_Lab4 ./$(EXEC)

VMARGS=2
runVM: 
	mpirun -np $(VMARGS) ./main