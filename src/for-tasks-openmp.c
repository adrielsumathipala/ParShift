#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>
#include <math.h>
#include "shift.h"
#include "util.h"
#include "timing.h"

#define MIN_DISTANCE 0.000001

int main(int argc, char const *argv[])
{
	if(argc != 5) {
		printf("Usage: ./for-tasks <n> <p> <bandwidth> <number of threads>\n");
	}
	
	int n = atoi(argv[1]);
	int p = atoi(argv[2]);
	double bw = atof(argv[3]);
	int numThreads = atoi(argv[4]);
	const char *stringN = argv[1];
	const char *stringP = argv[2];
	const char *bwStr = argv[3];
	omp_set_num_threads(numThreads);
	printf("n = %d, p = %d, bandwidth = %f, number of threads = %d\n", n, p, bw, numThreads);

	// Read in data:
  	int fileNameLength = 6 + (int) strlen(stringN) + (int) strlen(stringP);
	char fileName[fileNameLength];
	strcpy(fileName, "p=");
	strncat(fileName, stringP, strlen(stringP));
	strncat(fileName, "_n=", 3);
	strncat(fileName, stringN, strlen(stringN));

	printf("File Name: '%s'\n", fileName);

	FILE *f = fopen(fileName, "r");
	if(f == NULL) {
		printf("File '%s' does not exists\n", fileName);
		fclose(f);
		return 1;
	}

	double *data = malloc(sizeof(double) * n * p);
	double *shift_points = malloc(sizeof(double) * n * p);
	double *tmpHolder = malloc(sizeof(double) * n * p);
	double min_distance = (double) MIN_DISTANCE;
	
	double val; int numData = 0;
	while(fscanf(f, "%lf", &val) != EOF){
		data[numData] = val;
		shift_points[numData] = val;
		numData += 1;
	}

	fclose(f);

	bool * still_shifting = malloc(sizeof(bool) * n * p);
	for(int i = 0; i < (n*p); i++) {
		still_shifting[i] = true;
	}

	double max_min_dist = 1.0;
	double startWC, endWC;
	double cpuStart, cpuEnd;
	int iteration_number = 0;
	timing(&startWC, &cpuStart);
	#pragma omp parallel default(none) shared(tmpHolder, data, still_shifting, max_min_dist, iteration_number, bw, n, p, shift_points, min_distance) 
	{
		#pragma omp single 
	    {
		while(max_min_dist > MIN_DISTANCE) 
		{
	        // Single thread generates tasks:
		        max_min_dist = 0;
		        iteration_number += 1;
		        for(int i = 0; i < n; i++) {
		        	if(still_shifting[i] == false) {
		                continue;
		        	}
		        	#pragma omp task default(none) shared(tmpHolder, data, still_shifting, max_min_dist, iteration_number, bw, n, p, shift_points, min_distance) firstprivate(i)
		        	{
			        	double *p_new = malloc(sizeof(double) * p);
						double *p_old = malloc(sizeof(double) * p);
			        	// Linear NOT strided memory access!!
			        	// Loaded by ROWS not columns
			        	loadPoint(p_new, shift_points, i, p);
			        	loadPoint(p_old, shift_points, i, p);

			        	omp_shift(p_new, data, bw, n, p);
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
		    	// Copy data in tmp to shifted points array:
		    	copyTmp(tmpHolder, shift_points, n, p);
			}
		}
	}

	timing(&endWC, &cpuEnd);
	double computeTime = endWC - startWC;
	printf("Time to compute = %f\n", computeTime);

	// Group Points:
	timing(&startWC, &cpuStart);
	
	int *group_assignments = group_points(shift_points, n, p);
	
	timing(&endWC, &cpuEnd);
	double groupTime = endWC - startWC;
	printf("Time to group points = %f\n", groupTime);
	printf("Number of Iterations Required = %d\n", iteration_number);

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
	fprintf(output, "Number of Iterations Required = %d\n", iteration_number);

	// Write Final Shifted Points:
	writeShiftedPoints(output, shift_points, n, p);

	// Write Group Assignments:
	writeAssignments(output, group_assignments, n);

	fclose(output);

	free(data);
	free(shift_points);
	free(group_assignments);
	free(still_shifting);
	printf("Done!\n");
}
