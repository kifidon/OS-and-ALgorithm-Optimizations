#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// simulates user and sys time of about 27 and 3 seconds respectivly for 200000 iterations and 6 and 1 for 100000 iterations 

int main() {
    // Set up signal handlers

    printf("Running subprocess\n");

    // Open a file to write factors to
    FILE *file = fopen("test-files/factors.txt", "w");
    if (file == NULL) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    // for (int i = 1; i < 200000; i++) { // Start from 1 to avoid division by zero
    for (int i = 1; i < 100000; i++) { // Start from 1 to avoid division by zero
        for (int j = 1; j <= i; j++) { // Change condition to j <= i for valid factors
            if (i % j == 0) {
                fprintf(file, "Found a factor of i for i = %d, j = %d\n", i, j);
                fflush(file);  // Ensure the output is written immediately

                // Optionally, sleep to simulate longer processing time
                // usleep(100);  // Sleep for 100 microseconds
            }
        }
    }

    fclose(file); // Close the file to flush and release resources

    return 0;
}