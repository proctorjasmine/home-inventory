#include <stdio.h>
#include "database.h"

int database_open(Database *db, const char *filename)
{
    int rc = sqlite3_open(filename, &db->connection);

    if (rc != SQLITE_OK){
        fprintf(stderr,
                "Could not open database: %s\n", sqlite3_errmsg(db->connection));
        return 1;
    }

    rc = sqlite3_exec(
        db->connection,
        "PRAGMA foreign_keys = ON;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK){
        fprintf(stderr,
                "Could not enable foreign keys: %s\n", sqlite3_errmsg(db->connection));
        sqlite3_close(db->connection);
        db->connection = NULL;

        return 1;
    }

    return 0;
}
void database_close(Database *db){
    if (db->connection != NULL){
        sqlite3_close(db->connection);
        db->connection = NULL;
    }
}