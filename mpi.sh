#!/bin/bash
#SBATCH --partition=bigmem
# set total number of MPI processes
#SBATCH --ntasks=32
# set number of MPI processes per node
# (number of nodes is calculated by Slurm)
#SBATCH --ntasks-per-node=16
# set number of cpus per MPI process
#SBATCH --cpus-per-task=1
# set memory per cpu
#SBATCH --mem-per-cpu=4gb
#SBATCH --job-name=MEAN_SHIFT_MPI_RUN
#SBATCH --time=5:00

module load Langs/Intel/15.0.2
module load Langs/Intel/15 MPI/OpenMPI/2.1.1-intel15
pwd
# echo some environment variables
echo $SLURM_JOB_NODELIST
echo $SLURM_NTASKS_PER_NODE
make mpi

# See source code for input arguments
# e.g. time ./mpi <n> <p> <bandwidth>
time mpirun ./mpi 50000 2 0.8