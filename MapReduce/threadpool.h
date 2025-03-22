#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>

#define DEBUG false
typedef void (*thread_func_t)(void *arg);

typedef struct ThreadPool_job_t {
    thread_func_t func;              // function pointer
    void *arg;                       // arguments for that function
    struct ThreadPool_job_t *next;   // pointer to the next job in the queue
    size_t jobSize;                 // Holds the size (estimated time) for a file 
} ThreadPool_job_t;

typedef struct {
    unsigned int size;               // no. jobs in the queue
    ThreadPool_job_t *head;          // pointer to the first (shortest) job
    // add other members if needed
} ThreadPool_job_queue_t;

typedef struct {
    pthread_t *threads;              // pointer to the array of thread handles
    ThreadPool_job_queue_t jobs;     // queue of jobs waiting for a thread to run
    int num_workers;                 // Number of threads in the pool
    int shutdown;                    // Is the pool Shutting down?
    pthread_mutex_t lock;            // Mutex Lock
    pthread_cond_t isWorkToDo;       // Condition variable to isWorkToDo threads
    pthread_cond_t isIdle;           // Condition variable to notify when a thread is entering idle state
    pthread_cond_t isFull;           // Condition variable to notifiy when threads are full 
} ThreadPool_t;

typedef struct ThreadArgs {
    unsigned int partId;                                     // Partition ID
    void (*reducer)(char *key, unsigned int partition_idx);  // Function pointer to the reducer
    size_t size;                                             // Bucket size
} ThreadArgs;

void *Thread_run(ThreadPool_t *tp);
/**
* C style constructor for creating a new ThreadPool object
* Parameters:
*     num - Number of threads to create
* Return:
*     ThreadPool_t* - Pointer to the newly created ThreadPool object
*/
ThreadPool_t *ThreadPool_create(unsigned int num){
    ThreadPool_t *pool = (ThreadPool_t *)malloc(sizeof(ThreadPool_t));
    if (pool == NULL) {
        return NULL; // check for failure 
    }

    pool->threads=(pthread_t *)malloc(sizeof(pthread_t)* num);
    if (pool->threads== NULL){
        free(pool);
        return NULL;
    }
    pool->num_workers = num;
    pool->shutdown = 0;
    
    pool->jobs.size=0; 
    pool->jobs.head = NULL;
    
    //init sync variables
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->isWorkToDo, NULL);
    pthread_cond_init(&pool->isFull, NULL);
    pthread_cond_init(&pool->isIdle, NULL);
    for(unsigned int i = 0; i <num; i++){
        pthread_create(&pool->threads[i], NULL, (void *(*)(void *))Thread_run, (void*) pool);
    }

    return pool;
}

/**
* C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t *tp){
    pthread_mutex_lock(&tp->lock);
    
    tp->shutdown = 1;
    pthread_cond_broadcast(&tp->isWorkToDo); // Wake up all threads
    pthread_mutex_unlock(&tp->lock);

    // Join all worker threads
    for (int i = 0; i < tp->num_workers; i++) {
        pthread_join(tp->threads[i], NULL);
        pthread_cond_signal(&tp->isIdle);
    }

    free(tp->threads);
    pthread_mutex_destroy(&tp->lock);
    pthread_cond_destroy(&tp->isWorkToDo);
    pthread_cond_destroy(&tp->isFull);
    free(tp);
}

/**
* Add a job to the ThreadPool's job queue
* Parameters:
*     tp   - Pointer to the ThreadPool object
*     func - Pointer to the function that will be called by the serving thread
*     arg  - Arguments for that function
* Return:
*     true  - On success
*     false - Otherwise
*/
bool ThreadPool_add_job(ThreadPool_t *tp, thread_func_t func, void *arg){

    struct stat fileInfo;
    ThreadPool_job_t *task = (ThreadPool_job_t*)malloc(sizeof(ThreadPool_job_t));
    task->next = NULL;
    task->func = func;
    char*str = (char*)arg;
    if(stat(str, &fileInfo)==0) { // argument is a file name and thus mapper 
        task->jobSize = fileInfo.st_size;
        task->arg = str;
    }
    else{ // argument is not a valid file, type casting failed -> reducer function must be applied
        ThreadArgs *threadArg= (ThreadArgs* )arg;
        task->jobSize = threadArg->size;
        task->arg = threadArg;
    }
    // sjf
    pthread_mutex_lock(&tp->lock);
    if (tp->jobs.head == NULL) { // First job case
        tp->jobs.head = task;
    }
    else { // Traverse the list
        ThreadPool_job_t *previousTask = NULL;
        ThreadPool_job_t *curentTask = tp->jobs.head;

        while (curentTask != NULL && curentTask->jobSize <= task->jobSize) {
            previousTask = curentTask;  // Store the previous task
            curentTask = curentTask->next;  // Move to next task
        }

        // Inserting at the correct position
        task->next = curentTask;
        if (previousTask == NULL) {
            tp->jobs.head = task;  // Insert at the head if previousTask is NULL
        } else {
            previousTask->next = task;  // Insert after previousTask
        }
    }

    tp->jobs.size++;
    if(DEBUG){printf("\nJob Pool Size ++ : %i", tp->jobs.size);
        fflush(stdout);}
    // Notify one worker thread
    pthread_cond_signal(&tp->isWorkToDo);
    
    pthread_mutex_unlock(&tp->lock);
    return true;
}

/**
* Get a job from the job queue of the ThreadPool object
* Parameters:
*     tp - Pointer to the ThreadPool object
* Return:
*     ThreadPool_job_t* - Next job to run
*/
ThreadPool_job_t *ThreadPool_get_job(ThreadPool_t *tp){
    pthread_mutex_lock(&tp->lock);
    ThreadPool_job_t *task;
    while (tp->jobs.head == NULL && !tp->shutdown) {
        pthread_cond_signal(&tp->isIdle);
        pthread_cond_wait(&tp->isWorkToDo, &tp->lock);  // Wait for a job
    }
    // If the pool is shutting down, break out of the loop
    if (tp->shutdown) {
        pthread_mutex_unlock(&tp->lock);
        return NULL;
    }
    task = tp->jobs.head;
    if( task ==NULL){
        perror("Attempting to grab task when linked list is empty?");
    }
    tp->jobs.head = tp->jobs.head->next; // re-oder linked list 
    pthread_mutex_unlock(&tp->lock);
    
    return task; 

}

/**
* Start routine of each thread in the ThreadPool Object
* In a loop, check the job queue, get a job (if any) and run it
* Parameters:
*     tp - Pointer to the ThreadPool object containing this thread
*/
void *Thread_run(ThreadPool_t *tp){
    while(1){
        ThreadPool_job_t *task = ThreadPool_get_job(tp); // waits internally 
        if(task ==NULL){
            break;
        }
        
        task->func(task->arg);

        pthread_mutex_lock(&tp->lock);
        tp->jobs.size --;
        if(DEBUG){
            printf("\nJob Pool Size -- : %i", tp->jobs.size);
            fflush(stdout);
            printf("\n\tJob Size: %zu", task->jobSize);
            fflush(stdout);
            printf("\n\tThread ID: %lu", (unsigned long)pthread_self());
            fflush(stdout);
            }
        free(task);
        pthread_mutex_unlock(&tp->lock);
    }
    return NULL;
}
/**
* Ensure that all threads are idle and the job queue is empty before returning
* Parameters:
*     tp - Pointer to the ThreadPool object that will be destroyed
*/
void ThreadPool_check(ThreadPool_t *tp){
    pthread_mutex_lock(&tp->lock);
    

    while (tp->jobs.size>0 || tp->jobs.head != NULL){
        // pthread_cond_signal(&tp->isWorkToDo);
        pthread_cond_wait(&tp->isIdle, &tp->lock);
    }
    pthread_mutex_unlock(&tp->lock);
    return; 
}

#endif
