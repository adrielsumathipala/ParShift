# ParShift — An Open MP and MPI implementation of the mean shift algorithm in C

**A thorough explanation of the implementation and benchmarking of ParShift can be found in [ParShift Paper](ParShift_Paper.pdf) document**

## Introduction:

The mean shift algorithm is a technique for obtaining the probability density maxima in a non-parametric feature space using a kernel density function and gradient ascent. An unsupervised machine learning technique, mean shift is used for cluster analysis, image segmentation, and object tracking. Unlike the *k*-means algorithm, the mean shift uses a single, tunable bandwidth parameter and does not require a pre-specified number of clusters.

One primary disadvantage of the mean shift algorithm is its quadratic time-complexity. Given *n* datapoints and *T* iterations to reach convergence, the algorithm is *O(T n ²)*. Developing a fast, parallel implementation is essential to using the mean shift to process massive high-dimensional datasets, video libraries, and image collections. 

## Implementation Overview:

ParShift uses two approaches to parallelization: OpenMP (multi-threading) and OpenMP + MPI (multi-threading on multiple nodes). Depending on the application, computing resources (e.g. single vs. multi-node) and data dimensionality, some implementations will be more effective than others.

### OpenMP Implementation:

A single-node implementation for multi-threading. Three implementations using OpenMP's Loops and Tasks paradigm are availiable.

#### Parallel Loops 
— Usage: `./for <n> <p> <bandwidth> <number of threads>`. 
- For certain problems, users may need to adjust between dynamic and static loop allocation for optimal load balancing.

#### Tasks 
— Usage: `./tasks <n> <p> <bandwidth> <number of threads>`. 
- This implementation treats computing the shift for each point as independent tasks. In some cases, using multiple-thread task generation instead of single-thread task generation may improve performance.

#### Parallel Loops and Tasks 
— Usage: `./for-tasks <n> <p> <bandwidth> <number of threads>`. 
- In testing with randomly sampled data, the overhead of managing both loops and tasks generally outweighed the increased parallelization benefits, but, in some applications, using tasks and loops together may lead to improved performance.

### OpenMP + MPI Implementation:

A multi-node implementation with built-in multi-threading for maximum performance using OpenMP and MPI. If OpenMP is not availiable on nodes, an MPI-only implementation can be found in `mpi.c`. If using a cluster with slurm, the `mpi.sh` file can be used to execute the code, with `ntasks`, `nodes`, and `cpus-per-task` adjusted as needed to run the job.

#### MPI + OpenMP Loops 
— Usage: Edit `./mpi-for.sh` and set `mpirun ./mpi-for <n> <p> <bandwidth>` appropriately. Then, run `./mpi-for.sh`. 
- This program uses the Open MP loops paradigm, which, in the case of well-balanced workloads, can perform better than the Open MP tasks.

#### MPI + OpenMP Tasks 
— Usage: Edit `./mpi-tasks.sh` and set `mpirun ./mpi-tasks <n> <p> <bandwidth>` appropriately. Then, run `./mpi-tasks.sh`.
- This program uses the Open MP tasks paradigm. This is generally the optimal multi-node program and using tasks greatly improves load balancing, leading to substantial performance improvements. Thus, for most applications the MPI + OpenMP tasks program will be suitable.

### Benchmarking:

For benchmarking results, see the [ParShift report](ParShift_Paper.pdf). Briefly, the implementation scaled properly and, running on Yale's Omega Cluster, the Open MP + MPI implementation succesfully scaled to 64 nodes with 8 cores and 32GB RAM each. Additional benchmarking can be performed using a serial implementation of the mean shift algorithm (shift.c).

## Implementation Details:

When running on a cluster, make sure the ICC compiler and Open MPI libraries are loaded:

```
>> module load Langs/Intel/15.0.2
>> module load Langs/Intel/15.0.2 MPI/OpenMPI/2.1.1-intel15
```

ParShift uses the Gaussian kernel. Other kernels can be implemented, such as those listed [here](https://en.wikipedia.org/wiki/Kernel_(statistics)#Nonparametric_statistics).