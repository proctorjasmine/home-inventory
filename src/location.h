#ifndef LOCATION_H
#define LOCATION_H

#include "database.h"

#define LOCATION_NAME_MAX 128

typedef struct
{
    int id;
    int parent_id;
    int has_parent;

    char name[LOCATION_NAME_MAX];
} Location;

int location_list(
    Database *db,
    Location *locations,
    int max_locations,
    int *location_count
);

#endif