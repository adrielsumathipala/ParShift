#include <stdlib.h>
#include "util.h"

void serial_shift(double *vec, double *data, double bw, int n, int p);

void omp_shift(double *vec, double *data, double bw, int n, int p);

void copyTmp(double *tmp, double *shifted, int n, int p);
