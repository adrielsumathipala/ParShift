# Before making executables, make sure ICC compiler is loaded using:  > module load Langs/Intel/15.0.2
CC = icc
CFLAGS = -g -O3 -xHost -fno-alias -std=c99

serial: serial.c shift.c util.c timing.c
	$(CC) $(CFLAGS) -o serial serial.c shift.c util.c timing.c -lm

tasks: tasks-openmp.c shift.c util.c timing.c
	$(CC) $(CFLAGS) -openmp -o tasks tasks-openmp.c shift.c util.c timing.c -lm

for: for-openmp.c shift.c util.c timing.c
	$(CC) $(CFLAGS) -openmp -o for for-openmp.c shift.c util.c timing.c -lm

for-tasks: for-tasks-openmp.c shift.c util.c timing.c
	$(CC) $(CFLAGS) -openmp -o for-tasks for-tasks-openmp.c shift.c util.c timing.c -lm
mpi: mpi.c shift.c util.c timing.c
	mpicc $(CFLAGS) -openmp -o mpi mpi.c shift.c util.c timing.c

mpi-for: mpi-for.c shift.c util.c timing.c
	mpicc $(CFLAGS) -openmp -o mpi-for mpi-for.c shift.c util.c timing.c

mpi-tasks: mpi-tasks.c shift.c util.c timing.c
	mpicc $(CFLAGS) -openmp -o mpi-tasks mpi-tasks.c shift.c util.c timing.c

clean:
	rm -f $(EXECUTABLES) *.o
