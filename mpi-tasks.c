#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>
#include <math.h>
#include <mpi.h>
#include "shift.h"
#include "util.h"
#include "timing.h"

// Load MPI Library:
// module load Langs/Intel/15.0.2 MPI/OpenMPI/2.1.1-intel15

#define MIN_DISTANCE 0.000001
#define ROOT 0

int main(int argc, char const *argv[])
{
	MPI_Init(&argc, (char ***) &argv);
	int rank, numProcesses;
	double start, finish, computeTime;
	double startWC, cpuStart, endWC, cpuEnd, groupTime;
	MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	if(argc != 4) {
		if(rank == ROOT) printf("Usage: ./mpi <n> <p> <bandwidth>\n");
		return 1;
	}
	
	int n = atoi(argv[1]);
	int p = atoi(argv[2]);
	double bw = atof(argv[3]);
	const char *stringN = argv[1];
	const char *stringP = argv[2];
	const char *bwStr = argv[3];
	if(rank == ROOT) printf("n = %d, p = %d, bandwidth = %f\n", n, p, bw);

	// Read in data:
  	int fileNameLength = 6 + (int) strlen(stringN) + (int) strlen(stringP);
	char fileName[fileNameLength];
	strcpy(fileName, "p=");
	strncat(fileName, stringP, strlen(stringP));
	strncat(fileName, "_n=", 3);
	strncat(fileName, stringN, strlen(stringN));

	if(rank == ROOT) printf("File Name: '%s'\n", fileName);

	FILE *f = fopen(fileName, "r");
	if(f == NULL) {
		if(rank == ROOT) printf("File '%s' does not exists\n", fileName);
		fclose(f);
		return 1;
	}

	double *data = malloc(sizeof(double) * n * p);
	double *shift_points_global;
	double val; int numData = 0;
	if(rank == ROOT) {
		printf("MPI: Number of Processes = %d\n", numProcesses);
		shift_points_global = malloc(sizeof(double) * n * p);
	}
	while(fscanf(f, "%lf", &val) != EOF){
		data[numData] = val;
		if(rank == ROOT) {
			shift_points_global[numData] = val;
		}
		numData += 1;
	}

	fclose(f);

	// Determine displs and sendcounts:
	int *sendcounts = malloc(sizeof(int) * numProcesses);
	int *displs = malloc(sizeof(int) * numProcesses);
	int pointsPerProcess = n / numProcesses;
	displs[0] = 0;
	for(int i = 1; i < numProcesses; i++) {
		displs[i] = displs[(i-1)] + (pointsPerProcess * p);
	}

	sendcounts[0] = displs[1];
    sendcounts[(numProcesses - 1)] = (n * p) - displs[(numProcesses-1)];
    for(int block = 1; block < (numProcesses-1); block++) {
      sendcounts[block] = displs[(block + 1)] - displs[block];
    }

    if(rank == ROOT) {
    	printf("Sendcounts: ");
    	for(int i = 0; i < numProcesses; i++){
    		printf("%d ", sendcounts[i]);
    	}
    	printf("\n Displs: ");
    	for (int i = 0; i < numProcesses; i++)
    	{
    		printf("%d ", displs[i]);
    	}
    	printf("\n");
    }

    // Number of data points each rank is responsible for shifting:
    int nLocal = sendcounts[rank] / p;
    // Initialize local variables used by each process:
	bool * still_shifting = malloc(sizeof(bool) * nLocal);
	int remainingPoints = nLocal;
	for(int i = 0; i < nLocal; i++) {
		still_shifting[i] = true;
	}
	double max_min_dist = 1.0;
	int vecCounter;
	double dist;
	int iteration_number = 0;
	double *p_new = malloc(sizeof(double) * p);
	double *p_old = malloc(sizeof(double) * p);
	double *shift_points = malloc(sizeof(double) * nLocal * p);
	double *tmpHolder = malloc(sizeof(double) * nLocal * p);

	if(rank == ROOT) {
		start = MPI_Wtime();
	}
	MPI_Scatterv(shift_points_global, sendcounts, displs, MPI_DOUBLE, 
		shift_points, sendcounts[rank], MPI_DOUBLE, 
		ROOT, MPI_COMM_WORLD);
	
	#pragma omp parallel default(none) shared(tmpHolder, data, still_shifting, max_min_dist, iteration_number, bw, n, p, shift_points, nLocal) 
	{
		#pragma omp single 
	    {
	    double *tmpPointer;
		while(max_min_dist > MIN_DISTANCE) 
		{
	        // Single thread generates tasks:
		        max_min_dist = 0;
		        iteration_number += 1;
		        for(int i = 0; i < nLocal; i++) {
		        	if(still_shifting[i] == false) {
		                continue;
		        	}
		        	#pragma omp task default(none) shared(tmpHolder, data, still_shifting, max_min_dist, iteration_number, bw, n, p, shift_points) firstprivate(i)
		        	{
			        	double *p_new = malloc(sizeof(double) * p);
						double *p_old = malloc(sizeof(double) * p);
			        	// Linear NOT strided memory access!!
			        	// Loaded by ROWS not columns
			        	loadPoint(p_new, shift_points, i, p);
			        	loadPoint(p_old, shift_points, i, p);

			        	serial_shift(p_new, data, bw, n, p);
			        	double dist = euclidean_dist(p_new, p_old, p);

			        	
			        	if(dist > max_min_dist) {
			        		max_min_dist = dist;
			        	}
			        	
			        	if(dist < MIN_DISTANCE) {
			        		still_shifting[i] = false;
			        	}

			        	int vecCounter = 0;
			        	for(int k = (i*p); k < ((i*p) + p); k++) {
			        		tmpHolder[k] = p_new[vecCounter];
			        		vecCounter += 1;
			        	}
			        	free(p_new);
			        	free(p_old);
		        	}
		    	}
		    	#pragma omp taskwait
		    	tmpPointer = shift_points;
		    	shift_points = tmpHolder;
		    	tmpHolder = tmpPointer;
			}
		}
	}

	MPI_Gatherv(shift_points, sendcounts[rank], MPI_DOUBLE, 
		shift_points_global, sendcounts, displs, MPI_DOUBLE, 
		ROOT, MPI_COMM_WORLD);

	if(rank == ROOT) {
		finish = MPI_Wtime();
		computeTime = finish - start;
		printf("\n\n\nTime to compute = %f\n", computeTime);
		// Group Points
		timing(&startWC, &cpuStart);
		int *group_assignments = group_points(shift_points_global, n, p);
		timing(&endWC, &cpuEnd);
		groupTime = endWC - startWC;
		printf("Time to group points = %f\n", groupTime);

		// Write data to text file:
		int outputFileLength = 12 + fileNameLength + (int) strlen(bwStr);
		char outputFileName[outputFileLength];
		char *inputFileName = fileName;
		strcpy(outputFileName, "output_");
		strncat(outputFileName, inputFileName, fileNameLength);
		strncat(outputFileName, "_bw=", 4);
		strncat(outputFileName, bwStr, strlen(bwStr));
		printf("Output file name: '%s'\n", outputFileName);
		FILE * output = fopen(outputFileName, "w");

		// Write Timing Data:
		fprintf(output, "Time to compute = %f\n", computeTime);
		fprintf(output, "Time to group points = %f\n", groupTime);
		fprintf(output, "Number of Iterations Required (Rank 0) = %d\n", iteration_number);

		// Write Final Shifted Points:
		writeShiftedPoints(output, shift_points_global, n, p);

		// Write Group Assignments:
		writeAssignments(output, group_assignments, n);

		fprintf(output, "MPI: Number of Processes = %d\n", numProcesses);

		fclose(output);
		free(shift_points_global);
		free(group_assignments);
	}

	free(data);
	free(shift_points);
	free(sendcounts);
	free(displs);
	free(still_shifting);
	free(p_new);
	free(p_old);
	free(tmpHolder);
	MPI_Finalize();
	if(rank == ROOT) printf("Done!\n");
}
