#include <stdio.h>

#include "location.h"

int location_list(
    Database *db,
    Location *locations,
    int max_locations,
    int *location_count
)
{
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT id, name, parent_id "
        "FROM locations "
        "ORDER BY id;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare location list: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (count >= max_locations)
        {
            break;
        }

        Location *location = &locations[count];

        location->id =
            sqlite3_column_int(stmt, 0);

        snprintf(
            location->name,
            sizeof(location->name),
            "%s",
            sqlite3_column_text(stmt, 1)
        );

        if (sqlite3_column_type(stmt, 2) == SQLITE_NULL)
        {
            location->parent_id = 0;
            location->has_parent = 0;
        }
        else
        {
            location->parent_id =
                sqlite3_column_int(stmt, 2);

            location->has_parent = 1;
        }

        count++;
    }

    if (rc != SQLITE_DONE && count < max_locations)
    {
        fprintf(stderr,
                "Could not read location list: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    *location_count = count;

    return 0;
}