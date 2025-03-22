#ifndef MAPREDUCE_H
#define MAPREDUCE_H


#include "threadpool.h"
#include <string.h>
#include <assert.h>
// function pointer typedefs
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, unsigned int partition_idx);

// library functions that must be implemented
typedef struct KeyValue {
    char *key;
    char *value;
    struct KeyValue *next;                      // Linked List 
} KeyValue;

typedef struct Bucket{
    KeyValue *head;                         // Array of Keyvalue partitions 
    pthread_mutex_t partitionMutex;
    size_t size;
} Bucket;

typedef struct Partitions{
    Bucket ** bucket;
    unsigned int numParts;

} Partitions;

Partitions partitions;

void initPartitions(unsigned int num_parts){
    partitions.numParts = num_parts;
    partitions.bucket = (Bucket **)malloc(num_parts * sizeof(Bucket *));
    for(unsigned int i =0; i < num_parts; i++){
        partitions.bucket[i] = (Bucket *)malloc(num_parts * sizeof(Bucket ));
        partitions.bucket[i]->head = NULL; // sets bucket/partition to empty
        pthread_mutex_init(&partitions.bucket[i]->partitionMutex, NULL); 
        partitions.bucket[i]->size = 0;
    }
}

void destroyPartitions() {
    for (unsigned int i = 0; i < partitions.numParts; i++) {
        KeyValue *current = partitions.bucket[i]->head;
        while (current != NULL) {
            KeyValue *temp = current;
            current = current->next;
            free(temp->key);
            free(temp->value);
            // Free the KeyValue node itself
            free(temp);
        }
        
        pthread_mutex_destroy(&partitions.bucket[i]->partitionMutex);
        free(partitions.bucket[i]);
    }
    free(partitions.bucket);
}

void MR_Reduce(void *threadarg);
/**
* Run the MapReduce framework
* Parameters:
*     file_count   - Number of files (i.e. input splits)
*     file_names   - Array of filenames
*     mapper       - Function pointer to the map function
*     reducer      - Function pointer to the reduce function
*     num_workers  - Number of threads in the thread pool
*     num_parts    - Number of partitions to be created
*/
void MR_Run(
    unsigned int file_count, char *file_names[],
    Mapper mapper, Reducer reducer, 
    unsigned int num_workers, unsigned int num_parts){
        ThreadPool_t *pool = ThreadPool_create(num_workers);
        if(DEBUG){printf("\nCreating Thread Pool");
            fflush(stdout);
        }
        initPartitions(num_parts);
        for(int i = 0; i < file_count; i ++){
            ThreadPool_add_job(pool,(thread_func_t)mapper, file_names[i]);

        }
        if(DEBUG)
            {
            printf("\nSubmit Mapper Jobs");
            fflush(stdout);
            }
        ThreadPool_check(pool);
        if(DEBUG){printf("\nMapper jobs finished");
            fflush(stdout);}

        for(int i =0; i < num_parts; i ++){
            pthread_mutex_lock(&partitions.bucket[i]->partitionMutex);
            if(partitions.bucket[i]->size ==0){
                pthread_mutex_unlock(&partitions.bucket[i]->partitionMutex);
                continue;
            }
            pthread_mutex_unlock(&partitions.bucket[i]->partitionMutex);
            ThreadArgs *threadarg = malloc(sizeof(ThreadArgs));
            threadarg->partId = i;
            threadarg->reducer = reducer; 
            threadarg->size = partitions.bucket[i]->size;
            ThreadPool_add_job(pool, (thread_func_t)MR_Reduce, threadarg);
        }

        if(DEBUG){printf("\nSubmit Reducers Jobs");
            fflush(stdout);}
        ThreadPool_check(pool);
        if(DEBUG){printf("\nReducer jobs finished");
            fflush(stdout);}

        ThreadPool_destroy(pool);
        destroyPartitions();

    }

unsigned int MR_Partitioner(char *key, unsigned int num_partitions); // Protype Partitioner function
/**
* Write a specifc map output, a <key, value> pair, to a partition
* Parameters:
*     key           - Key of the output
*     value         - Value of the output
*/
void MR_Emit(char *key, char *value){
    KeyValue* node = (KeyValue *)malloc(sizeof(KeyValue));
    // Allocate memory for the key and value and copy the data
    node->key = (char *)malloc(strlen(key) + 1);
    strcpy(node->key, key);  
    node->value = (char *)malloc(strlen(value) + 1);  
    strcpy(node->value, value);  
    node->next= NULL;

    unsigned int partId = MR_Partitioner(key, partitions.numParts);
    Bucket *bucket = partitions.bucket[partId];
    
    // Modify this partition 
    pthread_mutex_lock(&bucket->partitionMutex);
    if(bucket->head == NULL){ // first item case  
        bucket->head = node;
    }
    else{
        // Insert the node in sorted order
        KeyValue *current = bucket->head;
        KeyValue *previous = NULL;

        // Traverse the list to find the correct position for the new node
        while (current != NULL && strcmp(current->key, node->key) <= 0) {
            previous = current;
            current = current->next;
        }
        // Insert the node at the found position
        node->next = current;
        if(previous == NULL){
            bucket->head = node;
        }
        else{
            previous->next = node;
        }
    }
    bucket->size ++;
    pthread_mutex_unlock(&bucket->partitionMutex);
}
    
/**
* Hash a mapper's output to determine the partition that will hold it
* Parameters:
*     key           - Key of a specifc map output
*     num_partitions- Total number of partitions
* Return:
*     unsigned int  - Index of the partition
*/
unsigned int MR_Partitioner(char *key, unsigned int num_partitions){
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
    hash = hash * 33 + c;
    return hash % num_partitions;
}

/**
* Run the reducer callback function for each <key, (list of values)> 
* retrieved from a partition
* Parameters:
*     threadarg     - Pointer to a hidden args object
*/
void MR_Reduce(void *threadarg){
    ThreadArgs *args = (ThreadArgs *)threadarg;
    Bucket *bucket = partitions.bucket[args->partId];
    pthread_mutex_lock(&bucket->partitionMutex);
    // if(DEBUG){
    //     if(args->partId == 9){
    //         assert(bucket->size == 5000);
    //     }
    // }
    if(DEBUG){
        printf("\nThread ID: %lu Reducing Partition: %i", (unsigned long)pthread_self(), args->partId);
        fflush(stdout);}
    
    KeyValue *node = bucket->head;
    while(node != NULL){
        // if (DEBUG) {printf("\nPartition: %i . New Key %s (%lu)", args->partId, node->key, (unsigned long)pthread_self());
        //     fflush(stdout);}
        args->reducer(node->key, args->partId);
        node = bucket->head;
    }
    free(threadarg);
    pthread_mutex_unlock(&bucket->partitionMutex);  
}

/**
* Get the next value of the given key in the partition
* Parameters:
*     key           - Key of the values being reduced
*     partition_idx - Index of the partition containing this key
* Return:
*     char *        - Value of the next <key, value> pair if its key is the current key
*     NULL          - Otherwise
*/
char *MR_GetNext(char *key, unsigned int partition_idx) {
    Bucket *bucket = partitions.bucket[partition_idx];
    
    if (bucket->head == NULL){ //list is empty 
        return NULL;
    }
    if(strcmp(bucket->head->key, key) !=0){ // current key has been reduced 
        return NULL;
    }
    KeyValue *node = bucket->head->next;
    // Case 1: Bucket head-> next matches key
    if (node != NULL && strcmp(node->key, key) == 0) { 
        // remove and return 
        bucket->head->next = node->next; //remove from list 
        char *value = strdup(node->value); // Duplicate the value
        // free(node->key);
        // free(node->value);
        // free(node);
        bucket->size--;  // Adjust the size of the bucket
        assert(bucket->size > 0);
        return value;
    }
    // Case 2: last instance in the list or next instence is different key 
    else if (node == NULL || strcmp(node->key, key) != 0 ){ 
        // pop bucket head and return 
        char *value = strdup(bucket->head->value); // Duplicate the value
        // free(bucket->head->key);
        // free(bucket->head->value);
        // free(bucket->head);
        bucket->head = node;
        bucket->size--;  // Adjust the size of the bucket
        // assert(bucket->size == 0 );
        return value;
    }
    else {
        return NULL;  // Fall through 
    }
}
#endif
