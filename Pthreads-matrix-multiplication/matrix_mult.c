#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "lab1_IO.h"
#include "timer.h"
#include <math.h>

#define debug 0
typedef struct {
    int **a, **b, **c;
    int n, startRow, startColumn, blockSize;
} ThreadData;

void *matrixMultiplyThread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    if(debug){
        printf("\tStart Row,Column: [%d,%d]\n", data->startRow*data->blockSize, data->startColumn*data->blockSize);
    }
    int i, j, k;
    for (i = data->startRow*data->blockSize; i < data->startRow*data->blockSize + data->blockSize; i++) {
        for (j = data->startColumn*data->blockSize; j < data->startColumn*data->blockSize + data->blockSize; j++) {
            data->c[i][j] = 0;
            for (k = 0; k < data->n; k++) {
                data->c[i][j] += data->a[i][k] * data->b[k][j];
            }
            if (debug) {
                printf("\t\tThread %lu: c[%d][%d] = %d\n", (unsigned long)pthread_self(), i, j, data->c[i][j]);
            }
        }
    }
    return NULL;
}

#include <stdlib.h>

int **createMatix(int n) {
    int **matrix = malloc(n * sizeof(int *)); // Allocate memory for rows
    if (matrix == NULL) {
        return NULL; // Return NULL if malloc fails
    }

    for (int i = 0; i < n; i++) {
        matrix[i] = malloc(n * sizeof(int)); // Allocate memory for columns in each row
        if (matrix[i] == NULL) {
            // If allocation fails, free the previous rows and return NULL
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return NULL;
        }
    }

    return matrix; // Return the pointer to the allocated matrix
}

int main(int argc, char *argv[]) {
    int **a, **b, **c, n, numThreads;
    double startTime, endTime;

    if (argc != 2) {
        printf("Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }
    // Set up for number of threads 
    numThreads = atoi(argv[1]);
    if(debug){
        printf("Number of Threads: %d\n", numThreads);
    }
    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    
    // Load matrices
    Lab1_loadinput(&a, &b, &n);

    // Allocate result matrix
    c = createMatix(n);

    int blockSize =  n/(int)sqrt(numThreads);
    if (debug){
        printf("Block size: %d\n", blockSize);
    }
    GET_TIME(startTime);
    for (int i = 0; i < numThreads; i++) {
        threadData[i] = (ThreadData){a, b, c, n, floor(i/sqrt(numThreads)), i%(int)sqrt(numThreads), blockSize};
        pthread_create(&threads[i], NULL, matrixMultiplyThread, &threadData[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    GET_TIME(endTime);

    // Save result
    Lab1_saveoutput(c, &n, endTime - startTime);

    // Free memory
    for (int i = 0; i < n; i++) {
        free(a[i]);
        free(b[i]);
        free(c[i]);
    }
    free(a);
    free(b);
    free(c);

    return 0;
}