#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lab1_IO.h"
#include "timer.h"

void freeMatrix(int **matrix, int n) {
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Function to generate test input for the matrices
void generateTestInput(int size, int bound, char *outpath) {
    // Format the command to run matrixgen
    char command[256];
    snprintf(command, sizeof(command), "./matrixgen -s %d -b %d -o %s", size, bound, outpath);

    // Run the matrixgen executable to generate the input matrices
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Error generating test input with command: %s\n", command);
        exit(1);
    }
}

// Function to run the matrix multiplication using the main executable
void runMatrixMultiplication(int numThreads) {
    // Format the command to run the matrix multiplication with the number of threads
    char command[256];
    snprintf(command, sizeof(command), "./main %d", numThreads);

    // Run the matrix multiplication executable
    printf("Running matrix multiplication with %d threads...\n", numThreads);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Error running matrix multiplication executable with %d threads\n", numThreads);
        exit(1);
    }
}

void matrixMultiply(int **A, int **B, int **C, int n) {
    double start, end ;
    GET_TIME(start);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;  // Initialize result cell
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    GET_TIME(end);
    // Open file in append mode
    FILE *file = fopen("timing_results.txt", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing.\n");
        exit(1);
    }

    // Append the start and end times
    fprintf(file, "\tSerial Calculation Processing time: %.6f seconds\n", end - start);
    fprintf(file, "\tStart, End: %.6f, %.6f\n", start, end);

    // Close the file
    fclose(file);
}

void loadMatricesFromFile(const char *filename, int ***matrixA, int ***matrixB) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        exit(1);
    }

    int *n = (int*)malloc(sizeof(int));
    // Read the matrix size (n x n)
    fscanf(file, "%d", n);

    // Allocate memory for the first matrix (A)
    *matrixA = malloc(*n * sizeof(int *));
    for (int i = 0; i < *n; i++) {
        (*matrixA)[i] = malloc(*n * sizeof(int));
        for (int j = 0; j < *n; j++) {
            fscanf(file, "%d", &(*matrixA)[i][j]);
        }
    }
    if(matrixB == NULL){
        fclose(file);
        free(n);
        return;
    }
    // Allocate memory for the second matrix (B)
    *matrixB = malloc(*n * sizeof(int *));
    for (int i = 0; i < *n; i++) {
        (*matrixB)[i] = malloc(*n * sizeof(int));
        for (int j = 0; j < *n; j++) {
            fscanf(file, "%d", &(*matrixB)[i][j]);
        }
    }

    fclose(file);
    free(n);
}

// Function to compare the result matrix C with the expected matrix expectedC
int compareMatrices(int **C, int **expectedC, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (C[i][j] != expectedC[i][j]) {
                return 0;  // Matrices are not the same
            }
        }
    }
    return 1;  // Matrices are identical
}

int compare(int n, int TestNum){
     // Open file in append mode
    FILE *file = fopen("timing_results.txt", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing.\n");
        exit(1);
    }

    // Append the start and end times
    fprintf(file, "TEST NUM: %d\n", TestNum);

    // Close the file
    fclose(file);
    printf("Comparing matrices...\n");
    int **A, **B, **C, **expectedC;
    
    C = malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        C[i] = malloc(n * sizeof(int));
    }

    loadMatricesFromFile("data_input", &A, &B);
    loadMatricesFromFile("data_output", &expectedC, NULL);
    matrixMultiply(A, B, C, n);
    freeMatrix(A, n);
    freeMatrix(B, n);


    if (compareMatrices(C, expectedC, n)) {
        printf("\tMatrices are identical\n\n");
        freeMatrix(C, n);
        freeMatrix(expectedC, n);
        return 1;
    } else {
        printf("\tMatrices are not identical\n\n");
        freeMatrix(C, n);
        freeMatrix(expectedC, n);    
        return 0;
    }

}

// Main function that generates test inputs and calls the main executable
int main(int argc, char *argv[]) {
    if (argc == 3) {
        // Read matrix size n and number of threads from command line arguments
        int n = atoi(argv[1]); // Convert first argument to integer for matrix size
        int num_threads = atoi(argv[2]); // Convert second argument to integer for num_threads
        printf("Running test case 1: %dx%d matrices with values between -5 and 5 using %d threads\n", n,n,num_threads);
        generateTestInput(n, 5, "data_input");
        runMatrixMultiplication(num_threads);
        if(!compare(n,1)){
            printf("Test case 1 failed\n");
            return 1;
        }
    }
    else {
        // Test case 1: 4x4 matrices with values between -5 and 5 and 2 threads
        printf("Running test case 1: 4x4 matrices with values between -5 and 5 using 4 threads\n");
        generateTestInput(4, 5, "data_input");
        runMatrixMultiplication(4);
        if(!compare(4,1)){
            printf("Test case 1 failed\n");
            return 1;
        }

        // Test case 2: 6x6 matrices with values between -10 and 10 and 4 threads
        printf("Running test case 2: 6x6 matrices with values between -10 and 10 using 9 threads\n");
        generateTestInput(6, 10, "data_input");
        runMatrixMultiplication(9);
        if (!compare(6,2)) {
            printf("Test case 2 failed\n");
            return 1;
        }

        // Test case 4: 8x8 matrices with values between -15 and 15 and 8 threads
        printf("Running test case 3: 100x100 matrices with values between -15 and 15 using 25 threads\n");
        generateTestInput(100, 15, "data_input");
        runMatrixMultiplication(100);
        if (!compare(100,3)) {
            printf("Test case 4 failed\n");
            return 1;
        }

        // Test case 5: 5x5 matrices with values between -15 and 15 and 25 threads
        printf("Running test case 4: 100x100 matrices with values between -15 and 15 using 25 threads\n");
        generateTestInput(5, 15, "data_input");
        runMatrixMultiplication(25);
        if (!compare(5,4)) {
            printf("Test case 5 failed\n");
            return 1;
        }

        printf("Running test case 4: 100x100 matrices with values between -15 and 15 using 25 threads\n");
        generateTestInput(64, 15, "data_input");
        runMatrixMultiplication(4);
        if (!compare(64,5)) {
            printf("Test case 5 failed\n");
            return 1;
        }

        printf("Running test case 4: 100x100 matrices with values between -15 and 15 using 25 threads\n");
        generateTestInput(32, 15, "data_input");
        runMatrixMultiplication(1);
        if (!compare(32,6)) {
            printf("Test case 5 failed\n");
            return 1;
        }

        printf("Running test case 4: 100x100 matrices with values between -15 and 15 using 25 threads\n");
        generateTestInput(1000, 15, "data_input");
        runMatrixMultiplication(25);
        if (!compare(1000,7)) {
            printf("Test case 5 failed\n");
            return 1;
        }

        return 0;
    }
}