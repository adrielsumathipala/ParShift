#!/bin/bash
#SBATCH --partition=day
#SBATCH --ntasks=128
#SBATCH --nodes=64
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=4gb
#SBATCH --job-name=MEAN_SHIFT_MPI_RUN
#SBATCH --time=4:00:00

module load Langs/Intel/15.0.2
module load Langs/Intel/15 MPI/OpenMPI/2.1.1-intel15
pwd
# echo some environment variables
echo $SLURM_JOB_NODELIST
echo $SLURM_NTASKS_PER_NODE
# My MPI program is task2
make mpi-tasks

# The following mpirun command will pick up required info on nodes and cpus from Slurm. 
# You can use mpirun's -n option to reduce the number of MPI processes started on the cpus. (At most 1 MPI proc per Slurm task.)
# You can use mpirun options to control the layout of MPI processes---e.g., to spread processes out onto multiple nodes
# In this example, we've asked Slurm for 4 tasks (2 each on 2 nodes), but we've asked mpirun for two MPI procs, which will go onto 1 node.
# (If "-n 2" is omitted, you'll get 4 MPI procs---1 per Slurm task)
time mpirun ./mpi-tasks 600000 2 0.8