#ifndef USER_H
#define USER_H

#include "database.h"

#define USER_NAME_MAX 128

typedef struct
{
    int id;
    char name[USER_NAME_MAX];
} User;

int user_list(
    Database *db,
    User *users,
    int max_users,
    int *user_count
);

#endif