#include <stdio.h>

#include "user.h"

int user_list(
    Database *db,
    User *users,
    int max_users,
    int *user_count
)
{
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT id, name "
        "FROM users "
        "ORDER BY name;";

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
                "Could not prepare user list: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (count >= max_users)
        {
            break;
        }

        User *user = &users[count];

        user->id = sqlite3_column_int(stmt, 0);

        snprintf(
            user->name,
            sizeof(user->name),
            "%s",
            sqlite3_column_text(stmt, 1)
        );

        count++;
    }

    if (rc != SQLITE_DONE && count < max_users)
    {
        fprintf(stderr,
                "Could not read user list: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    *user_count = count;

    return 0;
}