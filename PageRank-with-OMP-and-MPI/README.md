# Parallel PageRank Algorithm (MPI + OpenMP)

This program implements the **PageRank algorithm** using a hybrid parallel computing approach with **MPI (Message Passing Interface)** for distributed memory processing and **OpenMP (Open Multi-Processing)** for shared memory parallelization. 

## 🚀 Features
- **Hybrid Parallelization**: Uses MPI for multi-node computation and OpenMP for multi-threading.
- **Dynamic Work Distribution**: Efficiently distributes workload among processes.
- **Optimized PageRank Calculation**: Implements damping factor and convergence criteria.
- **Scalable**: Designed to handle large-scale graphs.

## 📂 File Overview
- **`main.c`** - Implements the parallel PageRank algorithm.
- **`Lab4_IO.h` / `Lab4_IO.c`** - Handles input/output operations.
- **`timer.h`** - Provides timing utilities.
- **`Makefile`** - Compilation instructions.
- **`data_input_meta`** - Metadata file specifying the number of nodes.

## 🛠️ Dependencies
Ensure the following dependencies are installed:
- **GCC** (for compilation)
- **OpenMP** (for shared memory parallelism)
- **MPI Library** (e.g., MPICH, OpenMPI)

# 🔍 Algorithm Breakdown

## 1. Initialization:
- The root process (rank 0) reads the number of nodes from `data_input_meta`.
- Node distribution is computed to balance workload across all processes.
- Initial PageRank values are set.

## 2. Parallel Execution:
- Each process computes local contributions to the PageRank scores.
- `MPI_Allgatherv` is used for communication among processes.
- OpenMP is used within each process to parallelize computations.

## 3. Iteration until Convergence:
- PageRank values are iteratively updated using the damping factor formula.
- Convergence is checked using relative error (`EPSILON = 0.00001`).
- Global error is computed using `MPI_Allreduce`.

## 4. Finalization:
- Output is saved.
- Memory is freed, and MPI is finalized.

# 📊 Performance Considerations
- **Load Balancing:** Dynamically distributes nodes across MPI processes.
- **Communication Optimization:** Minimizes MPI communication overhead.
- **Parallelization Efficiency:** Uses OpenMP within each MPI process.
