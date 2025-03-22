#define LAB4_EXTEND

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Lab4_IO.h"
#include "timer.h"
#include <mpi.h>
#include <omp.h>

#define DEBUG 0

#define EPSILON 0.00001
#define DAMPING_FACTOR 0.85

int main(int argc, char *argv[])
{
    // instantiate variables
    int rank, size, provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get rank
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get number of processes
    struct node *nodehead;
    
    int nodecount;
    int startNode, endNode;
    double *r, *rPre; // shared
    int *numLinks, *localNumLinks; // shared
    double *localR;
    int i, j, iterationcount;
    double start, end;
    /* INSTANTIATE MORE VARIABLES IF NECESSARY */
    double localERR, globalERR;
    
    //Rank 0 reads the total number of nodes and broadcasts it to the rest 
    if(rank==0){
        if (rank == 0)
        {
            FILE *ip = fopen("data_input_meta", "r");
            if (!ip)
            {
                printf("Error opening the data_input_meta file.\n");
                MPI_Abort(MPI_COMM_WORLD, 253);
            }
            fscanf(ip, "%d\n", &nodecount);
            fclose(ip);
        }
    }
    MPI_Bcast(&nodecount, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //determine distrobution counts from each proces 
    int *recvcount = malloc(size*sizeof(int));
    int *displacement = malloc(size*sizeof(int));
    int totalLocalNodes;
    for(int p =0; p < size; p++){
        startNode = (nodecount / size) * p;
        endNode = (p == size - 1) ? nodecount : (nodecount / size) * (p + 1);
        totalLocalNodes = endNode - startNode;
        recvcount[p] = totalLocalNodes;
        displacement[p] = startNode;
    }
    startNode = (nodecount / size) * rank;
    endNode = (rank == size - 1) ? nodecount : (nodecount / size) * (rank + 1);
    totalLocalNodes = recvcount[rank];
    if (DEBUG)
    {
        printf("2. COMM_Rank: %d,\tNumCounts: %d\n", rank, totalLocalNodes);
    }
    
    
    numLinks = malloc(nodecount * sizeof(int));
    localNumLinks = malloc(totalLocalNodes*sizeof(int));
    r = malloc(nodecount*sizeof(double));
    rPre = malloc(nodecount * sizeof(double));
    localR = malloc(totalLocalNodes * sizeof(double));
    node_init(&nodehead, startNode, endNode); 

    omp_set_num_threads(size);
    omp_set_dynamic(1);
    omp_set_nested(0);

    MPI_Barrier(MPI_COMM_WORLD);
    GET_TIME(start);
    
    // compute inital r values

    // double threadPageRank;

    #pragma omp parallel firstprivate(i, j, iterationcount)
    {
        #pragma omp for
        for (i = 0; i < totalLocalNodes; ++i){
            localR[i] = 1.0 / nodecount;
            localNumLinks[i] = nodehead[i].num_out_links;   
        }


        // Distrobute r and num links
        #pragma omp master
        {
            if(DEBUG){
                printf("COMM_RANK %d:\tNum Threads: %d\n",rank ,omp_get_num_threads());
            }
            MPI_Allgatherv(localNumLinks,totalLocalNodes,MPI_INT,numLinks,recvcount, displacement, MPI_INT,MPI_COMM_WORLD);
            MPI_Allgatherv(localR, totalLocalNodes, MPI_DOUBLE, r, recvcount, displacement, MPI_DOUBLE, MPI_COMM_WORLD);
        }
        #pragma omp barrier

        localR = malloc(totalLocalNodes * sizeof(double));   
        iterationcount = 0;

    

        // core calculation
        do
        {
            
            ++iterationcount;
            #pragma omp for
            for (i = 0; i < nodecount; ++i){
                rPre[i] = r[i];
            }
            #pragma omp for schedule(dynamic, 1)
            for (i = 0; i < totalLocalNodes; ++i)
            {

                localR[i] = (1 - DAMPING_FACTOR) / nodecount; // Random jump term
                double threadPageRank=0;                           // for reduction
                // #pragma omp for reduction (+:threadPageRank)
                for (j = 0; j < nodehead[i].num_in_links; ++j)
                {
                    int outLinksFromNode ;
                    double rankFromNode;
                    int inID = nodehead[i].inlinks[j]; // Incoming node
                    outLinksFromNode = numLinks[inID]; // Outgoing links from incoming node
                    rankFromNode = rPre[inID];
                    threadPageRank += DAMPING_FACTOR * (rankFromNode / outLinksFromNode);
                }
                localR[i] += threadPageRank;
            }

            #pragma omp master 
            {
                //Distrobute result
                MPI_Allgatherv(localR, totalLocalNodes, MPI_DOUBLE, r, recvcount, displacement, MPI_DOUBLE, MPI_COMM_WORLD);
                //Calculate Error
                localERR = rel_error(r, rPre, nodecount);
                MPI_Allreduce(&localERR, &globalERR, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            }
            #pragma omp barrier

        } while (globalERR >= EPSILON);
    }
    //Synchronize before ending the timer 
    // MPI_Barrier(MPI_COMM_WORLD); rank 0 will be the slowest 
    
    GET_TIME(end);
    Lab4_saveoutput(r, nodecount, end - start);

    if(DEBUG){
        MPI_Barrier(MPI_COMM_WORLD);
        printf("COMM_RANL %d: TIME: %.6f\n", rank, end - start);
    }
    // post processing
    free(r);
    free(rPre);
    free(localR);
    free(numLinks);
    free(localNumLinks);
    node_destroy(nodehead, endNode - startNode);
    if (DEBUG)
    {
        printf("Terminate. COMM_Rank: %d\n", rank);
    }
    MPI_Finalize();
    return 0;
}