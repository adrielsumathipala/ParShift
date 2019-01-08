#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "util.h"

#define M_PI 3.14159265358979323846
#define GROUP_DISTANCE_TOLERANCE 0.1
#define INITIAL_GROUP_SIZE 200
#define INITIAL_NUMBER_OF_GROUPS 50
#define NONE -1

struct group_struct {
	int capacity;
	int size;
	double *list;
};

struct group_list {
	int capacity;
	int size;
	group_struct **groups;
};



// Linear NOT strided memory access!!
// Loaded by ROWS not columns
void loadPoint(double *vec, double *data, int currentPoint, int p) {
	int startingIndex = currentPoint * p;
	int vecCounter = 0;
	for(int i = startingIndex; i < (startingIndex + p); i++) {
		vec[vecCounter] = data[i];
		vecCounter += 1;
	}
}

void writeShiftedPoints(FILE *output, double *shifted, int n, int p) {
	fprintf(output, "Shifted Points:\n");
	fprintf(output, "[");
	double *shiftedPoint = malloc(sizeof(double) * p);
	for(int i = 0; i < n; i++) {	
		loadPoint(shiftedPoint, shifted, i, p);
		writePoint(output, shiftedPoint, p);
		if(i != (n - 1)) fprintf(output, ", ");
	}
	fprintf(output, "]\n");
	free(shiftedPoint);
}

void writePoint(FILE *output, double *point, int p) {
	fprintf(output, "[");
	fprintf(output, "%f", point[0]);
	for(int i = 1; i < p; i++) {
		fprintf(output, ", %f", point[i]);
	}
	fprintf(output, "]");
}

void writeAssignments(FILE *output, int *assignments, int n) {
	fprintf(output, "\nGroup Assignments:\n");
	fprintf(output, "[");
	fprintf(output, "%d", assignments[0]);
	for(int i = 1; i < n; i++) {
		fprintf(output, ", %d", assignments[i]);
	}
	fprintf(output, "]");
	fprintf(output, "\n");
}

void printPoint(double *point, int p) {
	for(int i = 0; i < p; i++) {
		if(i == 0) printf("%f", point[i]);
		else printf(", %f", point[i]);
	}
	printf("\n");
}

double euclidean_dist(double *pointA, double *pointB, int p) {
	double total = 0.0;
	for(int dimension = 0; dimension < p; dimension++) {
		total += (pointA[dimension] - pointB[dimension]) * (pointA[dimension] - pointB[dimension]);
	}
	return sqrt(total);
}

double uni_gaussian(double dist, double bw) {
	return (1/(bw*sqrt(2*M_PI))) * exp(-0.5*((dist/bw)*(dist/bw)));
}


group_struct *createGroupStruct(int p) {
	group_struct *group = malloc(sizeof(group_struct));
	group->capacity = INITIAL_GROUP_SIZE;
	group->size = 0;
	group->list = malloc(sizeof(double) * group->capacity * p);
	return group;
}

void destroyGroupStruct(group_struct *group) {
	free(group->list);
	free(group);
	return;
}

void groupEmbiggen(group_struct *group, int p) {
	int oldSize = group->size;
	int newSize = oldSize * 2;
	double *oldList = group->list;
	group->list = malloc(sizeof(double) * newSize * p);
	
	if(group->list == NULL) {
		printf("Malloc for larger group array failed\n");
		return;
	}
	// Fill new list with old data with dimensions oldSize x p
	int startIndex;
	for(int i = 0; i < (oldSize * p); i++) {
		group->list[i] = oldList[i];
	}
	free(oldList);
}


void addPoint(group_struct *group, double *point, int p) {
	// Enlarge if needed:
	if((group->size + 2) > group->capacity) {
		groupEmbiggen(group, p);
	}
	// Load p-dimensional data:
	int startIndex = group->size * p;
	int vecCounter = 0;
	for(int i = startIndex; i < (startIndex + p); i++) {
		group->list[i] = point[vecCounter];
		vecCounter += 1;
	}
	// Increment group size:
	group->size += 1;
	return;
}

group_list *createGroupList(void) {
	group_list *glist = malloc(sizeof(group_list));
	glist->capacity = INITIAL_NUMBER_OF_GROUPS;
	glist->size = 0;
	glist->groups = malloc(sizeof(group_struct *) * INITIAL_NUMBER_OF_GROUPS);
	return glist;
}

void groupListEmbiggen(group_list *glist) {
	int oldSize = glist->capacity;
	int newSize = oldSize * 2;
	group_struct **oldList = glist->groups;
	glist->groups = malloc(sizeof(group_struct *) * newSize);
	if(glist->groups == NULL) {
		printf("Failed to enlargen grouplist!\n");
		return;
	}
	for(int i = 0; i < oldSize; i++) {
		glist->groups[i] = oldList[i];
	}
	glist->capacity = newSize;
	free(oldList);
}

void addGroup(group_list *glist, group_struct *group) {
	if((glist->size + 5) > glist->capacity) {
		printf("Making bigger group list\n");
		groupListEmbiggen(glist);
	}
	glist->groups[glist->size] = group;
	glist->size += 1;
}

void destroyGroupList(group_list *glist) {
	for(int i = 0; i < glist->size; i++) {
		destroyGroupStruct(glist->groups[i]);
	}
	free(glist->groups);
	free(glist);
}

int *group_points(double *points, int n, int p) {
	
	int *group_assignments = malloc(sizeof(int) * n);
	double *currentPoint = malloc(sizeof(double) * p);
	
	group_list *glist = createGroupList();
	
	int nearest_group_index;
	int group_index = 0;

	for(int i = 0; i < n; i++) {
		// printf("Grouping point %d\n", i);
		loadPoint(currentPoint, points, i, p);
		nearest_group_index =  determine_nearest_group(currentPoint, glist, n, p);
		
		if(nearest_group_index == NONE) {
			printf("Adding group %d\n", group_index);
			// Create NEW Group:
			group_struct *newGroup = createGroupStruct(p);
			// Add current point into newGroup:
			addPoint(newGroup, currentPoint, p);
			// Add new group to group list:
			addGroup(glist, newGroup);
			// Add new group to point assignment list:
			group_assignments[i] = group_index;
			group_index += 1;
		}
		// Point belongs to an EXISTING group:
		else {
			// Set group index for this point:
			group_assignments[i] = nearest_group_index;
			// Add this point to the group:
			addPoint(glist->groups[nearest_group_index], currentPoint, p);
		}
	}

	destroyGroupList(glist);
	free(currentPoint);
	return group_assignments;
}

int determine_nearest_group(double *point, group_list *glist, int n, int p) {
	int nearest_group_index = NONE;
	double min_distance = DBL_MAX;
	int index = 0;
	double distToGroup;
	for(int i = 0; i < glist->size; i++) {
		
		distToGroup = distance_to_group(point, glist->groups[i], n, p);
		
		if(distToGroup < GROUP_DISTANCE_TOLERANCE && distToGroup < min_distance) {
			nearest_group_index = index;
			min_distance = distToGroup;
		}
		index += 1;
	}
	return nearest_group_index;

}


double distance_to_group(double *point, group_struct *group, int n, int p) {
	double dist;
	double min_distance = DBL_MAX;
	double *testPoint = malloc(sizeof(double) * p);
	for(int i = 0; i < group->size; i++) {
		loadPoint(testPoint, group->list, i, p);
		dist = euclidean_dist(point, testPoint, p);
		if(dist < min_distance) {
			min_distance = dist;
		}
	}
	free(testPoint);
	return min_distance;
}

