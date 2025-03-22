# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Timmy Ifidon
# SID : 11718228
# CCID : kifidon
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Project implements a basic MapReduce framework in C. The framework consists of ThreadPools, Map, and Reduce functions, as well as a partitioning system to distribute data processing.

## Features

- **ThreadPool**: A pool of worker threads for executing map and reduce tasks concurrently.
- **Partitioning**: Data is partitioned into multiple buckets to distribute work among threads.
- **Mapper and Reducer**: The core functions where users define the logic for processing the data.
- **Synchronization**: Mutex locks ensure thread safety when accessing shared data structures.
- **Dynamic Job Management**: The framework dynamically schedules jobs based on a Shortest job first algorithm, ensuring efficient resource usage and workload balancing.

## Structure 
1. **ThreadPool**: Handles the management of worker threads, job scheduling, and synchronization.
2. **KeyValue and Bucket**: Used to store key-value pairs, where each partition stores data in a linked list of `KeyValue` nodes.
3. **Partitioning**: The `MR_Partitioner` function hashes keys to determine which partition a key-value pair should belong to. Function was implimented using code from 
    The UofA CompSci department.
4. **MapReduce Workflow**: 
   - **Map phase**: The map function is applied to each input file, producing key-value pairs that are emitted to corresponding partitions.
   - **Reduce phase**: The reduce function processes each partition's list of key-value pairs.

## Functions

- **ThreadPool_create()**: Creates and initializes the thread pool with the specified number of worker threads.
- **ThreadPool_add_job()**: Adds a new job to the thread pool’s job queue, ensuring the shortest job is always at the head on the pool.
- **ThreadPool_get_job()**: Gets the next job from the thread pool’s job queue.
- **Thread_run()**: Worker thread’s main function, which continuously retrieves and executes jobs from the job queue.
- **MR_Emit()**: Emits a key-value pair to the appropriate partition.
- **MR_Partitioner()**: Hash function used to determine the partition index for a given key.
- **MR_Reduce()**: Runs the reduce callback function on each key-value pair from the partition.
- **MR_GetNext()**: Retrieves the next value associated with a key from a given partition.
 
## Clean-Up
- **ThreadPool_destroy()**: Destroys the thread pool and cleans up all associated resources.
- **destroyPartitions()**: Frees all allocated memory for the partitions and key-value pairs.

## References

1. **POSIX Threads (pthreads) Library**
   - The pthreads library is used for multithreading support in this project. For detailed information about the API and its functions, refer to the official manual.

2. **ChatGPT**
   - This project used assisted debugging with help from ChatGPT, an AI language model developed by OpenAI. ChatGPT provided guidance on various aspects of the code, 
   to speed up the workflow in debugging and testing various conditions and algorithms. ChatGPT was also used largely to format the structure of the README, promted by developer summaries of each code section.
     ```
     OpenAI, ChatGPT. https://chat.openai.com/
     ```