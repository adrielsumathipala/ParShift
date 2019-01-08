#include <stdlib.h>

struct group_list;
struct group_struct;

typedef struct group_list group_list;
typedef struct group_struct group_struct;

void loadPoint(double *vec, double *data, int currentPoint, int p);

void writeShiftedPoints(FILE *output, double *shifted, int n, int p);

void writePoint(FILE *output, double *point, int p);

void writeAssignments(FILE *output, int *assignments, int n);

void printPoint(double *point, int p);

double euclidean_dist(double *pointA, double *pointB, int p);

double uni_gaussian(double dist, double bw);

group_struct *createGroupStruct(int p);

void destroyGroupStruct(group_struct *g);

void groupEmbiggen(group_struct *group, int p);

void addPoint(group_struct *group, double *point, int p);

group_list *createGroupList(void);

void groupListEmbiggen(group_list *glist);

void addGroup(group_list *glist, group_struct *group);

void destroyGroupList(group_list *glist);

int *group_points(double *points, int n, int p);

int determine_nearest_group(double *point, group_list *glist, int n, int p);

double distance_to_group(double *point, group_struct *group, int n, int p);