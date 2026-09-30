#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

typedef struct{
    sqlite3 *connection;
} Database;

int database_open(Database *db, const char *filename);
void database_close(Database *db);

#endif

