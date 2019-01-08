#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>
#include <math.h>
#include "util.h"
#include "shift.h"

void serial_shift(double *vec, double *data, double bw, int n, int p) {
	double *shifted = malloc(sizeof(double) * p);
	for(int i = 0; i < p; i++) {
		shifted[i] = 0.0;
	}
	double *tmp_point = malloc(sizeof(double) * p);
	double dist, weight;
	double scale_factor = 0.0;
	for(int i = 0; i < n; i++) {
		// Load vector with test points:
		loadPoint(tmp_point, data, i, p);
		
		// Compute distance and weight of test point:
		dist = euclidean_dist(vec, tmp_point, p);
		weight = uni_gaussian(dist, bw);
		
		// Compute contribution of test point to shift:
		for(int l = 0; l < p; l++) {
			shifted[l] += weight * tmp_point[l];
		}
		scale_factor += weight;
	}

	for(int i = 0; i < p; i++) {
		vec[i] = shifted[i]/scale_factor;
	}

	free(shifted);
	free(tmp_point);
	return;
}

void omp_shift(double *vec, double *data, double bw, int n, int p) {
	double *shifted = malloc(sizeof(double) * p);
	for(int i = 0; i < p; i++) {
		shifted[i] = 0.0;
	}
	double scale_factor = 0.0;
	
	#pragma omp parallel default(none) shared(data, bw, n, p, shifted, scale_factor, vec)
	{
		double *local_shifted = malloc(sizeof(double) * p);
		for(int i = 0; i < p; i++) {
			local_shifted[i] = 0.0;
		}
		double *tmp_point = malloc(sizeof(double) * p);
		double dist, weight;
		double local_scale_factor = 0.0;
		// #pragma omp for default(none) shared(data, n, p, bw) firstprivate(local_shifted, local_scale_factor) private(dist, weight, tmp_point)
		#pragma omp for
		for(int i = 0; i < n; i++) {
			// Load vector with test points:
			loadPoint(tmp_point, data, i, p);
			
			// Compute distance and weight of test point:
			dist = euclidean_dist(vec, tmp_point, p);
			weight = uni_gaussian(dist, bw);
			
			// Compute contribution of test point to shift:
			for(int l = 0; l < p; l++) {
				local_shifted[l] += weight * tmp_point[l];
			}
			local_scale_factor += weight;
		}

		#pragma omp critical (reduction)
			scale_factor += local_scale_factor;
			for(int i = 0; i < p; i++) {
				shifted[i] += local_shifted[i];
			}
		free(tmp_point);
		free(local_shifted);
	}

	for(int i = 0; i < p; i++) {
		vec[i] = shifted[i]/scale_factor;
	}

	free(shifted);
	return;
}


void copyTmp(double *tmp, double *shifted, int n, int p) {
	#pragma omp parallel default(none) shared(tmp, shifted, n, p)
	{
		#pragma omp for
			for(int i = 0; i < (n * p); i++) {
				shifted[i] = tmp[i];
			}
	}
}



